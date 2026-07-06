// crypto.hpp - Phase 3 encryption replacement for HSEL.
//
// Original: 墨香【源码】\[Lib]HSEL\HSEL_STREAM.cpp - physical encryption
// dongle (DES triple + random keys). The dongle is EOL; we replace with
// a software-only implementation that preserves the IEncryptor interface
// and protocol-compatible behavior.
//
// Current implementation: stream cipher with random per-session keys
// (HSEL_ENCRYPTTYPE_RAND compatible). Production should swap in AES-256-GCM
// from OpenSSL/libsodium by reimplementing IEncryptor without changing the
// interface.

#pragma once

#include "mxh/net/net.hpp"

#include <array>
#include <cstdint>
#include <random>
#include <span>

namespace mxh::crypto {

// HSEL-compatible stream cipher (Phase 3.0 placeholder).
// This is a software-only replacement for the physical encryption dongle.
// Replace with AES-256-GCM (OpenSSL) for production hardening.
class HselCompatCipher final : public mxh::net::IEncryptor {
public:
    HselCompatCipher();
    ~HselCompatCipher() override = default;

    // Generate fresh random keys (matches HSEL HSEL_ENCRYPTTYPE_RAND).
    void seed() override;

    // Encrypt in place.
    mxh::net::NetError encrypt(std::span<std::uint8_t> data) override;
    mxh::net::NetError decrypt(std::span<std::uint8_t> data) override;

    // Export/import keys (for handshake between client and server).
    void export_key(std::array<std::uint8_t, 16>& out) const;
    void import_key(const std::array<std::uint8_t, 16>& key);

private:
    std::array<std::uint8_t, 16> key_{};
    std::array<std::uint8_t, 16> iv_{};
    std::uint64_t counter_ = 0;

    void keystream_block(std::uint8_t out[16]);
};

}  // namespace mxh::crypto