#pragma once

// Shared payload SHA-256 for the resource manifest test and mxh_payload_dump.
// Windows uses BCrypt. Other hosts use FIPS 180-4, bit-identical to mxh::net::sha256.
// One implementation so the test and the dump tool cannot drift.

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif

#include <array>
#include <cstdint>
#include <iomanip>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma comment(lib, "bcrypt.lib")
#endif

namespace mxh::test {

inline std::string sha256_bytes(std::span<const std::uint8_t> bytes) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (status != 0) return {};

    DWORD object_length = 0;
    DWORD result_length = 0;
    status = BCryptGetProperty(
        algorithm, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&object_length), sizeof(object_length),
        &result_length, 0);
    if (status != 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return {};
    }

    std::vector<UCHAR> object(object_length);
    std::array<UCHAR, 32> digest{};
    status = BCryptCreateHash(
        algorithm, &hash, object.data(), object_length, nullptr, 0, 0);
    if (status != 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return {};
    }

    if (!bytes.empty()) {
        status = BCryptHashData(
            hash,
            reinterpret_cast<PUCHAR>(const_cast<std::uint8_t*>(bytes.data())),
            static_cast<ULONG>(bytes.size()), 0);
    }
    if (status == 0) {
        status = BCryptFinishHash(
            hash, digest.data(), static_cast<ULONG>(digest.size()), 0);
    }
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status != 0) return {};

    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (const auto byte : digest) {
        result << std::setw(2) << static_cast<unsigned>(byte);
    }
    return result.str();
#else
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
    constexpr auto rotr32 = [](std::uint32_t x, std::uint32_t n) noexcept {
        return (x >> n) | (x << (32u - n));
    };
    const auto compress = [&](std::uint32_t h[8], const std::uint8_t block[64]) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(block[i * 4 + 0]) << 24) |
                   (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
                   static_cast<std::uint32_t>(block[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        std::uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t t1 = hh + s1 + ch + kSha256K[i] + w[i];
            const std::uint32_t s0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t t2 = s0 + maj;
            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    };

    std::uint32_t h[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
    };
    const std::uint64_t bit_len = static_cast<std::uint64_t>(bytes.size()) * 8u;
    std::size_t offset = 0;
    while (offset + 64u <= bytes.size()) {
        compress(h, bytes.data() + offset);
        offset += 64u;
    }
    std::array<std::uint8_t, 128> tail{};
    const std::size_t rem = bytes.size() - offset;
    for (std::size_t i = 0; i < rem; ++i) tail[i] = bytes[offset + i];
    tail[rem] = 0x80u;
    const std::size_t pad_end = (rem < 56u) ? 64u : 128u;
    for (int b = 0; b < 8; ++b) {
        tail[pad_end - 1 - static_cast<std::size_t>(b)] =
            static_cast<std::uint8_t>((bit_len >> (b * 8)) & 0xFFu);
    }
    compress(h, tail.data());
    if (pad_end == 128u) compress(h, tail.data() + 64u);

    std::array<std::uint8_t, 32> digest{};
    for (int k = 0; k < 8; ++k) {
        digest[static_cast<std::size_t>(k) * 4 + 0] = static_cast<std::uint8_t>((h[k] >> 24) & 0xFFu);
        digest[static_cast<std::size_t>(k) * 4 + 1] = static_cast<std::uint8_t>((h[k] >> 16) & 0xFFu);
        digest[static_cast<std::size_t>(k) * 4 + 2] = static_cast<std::uint8_t>((h[k] >> 8) & 0xFFu);
        digest[static_cast<std::size_t>(k) * 4 + 3] = static_cast<std::uint8_t>(h[k] & 0xFFu);
    }
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (const auto byte : digest) {
        result << std::setw(2) << static_cast<unsigned>(byte);
    }
    return result.str();
#endif
}

}  // namespace mxh::test
