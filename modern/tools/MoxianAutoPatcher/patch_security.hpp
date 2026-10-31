#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif

namespace mxh::patch {
namespace fs = std::filesystem;

inline bool is_safe_relative_path(const fs::path& path) {
    if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory())
        return false;
    for (const auto& part : path) {
        if (part == ".." || part == "." || part.empty()) return false;
        // Windows alternate data streams (for example `client.exe:stream`)
        // are not ordinary patch files and can bypass an apparent directory
        // boundary.  Reject any colon in a manifest component, including a
        // drive-like prefix that some path implementations do not classify
        // as a root name until it is combined with another path.
        if (part.native().find(':') != std::string::npos) return false;
    }
    return true;
}

inline bool is_user_data_path(const fs::path& path) {
    if (!is_safe_relative_path(path)) return false;
    auto value = path.generic_string();
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    // Updates must never remove settings, screenshots, logs, or the
    // updater's rollback journal/backup.  These paths are outside the
    // signed game-file payload and survive repair/rollback by design.
    return value == "_backup" || value.rfind("_backup/", 0) == 0 ||
           value == "moxian/settings.json" || value.rfind("moxian/", 0) == 0 ||
           value == "screenshots" || value.rfind("screenshots/", 0) == 0 ||
           value == "logs" || value.rfind("logs/", 0) == 0 ||
           value == ".update-journal";
}

inline std::string lower_hex(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

#ifndef _WIN32
namespace detail {

// FIPS 180-4 SHA-256. Used only where BCrypt is unavailable so file
// verification matches the Windows digest byte for byte.
constexpr std::uint32_t kSha256K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

inline std::uint32_t rotr32(std::uint32_t x, std::uint32_t n) noexcept {
    return (x >> n) | (x << (32u - n));
}

inline void sha256_compress(std::uint32_t h[8], const std::uint8_t block[64]) {
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

struct Sha256Stream {
    std::uint32_t h[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
    };
    std::uint8_t block[64] = {};
    std::size_t block_len = 0;
    std::uint64_t total = 0;

    void update(const std::uint8_t* data, std::size_t len) {
        total += static_cast<std::uint64_t>(len);
        while (len > 0) {
            const std::size_t n = std::min(len, static_cast<std::size_t>(64u) - block_len);
            std::memcpy(block + block_len, data, n);
            block_len += n;
            data += n;
            len -= n;
            if (block_len == 64u) {
                sha256_compress(h, block);
                block_len = 0;
            }
        }
    }

    std::string finish() {
        const std::uint64_t bit_len = total * 8u;
        std::uint8_t tail[128] = {};
        std::memcpy(tail, block, block_len);
        tail[block_len] = 0x80u;
        const std::size_t pad_end = (block_len < 56u) ? 64u : 128u;
        for (int b = 0; b < 8; ++b) {
            tail[pad_end - 1 - static_cast<std::size_t>(b)] =
                static_cast<std::uint8_t>((bit_len >> (8 * b)) & 0xFFu);
        }
        sha256_compress(h, tail);
        if (pad_end == 128u) sha256_compress(h, tail + 64);
        std::ostringstream output;
        output << std::hex << std::setfill('0');
        for (int k = 0; k < 8; ++k) {
            output << std::setw(2) << ((h[k] >> 24) & 0xFFu)
                   << std::setw(2) << ((h[k] >> 16) & 0xFFu)
                   << std::setw(2) << ((h[k] >> 8) & 0xFFu)
                   << std::setw(2) << (h[k] & 0xFFu);
        }
        return output.str();
    }
};

}  // namespace detail
#endif

inline std::string sha256_file(const fs::path& path) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD object_size = 0, bytes = 0, digest_size = 0;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        return {};
    if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                          reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &bytes, 0) < 0 ||
        BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
                          reinterpret_cast<PUCHAR>(&digest_size), sizeof(digest_size), &bytes, 0) < 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return {};
    }
    std::vector<unsigned char> object(object_size), digest(digest_size), buffer(1024 * 1024);
    if (BCryptCreateHash(algorithm, &hash, object.data(), object_size, nullptr, 0, 0) < 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return {};
    }
    std::ifstream input(path, std::ios::binary);
    while (input) {
        input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count > 0 && BCryptHashData(hash, buffer.data(), static_cast<ULONG>(count), 0) < 0) {
            BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(algorithm, 0); return {};
        }
    }
    const bool ok = input.eof() && BCryptFinishHash(hash, digest.data(), digest_size, 0) >= 0;
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!ok) return {};
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (unsigned char value : digest) output << std::setw(2) << static_cast<unsigned>(value);
    return output.str();
#else
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    detail::Sha256Stream stream;
    std::vector<unsigned char> buffer(1024 * 1024);
    while (input) {
        input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count > 0) stream.update(buffer.data(), static_cast<std::size_t>(count));
    }
    if (!input.eof()) return {};
    return stream.finish();
#endif
}

inline bool verify_file(const fs::path& path, std::uint64_t expected_size,
                        std::string_view expected_sha256) {
    std::error_code error;
    if (!fs::is_regular_file(path, error) || error || fs::file_size(path, error) != expected_size || error)
        return false;
    return lower_hex(sha256_file(path)) == lower_hex(std::string(expected_sha256));
}

} // namespace mxh::patch
