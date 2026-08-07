// hsel_encryptor_test.cpp
//
// Verifies HselStreamCipher (the HSEL IEncryptor adapter, Phase R-1):
// seed/roundtrip semantics, export/import handshake determinism, the
// size-preserving invariant, multi-packet session continuity, and a
// real TcpServer <-> TcpClient encrypted round trip through the modern
// net layer with legacy framing.

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #ifndef socklen_t
        using socklen_t = int;
    #endif
#endif

#include "mxh/crypto/hsel_encryptor.hpp"
#include "mxh/crypto/hsel_stream.hpp"
#include "mxh/net/net.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

using mxh::crypto::HselInit;
using mxh::crypto::HselStream;
using mxh::crypto::HselStreamCipher;
using mxh::net::ClientConfig;
using mxh::net::ConnectionId;
using mxh::net::IConnectionHandler;
using mxh::net::Message;
using mxh::net::MsgHeader;
using mxh::net::NetError;
using mxh::net::ServerConfig;
using mxh::net::TcpClient;
using mxh::net::TcpServer;

std::vector<std::uint8_t> make_buf(std::size_t n, std::uint8_t seed) {
    std::vector<std::uint8_t> out(n);
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = static_cast<std::uint8_t>(seed + i);
    }
    return out;
}

int find_free_port() {
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

// Handler that supplies the shared HSEL cipher via encryptor_for and
// records received messages.
struct HselNetHandler final : IConnectionHandler {
    explicit HselNetHandler(HselStreamCipher* cipher) : cipher_(cipher) {}

    bool on_connect(ConnectionId id, const std::string&) override {
        connects.fetch_add(1);
        last_id.store(id.value);
        return true;
    }

    void on_message(ConnectionId id, const Message& msg) override {
        messages.fetch_add(1);
        std::lock_guard<std::mutex> lk(mu);
        received.push_back(msg);
        last_id.store(id.value);
    }

    void on_disconnect(ConnectionId, NetError) override {
        disconnects.fetch_add(1);
    }

    mxh::net::IEncryptor* encryptor_for(ConnectionId) override {
        return cipher_;
    }

    HselStreamCipher* cipher_ = nullptr;
    std::atomic<std::size_t> connects{0};
    std::atomic<std::size_t> messages{0};
    std::atomic<std::size_t> disconnects{0};
    std::atomic<std::uint64_t> last_id{0};
    std::vector<Message> received;
    std::mutex mu;
};

bool wait_for(const std::atomic<std::size_t>& counter, std::size_t target) {
    for (int i = 0; i < 100; ++i) {
        if (counter.load() >= target) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return counter.load() >= target;
}

}  // namespace

TEST(HselStreamCipher, PeerRoundTripPreservesSize) {
    // Legacy HSEL uses separate en/de streams: the encryptor's stream
    // must never decrypt, and the peer's decryptor must never encrypt.
    // Both sides advance their key schedule in lockstep (one op per
    // message), so a peer decrypt at the same state inverts the
    // originator's encrypt.
    HselStreamCipher en;
    EXPECT_FALSE(en.initialized());
    en.seed();
    ASSERT_TRUE(en.initialized());
    HselInit init{};
    ASSERT_TRUE(en.export_init(init));
    HselStreamCipher de;
    ASSERT_TRUE(de.import_init(init));

    for (const std::size_t n : {1u, 7u, 8u, 31u, 64u, 255u, 1024u}) {
        auto enc = make_buf(n, 0x5Au);
        const auto orig = enc;
        EXPECT_EQ(en.encrypt(enc), NetError::Ok);
        EXPECT_EQ(enc.size(), orig.size()) << "size n=" << n;
        if (n >= 8u) {
            EXPECT_NE(enc, orig) << "ciphertext must differ for n=" << n;
        }
        EXPECT_EQ(de.decrypt(enc), NetError::Ok);
        EXPECT_EQ(enc, orig) << "peer roundtrip failed for n=" << n;
    }
}

TEST(HselStreamCipher, UninitializedEncryptDecryptFail) {
    HselStreamCipher c;
    auto buf = make_buf(16, 1u);
    EXPECT_EQ(c.encrypt(buf), NetError::EncryptionFailed);
    EXPECT_EQ(c.decrypt(buf), NetError::DecryptionFailed);
    HselInit init{};
    EXPECT_FALSE(c.export_init(init));
}

TEST(HselStreamCipher, ExportImportIsDeterministic) {
    HselStreamCipher a;
    a.seed();
    HselInit init{};
    ASSERT_TRUE(a.export_init(init));
    // initial() resolved the RAND type and marked the keys customize.
    EXPECT_EQ(init.iCustomize, mxh::crypto::HSEL_KEY_TYPE_CUSTOMIZE);

    HselStreamCipher b;
    ASSERT_TRUE(b.import_init(init));
    ASSERT_TRUE(b.initialized());
    // Third cipher used for the peer decrypt so the decrypt stream is
    // fresh at the same key state as the encrypt (en/de separation).
    HselStreamCipher de;
    ASSERT_TRUE(de.import_init(init));

    const auto orig = make_buf(128, 0x11u);
    auto x = orig;
    auto y = orig;
    EXPECT_EQ(a.encrypt(x), NetError::Ok);
    EXPECT_EQ(b.encrypt(y), NetError::Ok);
    EXPECT_EQ(x, y) << "identical keys must produce identical ciphertext";
    EXPECT_EQ(de.decrypt(x), NetError::Ok);
    EXPECT_EQ(x, orig) << "peer decrypt must restore the original";
}

TEST(HselStreamCipher, ImportMatchesRawHselStream) {
    HselStreamCipher cipher;
    cipher.seed();
    HselInit init{};
    ASSERT_TRUE(cipher.export_init(init));

    HselStream raw;
    ASSERT_NE(raw.initial(init), 0);
    const auto orig = make_buf(96, 0x22u);
    auto via_adapter = orig;
    auto via_raw = orig;
    EXPECT_EQ(cipher.encrypt(via_adapter), NetError::Ok);
    ASSERT_TRUE(raw.encrypt(
        reinterpret_cast<char*>(via_raw.data()),
        static_cast<std::int32_t>(via_raw.size())));
    EXPECT_EQ(via_adapter, via_raw);
}

TEST(HselStreamCipher, MultiPacketSessionContinuity) {
    HselStreamCipher a;
    a.seed();
    HselInit init{};
    ASSERT_TRUE(a.export_init(init));
    HselStreamCipher b;
    ASSERT_TRUE(b.import_init(init));

    const auto p1 = make_buf(40, 0x33u);
    const auto p2 = make_buf(17, 0x44u);
    const auto p3 = make_buf(255, 0x55u);
    auto c1 = p1, c2 = p2, c3 = p3;
    EXPECT_EQ(a.encrypt(c1), NetError::Ok);
    EXPECT_EQ(a.encrypt(c2), NetError::Ok);
    EXPECT_EQ(a.encrypt(c3), NetError::Ok);

    EXPECT_EQ(b.decrypt(c1), NetError::Ok);
    EXPECT_EQ(b.decrypt(c2), NetError::Ok);
    EXPECT_EQ(b.decrypt(c3), NetError::Ok);
    EXPECT_EQ(c1, p1);
    EXPECT_EQ(c2, p2);
    EXPECT_EQ(c3, p3);
}

TEST(HselStreamCipher, EmptyPayloadIsNoOp) {
    HselStreamCipher c;
    c.seed();
    std::vector<std::uint8_t> empty;
    EXPECT_EQ(c.encrypt(empty), NetError::Ok);
    EXPECT_EQ(c.decrypt(empty), NetError::Ok);
    EXPECT_TRUE(empty.empty());
}

TEST(HselStreamCipher, ConsecutiveSeedsDiffer) {
    HselStreamCipher a;
    HselStreamCipher b;
    a.seed();
    b.seed();
    HselInit ia{};
    HselInit ib{};
    ASSERT_TRUE(a.export_init(ia));
    ASSERT_TRUE(b.export_init(ib));
    EXPECT_FALSE(std::memcmp(&ia, &ib, sizeof(ia)) == 0)
        << "two seed() calls must produce distinct sessions";
}

TEST(HselStreamCipher, NetLayerEncryptedRoundTripWithLegacyFraming) {
    // Pre-share the session key the way a handshake would: server seeds,
    // exports the resolved HselInit, client imports it.
    HselStreamCipher server_cipher;
    HselStreamCipher client_cipher;
    server_cipher.seed();
    HselInit init{};
    ASSERT_TRUE(server_cipher.export_init(init));
    ASSERT_TRUE(client_cipher.import_init(init));

    HselNetHandler server_handler(&server_cipher);
    TcpServer server(server_handler);
    const int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig scfg;
    scfg.port = static_cast<std::uint16_t>(port);
    scfg.worker_thread_count = 1;
    scfg.use_legacy_framing = true;
    scfg.use_encryption = true;
    ASSERT_EQ(server.start(scfg), NetError::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    HselNetHandler client_handler(&client_cipher);
    TcpClient client(client_handler);
    ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port);
    ccfg.use_legacy_framing = true;
    ccfg.use_encryption = true;
    ASSERT_EQ(client.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(wait_for(server_handler.connects, 1u));

    Message out;
    out.header.category = 0x12;
    out.header.protocol = 0x34;
    out.header.object_id = 0x10203040u;
    out.payload = make_buf(64, 0x66u);
    ASSERT_EQ(client.send(out), NetError::Ok);
    ASSERT_TRUE(wait_for(server_handler.messages, 1u));

    Message got;
    {
        std::lock_guard<std::mutex> lk(server_handler.mu);
        ASSERT_EQ(server_handler.received.size(), 1u);
        got = server_handler.received[0];
    }
    EXPECT_EQ(got.header.category, out.header.category);
    EXPECT_EQ(got.header.protocol, out.header.protocol);
    EXPECT_EQ(got.header.object_id, out.header.object_id);
    EXPECT_EQ(got.payload, out.payload);

    // Server reply (also encrypted) must round-trip back to the client.
    Message reply;
    reply.header.category = 0xAB;
    reply.header.protocol = 0xCD;
    reply.header.object_id = 0x55667788u;
    reply.payload = make_buf(32, 0x77u);
    const ConnectionId server_conn{server_handler.last_id.load()};
    ASSERT_EQ(server.send(server_conn, reply), NetError::Ok);
    ASSERT_TRUE(wait_for(client_handler.messages, 1u));
    {
        std::lock_guard<std::mutex> lk(client_handler.mu);
        ASSERT_EQ(client_handler.received.size(), 1u);
        const Message& got_reply = client_handler.received[0];
        EXPECT_EQ(got_reply.header.category, reply.header.category);
        EXPECT_EQ(got_reply.header.protocol, reply.header.protocol);
        EXPECT_EQ(got_reply.header.object_id, reply.header.object_id);
        EXPECT_EQ(got_reply.payload, reply.payload);
    }

    client.disconnect();
    server.stop();
}
