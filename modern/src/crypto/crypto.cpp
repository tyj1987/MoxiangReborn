// crypto.cpp - AES-256-GCM via Windows CNG (bcrypt.dll).
//
// Phase 3: replaces the HSEL physical encryption dongle with a software-only
// AES-256-GCM implementation using the Windows CNG API (bcrypt.dll).
// No external dependencies — bcrypt.dll ships with Windows Vista+.
//
// Design:
//   - AES-256-GCM: 256-bit key, 96-bit IV/nonce, 128-bit auth tag.
//   - encrypt: appends 16-byte auth tag after ciphertext. Total output = input + 16.
//   - decrypt: verifies auth tag, strips it. Returns error on auth failure.
//   - seed(): generates a random 256-bit key and 96-bit IV via BCryptGenRandom.
//   - IEncryptor interface preserved — drop-in replacement for HselCompatCipher.
#include "mxh/crypto/crypto.hpp"

#include <windows.h>
#include <cstring>

// -----------------------------------------------------------------------------
// BCrypt API — resolved dynamically at runtime via GetProcAddress.
// This avoids bcrypt.h (C99/C++ incompatibilities) and bcrypt.lib link deps.
// -----------------------------------------------------------------------------

// --- BCrypt types and constants --------------------------------------------
using BCRYPT_ALG_HANDLE = void*;
using BCRYPT_KEY_HANDLE = void*;
using BCRYPT_HANDLE    = void*;
using NTSTATUS = long;
inline bool NT_SUCCESS(NTSTATUS s) { return s >= 0; }
#define STATUS_SUCCESS ((NTSTATUS)0)

constexpr ULONG AES_KEY_BYTES = 32;
constexpr ULONG AES_IV_BYTES  = 12;
constexpr ULONG AES_TAG_BYTES = 16;

// BCrypt constants
constexpr ULONG BCRYPT_RNG_USE_URNG_AUTOSEED = 0x00000001;
constexpr ULONG BCRYPT_AUTH_MODE_GCM_FLAG    = 0x00000004;

// GCM authenticated-cipher metadata (passed as pPaddingInfo to Encrypt/Decrypt).
struct BCRYPT_AUTH_INFO {
    ULONG cbSize       = sizeof(BCRYPT_AUTH_INFO);
    ULONG dwInfoVersion = 1;
    void* pbNonce      = nullptr;
    ULONG cbNonce      = 0;
    void* pbAuthData   = nullptr;
    ULONG cbAuthData   = 0;
    void* pbTag        = nullptr;
    ULONG cbTag        = 0;
    void* pbCipherText = nullptr;
    ULONG cbCipherText = 0;
    ULONG cbAAD        = 0;
    ULONG cbData       = 0;
    ULONG dwFlags      = 0;
};

// --- Dynamic BCrypt function pointers ---------------------------------------
// Each member is initialized by bcrypt_init() at startup.
static struct BCrypt {
    HMODULE h = nullptr;
    // clang-format off
    NTSTATUS (WINAPI *OpenAlg)       (BCRYPT_ALG_HANDLE*, LPCWSTR, LPCWSTR, ULONG) = nullptr;
    NTSTATUS (WINAPI *CloseAlg)      (BCRYPT_ALG_HANDLE, ULONG) = nullptr;
    NTSTATUS (WINAPI *GenRand)       (BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG) = nullptr;
    NTSTATUS (WINAPI *SetProp)       (BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG) = nullptr;
    NTSTATUS (WINAPI *GenKey)        (BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE*, PUCHAR, ULONG,
                                      PUCHAR, ULONG, ULONG) = nullptr;
    NTSTATUS (WINAPI *DestroyKey)    (BCRYPT_KEY_HANDLE) = nullptr;
    NTSTATUS (WINAPI *ExportKey)     (BCRYPT_KEY_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR,
                                      PUCHAR, ULONG, ULONG*, ULONG) = nullptr;
    NTSTATUS (WINAPI *Encrypt)       (BCRYPT_KEY_HANDLE, PUCHAR, ULONG, PVOID,
                                      PUCHAR, ULONG, PUCHAR, ULONG, ULONG*, ULONG) = nullptr;
    NTSTATUS (WINAPI *Decrypt)       (BCRYPT_KEY_HANDLE, PUCHAR, ULONG, PVOID,
                                      PUCHAR, ULONG, PUCHAR, ULONG, ULONG*, ULONG) = nullptr;
    NTSTATUS (WINAPI *FinishKey)     (BCRYPT_KEY_HANDLE, PUCHAR, ULONG, ULONG) = nullptr;
    // clang-format on

    // Resolve all BCrypt* entry points from bcrypt.dll.
    // Returns true on success; bcrypt.dll has been on Windows Vista+ since 2006.
    bool init() {
        if (h) return true;
        h = LoadLibraryW(L"bcrypt.dll");
        if (!h) return false;
        // clang-format off
        OpenAlg     = (decltype(OpenAlg))     GetProcAddress(h, "BCryptOpenAlgorithmProvider");
        CloseAlg    = (decltype(CloseAlg))    GetProcAddress(h, "BCryptCloseAlgorithmProvider");
        GenRand     = (decltype(GenRand))     GetProcAddress(h, "BCryptGenRandom");
        SetProp     = (decltype(SetProp))     GetProcAddress(h, "BCryptSetProperty");
        GenKey      = (decltype(GenKey))      GetProcAddress(h, "BCryptGenerateSymmetricKey");
        DestroyKey  = (decltype(DestroyKey))  GetProcAddress(h, "BCryptDestroyKey");
        ExportKey   = (decltype(ExportKey))    GetProcAddress(h, "BCryptExportKey");
        Encrypt     = (decltype(Encrypt))     GetProcAddress(h, "BCryptEncrypt");
        Decrypt     = (decltype(Decrypt))     GetProcAddress(h, "BCryptDecrypt");
        FinishKey   = (decltype(FinishKey))   GetProcAddress(h, "BCryptFinishKey");
        // clang-format on
        if (!OpenAlg || !CloseAlg || !GenRand || !SetProp || !GenKey ||
            !DestroyKey || !ExportKey || !Encrypt || !Decrypt || !FinishKey) {
            FreeLibrary(h); h = nullptr; return false;
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

    // Open AES algorithm handle.
    NTSTATUS st = bcrypt.OpenAlg(&m_aesAlg, L"AES", nullptr, 0);
    if (!NT_SUCCESS(st)) { m_initOk = false; return; }

    // Set chaining mode to GCM.
    static constexpr wchar_t kChainModeGcm[] = L"ChainingModeGCM";
    st = bcrypt.SetProp(m_aesAlg, L"ChainingMode",
                        reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(kChainModeGcm)),
                        sizeof(kChainModeGcm), 0);
    if (!NT_SUCCESS(st)) { bcrypt.CloseAlg(m_aesAlg, 0); m_initOk = false; return; }

    // Set AES key length to 256 bits.
    ULONG keyBits = 256;
    st = bcrypt.SetProp(m_aesAlg, L"KeyLength",
                        reinterpret_cast<PUCHAR>(&keyBits), sizeof(keyBits), 0);
    if (!NT_SUCCESS(st)) { bcrypt.CloseAlg(m_aesAlg, 0); m_initOk = false; return; }

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
    NTSTATUS st = bcrypt.GenKey(m_aesAlg, &m_key, nullptr, 0,
                                const_cast<std::uint8_t*>(key_bytes), key_len, 0);
    return NT_SUCCESS(st);
}

void Aes256GcmCipher::seed() {
    std::uint8_t key[AES_KEY_BYTES] = {};
    NTSTATUS st = bcrypt.GenRand(nullptr, key, sizeof(key), BCRYPT_RNG_USE_URNG_AUTOSEED);
    if (NT_SUCCESS(st)) init_key(key, sizeof(key));
    bcrypt.GenRand(nullptr, m_iv, sizeof(m_iv), BCRYPT_RNG_USE_URNG_AUTOSEED);
    m_counter = 0;
}

mxh::net::NetError Aes256GcmCipher::encrypt(std::span<std::uint8_t> data) {
    if (!m_initOk || !m_key) return mxh::net::NetError::EncryptionFailed;

    // Encode packet counter into IV (big-endian, last 4 bytes of the 12-byte nonce).
    UCHAR iv[AES_IV_BYTES] = {};
    std::memcpy(iv, m_iv, AES_IV_BYTES - 4);
    iv[8]  = static_cast<UCHAR>(m_counter >> 24);
    iv[9]  = static_cast<UCHAR>(m_counter >> 16);
    iv[10] = static_cast<UCHAR>(m_counter >> 8);
    iv[11] = static_cast<UCHAR>(m_counter);
    m_counter++;

    UCHAR tag[AES_TAG_BYTES] = {};
    ULONG out_len = 0;

    // BCryptEncrypt in GCM mode computes the auth tag internally.
    NTSTATUS st = bcrypt.Encrypt(
        m_key,
        data.data(), static_cast<ULONG>(data.size()),
        nullptr,                 // pPaddingInfo (no padding; GCM tag is handled below)
        iv, AES_IV_BYTES,
        data.data(), static_cast<ULONG>(data.size()),
        &out_len,
        BCRYPT_AUTH_MODE_GCM_FLAG);
    if (!NT_SUCCESS(st)) return mxh::net::NetError::EncryptionFailed;

    // Extract the computed auth tag.
    st = bcrypt.FinishKey(m_key, tag, AES_TAG_BYTES, 0);
    if (!NT_SUCCESS(st)) return mxh::net::NetError::EncryptionFailed;

    // Append 16-byte tag after ciphertext (in-place growth).
    std::memcpy(data.data() + data.size(), tag, AES_TAG_BYTES);
    return mxh::net::NetError::Ok;
}

mxh::net::NetError Aes256GcmCipher::decrypt(std::span<std::uint8_t> data) {
    if (!m_initOk || !m_key) return mxh::net::NetError::EncryptionFailed;
    if (data.size() < AES_TAG_BYTES) return mxh::net::NetError::EncryptionFailed;

    const std::size_t ct_size = data.size() - AES_TAG_BYTES;

    // Split ciphertext and auth tag.
    UCHAR tag[AES_TAG_BYTES] = {};
    std::memcpy(tag, data.data() + ct_size, AES_TAG_BYTES);

    // Rebuild IV from m_iv + counter.
    UCHAR iv[AES_IV_BYTES] = {};
    std::memcpy(iv, m_iv, AES_IV_BYTES - 4);
    iv[8]  = static_cast<UCHAR>(m_counter >> 24);
    iv[9]  = static_cast<UCHAR>(m_counter >> 16);
    iv[10] = static_cast<UCHAR>(m_counter >> 8);
    iv[11] = static_cast<UCHAR>(m_counter);
    m_counter++;

    // Set up GCM auth metadata for decryption (tag verification).
    BCRYPT_AUTH_INFO auth = {};
    auth.pbNonce = iv;
    auth.cbNonce = AES_IV_BYTES;
    auth.pbTag   = tag;
    auth.cbTag   = AES_TAG_BYTES;

    ULONG out_len = 0;
    NTSTATUS st = bcrypt.Decrypt(
        m_key,
        data.data(), static_cast<ULONG>(ct_size),
        &auth,
        iv, AES_IV_BYTES,
        data.data(), static_cast<ULONG>(ct_size),
        &out_len,
        BCRYPT_AUTH_MODE_GCM_FLAG);
    if (!NT_SUCCESS(st)) return mxh::net::NetError::EncryptionFailed;
    return mxh::net::NetError::Ok;
}

bool Aes256GcmCipher::export_key(std::array<std::uint8_t, 32>& out) const {
    if (!m_key) return false;
    // BCRYPT_KEY_DATA_BLOB: { ver(UCHAR), hdrLen(UCHAR), magic(USHORT), keyLen(ULONG), data[] }
    UCHAR buf[64] = {};
    ULONG buf_len = 0;
    NTSTATUS st = bcrypt.ExportKey(m_key, nullptr, L"KeyDataBlob",
                                   buf, sizeof(buf), &buf_len, 0);
    if (!NT_SUCCESS(st) || buf_len < 8) return false;
    std::memcpy(out.data(), buf + 8, 32);
    return true;
}

bool Aes256GcmCipher::import_key(const std::array<std::uint8_t, 32>& key) {
    return init_key(key.data(), 32);
}

bool Aes256GcmCipher::export_iv(std::array<std::uint8_t, 12>& out) const {
    if (!m_initOk) return false;
    std::memcpy(out.data(), m_iv, 12);
    return true;
}

bool Aes256GcmCipher::import_iv(const std::array<std::uint8_t, 12>& iv) {
    if (!m_initOk) return false;
    std::memcpy(m_iv, iv.data(), 12);
    return true;
}

} // namespace mxh::crypto
