// Read-only ground-truth dump of MH .bin payloads. Writes only --out.
//   mxh_payload_dump --dir <resource-dir> [--manifest <json>] --out <file.json>
//
// Recursively lists every *.bin under --dir. Each entry is
// {name, type, size, sha256, read_ok, error}. name is relative to --dir
// with forward slashes. No decoded payload bytes are written.
// With --manifest, comparisons[] uses status match / sha_mismatch /
// type_mismatch / missing (case-insensitive unique names are noted)
// plus a summary count. The directory is never modified.

#define _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING

#include "mxh/compat/mh_file_ex.hpp"
#include "../../support/payload_sha256.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Options {
    fs::path dir;
    fs::path out;
    std::optional<fs::path> manifest;
};

struct FileRecord {
    std::string name;
    std::uint32_t type = 0;
    std::uintmax_t size = 0;
    std::string sha256;
    bool read_ok = false;
    std::string error;
};

struct ManifestEntry {
    std::string name;
    std::uint32_t type = 0;
    std::uintmax_t size = 0;
    std::string sha256;
};

void usage() {
    std::cerr
        << "mxh_payload_dump --dir <resource-dir> [--manifest <json>] --out <file.json>\n"
        << "Read-only. Writes only --out. Does not modify --dir.\n";
}

bool ascii_iequal(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        const auto lower = [](unsigned char c) {
            return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a')
                                          : static_cast<char>(c);
        };
        if (lower(static_cast<unsigned char>(left[i])) !=
            lower(static_cast<unsigned char>(right[i]))) {
            return false;
        }
    }
    return true;
}

bool ends_with_bin(const fs::path& path) {
    return ascii_iequal(path.filename().string(), ".bin") ||
           (path.filename().string().size() >= 4 &&
            ascii_iequal(path.filename().string().substr(path.filename().string().size() - 4),
                         ".bin"));
}

std::string json_escape(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const unsigned char c : text) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20) {
                constexpr char kHex[] = "0123456789abcdef";
                out += "\\u00";
                out += kHex[c >> 4];
                out += kHex[c & 0x0F];
            } else {
                out += static_cast<char>(c);
            }
            break;
        }
    }
    return out;
}

const char* mh_error_name(mxh::compat::MhError error) {
    using mxh::compat::MhError;
    switch (error) {
    case MhError::Ok: return "";
    case MhError::FileNotFound: return "file_not_found";
    case MhError::PermissionDenied: return "permission_denied";
    case MhError::IoError: return "io_error";
    case MhError::InvalidHeader: return "invalid_header";
    case MhError::UnsupportedVersion: return "unsupported_version";
    case MhError::CrcMismatch: return "crc_mismatch";
    case MhError::UnsupportedOpaqueServerProfile: return "unsupported_opaque_server_profile";
    }
    return "unknown";
}

std::optional<Options> parse_args(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto need = [&](const char* flag) -> const char* {
            if (i + 1 >= argc) {
                std::cerr << "missing value for " << flag << "\n";
                return nullptr;
            }
            return argv[++i];
        };
        if (arg == "--dir") {
            const char* value = need("--dir");
            if (!value) return std::nullopt;
            options.dir = fs::u8path(value);
        } else if (arg == "--out") {
            const char* value = need("--out");
            if (!value) return std::nullopt;
            options.out = fs::u8path(value);
        } else if (arg == "--manifest") {
            const char* value = need("--manifest");
            if (!value) return std::nullopt;
            options.manifest = fs::u8path(value);
        } else if (arg == "--help" || arg == "-h") {
            usage();
            return std::nullopt;
        } else {
            std::cerr << "unknown argument: " << arg << "\n";
            usage();
            return std::nullopt;
        }
    }
    if (options.dir.empty() || options.out.empty()) {
        usage();
        return std::nullopt;
    }
    return options;
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

std::optional<std::vector<ManifestEntry>> parse_manifest(const fs::path& path) {
    const std::string json = read_text(path);
    if (json.empty()) return std::nullopt;
    const std::size_t files_key = json.find("\"files\"");
    const std::size_t array_start = json.find('[', files_key);
    const std::size_t array_end = json.rfind(']');
    if (files_key == std::string::npos || array_start == std::string::npos ||
        array_end == std::string::npos || array_end <= array_start) {
        return std::nullopt;
    }
    const std::string files_json = json.substr(array_start, array_end - array_start + 1);
    const std::regex entry_pattern(
        R"re(\{\s*"name"\s*:\s*"([^"]+)"\s*,\s*"type"\s*:\s*(\d+)\s*,\s*"size"\s*:\s*(\d+)\s*,\s*"sha256"\s*:\s*"([0-9a-fA-F]{64})"\s*\})re");
    std::vector<ManifestEntry> entries;
    for (std::sregex_iterator it(files_json.begin(), files_json.end(), entry_pattern), end;
         it != end; ++it) {
        ManifestEntry entry;
        entry.name = unescape_json_string((*it)[1].str());
        try {
            entry.type = static_cast<std::uint32_t>(std::stoul((*it)[2].str()));
            entry.size = static_cast<std::uintmax_t>(std::stoull((*it)[3].str()));
        } catch (...) {
            return std::nullopt;
        }
        entry.sha256 = (*it)[4].str();
        entries.push_back(std::move(entry));
    }
    if (entries.empty()) return std::nullopt;
    return entries;
}

bool collect_bins(const fs::path& root, std::vector<FileRecord>& records, std::string& error) {
    std::error_code ec;
    if (!fs::is_directory(root, ec)) {
        error = "not a directory: " + root.string();
        return false;
    }
    std::vector<fs::path> files;
    const fs::recursive_directory_iterator end;
    fs::recursive_directory_iterator it(
        root, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        error = "cannot read directory: " + root.string();
        return false;
    }
    for (; it != end; it.increment(ec)) {
        if (ec) {
            error = "cannot read directory entry under " + root.string();
            return false;
        }
        const fs::directory_entry& entry = *it;
        std::error_code entry_ec;
        if (entry.is_symlink(entry_ec)) {
            if (entry.is_directory(entry_ec)) it.disable_recursion_pending();
            continue;
        }
        if (!entry.is_regular_file(entry_ec)) continue;
        if (!ends_with_bin(entry.path())) continue;
        files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    records.reserve(files.size());
    for (const fs::path& path : files) {
        std::error_code rel_ec;
        const fs::path relative = fs::relative(path, root, rel_ec);
        FileRecord record;
        record.name = rel_ec ? path.filename().generic_string() : relative.generic_string();
        const auto result = mxh::compat::read_mh_bin(path);
        if (!result.ok()) {
            record.read_ok = false;
            record.error = mh_error_name(result.error);
            records.push_back(std::move(record));
            continue;
        }
        record.read_ok = true;
        record.type = result.value.header.type;
        record.size = result.value.data.size();
        record.sha256 = mxh::test::sha256_bytes(result.value.data);
        if (record.sha256.empty()) record.error = "sha256_failed";
        records.push_back(std::move(record));
    }
    return true;
}

struct Comparison {
    std::string name;
    std::string status;
    std::string name_match;
    std::string matched_name;
    std::uint32_t manifest_type = 0;
    std::uintmax_t manifest_size = 0;
    std::string manifest_sha256;
    std::uint32_t actual_type = 0;
    std::uintmax_t actual_size = 0;
    std::string actual_sha256;
    std::string error;
};

const FileRecord* find_record(
    const std::vector<FileRecord>& records,
    const std::map<std::string, std::size_t>& exact,
    const std::string& name,
    std::string& name_match,
    std::string& matched_name) {
    const auto exact_it = exact.find(name);
    if (exact_it != exact.end()) {
        name_match = "exact";
        matched_name = records[exact_it->second].name;
        return &records[exact_it->second];
    }
    const FileRecord* found = nullptr;
    int matches = 0;
    for (const auto& record : records) {
        if (!ascii_iequal(record.name, name)) continue;
        ++matches;
        found = &record;
    }
    if (matches == 1 && found != nullptr) {
        name_match = "case_insensitive";
        matched_name = found->name;
        return found;
    }
    if (matches > 1) name_match = "ambiguous";
    return nullptr;
}

std::vector<Comparison> compare_manifest(
    const std::vector<ManifestEntry>& entries,
    const std::vector<FileRecord>& records) {
    std::map<std::string, std::size_t> exact;
    for (std::size_t i = 0; i < records.size(); ++i) exact.emplace(records[i].name, i);
    std::vector<Comparison> comparisons;
    comparisons.reserve(entries.size());
    for (const auto& entry : entries) {
        Comparison comparison;
        comparison.name = entry.name;
        comparison.manifest_type = entry.type;
        comparison.manifest_size = entry.size;
        comparison.manifest_sha256 = entry.sha256;
        std::string name_match;
        std::string matched_name;
        const FileRecord* record = find_record(records, exact, entry.name, name_match, matched_name);
        comparison.name_match = name_match;
        comparison.matched_name = matched_name;
        if (record == nullptr) {
            comparison.status = "missing";
            comparisons.push_back(std::move(comparison));
            continue;
        }
        comparison.actual_type = record->type;
        comparison.actual_size = record->size;
        comparison.actual_sha256 = record->sha256;
        comparison.error = record->error;
        if (!record->read_ok || record->sha256.empty()) {
            comparison.status = "read_error";
        } else if (record->sha256 != entry.sha256) {
            comparison.status = "sha_mismatch";
        } else if (record->type != entry.type) {
            comparison.status = "type_mismatch";
        } else if (record->size != entry.size) {
            comparison.status = "sha_mismatch";
        } else {
            comparison.status = "match";
        }
        comparisons.push_back(std::move(comparison));
    }
    return comparisons;
}

void write_record(std::ostream& out, const FileRecord& record) {
    out << "    {\"name\":\"" << json_escape(record.name)
        << "\",\"type\":" << record.type
        << ",\"size\":" << record.size
        << ",\"sha256\":\"" << json_escape(record.sha256)
        << "\",\"read_ok\":" << (record.read_ok ? "true" : "false")
        << ",\"error\":\"" << json_escape(record.error) << "\"}";
}

bool write_output(
    const fs::path& out_path,
    const std::vector<FileRecord>& records,
    const std::optional<std::vector<Comparison>>& comparisons) {
    std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << "{\n  \"files\": [\n";
    for (std::size_t i = 0; i < records.size(); ++i) {
        write_record(out, records[i]);
        if (i + 1 != records.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n  \"file_count\": " << records.size();
    if (comparisons) {
        std::size_t match = 0, sha_mismatch = 0, type_mismatch = 0, missing = 0;
        std::size_t read_error = 0, case_insensitive = 0;
        out << ",\n  \"comparisons\": [\n";
        for (std::size_t i = 0; i < comparisons->size(); ++i) {
            const Comparison& comparison = (*comparisons)[i];
            if (comparison.status == "match") ++match;
            else if (comparison.status == "sha_mismatch") ++sha_mismatch;
            else if (comparison.status == "type_mismatch") ++type_mismatch;
            else if (comparison.status == "missing") ++missing;
            else if (comparison.status == "read_error") ++read_error;
            if (comparison.name_match == "case_insensitive") ++case_insensitive;
            out << "    {\"name\":\"" << json_escape(comparison.name)
                << "\",\"status\":\"" << comparison.status
                << "\",\"name_match\":\"" << json_escape(comparison.name_match)
                << "\",\"matched_name\":\"" << json_escape(comparison.matched_name)
                << "\",\"manifest_type\":" << comparison.manifest_type
                << ",\"manifest_size\":" << comparison.manifest_size
                << ",\"manifest_sha256\":\"" << json_escape(comparison.manifest_sha256)
                << "\",\"actual_type\":" << comparison.actual_type
                << ",\"actual_size\":" << comparison.actual_size
                << ",\"actual_sha256\":\"" << json_escape(comparison.actual_sha256)
                << "\",\"error\":\"" << json_escape(comparison.error) << "\"}";
            if (i + 1 != comparisons->size()) out << ",";
            out << "\n";
        }
        out << "  ],\n  \"summary\": {"
            << "\"manifest_entries\":" << comparisons->size()
            << ",\"match\":" << match
            << ",\"sha_mismatch\":" << sha_mismatch
            << ",\"type_mismatch\":" << type_mismatch
            << ",\"missing\":" << missing
            << ",\"read_error\":" << read_error
            << ",\"case_insensitive\":" << case_insensitive
            << "}";
    }
    out << "\n}\n";
    return static_cast<bool>(out);
}

}  // namespace

int main(int argc, char** argv) {
    const auto options = parse_args(argc, argv);
    if (!options) return 2;

    std::optional<std::vector<ManifestEntry>> manifest;
    if (options->manifest) {
        manifest = parse_manifest(*options->manifest);
        if (!manifest) {
            std::cerr << "cannot parse manifest: " << options->manifest->string() << "\n";
            return 1;
        }
    }

    std::vector<FileRecord> records;
    std::string error;
    if (!collect_bins(options->dir, records, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    std::optional<std::vector<Comparison>> comparisons;
    if (manifest) comparisons = compare_manifest(*manifest, records);
    if (!write_output(options->out, records, comparisons)) {
        std::cerr << "cannot write " << options->out.string() << "\n";
        return 1;
    }
    return 0;
}
