// crypto.hpp - AES-256-GCM via Windows CNG (bcrypt.dll).
//
// Phase 3: replaces the HSEL physical encryption dongle with a software-only
// AES-256-GCM implementation using the Windows CNG API (bcrypt.dll).
// No external dependencies — bcrypt.dll ships with Windows Vista+.
//
// Original: 墨香【源码】\[Lib]HSEL\HSEL_STREAM.cpp - physical DES Triple encryption
// dongle (EOL, HSEL.lib cannot link on Windows 11). We replace with a software-only
// implementation that preserves the IEncryptor interface.
//
// AES-256-GCM protocol:
//   encrypt: AES-256-CTR + HMAC-SHA-256 (combined by CNG BCRYPT_AUTH_MODE_GCM).
//            Output = ciphertext + 16-byte auth tag appended.
//   decrypt: verifies auth tag before returning plaintext. Returns error on tamper.
//   IV: 96-bit per-packet nonce (random seed + counter, big-endian in last 4 bytes).
//   Key: 256-bit session key (random seed per connection).
//
// For Linux cross-platform, replace BCrypt* calls with OpenSSL EVP_AEAD (AES-256-GCM).
// The IEncryptor interface is unchanged.

#pragma once

#include "mxh/net/net.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace mxh::crypto {

// Opaque BCrypt handle types (bcrypt.dll treats these as void*).
using BCRYPT_ALG_HANDLE = void*;
using BCRYPT_KEY_HANDLE = void*;

// AES-256-GCM cipher using Windows CNG (bcrypt.dll).
// Drop-in replacement for HSEL-compatible stream cipher.
class Aes256GcmCipher final : public mxh::net::IEncryptor {
public:
    // kKeyBytes  = 32 (AES-256)
    // kIvBytes   = 12 (GCM nonce = 96 bits)
    // kTagBytes  = 16 (GCM auth tag = 128 bits)
    static constexpr std::uint32_t kKeyBytes  = 32;
    static constexpr std::uint32_t kIvBytes   = 12;
    static constexpr std::uint32_t kTagBytes  = 16;

    Aes256GcmCipher();
    ~Aes256GcmCipher() override;

    // Generate fresh random 256-bit key + 96-bit IV (matches HSEL ENCRYPTTYPE_RAND).
    void seed() override;

    // Encrypt in place: appends 16-byte GCM auth tag after ciphertext.
    //   Input size  N → output size N+16 (tag appended in-place).
    //   Caller must ensure data span has kTagBytes extra space at the end.
    mxh::net::NetError encrypt(std::span<std::uint8_t> data) override;

    // Decrypt in place: verifies auth tag, strips it (data size reduced by 16).
    //   Returns CryptoError if authentication fails (tampering detected).
    mxh::net::NetError decrypt(std::span<std::uint8_t> data) override;

    // Export 256-bit session key (for handshake / secure key exchange).
    // Returns false if the cipher is not initialized.
    bool export_key(std::array<std::uint8_t, kKeyBytes>& out) const;

    // Import 256-bit session key (from handshake / secure key exchange).
    bool import_key(const std::array<std::uint8_t, kKeyBytes>& key);

    // Export 96-bit IV state (for multi-packet session continuity).
    bool export_iv(std::array<std::uint8_t, kIvBytes>& out) const;

    // Import 96-bit IV state (restore counter from saved session).
    bool import_iv(const std::array<std::uint8_t, kIvBytes>& iv);

    // True if BCrypt initialized successfully.
    bool ok() const { return m_initOk; }

private:
    bool init_key(const std::uint8_t* key_bytes, std::uint32_t key_len);

    bool              m_initOk = false;
    BCRYPT_ALG_HANDLE m_aesAlg = nullptr;
    BCRYPT_KEY_HANDLE m_key    = nullptr;
    std::uint8_t      m_iv[kIvBytes] = {};
    std::uint32_t     m_counter = 0;  // packet counter
};

}  // namespace mxh::crypto
