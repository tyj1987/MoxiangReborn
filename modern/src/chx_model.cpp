// ChxModel.cpp - .chx character manifest parser.
//
// See chx_model.hpp for the full format description. .chx is
// tab-separated plain text. The first recognized token is
// *MOD_FILE_NUM, which sets a count for the *MOD_FILE_NAME
// entries that follow. After those are exhausted, *MOTION_NUM sets
// a count for plain motion-path lines. No nested sections, no
// material list.

#include "mxh/compat/chx_model.hpp"
#include "mxh/compat/detail/text_parse.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace mxh::compat {

std::optional<ChxModel> ChxModel::parse(
    std::span<const std::uint8_t> bytes) {
    if (bytes.empty()) return std::nullopt;
    ChxModel m;
    m.plaintext_.assign(reinterpret_cast<const char*>(bytes.data()),
                        reinterpret_cast<const char*>(bytes.data() + bytes.size()));

    // State: how many *MOD_FILE_NAME entries we still expect, and
    // how many motion paths we still expect. -1 means "no count in
    // scope".
    long mod_files_remaining = -1;
    long motions_remaining   = -1;

    std::istringstream iss(m.plaintext_);
    std::string line_str;
    while (std::getline(iss, line_str)) {
        std::string_view line = mxh::compat::detail::trim(line_str);
        if (line.empty()) continue;
        if (line.size() >= 2 && line[0] == '/' && line[1] == '/') continue;

        auto tokens = mxh::compat::detail::tokenize(line);
        if (tokens.empty()) continue;
        std::string_view first = tokens.front();
        std::span<std::string_view> rest(tokens.data() + 1, tokens.size() - 1);

        if (first == "*MOD_FILE_NUM") {
            if (!rest.empty()) {
                try {
                    long n = std::stol(std::string(rest.front()));
                    mod_files_remaining = (n < 0) ? 0 : n;
                } catch (...) {
                    mod_files_remaining = 0;
                }
            } else {
                mod_files_remaining = 0;
            }
            continue;
        }
        if (first == "*MOTION_NUM") {
            if (!rest.empty()) {
                try {
                    long n = std::stol(std::string(rest.front()));
                    motions_remaining = (n < 0) ? 0 : n;
                } catch (...) {
                    motions_remaining = 0;
                }
            } else {
                motions_remaining = 0;
            }
            continue;
        }
        if (first == "*MOD_FILE_NAME") {
            // Defensive: a *MOD_FILE_NAME without a count header
            // could mean a malformed file. Treat each one as a
            // count of 1 by default — but only if no count is
            // currently active. This preserves the original
            // contents of files that have a leading *MOD_FILE_NAME
            // without an explicit *MOD_FILE_NUM (uncommon but
            // possible in hand-edited resources).
            if (mod_files_remaining < 0) {
                mod_files_remaining = 1;
            }
            if (mod_files_remaining > 0 && !rest.empty()) {
                m.mod_files.emplace_back(rest.front());
                --mod_files_remaining;
            }
            continue;
        }
        // *Other* token starting with '*' → unrecognized PID,
        // ignored. Don't reset counters.
        if (!first.empty() && first.front() == '*') {
            continue;
        }

        // Plain path token → must be a motion path.
        if (motions_remaining > 0) {
            m.motions.emplace_back(first);
            --motions_remaining;
            continue;
        }
        // Stray path token with no count in scope → drop silently
        // (legacy tolerated this).
    }

    if (m.mod_files.empty()) return std::nullopt;
    return m;
}

std::optional<ChxModel> ChxModel::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return std::nullopt;
    const auto size = static_cast<std::size_t>(f.tellg());
    if (size == 0) return std::nullopt;
    f.seekg(0);
    std::vector<std::uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    if (!f && !f.eof()) return std::nullopt;
    return parse(buf);
}

std::string ChxModel::serialize_text(
    const std::vector<std::string>& mod_files,
    const std::vector<std::string>& motions) noexcept {
    std::string out;
    out += "*MOD_FILE_NUM\t";
    out += std::to_string(mod_files.size());
    out += "\n";
    for (const auto& mf : mod_files) {
        out += "*MOD_FILE_NAME\t";
        out += mf;
        out += "\n";
    }
    out += "*MOTION_NUM\t";
    out += std::to_string(motions.size());
    out += "\n";
    for (const auto& mv : motions) {
        out += mv;
        out += "\n";
    }
    return out;
}

bool ChxModel::save_to_file(
    const std::filesystem::path& path,
    const std::vector<std::string>& mod_files,
    const std::vector<std::string>& motions) noexcept {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    const std::string text = serialize_text(mod_files, motions);
    f.write(text.data(), static_cast<std::streamsize>(text.size()));
    return f.good();
}

}  // namespace mxh::compat
