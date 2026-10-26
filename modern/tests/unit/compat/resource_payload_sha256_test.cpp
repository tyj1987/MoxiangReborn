// T1 payload SHA-256 byte-level verification for the canonical resource sets.

#define _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef _MSC_VER
#pragma comment(lib, "bcrypt.lib")
#endif

#include "mxh/compat/mh_file_ex.hpp"
#include "../../support/playdh_env.hpp"

namespace fs = std::filesystem;

namespace {

struct ManifestEntry {
    std::string name;
    std::uint32_t type = 0;
    std::uintmax_t size = 0;
    std::string sha256;
};

struct Manifest {
    std::string format;
    std::string source;
    std::vector<ManifestEntry> files;
};

#ifdef _WIN32
std::string sha256_bytes(std::span<const std::uint8_t> bytes) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (status != 0) return {};

    DWORD object_length = 0;
    DWORD result_length = 0;
    status = BCryptGetProperty(
        algorithm, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&object_length), sizeof(object_length),
        &result_length, 0);
    if (status != 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return {};
    }

    std::vector<UCHAR> object(object_length);
    std::array<UCHAR, 32> digest{};
    status = BCryptCreateHash(
        algorithm, &hash, object.data(), object_length, nullptr, 0, 0);
    if (status != 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return {};
    }

    if (!bytes.empty()) {
        status = BCryptHashData(
            hash,
            reinterpret_cast<PUCHAR>(const_cast<std::uint8_t*>(bytes.data())),
            static_cast<ULONG>(bytes.size()), 0);
    }
    if (status == 0) {
        status = BCryptFinishHash(
            hash, digest.data(), static_cast<ULONG>(digest.size()), 0);
    }
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status != 0) return {};

    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (const auto byte : digest) {
        result << std::setw(2) << static_cast<unsigned>(byte);
    }
    return result.str();
}
#else
// FIPS 180-4. Windows keeps BCrypt above; this path exists so Linux can
// verify the same payload bytes. It is bit-identical to mxh::net::sha256.
constexpr std::uint32_t kSha256K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

constexpr std::uint32_t rotr32(std::uint32_t x, std::uint32_t n) noexcept {
    return (x >> n) | (x << (32u - n));
}

void sha256_compress(std::uint32_t h[8], const std::uint8_t block[64]) {
    std::uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(block[i * 4 + 0]) << 24) |
               (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
               (static_cast<std::uint32_t>(block[i * 4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        const std::uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const std::uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
    std::uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int i = 0; i < 64; ++i) {
        const std::uint32_t s1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        const std::uint32_t ch = (e & f) ^ ((~e) & g);
        const std::uint32_t t1 = hh + s1 + ch + kSha256K[i] + w[i];
        const std::uint32_t s0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t t2 = s0 + maj;
        hh = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

std::string sha256_bytes(std::span<const std::uint8_t> bytes) {
    std::uint32_t h[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
    };
    const std::uint64_t bit_len = static_cast<std::uint64_t>(bytes.size()) * 8u;
    std::size_t offset = 0;
    while (offset + 64u <= bytes.size()) {
        sha256_compress(h, bytes.data() + offset);
        offset += 64u;
    }
    std::array<std::uint8_t, 128> tail{};
    const std::size_t rem = bytes.size() - offset;
    for (std::size_t i = 0; i < rem; ++i) tail[i] = bytes[offset + i];
    tail[rem] = 0x80u;
    const std::size_t pad_end = (rem < 56u) ? 64u : 128u;
    for (int b = 0; b < 8; ++b) {
        tail[pad_end - 1 - static_cast<std::size_t>(b)] =
            static_cast<std::uint8_t>((bit_len >> (b * 8)) & 0xFFu);
    }
    sha256_compress(h, tail.data());
    if (pad_end == 128u) sha256_compress(h, tail.data() + 64u);

    std::array<std::uint8_t, 32> digest{};
    for (int k = 0; k < 8; ++k) {
        digest[k * 4 + 0] = static_cast<std::uint8_t>((h[k] >> 24) & 0xFFu);
        digest[k * 4 + 1] = static_cast<std::uint8_t>((h[k] >> 16) & 0xFFu);
        digest[k * 4 + 2] = static_cast<std::uint8_t>((h[k] >> 8) & 0xFFu);
        digest[k * 4 + 3] = static_cast<std::uint8_t>(h[k] & 0xFFu);
    }
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (const auto byte : digest) {
        result << std::setw(2) << static_cast<unsigned>(byte);
    }
    return result.str();
}
#endif

std::string read_text(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    std::ostringstream content;
    content << input.rdbuf();
    return content.str();
}

std::string unescape_json_string(std::string value) {
    std::string result;
    result.reserve(value.size());
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (value[index] != '\\' || index + 1 >= value.size()) {
            result += value[index];
            continue;
        }
        const char escaped = value[++index];
        switch (escaped) {
        case 'n': result += '\n'; break;
        case 'r': result += '\r'; break;
        case 't': result += '\t'; break;
        case '\\': result += '\\'; break;
        case '"': result += '"'; break;
        default: result += escaped; break;
        }
    }
    return result;
}

std::optional<std::string> json_string_value(
    const std::string& json, const std::string& key) {
    const std::regex pattern(
        "\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) return std::nullopt;
    return unescape_json_string(match[1].str());
}

std::optional<Manifest> parse_manifest(const fs::path& path) {
    const std::string json = read_text(path);
    if (json.empty()) return std::nullopt;

    const auto format = json_string_value(json, "format");
    const auto source = json_string_value(json, "source");
    if (!format || !source || *format != "moxian-payload-sha256-v1") {
        return std::nullopt;
    }

    const std::size_t files_key = json.find("\"files\"");
    const std::size_t array_start = json.find('[', files_key);
    const std::size_t array_end = json.rfind(']');
    if (files_key == std::string::npos || array_start == std::string::npos ||
        array_end == std::string::npos || array_end <= array_start) {
        return std::nullopt;
    }

    const std::string files_json =
        json.substr(array_start, array_end - array_start + 1);
    const std::regex entry_pattern(
        R"re(\{\s*"name"\s*:\s*"([^"]+)"\s*,\s*"type"\s*:\s*(\d+)\s*,\s*"size"\s*:\s*(\d+)\s*,\s*"sha256"\s*:\s*"([0-9a-fA-F]{64})"\s*\})re");

    Manifest manifest;
    manifest.format = *format;
    manifest.source = *source;
    for (std::sregex_iterator it(files_json.begin(), files_json.end(), entry_pattern),
         end; it != end; ++it) {
        const auto& match = *it;
        ManifestEntry entry;
        entry.name = unescape_json_string(match[1].str());
        try {
            entry.type = static_cast<std::uint32_t>(std::stoul(match[2].str()));
            entry.size = static_cast<std::uintmax_t>(std::stoull(match[3].str()));
        } catch (...) {
            return std::nullopt;
        }
        entry.sha256 = match[4].str();
        manifest.files.push_back(std::move(entry));
    }
    if (manifest.files.empty()) return std::nullopt;
    return manifest;
}

// Test-only seam. Empty uses the tracked tree under MXH_SOURCE_DIR.
// The negative test points this at a nonexistent directory; it must not
// move or replace repo files.
std::optional<fs::path> g_deploy_resource_dir_override;

bool ascii_iequal(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        const auto lower = [](unsigned char c) {
            return (c >= 'A' && c <= 'Z')
                       ? static_cast<char>(c - 'A' + 'a')
                       : static_cast<char>(c);
        };
        if (lower(static_cast<unsigned char>(left[i])) !=
            lower(static_cast<unsigned char>(right[i]))) {
            return false;
        }
    }
    return true;
}

// Tracked deploy resources. MXH_SOURCE_DIR is the modern/ root from CMake,
// the same anchor find_manifest() uses. Do not walk cwd and do not read
// MXH_PLAYDH_ROOT for this tree.
fs::path deploy_resource_dir() {
    if (g_deploy_resource_dir_override.has_value())
        return *g_deploy_resource_dir_override;
    return fs::path(MXH_SOURCE_DIR) / "deploy" / "server" / "Distribute" / "Resource";
}

// Exact relative path first. A unique ASCII case-insensitive match covers
// the tracked names that differ only by case (Linux vs the Windows tree).
fs::path resolve_deploy_entry(const fs::path& base, const std::string& name) {
    const fs::path relative = fs::u8path(name);
    std::error_code error;
    const fs::path exact = base / relative;
    if (fs::is_regular_file(exact, error)) return exact;

    fs::path current = base;
    for (const fs::path& part : relative) {
        error.clear();
        const fs::path exact_part = current / part;
        if (fs::exists(exact_part, error)) {
            current = exact_part;
            continue;
        }
        if (!fs::is_directory(current, error)) return {};
        fs::path match;
        int matches = 0;
        for (const auto& entry : fs::directory_iterator(current, error)) {
            if (error) return {};
            if (!ascii_iequal(entry.path().filename().string(), part.string()))
                continue;
            ++matches;
            match = entry.path();
        }
        if (matches != 1) return {};
        current = match;
    }
    error.clear();
    return fs::is_regular_file(current, error) ? current : fs::path{};
}

fs::path find_playdh_resource_dir() {
    if (const auto playdh_override = mxh::test::playdh_dir_override())
        return *playdh_override / "Resource";
    static const char kPlayDhResourceU8[] =
        "\xE5\xA2\xA8\xE9\xA6\x99\xE3\x80\x90\xE6\xBA\x90\xE7\xA0\x81\xE9\x85\x8D\xE5\xA5\x97\xE8\xB5\x84\xE6\xBA\x90\xE3\x80\x91/PlayDH/Resource";
    fs::path current;
    try {
        current = fs::current_path();
    } catch (...) {
        return {};
    }
    for (int depth = 0; depth < 10 && !current.empty(); ++depth) {
        std::error_code error;
        const fs::path direct = current / "modern" / "data" / "PlayDH" / "Resource";
        if (fs::is_directory(direct, error)) return direct;
        if (current == current.root_path()) break;
        current = current.parent_path();
    }
    return {};
}

fs::path find_source_dir(const std::string& source) {
    if (source == "deploy/server/Distribute/Resource") {
        const fs::path deploy = deploy_resource_dir();
        std::error_code error;
        return fs::is_directory(deploy, error) ? deploy : fs::path{};
    }
    if (source == "PlayDH/Resource") return find_playdh_resource_dir();
    if (source == "PlayDH/Resource/Client") {
        const fs::path resource = find_playdh_resource_dir();
        if (resource.empty()) return {};
        return resource / "Client";
    }
    const fs::path absolute = fs::u8path(source);
    std::error_code error;
    return fs::is_directory(absolute, error) ? absolute : fs::path{};
}

// Manifest JSON is a repo fixture. MXH_SOURCE_DIR is the modern/ source
// root from CMake. Do not walk cwd and do not read MXH_PLAYDH_ROOT.
fs::path find_manifest(const std::string& name) {
    const fs::path candidate =
        fs::path(MXH_SOURCE_DIR) / "tests" / "unit" / "compat" / name;
    std::error_code error;
    if (!fs::is_regular_file(candidate, error)) {
        throw std::runtime_error(
            "Cannot locate fixture \"" + candidate.string() + "\"");
    }
    return candidate;
}

void run_manifest_verify(const std::string& manifest_name) {
    const fs::path manifest_path = find_manifest(manifest_name);
    ASSERT_TRUE(fs::is_regular_file(manifest_path))
        << "Cannot locate fixture \"" << manifest_path.string() << "\"";
    const auto manifest = parse_manifest(manifest_path);
    ASSERT_TRUE(manifest.has_value()) << "invalid manifest " << manifest_name;

    const bool deploy_source =
        manifest->source == "deploy/server/Distribute/Resource";
    fs::path base;
    if (deploy_source) {
        base = deploy_resource_dir();
        std::error_code error;
        if (!fs::is_directory(base, error)) {
            FAIL() << "deploy resource directory unavailable: " << base.string();
        }
    } else {
        base = find_source_dir(manifest->source);
        if (base.empty()) {
            GTEST_SKIP() << "resource source unavailable: " << manifest->source;
        }
    }

    const std::size_t manifest_entries = manifest->files.size();
    std::size_t checked = 0;
    std::vector<std::string> missing_names;
    for (const auto& entry : manifest->files) {
        const fs::path path = deploy_source
                                  ? resolve_deploy_entry(base, entry.name)
                                  : base / fs::u8path(entry.name);
        std::error_code error;
        if (path.empty() || !fs::is_regular_file(path, error)) {
            missing_names.push_back(entry.name);
            continue;
        }
        const auto result = mxh::compat::read_mh_bin(path);
        if (!result.ok()) {
            ADD_FAILURE() << "read_mh_bin failed for " << entry.name
                          << " (" << path.string() << ")";
            continue;
        }
        EXPECT_EQ(result.value.data.size(), entry.size) << entry.name;
        EXPECT_EQ(result.value.header.file_size, entry.size) << entry.name;
        EXPECT_EQ(result.value.header.type, entry.type) << entry.name;
        EXPECT_EQ(sha256_bytes(result.value.data), entry.sha256) << entry.name;
        ++checked;
    }
    if (!missing_names.empty()) {
        std::ostringstream listed;
        for (std::size_t index = 0; index < missing_names.size(); ++index) {
            if (index != 0) listed << ", ";
            listed << missing_names[index];
        }
        ADD_FAILURE() << "missing resource entries for " << manifest->source
                      << " (" << missing_names.size() << "): " << listed.str();
    }
    EXPECT_EQ(checked, manifest_entries)
        << manifest_name << " verified " << checked
        << " of " << manifest_entries << " manifest entries";
}

}  // namespace

TEST(MxhResourcePayloadSha256, VerifyManifest_Deploy) {
    run_manifest_verify("resource_payload_manifest_deploy.json");
}

TEST(MxhResourcePayloadSha256, VerifyManifest_Deploy_MissingDirectoryFails) {
    const fs::path missing =
        fs::temp_directory_path() / "mxh-deploy-resource-absent-8291";
    std::error_code error;
    fs::remove_all(missing, error);
    ASSERT_FALSE(fs::exists(missing, error)) << missing.string();

    struct OverrideGuard {
        explicit OverrideGuard(fs::path path) {
            g_deploy_resource_dir_override = std::move(path);
        }
        ~OverrideGuard() { g_deploy_resource_dir_override.reset(); }
    } guard(missing);

    EXPECT_FATAL_FAILURE(
        run_manifest_verify("resource_payload_manifest_deploy.json"),
        "deploy resource directory unavailable");
}

TEST(MxhResourcePayloadSha256, VerifyManifest_PlayDh) {
    run_manifest_verify("resource_payload_manifest_playdh.json");
}

TEST(MxhResourcePayloadSha256, VerifyManifest_Client) {
    run_manifest_verify("resource_payload_manifest_client.json");
}
