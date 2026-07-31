#pragma once
// MurimNetCrypt: 1:1 byte-port of legacy [Server]MurimNet/Crypt.cpp CCrypt class.
// Wraps mxh::crypto::HselStream to provide a CCrypt-compatible Init/Encrypt/Decrypt
// surface so the modern MurimNet network layer can encrypt wire bytes the same way
// the legacy client expects. This is a wire-encoding wrapper; the underlying HSEL
// stream cipher itself is fully implemented in modern/include/mxh/crypto/hsel_stream.hpp.
#include "mxh/crypto/hsel_stream.hpp"
#include <cstdint>
namespace mxh::server {
class MurimNetCrypt final {
public:
    MurimNetCrypt() = default;
    ~MurimNetCrypt() = default;
    // Generate a fresh random keypair (HSEL_ENCRYPTTYPE_RAND + TRIPLE_DES + default key + SWAP on).
    // Returns true on success.
    bool Create();
    // Initialize with explicit keys. The legacy semantics swap: server dekey is
    // the client enkey and vice versa, so the encrypt stream takes the de-init and
    // the decrypt stream takes the en-init. Both streams initial() must succeed.
    bool Init(const mxh::crypto::HselInit& en_init, const mxh::crypto::HselInit& de_init);
    // In-place encrypt / decrypt. Returns true on success, false if not initialized
    // or the underlying stream rejects the buffer.
    bool Encrypt(char* buf, int32_t size);
    bool Decrypt(char* buf, int32_t size);
    // CRC accessors (1 byte per legacy CCrypt::GetEnCRCConvertChar pattern).
    std::int8_t GetEnCrcChar() const;
    std::int8_t GetDeCrcChar() const;
    bool IsInitialized() const noexcept { return m_bInited; }
private:
    mxh::crypto::HselStream m_hEnStream;
    mxh::crypto::HselStream m_hDeStream;
    bool m_bInited = false;
};
}
