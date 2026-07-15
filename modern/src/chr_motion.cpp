// ChrModel.cpp - .chr character manifest parser.
//
// See chr_motion.hpp for the full format description and PID token list.
// The .chr file is plain text. We tokenize once, then walk the token
// stream with a tiny state machine in parse() below. consume_line()
// only handles the *MOD_FILE_NAME header (which needs to mutate the
// model); the running MOTION_NUM / MATERIAL_NUM counts live in parse()
// so they're visible across line iterations.

#include "mxh/compat/chr_motion.hpp"
#include "mxh/compat/detail/text_parse.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace mxh::compat {

namespace detail {

bool consume_line(ChrModel& model,
                  std::string_view token,
                  std::span<std::string_view> rest_of_line) noexcept {
    auto& sections = model.sections_internal();
    (void)rest_of_line;

    if (token == "*MOD_FILE_NAME") {
        if (rest_of_line.empty()) {
            // *MOD_FILE_NAME with no path → tolerated, no new section.
            return true;
        }
        if (sections.size() >= kChrModelMaxSections) {
            return true;  // cap, drop excess
        }
        ChrModelSection s{};
        const std::string_view path = rest_of_line.front();
        const std::size_t n = std::min(path.size(), kChrModelMaxModPath - 1);
        std::memcpy(s.mod_file, path.data(), n);
        s.mod_file[n] = '\0';
        sections.push_back(std::move(s));
        return true;
    }

    // *Other* token that starts with '*' → unrecognized PID, drop.
    if (!token.empty() && token.front() == '*') {
        return true;
    }

    // Plain path token — parse() handles placement based on its
    // running counts. Return false so parse() routes it.
    return false;
}

}  // namespace detail

std::optional<ChrModel> ChrModel::parse(
    std::span<const std::uint8_t> bytes) {
    if (bytes.empty()) return std::nullopt;
    ChrModel m;
    m.plaintext_.assign(reinterpret_cast<const char*>(bytes.data()),
                        reinterpret_cast<const char*>(bytes.data() + bytes.size()));

    // State for the count-down of *MOTION_NUM / *MATERIAL_NUM. -1
    // means "no current count" (lines are then only consumed by PID
    // tokens, never as paths).
    long motions_remaining   = -1;
    long materials_remaining = -1;

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

        // Count headers live in parse() because their effects span
        // multiple subsequent lines.
        if (first == "*MOTION_NUM") {
            if (!m.sections().empty() && !rest.empty()) {
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
        if (first == "*MATERIAL_NUM") {
            if (!m.sections().empty() && !rest.empty()) {
                try {
                    long n = std::stol(std::string(rest.front()));
                    materials_remaining = (n < 0) ? 0 : n;
                } catch (...) {
                    materials_remaining = 0;
                }
            } else {
                materials_remaining = 0;
            }
            continue;
        }

        if (detail::consume_line(m, first, rest)) {
            if (first == "*MOD_FILE_NAME") {
                // New section → reset running counts. A *MOTION_NUM
                // with leftovers that didn't fully consume (i.e. the
                // next line was *MOD_FILE_NAME instead of a path) is
                // effectively abandoned. Legacy resources never do
                // this, so we just take the conservative reset.
                motions_remaining   = -1;
                materials_remaining = -1;
            }
            continue;
        }

        // Plain path token → must belong to a running list.
        if (motions_remaining > 0 && !m.sections().empty()) {
            m.sections_.back().motions.emplace_back(first);
            --motions_remaining;
            continue;
        }
        if (materials_remaining > 0 && !m.sections().empty()) {
            m.sections_.back().materials.emplace_back(first);
            --materials_remaining;
            continue;
        }
        // Stray path token with no count in scope → drop silently
        // (legacy tolerated this for files with garbled counters).
    }

    if (m.sections().empty()) return std::nullopt;
    return m;
}

std::optional<ChrModel> ChrModel::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return std::nullopt;
    const auto size = static_cast<std::size_t>(f.tellg());
    if (size == 0) return std::nullopt;
    f.seekg(0);
    std::vector<std::uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    if (!f && !f.eof()) return std::nullopt;  // partial read
    return parse(buf);
}

std::string ChrModel::serialize_text(
    const std::vector<ChrModelSection>& sections) noexcept {
    std::string out;
    for (const auto& s : sections) {
        out += "*MOD_FILE_NAME\t";
        out += (s.mod_file[0] ? std::string(s.mod_file) : std::string("unknown.MOD"));
        out += "\n";
        out += "\t*MOTION_NUM\t";
        out += std::to_string(s.motions.size());
        out += "\n";
        for (const auto& m : s.motions) {
            out += "\t\t";
            out += m;
            out += "\n";
        }
        out += "\t*MATERIAL_NUM\t";
        out += std::to_string(s.materials.size());
        out += "\n";
        for (const auto& mat : s.materials) {
            out += "\t\t";
            out += mat;
            out += "\n";
        }
    }
    return out;
}

bool ChrModel::save_to_file(
    const std::filesystem::path& path,
    const std::vector<ChrModelSection>& sections) noexcept {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    const std::string text = serialize_text(sections);
    f.write(text.data(), static_cast<std::streamsize>(text.size()));
    return f.good();
}

}  // namespace mxh::compat
