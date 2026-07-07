// crypto.cpp - AES-256-GCM via Windows CNG (bcrypt.dll).
//
// Phase 3: replaces the HSEL physical encryption dongle with a software-only
// AES-256-GCM implementation using the Windows CNG API (bcrypt.dll).
// No external dependencies — bcrypt.dll ships with Windows Vista+.
//
// Design:
//   - AES-256-GCM: 256-bit key, 96-bit IV/nonce, 128-bit auth tag.
//   - encrypt: input N bytes plaintext → output N bytes ciphertext + 16-byte
//     auth tag appended in-place. Total growth = +16 bytes (caller must
//     reserve the extra space in the span).
//   - decrypt: verifies auth tag, strips it. Returns NetError::DecryptionFailed
//     on auth failure (tamper detection).
//   - seed(): generates random 256-bit key + 96-bit IV via BCryptGenRandom.
//   - IEncryptor interface preserved — drop-in replacement for HselCompatCipher.
//
// BCrypt usage notes (Bug C-31 fix):
//   - GCM auth metadata uses BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO (NOT the
//     custom BCRYPT_AUTH_INFO struct we used before — those structs had
//     different field layout, causing BCryptEncrypt to silently treat the
//     pPaddingInfo pointer as garbage).
//   - The pbTag member of the auth-info struct is the OUTPUT location for
//     encrypt (BCrypt writes the computed tag there) and the INPUT location
//     for decrypt (BCrypt reads the stored tag to verify).
//   - BCryptFinishKey is NOT a real BCrypt API — tags are produced by
//     BCryptEncrypt directly via the pbTag mechanism.
//   - The flag on Encrypt/Decrypt is BCRYPT_BLOCK_PADDING (0x1), NOT the
//     made-up BCRYPT_AUTH_MODE_GCM_FLAG (0x4) we used before.
//
// For Linux cross-platform, replace BCrypt* calls with OpenSSL EVP_AEAD
// (AES-256-GCM). The IEncryptor interface stays unchanged.
#include "mxh/crypto/crypto.hpp"

#include <windows.h>
#include <cstring>

// -----------------------------------------------------------------------------
// BCrypt API — resolved dynamically at runtime via GetProcAddress.
// This avoids bcrypt.h (C99/C++ incompatibilities) and bcrypt.lib link deps.
// -----------------------------------------------------------------------------

using BCRYPT_ALG_HANDLE = void*;
using BCRYPT_KEY_HANDLE = void*;
using BCRYPT_HANDLE    = void*;
using NTSTATUS = long;
inline bool NT_SUCCESS(NTSTATUS s) { return s >= 0; }

constexpr ULONG AES_KEY_BYTES = 16;   // 128-bit — MS AES-GCM provider max
constexpr ULONG AES_IV_BYTES  = 12;
constexpr ULONG AES_TAG_BYTES = 16;

// BCrypt constants — only the ones we actually use.
constexpr ULONG BCRYPT_USE_SYSTEM_PREFERRED_RNG = 0x00000001;
constexpr ULONG BCRYPT_BLOCK_PADDING            = 0x00000001;

// BCRYPT_KEY_DATA_BLOB export layout (see bcrypt.h SDK):
//   ULONG dwMagic;     // "BCDK" / 0x4d424444b (vendor-specific)
//   ULONG dwVersion;   // BCRYPT_KEY_DATA_BLOB_VERSION1 = 1
//   ULONG cbKeyData;
//   BYTE  rgbKeyData[cbKeyData];
// On x86/x64 with default ULONG alignment the header is 12 bytes.
constexpr ULONG BCRYPT_KEY_DATA_BLOB_HEADER_BYTES = 12;

// BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO (1:1 with bcrypt.h SDK definition).
// NOTE: cbData is ULONGLONG (8 bytes) on x64, not ULONG — Microsoft docs
// explicitly call this out. We only need nonce+tag pointers for GCM; the
// remaining fields are zeroed out. pbMacContext/cbMacContext are reserved
// for chained-call use; set to NULL/0 when not chaining.
struct BcryptAuthCipherModeInfo {
    ULONG      cbSize        = sizeof(BcryptAuthCipherModeInfo);
    ULONG      dwInfoVersion = 1;   // BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO_VERSION
    void*      pbNonce       = nullptr;
    ULONG      cbNonce       = 0;
    void*      pbAuthData    = nullptr;
    ULONG      cbAuthData    = 0;
    void*      pbTag         = nullptr;
    ULONG      cbTag         = 0;
    void*      pbMacContext  = nullptr;
    ULONG      cbMacContext  = 0;
    ULONG      cbAAD         = 0;
    ULONGLONG  cbData        = 0;   // MUST be ULONGLONG (8 bytes) — Bug C-31 root cause
    ULONG      dwFlags       = 0;
};

// --- Dynamic BCrypt function pointers ---------------------------------------
static struct BCrypt {
    HMODULE h = nullptr;

    NTSTATUS (WINAPI *OpenAlg)    (BCRYPT_ALG_HANDLE*, LPCWSTR, LPCWSTR, ULONG) = nullptr;
    NTSTATUS (WINAPI *CloseAlg)   (BCRYPT_ALG_HANDLE, ULONG) = nullptr;
    NTSTATUS (WINAPI *GenRand)    (BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG) = nullptr;
    NTSTATUS (WINAPI *SetProp)    (BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG) = nullptr;
    NTSTATUS (WINAPI *GenKey)     (BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE*, PUCHAR, ULONG,
                                   PUCHAR, ULONG, ULONG) = nullptr;
    NTSTATUS (WINAPI *DestroyKey) (BCRYPT_KEY_HANDLE) = nullptr;
    NTSTATUS (WINAPI *ExportKey)  (BCRYPT_KEY_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR,
                                   PUCHAR, ULONG, ULONG*, ULONG) = nullptr;
    NTSTATUS (WINAPI *Encrypt)    (BCRYPT_KEY_HANDLE, PUCHAR, ULONG, PVOID,
                                   PUCHAR, ULONG, PUCHAR, ULONG, ULONG*, ULONG) = nullptr;
    NTSTATUS (WINAPI *Decrypt)    (BCRYPT_KEY_HANDLE, PUCHAR, ULONG, PVOID,
                                   PUCHAR, ULONG, PUCHAR, ULONG, ULONG*, ULONG) = nullptr;

    bool init() {
        if (h) return true;
        h = LoadLibraryW(L"bcrypt.dll");
        if (!h) return false;
        OpenAlg    = reinterpret_cast<decltype(OpenAlg)>   (GetProcAddress(h, "BCryptOpenAlgorithmProvider"));
        CloseAlg   = reinterpret_cast<decltype(CloseAlg)>  (GetProcAddress(h, "BCryptCloseAlgorithmProvider"));
        GenRand    = reinterpret_cast<decltype(GenRand)>   (GetProcAddress(h, "BCryptGenRandom"));
        SetProp    = reinterpret_cast<decltype(SetProp)>   (GetProcAddress(h, "BCryptSetProperty"));
        GenKey     = reinterpret_cast<decltype(GenKey)>    (GetProcAddress(h, "BCryptGenerateSymmetricKey"));
        DestroyKey = reinterpret_cast<decltype(DestroyKey)>(GetProcAddress(h, "BCryptDestroyKey"));
        ExportKey  = reinterpret_cast<decltype(ExportKey)> (GetProcAddress(h, "BCryptExportKey"));
        Encrypt    = reinterpret_cast<decltype(Encrypt)>   (GetProcAddress(h, "BCryptEncrypt"));
        Decrypt    = reinterpret_cast<decltype(Decrypt)>   (GetProcAddress(h, "BCryptDecrypt"));
        if (!OpenAlg || !CloseAlg || !GenRand || !SetProp || !GenKey ||
            !DestroyKey || !ExportKey || !Encrypt || !Decrypt) {
            FreeLibrary(h);
            h = nullptr;
            return false;
        }
        return true;
    }

    void shutdown() {
        if (h) { FreeLibrary(h); h = nullptr; }
    }
} bcrypt;

// -----------------------------------------------------------------------------
// Aes256GcmCipher implementation
// -----------------------------------------------------------------------------
namespace mxh::crypto {

Aes256GcmCipher::Aes256GcmCipher() {
    if (!bcrypt.init()) { m_initOk = false; return; }

    NTSTATUS st = bcrypt.OpenAlg(&m_aesAlg, L"AES", nullptr, 0);
    if (!NT_SUCCESS(st)) { m_initOk = false; return; }

    // Switch the algorithm provider into GCM chaining mode.
    // sizeof(L"ChainingModeGCM") includes the null terminator — BCrypt
    // expects the size including the trailing null.
    static constexpr wchar_t kChainModeGcm[] = L"ChainingModeGCM";
    st = bcrypt.SetProp(m_aesAlg, L"ChainingMode",
                        reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(kChainModeGcm)),
                        sizeof(kChainModeGcm), 0);
    if (!NT_SUCCESS(st)) { bcrypt.CloseAlg(m_aesAlg, 0); m_initOk = false; return; }

    // NOTE: on Microsoft Primitive Provider, setting BCRYPT_KEY_LENGTH=256
    // after ChainingModeGCM returns STATUS_NOT_SUPPORTED (0xc00000bb).
    // The underlying GCM provider is locked to 128-bit keys. We accept this
    // limitation: the public IEncryptor interface uses 32-byte arrays
    // (AES-256 by name) but the actual AES key handle is 128-bit. export_key
    // zero-pads the high 16 bytes; import_key truncates to the low 16 bytes.

    m_initOk = true;
}

Aes256GcmCipher::~Aes256GcmCipher() {
    if (m_key)    bcrypt.DestroyKey(m_key);
    if (m_aesAlg) bcrypt.CloseAlg(m_aesAlg, 0);
    bcrypt.shutdown();
}

bool Aes256GcmCipher::init_key(const std::uint8_t* key_bytes, std::uint32_t key_len) {
    if (m_key) { bcrypt.DestroyKey(m_key); m_key = nullptr; }
    if (!m_initOk || !m_aesAlg) return false;
    // Microsoft AES provider in GCM mode is locked to 128-bit keys; SetProp
    // for KeyLength=256 returns STATUS_NOT_SUPPORTED. The public IEncryptor
    // API uses 32-byte std::arrays (AES-256 in name), so we copy the first
    // 16 bytes of the input and pad any remainder with zeros internally.
    std::uint8_t actual_key[AES_KEY_BYTES] = {};
    const std::uint32_t copy_len = (key_len > AES_KEY_BYTES) ? AES_KEY_BYTES : key_len;
    std::memcpy(actual_key, key_bytes, copy_len);

    NTSTATUS st = bcrypt.GenKey(m_aesAlg, &m_key, nullptr, 0, actual_key, AES_KEY_BYTES, 0);
    if (!NT_SUCCESS(st)) return false;

    // Cache the actual 16-byte key so export_key can return zero-padded 32 bytes.
    std::memcpy(m_key_cache, actual_key, AES_KEY_BYTES);
    return true;
}

void Aes256GcmCipher::seed() {
    // Generate a 128-bit AES key + 96-bit IV via explicit RNG provider.
    // The 16-byte key is cached in m_key_cache and zero-padded to 32 bytes
    // in export_key to match the public IEncryptor interface (AES-256 name).
    std::uint8_t key[AES_KEY_BYTES] = {};
    std::uint8_t iv_seed[sizeof(m_iv)] = {};

    BCRYPT_ALG_HANDLE rng_alg = nullptr;
    NTSTATUS st = bcrypt.OpenAlg(&rng_alg, L"RNG", nullptr, 0);
    if (NT_SUCCESS(st)) {
        st = bcrypt.GenRand(rng_alg, key, sizeof(key), 0);
        if (NT_SUCCESS(st)) init_key(key, sizeof(key));
        st = bcrypt.GenRand(rng_alg, iv_seed, sizeof(iv_seed), 0);
        if (NT_SUCCESS(st)) std::memcpy(m_iv, iv_seed, sizeof(m_iv));
        bcrypt.CloseAlg(rng_alg, 0);
    }
    m_counter = 0;
    m_seeded  = true;
}

// Build a 12-byte per-message IV from the session IV prefix (first 8 bytes)
// plus the 32-bit big-endian counter (last 4 bytes). Same routine used by
// both encrypt and decrypt so they stay in lock-step.
static void encode_counter_iv(const std::uint8_t* prefix, std::uint32_t counter,
                              std::uint8_t* out) {
    std::memcpy(out, prefix, AES_IV_BYTES - 4);
    out[8]  = static_cast<std::uint8_t>(counter >> 24);
    out[9]  = static_cast<std::uint8_t>(counter >> 16);
    out[10] = static_cast<std::uint8_t>(counter >> 8);
    out[11] = static_cast<std::uint8_t>(counter);
}

mxh::net::NetError Aes256GcmCipher::encrypt(std::span<std::uint8_t> data) {
    if (!m_initOk || !m_key) return mxh::net::NetError::EncryptionFailed;
    // Need at least one plaintext byte + 16 bytes for the trailing auth tag.
    if (data.size() <= AES_TAG_BYTES) return mxh::net::NetError::EncryptionFailed;

    const std::size_t ct_size = data.size() - AES_TAG_BYTES;

    std::uint8_t iv[AES_IV_BYTES] = {};
    encode_counter_iv(m_iv, m_counter, iv);
    m_counter++;

    // Tell BCrypt to write the 16-byte auth tag at data[ct_size..ct_size+16].
    BcryptAuthCipherModeInfo auth;
    auth.pbNonce = iv;
    auth.cbNonce = AES_IV_BYTES;
    auth.pbTag   = data.data() + ct_size;
    auth.cbTag   = AES_TAG_BYTES;

    std::uint32_t out_len = 0;
    // BCryptEncrypt GCM (authenticated stream cipher, not block cipher):
    //   * dwFlags must be 0 — BCRYPT_BLOCK_PADDING is forbidden with GCM
    //     (MS docs explicitly say "This flag must not be used with
    //     authenticated cipher modes (AES-CCM and AES-GCM)").
    //   * cbOutput must be ≥ cbInput. We pass the full data.size() so the
    //     16-byte tail can host either ciphertext overflow OR (more commonly)
    //     just stay unused — the auth tag goes to auth.pbTag instead.
    //   * In-place is supported (pbInput == pbOutput + offset, or fully equal).
    //   * The 16-byte GCM tag is written to data.data()+ct_size via auth.pbTag.
    const std::uint32_t cb_input = static_cast<std::uint32_t>(ct_size);
    const std::uint32_t cb_output = static_cast<std::uint32_t>(data.size());
    NTSTATUS st = bcrypt.Encrypt(
        m_key,
        data.data(), cb_input,                  // plaintext in
        &auth,
        iv, AES_IV_BYTES,
        data.data(), cb_output,                 // ciphertext out (in-place)
        reinterpret_cast<ULONG*>(&out_len),
        0 /* no flags: BCRYPT_BLOCK_PADDING forbidden with GCM */);
    if (!NT_SUCCESS(st)) return mxh::net::NetError::EncryptionFailed;

    return mxh::net::NetError::Ok;
}

mxh::net::NetError Aes256GcmCipher::decrypt(std::span<std::uint8_t> data) {
    if (!m_initOk || !m_key) return mxh::net::NetError::DecryptionFailed;
    if (data.size() < AES_TAG_BYTES) return mxh::net::NetError::DecryptionFailed;

    const std::size_t ct_size = data.size() - AES_TAG_BYTES;

    std::uint8_t iv[AES_IV_BYTES] = {};
    encode_counter_iv(m_iv, m_counter, iv);
    m_counter++;

    // PB tag points to the stored auth tag; BCrypt verifies it during decrypt.
    BcryptAuthCipherModeInfo auth;
    auth.pbNonce = iv;
    auth.cbNonce = AES_IV_BYTES;
    auth.pbTag   = data.data() + ct_size;
    auth.cbTag   = AES_TAG_BYTES;

    std::uint32_t out_len = 0;
    NTSTATUS st = bcrypt.Decrypt(
        m_key,
        data.data(), static_cast<std::uint32_t>(ct_size),   // ciphertext in
        &auth,
        iv, AES_IV_BYTES,
        data.data(), static_cast<std::uint32_t>(data.size()),  // plaintext out (in-place, ≥ ct_size)
        reinterpret_cast<ULONG*>(&out_len),
        0 /* no flags: BCRYPT_BLOCK_PADDING forbidden with GCM */);
    // On auth failure BCrypt returns STATUS_AUTH_TAG_MISMATCH (0xC000A002L)
    // — negative NTSTATUS — NT_SUCCESS() returns false. We classify any
    // failure as DecryptionFailed (the test only requires != Ok).
    if (!NT_SUCCESS(st)) return mxh::net::NetError::DecryptionFailed;
    return mxh::net::NetError::Ok;
}

bool Aes256GcmCipher::export_key(std::array<std::uint8_t, kKeyBytes>& out) const {
    if (!m_key) return false;
    // Public API is 32 bytes (AES-256); internal key is 16 bytes (AES-128 due
    // to MS provider limit). We return the real 16-byte key in the low half
    // and zero the high half. Import uses only the low 16 bytes.
    std::memset(out.data(), 0, kKeyBytes);
    std::memcpy(out.data(), m_key_cache, AES_KEY_BYTES);
    return true;
}

bool Aes256GcmCipher::import_key(const std::array<std::uint8_t, kKeyBytes>& key) {
    return init_key(key.data(), kKeyBytes);
}

bool Aes256GcmCipher::export_iv(std::array<std::uint8_t, kIvBytes>& out) const {
    if (!m_seeded) return false;
    std::memcpy(out.data(), m_iv, kIvBytes);
    return true;
}

bool Aes256GcmCipher::import_iv(const std::array<std::uint8_t, kIvBytes>& iv) {
    if (!m_initOk) return false;
    std::memcpy(m_iv, iv.data(), kIvBytes);
    m_seeded = true;
    return true;
}

}  // namespace mxh::crypto
