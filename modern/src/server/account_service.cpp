#include "mxh/server/account_service.hpp"

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif

#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>

namespace mxh::server {
namespace {

constexpr unsigned long kIterations = 210000;
constexpr std::size_t kSaltBytes = 16;
constexpr std::size_t kHashBytes = 32;
constexpr std::string_view kPrefix = "pbkdf2-sha256$";

std::string hex_encode(const std::uint8_t* data, std::size_t size) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < size; ++i) out << std::setw(2) << unsigned(data[i]);
    return out.str();
}

bool hex_decode(std::string_view text, std::vector<std::uint8_t>& out) {
    if ((text.size() & 1u) != 0u) return false;
    out.clear();
    out.reserve(text.size() / 2);
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (std::size_t i = 0; i < text.size(); i += 2) {
        const int hi = nibble(text[i]);
        const int lo = nibble(text[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }
    return true;
}

bool derive(std::string_view password, const std::uint8_t* salt,
            std::size_t salt_size, unsigned long iterations,
            std::array<std::uint8_t, kHashBytes>& output) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
                                    BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0) return false;
    const auto status = BCryptDeriveKeyPBKDF2(
        algorithm,
        reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),
        static_cast<ULONG>(password.size()),
        const_cast<PUCHAR>(salt), static_cast<ULONG>(salt_size), iterations,
        output.data(), static_cast<ULONG>(output.size()), 0);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return status >= 0;
#else
    (void)password; (void)salt; (void)salt_size; (void)iterations; (void)output;
    return false;
#endif
}

}  // namespace

bool valid_account_name(std::string_view account) noexcept {
    if (account.size() < 3 || account.size() > 16) return false;
    for (const unsigned char c : account) {
        if (!(std::isalnum(c) || c == '_')) return false;
    }
    return true;
}

bool valid_account_password(std::string_view password) noexcept {
    if (password.size() < 8 || password.size() > 16) return false;
    bool letter = false, digit = false;
    for (const unsigned char c : password) {
        if (c < 0x21 || c > 0x7e) return false;
        letter = letter || std::isalpha(c);
        digit = digit || std::isdigit(c);
    }
    return letter && digit;
}

std::string hash_account_password(std::string_view password) {
    std::array<std::uint8_t, kSaltBytes> salt{};
    std::array<std::uint8_t, kHashBytes> hash{};
#ifdef _WIN32
    if (BCryptGenRandom(nullptr, salt.data(), static_cast<ULONG>(salt.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) return {};
#else
    return {};
#endif
    if (!derive(password, salt.data(), salt.size(), kIterations, hash)) return {};
    return std::string(kPrefix) + std::to_string(kIterations) + "$" +
           hex_encode(salt.data(), salt.size()) + "$" +
           hex_encode(hash.data(), hash.size());
}

bool verify_account_password(std::string_view password,
                             std::string_view stored) noexcept {
    if (stored.size() < kPrefix.size() || stored.substr(0, kPrefix.size()) != kPrefix)
        return password == stored; // legacy rows
    try {
        const auto iter_begin = kPrefix.size();
        const auto first = stored.find('$', iter_begin);
        const auto second = first == std::string_view::npos
            ? first : stored.find('$', first + 1);
        if (first == std::string_view::npos || second == std::string_view::npos) return false;
        const auto iterations = std::stoul(std::string(stored.substr(iter_begin, first - iter_begin)));
        if (iterations < 100000 || iterations > 1000000) return false;
        std::vector<std::uint8_t> salt, expected;
        if (!hex_decode(stored.substr(first + 1, second - first - 1), salt) ||
            !hex_decode(stored.substr(second + 1), expected) ||
            salt.size() != kSaltBytes || expected.size() != kHashBytes) return false;
        std::array<std::uint8_t, kHashBytes> actual{};
        if (!derive(password, salt.data(), salt.size(), iterations, actual)) return false;
        std::uint8_t difference = 0;
        for (std::size_t i = 0; i < actual.size(); ++i) difference |= actual[i] ^ expected[i];
        return difference == 0;
    } catch (...) {
        return false;
    }
}

AccountCreateResult create_account(mxh::db::IDbAdapter& db,
                                   std::string_view account,
                                   std::string_view password) {
    if (!valid_account_name(account)) return {AccountCreateStatus::InvalidAccount, "account must be 3-16 ASCII letters, digits, or underscore"};
    if (!valid_account_password(password)) return {AccountCreateStatus::WeakPassword, "password must be 8-16 printable ASCII characters with a letter and digit"};
    mxh::db::ResultSet existing;
    const std::vector<mxh::db::Bind> lookup{mxh::db::bind(std::string(account))};
    const auto query = db.query("SELECT 1 FROM chr_log_info WHERE id = ?", lookup, existing);
    if (!query.ok()) return {AccountCreateStatus::DatabaseError, query.error_message};
    if (!existing.empty()) return {AccountCreateStatus::AlreadyExists, "account already exists"};
    const auto hash = hash_account_password(password);
    if (hash.empty()) return {AccountCreateStatus::CryptoError, "password hashing failed"};
    const std::vector<mxh::db::Bind> insert{
        mxh::db::bind(std::string(account)), mxh::db::bind(hash)};
    const auto result = db.execute(
        "INSERT INTO chr_log_info (id, pw, userlevel) VALUES (?, ?, 0)", insert);
    if (!result.ok()) return {AccountCreateStatus::DatabaseError, result.error_message};
    return {AccountCreateStatus::Ok, "account created"};
}

}  // namespace mxh::server
