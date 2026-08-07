//
// E2 T2 capture handler -- wraps an IConnectionHandler, records every
// on_message call as (timestamp, connection, wire_bytes), and exposes
// a SHA-256 fingerprint over the full byte sequence. The fingerprint
// is the regression anchor for E2 T2 (wire SHA-256 replay):
//
//   - Capture against the modern server -> save -> reload -> SHA-256
//     must equal the SHA-256 of any replay run. Drift = bug.
//   - Capture against legacy server (SWorking/*) and modern server;
//     the SHA-256 of identical scenarios must match (modulo encryption).
//
// Wire-format is byte-identical to what TcpServer actually transmits
// (length prefix + 8B MSGBASE + payload) so the captured bytes can
// be diffed against MoxianSideBySide golden traces.
//
// Captured packets are stored in a thread-safe buffer; the snapshot()
// returns a copy. The handler forwards every on_connect / on_message /
// on_disconnect to the inner handler so production behavior is
// preserved unchanged when capture is wrapped around a real server.
#pragma once

#include "mxh/net/net.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::net {

// 32-byte SHA-256 digest. Hex-formatted helper for logging/filenames.
struct Sha256Digest {
    static constexpr std::size_t kBytes = 32;
    std::array<std::uint8_t, kBytes> bytes{};
    [[nodiscard]] std::string to_hex() const;
    [[nodiscard]] bool operator==(const Sha256Digest& o) const noexcept;
};

// Compute SHA-256 over a single byte buffer. Uses Windows BCrypt when
// available; falls back to a pure-C++ SHA-256 if BCrypt fails. Pure
// implementation is bit-identical to FIPS 180-4 so the hex output is
// portable across both paths.
Sha256Digest sha256(std::span<const std::uint8_t> data);
Sha256Digest sha256_concat(const std::vector<std::vector<std::uint8_t>>& chunks);

// One captured packet: timestamp + connection + decoded Message + wire bytes.
struct CapturedPacket {
    std::uint64_t      timestamp_ns = 0;
    ConnectionId        connection{};
    Message             message{};
    std::vector<std::uint8_t> wire_bytes;
};

// Decorator: any IConnectionHandler wrapped by CaptureHandler records
// every on_message + forwards it to the inner handler. Recording is
// mutex-guarded so concurrent on_message calls (TcpServer worker threads)
// cannot corrupt the captured sequence.
class CaptureHandler final : public IConnectionHandler {
public:
    explicit CaptureHandler(IConnectionHandler& inner) : inner_(inner) {}

    // IConnectionHandler interface (forwards to inner_ after recording)
    bool on_connect(ConnectionId id, const std::string& remote_addr) override;
    void on_message(ConnectionId id, const Message& msg) override;
    void on_disconnect(ConnectionId id, NetError reason) override;

    // Capture access (copy out for thread safety).
    [[nodiscard]] std::vector<CapturedPacket> snapshot() const;
    void clear();
    [[nodiscard]] std::size_t size() const noexcept;

    // SHA-256 fingerprint over the concatenation of all captured wire
// bytes in capture order. Used by E2 T2 wire SHA-256 replay to lock
// byte-stability across runs.
    [[nodiscard]] Sha256Digest fingerprint() const;

    // Persist / restore capture to disk. Format is line-based: each line
    // = `<ts_ns> <connection_value> <hex_wire_bytes>`. Lines starting
    // with `#` are comments. Empty lines are skipped. Round-trip
    // preserves wire bytes exactly (load() re-derives SHA-256 from the
    // file content, so it must equal fingerprint() at save time).
    [[nodiscard]] bool save(std::string_view path) const;
    [[nodiscard]] bool load(std::string_view path);

private:
    IConnectionHandler&       inner_;
    mutable std::mutex        mu_;
    std::vector<CapturedPacket> captured_;
    std::atomic<std::uint64_t> ts_counter_{0};
};

}  // namespace mxh::net
