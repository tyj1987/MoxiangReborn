// hsel_encryptor.hpp - HSEL stream cipher as an mxh::net::IEncryptor.
//
// Phase R-1: wires the byte-compatible HselStream cipher (hsel_stream.hpp)
// into the modern net encryption hook so `use_encryption=true` can run
// the legacy HSEL transform end-to-end. The transform is size-preserving,
// unlike Aes256GcmCipher which appends/strips a 16-byte auth tag.
//
// Legacy stream semantics: HSEL uses SEPARATE en/de streams per side. A
// stream that encrypts must never decrypt (and vice versa); both peers
// advance their key schedule in lockstep, one operation per message. The
// net layer satisfies this naturally -- each side runs exactly one
// encrypt or decrypt per message in its direction -- so a single
// HselStreamCipher per connection is correct there. Same-stream
// encrypt-then-decrypt is NOT a valid usage pattern (matches legacy).
//
// Session keys: seed() generates a fresh random HselInit (RAND type,
// triple DES, swap on, default key customization) seeded from the steady
// clock, exactly mirroring legacy CCrypt::Create() semantics. The fully
// resolved HselInit (concrete encrypt type + 12 key fields, 64 bytes) can
// be exported for a handshake and imported on the peer so both ends
// derive identical ciphertext.

#pragma once

#include "mxh/crypto/hsel_stream.hpp"
#include "mxh/net/net.hpp"

#include <cstdint>
#include <span>

namespace mxh::crypto {

// IEncryptor adapter over the legacy HSEL stream cipher.
class HselStreamCipher final : public mxh::net::IEncryptor {
public:
    // HselInit = 16 x int32 = 64 bytes of handshake key material.
    static constexpr std::uint32_t kInitBytes = 64u;

    HselStreamCipher() = default;
    ~HselStreamCipher() override = default;

    // Generate a fresh random session (RAND type, triple DES, swap on,
    // default keys) seeded from the steady clock + a monotonic counter so
    // two consecutive seeds never collide.
    void seed() override;

    // In-place size-preserving encrypt/decrypt. Returns Ok on success,
    // EncryptionFailed/DecryptionFailed on a stream error. Before
    // seed()/import_init() the calls are pass-through Ok -- 1:1 with
    // legacy CCrypt::Encrypt/Decrypt (`if (!m_bInited) return TRUE`),
    // which lets the key-delivery message ride the plaintext phase.
    // Empty spans are a no-op Ok (matches legacy zero-size skip).
    mxh::net::NetError encrypt(std::span<std::uint8_t> data) override;
    mxh::net::NetError decrypt(std::span<std::uint8_t> data) override;

    // Handshake helpers: export/import the fully resolved HselInit.
    // export_init returns false until seed()/import_init() succeeded.
    bool export_init(HselInit& out) const;
    bool import_init(const HselInit& init);

    // Drop the session and return to the unseeded pass-through state.
    // Used by the key-delivery handshake: seed -> export -> reset ->
    // send key message (plaintext) -> import (re-arm with same keys).
    void reset() noexcept;

    bool initialized() const noexcept { return ready_; }
    const HselInit& init() const noexcept { return init_; }

private:
    HselStream stream_;
    HselInit   init_{};
    bool       ready_ = false;
};

}  // namespace mxh::crypto
