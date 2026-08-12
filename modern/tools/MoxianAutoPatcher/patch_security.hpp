#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif

namespace mxh::patch {
namespace fs = std::filesystem;

inline bool is_safe_relative_path(const fs::path& path) {
    if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory())
        return false;
    for (const auto& part : path) {
        if (part == ".." || part == "." || part.empty()) return false;
    }
    return true;
}

inline std::string lower_hex(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

inline std::string sha256_file(const fs::path& path) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD object_size = 0, bytes = 0, digest_size = 0;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        return {};
    if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                          reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &bytes, 0) < 0 ||
        BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
                          reinterpret_cast<PUCHAR>(&digest_size), sizeof(digest_size), &bytes, 0) < 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return {};
    }
    std::vector<unsigned char> object(object_size), digest(digest_size), buffer(1024 * 1024);
    if (BCryptCreateHash(algorithm, &hash, object.data(), object_size, nullptr, 0, 0) < 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return {};
    }
    std::ifstream input(path, std::ios::binary);
    while (input) {
        input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count > 0 && BCryptHashData(hash, buffer.data(), static_cast<ULONG>(count), 0) < 0) {
            BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(algorithm, 0); return {};
        }
    }
    const bool ok = input.eof() && BCryptFinishHash(hash, digest.data(), digest_size, 0) >= 0;
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!ok) return {};
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (unsigned char value : digest) output << std::setw(2) << static_cast<unsigned>(value);
    return output.str();
#else
    (void)path;
    return {};
#endif
}

inline bool verify_file(const fs::path& path, std::uint64_t expected_size,
                        std::string_view expected_sha256) {
    std::error_code error;
    if (!fs::is_regular_file(path, error) || error || fs::file_size(path, error) != expected_size || error)
        return false;
    return lower_hex(sha256_file(path)) == lower_hex(std::string(expected_sha256));
}

} // namespace mxh::patch
