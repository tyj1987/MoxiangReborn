// modern/src/portal/jwt_token.cpp
// M5.2: Minimal JWT HS256 using Windows BCrypt + RFC4648 base64url (no padding).

#include "portal/jwt_token.hpp"
#include "portal/portal_log.hpp"

#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <windows.h>
#include <bcrypt.h>

namespace mxh::portal {
namespace {

// ---------------------------------------------------------------------------
// BCrypt helpers (dynamically loaded, WIN32 only)
// ---------------------------------------------------------------------------

#ifdef WIN32

using BCryptOpenAlgorithmProvider_fn = NTSTATUS(NTAPI*)(
    BCRYPT_ALG_HANDLE*, LPCWSTR, LPCWSTR, ULONG);
using BCryptCloseAlgorithmProvider_fn = NTSTATUS(NTAPI*)(BCRYPT_ALG_HANDLE, ULONG);
using BCryptCreateHash_fn = NTSTATUS(NTAPI*)(
    BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE*, PUCHAR, ULONG,
    PUCHAR, ULONG, ULONG);
using BCryptHashData_fn = NTSTATUS(NTAPI*)(
    BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
using BCryptFinishHash_fn = NTSTATUS(NTAPI*)(
    BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
using BCryptDestroyHash_fn = NTSTATUS(NTAPI*)(BCRYPT_HASH_HANDLE, ULONG);

static constexpr ULONG KHMAC_FLAG = 0x00000008U;

static BCryptOpenAlgorithmProvider_fn  g_bcrypt_OpenAlg    = nullptr;
static BCryptCloseAlgorithmProvider_fn g_bcrypt_CloseAlg   = nullptr;
static BCryptCreateHash_fn           g_bcrypt_CreateHash = nullptr;
static BCryptHashData_fn             g_bcrypt_HashData   = nullptr;
static BCryptFinishHash_fn           g_bcrypt_FinishHash = nullptr;
static BCryptDestroyHash_fn          g_bcrypt_DestroyHash= nullptr;
static bool g_bcrypt_init = false;

bool init_bcrypt() {
    if (g_bcrypt_init) return true;
    auto h = LoadLibraryW(L"bcrypt.dll");
    if (!h) return false;
    g_bcrypt_OpenAlg    = (BCryptOpenAlgorithmProvider_fn)
        GetProcAddress(h, "BCryptOpenAlgorithmProvider");
    g_bcrypt_CloseAlg   = (BCryptCloseAlgorithmProvider_fn)
        GetProcAddress(h, "BCryptCloseAlgorithmProvider");
    g_bcrypt_CreateHash = (BCryptCreateHash_fn)
        GetProcAddress(h, "BCryptCreateHash");
    g_bcrypt_HashData   = (BCryptHashData_fn)
        GetProcAddress(h, "BCryptHashData");
    g_bcrypt_FinishHash = (BCryptFinishHash_fn)
        GetProcAddress(h, "BCryptFinishHash");
    g_bcrypt_DestroyHash= (BCryptDestroyHash_fn)
        GetProcAddress(h, "BCryptDestroyHash");
    g_bcrypt_init = true;
    return g_bcrypt_OpenAlg != nullptr;
}

// HMAC-SHA256 implementation (called by public hmac_sha256 wrapper).
bool hmac_sha256_impl(const std::uint8_t* key, std::size_t key_len,
                      const std::uint8_t* data, std::size_t data_len,
                      std::uint8_t out[32]) {
    if (!init_bcrypt()) return false;

    BCRYPT_ALG_HANDLE h = nullptr;
    NTSTATUS st = g_bcrypt_OpenAlg(&h, L"HMAC", nullptr, KHMAC_FLAG);
    if (st < 0) return false;

    BCRYPT_HASH_HANDLE hh = nullptr;
    st = g_bcrypt_CreateHash(h, &hh, nullptr, 0,
                             const_cast<PUCHAR>(key),
                             static_cast<ULONG>(key_len), 0);
    if (st < 0) { g_bcrypt_CloseAlg(h, 0); return false; }

    st = g_bcrypt_HashData(hh, const_cast<PUCHAR>(data),
                           static_cast<ULONG>(data_len), 0);
    if (st < 0) { g_bcrypt_DestroyHash(hh, 0); g_bcrypt_CloseAlg(h, 0); return false; }

    st = g_bcrypt_FinishHash(hh, out, 32, 0);
    g_bcrypt_DestroyHash(hh, 0);
    g_bcrypt_CloseAlg(h, 0);
    return st >= 0;
}

#endif  // WIN32

// base64url helpers (used by public encode/decode wrappers)
static const char kBase64Url[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

inline int base64url_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

}  // anonymous namespace

// ---------------------------------------------------------------------------
// Public helpers (declared inline in jwt_token.hpp)
// ---------------------------------------------------------------------------

inline std::string base64url_encode(const std::uint8_t* data, std::size_t len) {
    std::string out;
    out.reserve((len * 8 + 5) / 6);
    std::size_t i = 0;
    while (i + 2 < len) {
        std::uint32_t triple = (std::uint32_t(data[i]) << 16)
                             | (std::uint32_t(data[i+1]) << 8)
                             | data[i+2];
        out.push_back(kBase64Url[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Url[(triple >> 12) & 0x3F]);
        out.push_back(kBase64Url[(triple >>  6) & 0x3F]);
        out.push_back(kBase64Url[triple & 0x3F]);
        i += 3;
    }
    if (i + 1 < len) {
        std::uint32_t double_word = (std::uint32_t(data[i]) << 16)
                                  | (std::uint32_t(data[i+1]) << 8);
        out.push_back(kBase64Url[(double_word >> 18) & 0x3F]);
        out.push_back(kBase64Url[(double_word >> 12) & 0x3F]);
        out.push_back(kBase64Url[(double_word >>  6) & 0x3F]);
    } else if (i < len) {
        std::uint32_t single = data[i] << 16;
        out.push_back(kBase64Url[(single >> 18) & 0x3F]);
        out.push_back(kBase64Url[(single >> 12) & 0x3F]);
    }
    return out;
}

inline bool base64url_decode(const char* src, std::size_t len,
                            std::vector<std::uint8_t>& out) {
    out.clear();
    out.reserve(len * 3 / 4);
    std::size_t i = 0;
    while (i < len) {
        int a = i < len ? base64url_decode_char(src[i]) : 0;
        int b = (i + 1 < len) ? base64url_decode_char(src[i+1]) : 0;
        int c = (i + 2 < len) ? base64url_decode_char(src[i+2]) : 0;
        int d = (i + 3 < len) ? base64url_decode_char(src[i+3]) : 0;
        if (a < 0 || b < 0) return false;
        std::uint32_t word = (static_cast<std::uint32_t>(a) << 18)
                           | (static_cast<std::uint32_t>(b) << 12)
                           | (static_cast<std::uint32_t>(c & 0x3F) << 6)
                           |  static_cast<std::uint32_t>(d & 0x3F);
        out.push_back(static_cast<std::uint8_t>((word >> 16) & 0xFF));
        if (i + 2 < len) out.push_back(static_cast<std::uint8_t>((word >> 8) & 0xFF));
        if (i + 3 < len) out.push_back(static_cast<std::uint8_t>(word & 0xFF));
        i += 4;
    }
    return true;
}

inline bool hmac_sha256(const std::uint8_t* key, std::size_t key_len,
                        const std::uint8_t* data, std::size_t data_len,
                        std::uint8_t out[32]) {
#ifdef WIN32
    return hmac_sha256_impl(key, key_len, data, data_len, out);
#else
    return false;
#endif
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::string create_jwt(std::string_view secret,
                        std::string_view sub,
                        std::uint32_t user_idx,
                        std::uint64_t expiry_seconds) {
    const char* header_json = R"({"alg":"HS256","typ":"JWT"})";
    std::string header_enc = base64url_encode(
        reinterpret_cast<const std::uint8_t*>(header_json),
        std::strlen(header_json));

    auto now = static_cast<std::uint64_t>(std::time(nullptr));
    auto exp = now + expiry_seconds;

    std::ostringstream payload;
    payload << R"({"sub":")" << sub << R"(","user_idx":)" << user_idx
           << R"(,"exp":)" << exp << R"(,"iat":)" << now << "}";

    std::string payload_enc = base64url_encode(
        reinterpret_cast<const std::uint8_t*>(payload.str().data()),
        payload.str().size());

    std::string signing_input = header_enc + "." + payload_enc;
    std::uint8_t sig[32];
    if (!hmac_sha256(
            reinterpret_cast<const std::uint8_t*>(secret.data()), secret.size(),
            reinterpret_cast<const std::uint8_t*>(signing_input.data()), signing_input.size(),
            sig)) {
        return {};
    }
    std::string sig_enc = base64url_encode(sig, 32);
    return header_enc + "." + payload_enc + "." + sig_enc;
}

std::optional<std::string> verify_jwt(std::string_view secret,
                                       std::string_view token,
                                       JwtPayload& out) {
    auto dot1 = token.find('.');
    auto dot2 = token.find('.', dot1 + 1);
    if (dot1 == std::string_view::npos || dot2 == std::string_view::npos)
        return std::string("invalid token format");

    auto header_enc = token.substr(0, dot1);
    auto payload_enc = token.substr(dot1 + 1, dot2 - dot1 - 1);
    auto sig_enc = token.substr(dot2 + 1);

    std::string signing_input = std::string(header_enc) + "." + std::string(payload_enc);
    std::uint8_t expected[32];
    if (!hmac_sha256(
            reinterpret_cast<const std::uint8_t*>(secret.data()), secret.size(),
            reinterpret_cast<const std::uint8_t*>(signing_input.data()), signing_input.size(),
            expected)) {
        return std::string("HMAC computation failed");
    }

    std::vector<std::uint8_t> actual;
    if (!base64url_decode(sig_enc.data(), sig_enc.size(), actual) || actual.size() != 32)
        return std::string("invalid signature encoding");
    for (std::size_t i = 0; i < 32; i++) {
        if (actual[i] != expected[i]) return std::string("signature mismatch");
    }

    std::vector<std::uint8_t> payload_bytes;
    if (!base64url_decode(payload_enc.data(), payload_enc.size(), payload_bytes))
        return std::string("invalid payload encoding");

    std::string payload_str(reinterpret_cast<char*>(payload_bytes.data()),
                            payload_bytes.size());

    out.sub.clear();
    out.exp.clear();
    out.iat.clear();
    out.user_idx = 0;

    auto sub_pos = payload_str.find(R"("sub":")");
    if (sub_pos != std::string::npos) {
        auto q1 = payload_str.find('"', sub_pos + 6);
        if (q1 != std::string::npos)
            out.sub = payload_str.substr(sub_pos + 6, q1 - (sub_pos + 6));
    }

    auto exp_pos = payload_str.find(R"("exp":")");
    if (exp_pos != std::string::npos) {
        auto n1 = payload_str.find_first_of("0123456789", exp_pos + 6);
        auto n2 = payload_str.find_first_not_of("0123456789", n1);
        if (n1 != std::string::npos)
            out.exp = payload_str.substr(n1, n2 - n1);
    }

    auto iat_pos = payload_str.find(R"("iat":")");
    if (iat_pos != std::string::npos) {
        auto n1 = payload_str.find_first_of("0123456789", iat_pos + 6);
        auto n2 = payload_str.find_first_not_of("0123456789", n1);
        if (n1 != std::string::npos)
            out.iat = payload_str.substr(n1, n2 - n1);
    }

    auto uid_pos = payload_str.find(R"("user_idx":")");
    if (uid_pos != std::string::npos) {
        auto n1 = payload_str.find_first_of("0123456789", uid_pos + 11);
        auto n2 = payload_str.find_first_not_of("0123456789", n1);
        if (n1 != std::string::npos)
            out.user_idx = static_cast<std::uint32_t>(
                std::stoul(payload_str.substr(n1, n2 - n1)));
    }

    if (!out.exp.empty()) {
        auto exp_ts = std::stoull(out.exp);
        auto now = static_cast<std::uint64_t>(std::time(nullptr));
        if (now > exp_ts) return std::string("token expired");
    }

    return std::nullopt;
}

}  // namespace mxh::portal
