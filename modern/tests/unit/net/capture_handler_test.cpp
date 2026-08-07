//
// E2 T2 capture handler unit tests.
//
// Locks the wire-byte round-trip and SHA-256 fingerprint stability of
// the CaptureHandler wrapper around IConnectionHandler. This is the
// foundational regression anchor for E2 T2 (wire SHA-256 replay) and
// the prerequisite for any side-by-side behavior comparison against
// the legacy SWorking/* server.
//
// All tests use mock IConnectionHandlers so we never bind a socket;
// the wrapper's contract is pure (record -> forward).
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #ifndef socklen_t
        using socklen_t = int;
    #endif
#endif

#include <gtest/gtest.h>

#include <cstdint>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "mxh/net/capture_handler.hpp"
#include "mxh/net/net.hpp"

using namespace mxh::net;
namespace fs = std::filesystem;

namespace {

// Mock inner handler that records every callback for forwarding assertions.
// Thread-safe: concurrent tests call on_message from multiple threads,
// so every mutation is mutex-guarded.
struct MockInner final : IConnectionHandler {
    mutable std::mutex mu;
    int connect_count = 0;
    int message_count = 0;
    int disconnect_count = 0;
    std::vector<ConnectionId> connects;
    std::vector<Message> messages;
    std::vector<NetError> disconnects;

    bool on_connect(ConnectionId id, const std::string& remote) override {
        std::lock_guard<std::mutex> lk(mu);
        ++connect_count;
        connects.push_back(id);
        (void)remote;
        return true;
    }
    void on_message(ConnectionId id, const Message& msg) override {
        std::lock_guard<std::mutex> lk(mu);
        ++message_count;
        messages.push_back(msg);
        (void)id;
    }
    void on_disconnect(ConnectionId id, NetError reason) override {
        std::lock_guard<std::mutex> lk(mu);
        ++disconnect_count;
        disconnects.push_back(reason);
        (void)id;
    }
};

// Build a deterministic Message for round-trip tests.
Message make_msg(std::uint8_t cat, std::uint8_t proto,
                std::uint32_t obj, std::vector<std::uint8_t> payload) {
    Message m;
    m.header.category = cat;
    m.header.protocol = proto;
    m.header.object_id = obj;
    m.header.checksum = 0;
    m.header.code = 0;
    m.payload = std::move(payload);
    return m;
}

// Standard temp directory under the test scratch space.
fs::path scratch_dir() {
    auto p = fs::temp_directory_path() / "mxh_capture_handler_tests";
    std::error_code ec;
    fs::create_directories(p, ec);
    return p;
}

// Known SHA-256 of empty input (FIPS 180-4 standard test vector).
constexpr const char* kSha256OfEmpty =
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

constexpr const char* kSha256OfAbc =
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

}  // namespace

// ===== Sha256Digest basics =====

TEST(Sha256Digest, ToHexProducesLowercase32Bytes) {
    Sha256Digest d{};
    d.bytes.fill(0xABu);
    const auto hex = d.to_hex();
    EXPECT_EQ(hex.size(), 64u);
    // Each 0xAB byte is rendered as two lowercase hex chars 'a' and 'b'.
    std::string expected;
    expected.reserve(64);
    for (int i = 0; i < 32; ++i) expected += "ab";
    EXPECT_EQ(hex, expected);
}

TEST(Sha256Digest, EqualityComparesByteArray) {
    Sha256Digest a{};
    Sha256Digest b{};
    a.bytes[0] = 0x01;
    EXPECT_NE(a, b);
    b.bytes[0] = 0x01;
    EXPECT_EQ(a, b);
}

// ===== SHA-256 against FIPS 180-4 test vectors =====

TEST(Sha256FipsVector, EmptyStringHashMatchesFipsVector) {
    const std::vector<std::uint8_t> empty;
    auto d = sha256(std::span<const std::uint8_t>(empty.data(), empty.size()));
    ASSERT_NE(d.to_hex(), "0000000000000000000000000000000000000000000000000000000000000000")
        << "BCrypt SHA-256 unavailable on this platform";
    EXPECT_EQ(d.to_hex(), kSha256OfEmpty);
}

TEST(Sha256FipsVector, AbcStringHashMatchesFipsVector) {
    const std::uint8_t data[] = {'a', 'b', 'c'};
    auto d = sha256(std::span<const std::uint8_t>(data, 3));
    ASSERT_NE(d.to_hex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(d.to_hex(), kSha256OfAbc);
}

TEST(Sha256FipsVector, ConcatDigestIsHashOfConcatenation) {
    const std::uint8_t abc[] = {'a', 'b', 'c'};
    const std::uint8_t def[] = {'d', 'e', 'f'};
    const std::uint8_t abcdef[] = {'a', 'b', 'c', 'd', 'e', 'f'};
    auto concat = sha256(std::span<const std::uint8_t>(abcdef, 6));
    std::vector<std::vector<std::uint8_t>> chunks = {
        {std::begin(abc), std::end(abc)},
        {std::begin(def), std::end(def)}};
    auto via_concat = sha256_concat(chunks);
    EXPECT_EQ(concat, via_concat);
}

// ===== CaptureHandler wire-byte capture =====

TEST(CaptureHandler, WrappedOnMessageForwardsToInner) {
    MockInner inner;
    CaptureHandler cap(inner);
    cap.on_message(make_connection_id(1), make_msg(7, 1, 9999, {1, 2, 3, 4}));
    EXPECT_EQ(inner.message_count, 1);
    EXPECT_EQ(inner.messages[0].header.category, 7u);
    EXPECT_EQ(cap.size(), 1u);
}

TEST(CaptureHandler, CapturedWireBytesAreLengthPrefixPlusHeaderPlusPayload) {
    Message m = make_msg(7, 1, 0x12345678u, {0xAA, 0xBB, 0xCC, 0xDD});
    MockInner inner;
    CaptureHandler cap(inner);
    cap.on_message(make_connection_id(1), m);
    const auto snap = cap.snapshot();
    ASSERT_EQ(snap.size(), 1u);
    // 2B length prefix + 8B header + 4B payload = 14 bytes
    ASSERT_EQ(snap[0].wire_bytes.size(), 14u);
    // Length prefix (LE u16): 8 + 4 = 12 = 0x0C 0x00
    EXPECT_EQ(snap[0].wire_bytes[0], 0x0Cu);
    EXPECT_EQ(snap[0].wire_bytes[1], 0x00u);
    EXPECT_EQ(snap[0].wire_bytes[2], 0x00u);  // checksum
    EXPECT_EQ(snap[0].wire_bytes[3], 0x00u);  // code
    EXPECT_EQ(snap[0].wire_bytes[4], 7u);    // category
    EXPECT_EQ(snap[0].wire_bytes[5], 1u);    // protocol
    EXPECT_EQ(snap[0].wire_bytes[6], 0x78u); // object_id LE byte 0
    EXPECT_EQ(snap[0].wire_bytes[9], 0x12u); // object_id LE byte 3
    EXPECT_EQ(snap[0].wire_bytes[10], 0xAAu);
    EXPECT_EQ(snap[0].wire_bytes[13], 0xDDu);
}

TEST(CaptureHandler, CapturedMessageHeaderMatchesWireDecoding) {
    Message m = make_msg(11, 5, 0xDEADBEEFu, {0x01, 0x02});
    MockInner inner;
    CaptureHandler cap(inner);
    cap.on_message(make_connection_id(42), m);
    const auto snap = cap.snapshot();
    ASSERT_EQ(snap.size(), 1u);
    EXPECT_EQ(snap[0].connection.value, 42u);
    EXPECT_EQ(snap[0].message.header.category, 11u);
    EXPECT_EQ(snap[0].message.header.protocol, 5u);
    EXPECT_EQ(snap[0].message.header.object_id, 0xDEADBEEFu);
    EXPECT_EQ(snap[0].message.payload.size(), 2u);
}

TEST(CaptureHandler, OnConnectAndOnDisconnectForwardToInner) {
    MockInner inner;
    CaptureHandler cap(inner);
    EXPECT_TRUE(cap.on_connect(make_connection_id(7), "127.0.0.1"));
    cap.on_disconnect(make_connection_id(7), NetError::Disconnected);
    EXPECT_EQ(inner.connect_count, 1);
    EXPECT_EQ(inner.disconnect_count, 1);
    EXPECT_EQ(inner.disconnects[0], NetError::Disconnected);
}

TEST(CaptureHandler, ClearEmptiesCapturedSequence) {
    MockInner inner;
    CaptureHandler cap(inner);
    cap.on_message(make_connection_id(1), make_msg(1, 0, 0, {}));
    cap.on_message(make_connection_id(1), make_msg(1, 1, 0, {}));
    EXPECT_EQ(cap.size(), 2u);
    cap.clear();
    EXPECT_EQ(cap.size(), 0u);
    // After clear, fingerprint is the empty-input SHA-256.
    EXPECT_EQ(cap.fingerprint().to_hex(), kSha256OfEmpty);
}

// ===== SHA-256 fingerprint stability =====

TEST(CaptureHandler, FingerprintIsDeterministicAcrossCalls) {
    MockInner inner;
    CaptureHandler cap(inner);
    cap.on_message(make_connection_id(1), make_msg(7, 1, 9999, {1, 2, 3, 4}));
    cap.on_message(make_connection_id(2), make_msg(11, 5, 0xDEADu, {0xFF}));
    const auto fp1 = cap.fingerprint();
    const auto fp2 = cap.fingerprint();
    EXPECT_EQ(fp1, fp2);
}

TEST(CaptureHandler, FingerprintChangesWhenMessageIsAltered) {
    MockInner inner;
    CaptureHandler cap1(inner);
    cap1.on_message(make_connection_id(1), make_msg(7, 1, 9999, {1, 2, 3, 4}));
    const auto fp1 = cap1.fingerprint();

    CaptureHandler cap2(inner);
    cap2.on_message(make_connection_id(1), make_msg(7, 1, 9999, {1, 2, 3, 5}));  // last byte differs
    const auto fp2 = cap2.fingerprint();

    EXPECT_NE(fp1, fp2);
}

TEST(CaptureHandler, FingerprintChangesWhenOrderIsSwapped) {
    MockInner inner;
    CaptureHandler cap1(inner);
    cap1.on_message(make_connection_id(1), make_msg(7, 1, 1, {0xAA}));
    cap1.on_message(make_connection_id(1), make_msg(7, 1, 2, {0xBB}));
    const auto fp1 = cap1.fingerprint();

    CaptureHandler cap2(inner);
    cap2.on_message(make_connection_id(1), make_msg(7, 1, 2, {0xBB}));
    cap2.on_message(make_connection_id(1), make_msg(7, 1, 1, {0xAA}));
    const auto fp2 = cap2.fingerprint();

    EXPECT_NE(fp1, fp2);
}

// ===== Save / load round-trip =====

TEST(CaptureHandler, SaveLoadRoundTripPreservesFingerprint) {
    MockInner inner;
    CaptureHandler cap(inner);
    cap.on_message(make_connection_id(1), make_msg(7, 1, 0x11111111u, {0x01, 0x02, 0x03}));
    cap.on_message(make_connection_id(2), make_msg(11, 4, 0x22222222u, {0xAA, 0xBB}));
    cap.on_message(make_connection_id(3), make_msg(8, 0, 0x33333333u, {}));
    const auto fp_before = cap.fingerprint();

    const auto path = scratch_dir() / "capture_roundtrip.txt";
    ASSERT_TRUE(cap.save(path.string()));
    ASSERT_TRUE(fs::exists(path));

    CaptureHandler cap2(inner);
    ASSERT_TRUE(cap2.load(path.string()));
    EXPECT_EQ(cap2.size(), 3u);
    const auto fp_after = cap2.fingerprint();
    EXPECT_EQ(fp_before, fp_after);
}

TEST(CaptureHandler, LoadPreservesWireBytesByteForByte) {
    MockInner inner;
    CaptureHandler cap(inner);
    Message m = make_msg(7, 1, 0xABCDEF01u, {0x10, 0x20, 0x30, 0x40, 0x50});
    cap.on_message(make_connection_id(99), m);
    const auto wire_before = cap.snapshot()[0].wire_bytes;

    const auto path = scratch_dir() / "capture_bytes.txt";
    ASSERT_TRUE(cap.save(path.string()));

    CaptureHandler cap2(inner);
    ASSERT_TRUE(cap2.load(path.string()));
    const auto wire_after = cap2.snapshot()[0].wire_bytes;
    EXPECT_EQ(wire_before, wire_after);
}

TEST(CaptureHandler, LoadOnEmptyFileFailsGracefully) {
    const auto path = scratch_dir() / "empty_capture.txt";
    {
        std::ofstream f(path);  // create empty file
    }
    MockInner inner;
    CaptureHandler cap(inner);
    EXPECT_TRUE(cap.load(path.string()));  // empty file is valid (zero packets)
    EXPECT_EQ(cap.size(), 0u);
}

TEST(CaptureHandler, LoadOnMissingFileFailsGracefully) {
    MockInner inner;
    CaptureHandler cap(inner);
    EXPECT_FALSE(cap.load((scratch_dir() / "no_such_file_xyz.txt").string()));
}

// ===== Concurrent capture safety =====

TEST(CaptureHandler, ConcurrentOnMessageCapturesAllPacketsInOrder) {
    MockInner inner;
    CaptureHandler cap(inner);
    
    constexpr int kThreads = 4;
    constexpr int kPerThread = 25;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&cap, t]() {
            
            for (int i = 0; i < kPerThread; ++i) {
                Message m = make_msg(
                    static_cast<std::uint8_t>(t),
                    static_cast<std::uint8_t>(i),
                    static_cast<std::uint32_t>(t * 1000 + i),
                    {static_cast<std::uint8_t>(t), static_cast<std::uint8_t>(i)});
                cap.on_message(make_connection_id(t), m);
            }
            
        });
    }
    
    for (auto& th : threads) th.join();
    
    EXPECT_EQ(cap.size(), static_cast<std::size_t>(kThreads * kPerThread));
    EXPECT_EQ(inner.message_count, kThreads * kPerThread);
    const auto fp1 = cap.fingerprint();
    const auto fp2 = cap.fingerprint();
}

// ===== E2 T2: 1000+ real-packet replay, SHA-256 must be byte-stable =====

namespace {

int capture_find_free_port() {
    SOCKET tmp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tmp == INVALID_SOCKET) return 0;
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    socklen_t len = sizeof(addr);
    if (bind(tmp, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closesocket(tmp);
        return 0;
    }
    if (getsockname(tmp, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        closesocket(tmp);
        return 0;
    }
    int port = ntohs(addr.sin_port);
    closesocket(tmp);
    return port;
}

}  // namespace

TEST(CaptureHandler, BulkReplayFingerprintStableAcrossThousandPackets) {
    constexpr int kPacketCount = 1001;

    MockInner server_inner;
    CaptureHandler server_capture(server_inner);
    TcpServer server(server_capture);
    const int port = capture_find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig scfg;
    scfg.port = static_cast<std::uint16_t>(port);
    scfg.worker_thread_count = 1;
    scfg.use_legacy_framing = true;
    ASSERT_EQ(server.start(scfg), NetError::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    MockInner client_inner;
    TcpClient client(client_inner);
    ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(client.connect(ccfg), NetError::Ok);

    // 1001 messages spanning all 96 categories with varied protocols and
    // payload sizes (0..255 bytes) -- a realistic cross-category sweep.
    for (int i = 0; i < kPacketCount; ++i) {
        const std::uint8_t cat = static_cast<std::uint8_t>(i % 96);
        const std::uint8_t proto = static_cast<std::uint8_t>((i * 7) % 256);
        const std::uint32_t obj = static_cast<std::uint32_t>(i * 2654435761u);
        std::vector<std::uint8_t> payload(static_cast<std::size_t>(i % 256));
        for (std::size_t k = 0; k < payload.size(); ++k) {
            payload[k] = static_cast<std::uint8_t>(i + k);
        }
        Message m = make_msg(cat, proto, obj, std::move(payload));
        ASSERT_EQ(client.send(m), NetError::Ok);
    }

    for (int i = 0; i < 200 && server_capture.size() < kPacketCount; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_EQ(server_capture.size(), static_cast<std::size_t>(kPacketCount));
    EXPECT_EQ(server_inner.message_count, kPacketCount);

    const auto fp_original = server_capture.fingerprint();
    const auto cap_path = scratch_dir() / "bulk_replay_capture.txt";
    ASSERT_TRUE(server_capture.save(cap_path.string()));

    // Replay into a fresh handler: fingerprint and bytes must match
    // exactly (diff = 0).
    MockInner loaded_inner;
    CaptureHandler loaded_handler(loaded_inner);
    ASSERT_TRUE(loaded_handler.load(cap_path.string()));
    EXPECT_EQ(loaded_handler.size(), static_cast<std::size_t>(kPacketCount));

    // Replay the loaded capture into a FRESH handler; the replayed run's
    // fingerprint and wire bytes must equal the original run (diff = 0).
    MockInner replay_inner;
    CaptureHandler replay_handler(replay_inner);
    const auto delivered = replay_capture(
        loaded_handler.snapshot(), replay_handler);
    EXPECT_EQ(delivered, static_cast<std::size_t>(kPacketCount));
    EXPECT_EQ(replay_inner.message_count, kPacketCount);
    EXPECT_EQ(replay_handler.fingerprint(), fp_original);

    // Byte-for-byte wire equality across the original and replayed runs.
    const auto orig_packets = server_capture.snapshot();
    const auto replay_packets = replay_handler.snapshot();
    ASSERT_EQ(orig_packets.size(), replay_packets.size());
    for (std::size_t i = 0; i < orig_packets.size(); ++i) {
        EXPECT_EQ(orig_packets[i].wire_bytes, replay_packets[i].wire_bytes);
        EXPECT_EQ(orig_packets[i].message.header.category,
                  replay_packets[i].message.header.category);
        EXPECT_EQ(orig_packets[i].message.header.protocol,
                  replay_packets[i].message.header.protocol);
        EXPECT_EQ(orig_packets[i].message.header.object_id,
                  replay_packets[i].message.header.object_id);
    }

    client.disconnect();
    server.stop();
}
