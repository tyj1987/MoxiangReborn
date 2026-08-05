// T1 payload SHA-256 byte-level verification for the canonical resource sets.

#define _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING

#include <gtest/gtest.h>

#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>
#include <span>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

#include "mxh/compat/mh_file_ex.hpp"

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

fs::path find_repo_root() {
    fs::path current;
    try {
        current = fs::current_path();
    } catch (...) {
        return {};
    }
    for (int depth = 0; depth < 10 && !current.empty(); ++depth) {
        std::error_code error;
        if (fs::is_directory(current / "modern", error) &&
            fs::is_directory(current / "deploy", error)) {
            return current;
        }
        if (current == current.root_path()) break;
        current = current.parent_path();
    }
    return {};
}

fs::path find_playdh_resource_dir() {
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
        const fs::path direct = current / fs::u8path(kPlayDhResourceU8);
        if (fs::is_directory(direct, error)) return direct;
        if (current == current.root_path()) break;
        current = current.parent_path();
    }
    return {};
}

fs::path find_source_dir(const std::string& source) {
    const fs::path root = find_repo_root();
    if (source == "deploy/server/Distribute/Resource") {
        if (root.empty()) return {};
        return root / fs::u8path(source);
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

fs::path find_manifest(const std::string& name) {
    const fs::path root = find_repo_root();
    if (!root.empty()) {
        const fs::path candidate = root / "modern/tests/unit/compat" / name;
        std::error_code error;
        if (fs::is_regular_file(candidate, error)) return candidate;
    }
    return {};
}

void run_manifest_verify(const std::string& manifest_name) {
    const fs::path manifest_path = find_manifest(manifest_name);
    if (manifest_path.empty()) {
        GTEST_SKIP() << manifest_name << " not found";
    }
    const auto manifest = parse_manifest(manifest_path);
    ASSERT_TRUE(manifest.has_value()) << "invalid manifest " << manifest_name;

    const fs::path base = find_source_dir(manifest->source);
    if (base.empty()) {
        GTEST_SKIP() << "resource source unavailable: " << manifest->source;
    }

    std::size_t checked = 0;
    std::size_t missing = 0;
    for (const auto& entry : manifest->files) {
        const fs::path path = base / fs::u8path(entry.name);
        std::error_code error;
        if (!fs::is_regular_file(path, error)) {
            ++missing;
            ADD_FAILURE() << "missing resource " << path.string();
            continue;
        }
        const auto result = mxh::compat::read_mh_bin(path);
        ASSERT_TRUE(result.ok()) << "read_mh_bin failed for " << path.string();
        EXPECT_EQ(result.value.data.size(), entry.size) << entry.name;
        EXPECT_EQ(result.value.header.file_size, entry.size) << entry.name;
        EXPECT_EQ(result.value.header.type, entry.type) << entry.name;
        EXPECT_EQ(sha256_bytes(result.value.data), entry.sha256) << entry.name;
        ++checked;
    }
    EXPECT_EQ(missing, 0u) << manifest_name;
    EXPECT_EQ(checked, manifest->files.size()) << manifest_name;
}

}  // namespace

TEST(MxhResourcePayloadSha256, VerifyManifest_Deploy) {
    run_manifest_verify("resource_payload_manifest_deploy.json");
}

TEST(MxhResourcePayloadSha256, VerifyManifest_PlayDh) {
    run_manifest_verify("resource_payload_manifest_playdh.json");
}

TEST(MxhResourcePayloadSha256, VerifyManifest_Client) {
    run_manifest_verify("resource_payload_manifest_client.json");
}
