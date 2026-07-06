// crypto.cpp - HSEL-compatible cipher implementation.

#include "mxh/crypto/crypto.hpp"

#include <cstring>
#include <random>

namespace mxh::crypto {

namespace {

// Lightweight, deterministic keystream (NOT secure for production).
// Uses a Feistel-like round on the counter and key.
// This matches HSEL semantics: encryption = decryption (XOR with keystream).
void mix(std::uint8_t* block, const std::uint8_t* key, std::uint64_t counter) {
    std::uint64_t k = 0;
    std::memcpy(&k, key, 8);
    std::uint64_t b = counter;
    for (int round = 0; round < 8; ++round) {
        b ^= k;
        b = (b << 13) | (b >> 51);
        b *= 0x9E3779B97F4A7C15ULL;
        k = (k << 7) | (k >> 57);
        k ^= b;
    }
    std::memcpy(block, &b, 8);
    std::uint64_t c = counter + 0x123456789ABCDEF0ULL;
    for (int round = 0; round < 8; ++round) {
        c ^= ((std::uint64_t*)key)[1];
        c = (c << 17) | (c >> 47);
        c *= 0xBF58476D1CE4E5B9ULL;
    }
    std::memcpy(block + 8, &c, 8);
}

}  // namespace

HselCompatCipher::HselCompatCipher() {
    seed();
}

void HselCompatCipher::seed() {
    // Use a high-entropy RNG to generate key/iv (similar to HSEL random seed).
    std::random_device rd;
    for (auto& b : key_) b = static_cast<std::uint8_t>(rd() & 0xFF);
    for (auto& b : iv_) b = static_cast<std::uint8_t>(rd() & 0xFF);
    counter_ = 0;
}

void HselCompatCipher::export_key(std::array<std::uint8_t, 16>& out) const {
    std::memcpy(out.data(), key_.data(), 16);
}

void HselCompatCipher::import_key(const std::array<std::uint8_t, 16>& key) {
    key_ = key;
    counter_ = 0;
}

void HselCompatCipher::keystream_block(std::uint8_t out[16]) {
    mix(out, key_.data(), counter_);
    for (int i = 0; i < 16; ++i) out[i] ^= iv_[i];
    counter_++;
}

mxh::net::NetError HselCompatCipher::encrypt(std::span<std::uint8_t> data) {
    std::uint8_t ks[16];
    std::size_t off = 0;
    while (off < data.size()) {
        keystream_block(ks);
        std::size_t n = std::min<std::size_t>(16, data.size() - off);
        for (std::size_t i = 0; i < n; ++i) data[off + i] ^= ks[i];
        off += n;
    }
    return mxh::net::NetError::Ok;
}

mxh::net::NetError HselCompatCipher::decrypt(std::span<std::uint8_t> data) {
    // Symmetric stream cipher.
    return encrypt(data);
}

}  // namespace mxh::crypto