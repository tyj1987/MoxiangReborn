//
// E2 T2 capture handler implementation.
//
// SHA-256 is implemented in pure C++ (FIPS 180-4) so the fingerprint
// is byte-identical across platforms and free of any SDK or dynamic
// loader dependency. The crypto library's BCrypt usage stays scoped to
// AES-256-GCM.
#include "mxh/net/capture_handler.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <vector>

namespace mxh::net {

// ===== Sha256Digest =====

std::string Sha256Digest::to_hex() const {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(kBytes * 2u);
    for (auto b : bytes) {
        out.push_back(kHex[(b >> 4) & 0x0F]);
        out.push_back(kHex[b & 0x0F]);
    }
    return out;
}

bool Sha256Digest::operator==(const Sha256Digest& o) const noexcept {
    return bytes == o.bytes;
}

// ===== Pure-C++ SHA-256 (FIPS 180-4) =====

static constexpr std::uint32_t kSha256K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

static inline std::uint32_t rotr32(std::uint32_t x, std::uint32_t n) noexcept {
    return (x >> n) | (x << (32u - n));
}

static void sha256_compress(std::uint32_t h[8], const std::uint8_t block[64]) {
    std::uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(block[i * 4 + 0]) << 24) |
               (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
               (static_cast<std::uint32_t>(block[i * 4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        const std::uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const std::uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
    std::uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int i = 0; i < 64; ++i) {
        const std::uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        const std::uint32_t ch = (e & f) ^ ((~e) & g);
        const std::uint32_t t1 = hh + S1 + ch + kSha256K[i] + w[i];
        const std::uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        const std::uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t t2 = S0 + mj;
        hh = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

Sha256Digest sha256(std::span<const std::uint8_t> data) {
    std::uint32_t h[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
    };
    const std::uint64_t bit_len = static_cast<std::uint64_t>(data.size()) * 8u;

    std::size_t i = 0;
    while (i + 64u <= data.size()) {
        sha256_compress(h, data.data() + i);
        i += 64u;
    }

    std::array<std::uint8_t, 128> tail{};
    const std::size_t rem = data.size() - i;
    for (std::size_t k = 0; k < rem; ++k) tail[k] = data[i + k];
    tail[rem] = 0x80u;
    const std::size_t pad_end = (rem < 56u) ? 64u : 128u;
    for (int b = 0; b < 8; ++b) {
        tail[pad_end - 1 - b] = static_cast<std::uint8_t>((bit_len >> (b * 8)) & 0xFFu);
    }
    sha256_compress(h, tail.data());
    if (pad_end == 128u) sha256_compress(h, tail.data() + 64u);

    Sha256Digest out{};
    for (int k = 0; k < 8; ++k) {
        out.bytes[k * 4 + 0] = static_cast<std::uint8_t>((h[k] >> 24) & 0xFFu);
        out.bytes[k * 4 + 1] = static_cast<std::uint8_t>((h[k] >> 16) & 0xFFu);
        out.bytes[k * 4 + 2] = static_cast<std::uint8_t>((h[k] >> 8) & 0xFFu);
        out.bytes[k * 4 + 3] = static_cast<std::uint8_t>(h[k] & 0xFFu);
    }
    return out;
}

Sha256Digest sha256_concat(const std::vector<std::vector<std::uint8_t>>& chunks) {
    std::size_t total = 0;
    for (const auto& c : chunks) total += c.size();
    std::vector<std::uint8_t> buf(total);
    std::size_t off = 0;
    for (const auto& c : chunks) {
        std::copy(c.begin(), c.end(), buf.begin() + off);
        off += c.size();
    }
    return sha256(std::span<const std::uint8_t>(buf.data(), buf.size()));
}

// ===== CaptureHandler =====

// Reconstruct the on-the-wire byte sequence from a Message so capture
// is byte-identical to what TcpServer.send/recv actually transmits.
// Format: 2B length prefix (LE u16) + 8B MSGBASE + payload.
static std::vector<std::uint8_t> encode_wire(const Message& msg) {
    const auto body_len = static_cast<std::uint16_t>(
        sizeof(MsgHeader) + msg.payload.size());
    std::vector<std::uint8_t> out(2u + body_len);
    out[0] = static_cast<std::uint8_t>(body_len & 0xFFu);
    out[1] = static_cast<std::uint8_t>((body_len >> 8) & 0xFFu);
    out[2] = msg.header.checksum;
    out[3] = static_cast<std::uint8_t>(msg.header.code);
    out[4] = msg.header.category;
    out[5] = msg.header.protocol;
    out[6] = static_cast<std::uint8_t>(msg.header.object_id & 0xFFu);
    out[7] = static_cast<std::uint8_t>((msg.header.object_id >> 8) & 0xFFu);
    out[8] = static_cast<std::uint8_t>((msg.header.object_id >> 16) & 0xFFu);
    out[9] = static_cast<std::uint8_t>((msg.header.object_id >> 24) & 0xFFu);
    std::copy(msg.payload.begin(), msg.payload.end(), out.begin() + 10);
    return out;
}

bool CaptureHandler::on_connect(ConnectionId id, const std::string& remote_addr) {
    return inner_.on_connect(id, remote_addr);
}

void CaptureHandler::on_disconnect(ConnectionId id, NetError reason) {
    inner_.on_disconnect(id, reason);
}

void CaptureHandler::on_message(ConnectionId id, const Message& msg) {
    const auto ts = ++ts_counter_;
    const auto wire = encode_wire(msg);
    {
        std::lock_guard<std::mutex> lock(mu_);
        CapturedPacket p;
        p.timestamp_ns = ts;
        p.connection = id;
        p.message = msg;
        p.wire_bytes = wire;
        captured_.push_back(std::move(p));
    }
    inner_.on_message(id, msg);
}

std::vector<CapturedPacket> CaptureHandler::snapshot() const {
    std::lock_guard<std::mutex> lock(mu_);
    return captured_;
}

void CaptureHandler::clear() {
    std::lock_guard<std::mutex> lock(mu_);
    captured_.clear();
    ts_counter_.store(0);
}

std::size_t CaptureHandler::size() const noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    return captured_.size();
}

Sha256Digest CaptureHandler::fingerprint() const {
    std::vector<std::vector<std::uint8_t>> chunks;
    {
        std::lock_guard<std::mutex> lock(mu_);
        chunks.reserve(captured_.size());
        for (const auto& p : captured_) chunks.push_back(p.wire_bytes);
    }
    return sha256_concat(chunks);
}

static std::string to_hex(const std::vector<std::uint8_t>& v) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string s;
    s.reserve(v.size() * 2u);
    for (auto b : v) {
        s.push_back(kHex[(b >> 4) & 0x0F]);
        s.push_back(kHex[b & 0x0F]);
    }
    return s;
}

static std::vector<std::uint8_t> from_hex(std::string_view s) {
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::vector<std::uint8_t> out;
    out.reserve(s.size() / 2u);
    for (std::size_t i = 0; i + 1 < s.size(); i += 2) {
        const int hi = nibble(s[i]);
        const int lo = nibble(s[i + 1]);
        if (hi < 0 || lo < 0) return {};
        out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }
    return out;
}

bool CaptureHandler::save(std::string_view path) const {
    std::ofstream out(std::string(path), std::ios::binary);
    if (!out) return false;
    out << "# MoxianCapture v1\n";
    std::vector<CapturedPacket> copy;
    {
        std::lock_guard<std::mutex> lock(mu_);
        copy = captured_;
    }
    for (const auto& p : copy) {
        out << p.timestamp_ns << ' ' << p.connection.value << ' '
            << to_hex(p.wire_bytes) << "\n";
    }
    return out.good();
}

bool CaptureHandler::load(std::string_view path) {
    std::ifstream in(std::string(path), std::ios::binary);
    if (!in) return false;
    std::vector<CapturedPacket> loaded;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream is(line);
        std::uint64_t ts;
        std::uint64_t conn_val;
        std::string hex;
        if (!(is >> ts >> conn_val >> hex)) return false;
        auto bytes = from_hex(hex);
        if (bytes.size() < 10u) return false;
        CapturedPacket p;
        p.timestamp_ns = ts;
        p.connection = make_connection_id(conn_val);
        p.message.header.checksum = bytes[2];
        p.message.header.code = static_cast<std::int8_t>(bytes[3]);
        p.message.header.category = bytes[4];
        p.message.header.protocol = bytes[5];
        p.message.header.object_id =
            static_cast<std::uint32_t>(bytes[6]) |
            (static_cast<std::uint32_t>(bytes[7]) << 8) |
            (static_cast<std::uint32_t>(bytes[8]) << 16) |
            (static_cast<std::uint32_t>(bytes[9]) << 24);
        p.message.payload.assign(bytes.begin() + 10, bytes.end());
        p.wire_bytes = std::move(bytes);
        loaded.push_back(std::move(p));
    }
    {
        std::lock_guard<std::mutex> lock(mu_);
        captured_ = std::move(loaded);
        ts_counter_.store(captured_.empty() ? 0 : captured_.back().timestamp_ns);
    }
    return true;
}

}  // namespace mxh::net
