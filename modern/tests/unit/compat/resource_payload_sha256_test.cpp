// T1 payload SHA-256 byte-level verification for the canonical resource sets.

#define _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "mxh/compat/mh_file_ex.hpp"
#include "../../support/payload_sha256.hpp"
#include "../../support/playdh_env.hpp"

namespace fs = std::filesystem;

namespace {

struct ManifestEntry {
    std::string name;
    std::uint32_t type = 0;
    std::uintmax_t size = 0;
    std::string sha256;
};

struct Provenance {
    bool present = false;
    std::string source_path;
    std::string reader;
    std::string reader_commit;
    std::uint32_t verified_match = 0;
    std::uint32_t verified_total = 0;
};

struct Manifest {
    std::string format;
    std::string source;
    Provenance provenance;
    std::vector<ManifestEntry> files;
};


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

bool is_json_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Value of a top-level (or nested) key whose value is an array or object.
// Bracket depth ignores the other bracket kind and skips strings, so a
// sibling object such as provenance cannot swallow the files array.
std::optional<std::string> json_bracket_value(
    const std::string& json, std::string_view key, char open_ch, char close_ch) {
    const std::string token = "\"" + std::string(key) + "\"";
    std::size_t search = 0;
    while (search < json.size()) {
        const auto pos = json.find(token, search);
        if (pos == std::string::npos) return std::nullopt;
        std::size_t index = pos + token.size();
        while (index < json.size() && is_json_space(json[index])) ++index;
        if (index >= json.size() || json[index] != ':') {
            search = pos + 1;
            continue;
        }
        ++index;
        while (index < json.size() && is_json_space(json[index])) ++index;
        if (index >= json.size() || json[index] != open_ch) {
            search = pos + 1;
            continue;
        }
        int depth = 0;
        bool in_string = false;
        bool escape = false;
        for (std::size_t cursor = index; cursor < json.size(); ++cursor) {
            const char c = json[cursor];
            if (in_string) {
                if (escape) escape = false;
                else if (c == '\\') escape = true;
                else if (c == '"') in_string = false;
                continue;
            }
            if (c == '"') {
                in_string = true;
                continue;
            }
            if (c == open_ch) ++depth;
            else if (c == close_ch) {
                --depth;
                if (depth == 0) return json.substr(index, cursor - index + 1);
            }
        }
        return std::nullopt;
    }
    return std::nullopt;
}

Provenance parse_provenance(const std::string& json) {
    Provenance provenance;
    const auto object = json_bracket_value(json, "provenance", '{', '}');
    if (!object) return provenance;
    provenance.present = true;
    if (const auto source_path = json_string_value(*object, "source_path"))
        provenance.source_path = *source_path;
    if (const auto reader = json_string_value(*object, "reader"))
        provenance.reader = *reader;
    if (const auto reader_commit = json_string_value(*object, "reader_commit"))
        provenance.reader_commit = *reader_commit;
    const std::regex match_pattern(R"re("verified_match"\s*:\s*(\d+))re");
    const std::regex total_pattern(R"re("verified_total"\s*:\s*(\d+))re");
    std::smatch match;
    try {
        if (std::regex_search(*object, match, match_pattern))
            provenance.verified_match = static_cast<std::uint32_t>(std::stoul(match[1].str()));
        if (std::regex_search(*object, match, total_pattern))
            provenance.verified_total = static_cast<std::uint32_t>(std::stoul(match[1].str()));
    } catch (...) {
        provenance.present = false;
    }
    return provenance;
}

std::optional<Manifest> parse_manifest(const fs::path& path) {
    const std::string json = read_text(path);
    if (json.empty()) return std::nullopt;

    const auto format = json_string_value(json, "format");
    const auto source = json_string_value(json, "source");
    if (!format || !source || *format != "moxian-payload-sha256-v1") {
        return std::nullopt;
    }

    const auto files_json = json_bracket_value(json, "files", '[', ']');
    if (!files_json) return std::nullopt;
    const std::regex entry_pattern(
        R"re(\{\s*"name"\s*:\s*"([^"]+)"\s*,\s*"type"\s*:\s*(\d+)\s*,\s*"size"\s*:\s*(\d+)\s*,\s*"sha256"\s*:\s*"([0-9a-fA-F]{64})"\s*\})re");

    Manifest manifest;
    manifest.format = *format;
    manifest.source = *source;
    manifest.provenance = parse_provenance(json);
    for (std::sregex_iterator it(files_json->begin(), files_json->end(), entry_pattern),
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
        EXPECT_EQ(mxh::test::sha256_bytes(result.value.data), entry.sha256) << entry.name;
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

TEST(MxhResourcePayloadSha256, DeployManifest_Keeps180AndRecordsProvenance) {
    const auto manifest = parse_manifest(find_manifest("resource_payload_manifest_deploy.json"));
    ASSERT_TRUE(manifest.has_value());
    EXPECT_EQ(manifest->format, "moxian-payload-sha256-v1");
    EXPECT_EQ(manifest->source, "deploy/server/Distribute/Resource");
    EXPECT_EQ(manifest->files.size(), 180u);
    ASSERT_TRUE(manifest->provenance.present);
    EXPECT_EQ(manifest->provenance.source_path, "D:\\MX\\src\\SWorking\\Resource");
    EXPECT_EQ(manifest->provenance.reader, "read_mh_bin in mh_file_ex.cpp");
    EXPECT_EQ(manifest->provenance.reader_commit, "70bb2dd6");
    EXPECT_EQ(manifest->provenance.verified_match, 180u);
    EXPECT_EQ(manifest->provenance.verified_total, 180u);

    const auto has_name = [&](const char* name) {
        for (const auto& entry : manifest->files) {
            if (entry.name == name) return true;
        }
        return false;
    };
    EXPECT_TRUE(has_name("BobusangInfo.bin"));
    EXPECT_TRUE(has_name("Server/ItemMixList.bin"));
    EXPECT_TRUE(has_name("PenaltyTime.bin"));

    for (const char* name : {"resource_payload_manifest_playdh.json",
                             "resource_payload_manifest_client.json"}) {
        const auto other = parse_manifest(find_manifest(name));
        ASSERT_TRUE(other.has_value()) << name;
        EXPECT_FALSE(other->provenance.present) << name;
        EXPECT_FALSE(other->files.empty()) << name;
    }
}

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
