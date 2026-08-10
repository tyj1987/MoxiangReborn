#include "mxh/compat/monster_catalog.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <sstream>

namespace mxh::compat {
namespace {
std::vector<std::string_view> splitTabs(std::string_view line) {
    std::vector<std::string_view> fields;
    std::size_t begin = 0;
    while (begin <= line.size()) {
        const auto end = line.find('\t', begin);
        fields.push_back(line.substr(begin, end == std::string_view::npos ? end : end - begin));
        if (end == std::string_view::npos) break;
        begin = end + 1;
    }
    return fields;
}
}

std::optional<MonsterCatalog> MonsterCatalog::parse_text(
    std::span<const std::uint8_t> payload) {
    if (payload.empty()) return std::nullopt;
    MonsterCatalog catalog;
    std::string text(reinterpret_cast<const char*>(payload.data()), payload.size());
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto fields = splitTabs(line);
        if (fields.size() < 8 || fields[6].empty()) continue;
        unsigned kind = 0;
        const auto parsed = std::from_chars(fields[0].data(), fields[0].data() + fields[0].size(), kind);
        if (parsed.ec != std::errc{} || kind == 0 || kind > 65535u) continue;
        MonsterVisual visual;
        visual.kind = static_cast<std::uint16_t>(kind);
        visual.name.assign(fields[1]);
        visual.chx_name.assign(fields[6]);
        try { visual.scale = std::stof(std::string(fields[7])); }
        catch (...) { visual.scale = 1.0f; }
        if (!(visual.scale > 0.0f && visual.scale < 100.0f)) visual.scale = 1.0f;
        catalog.entries_.push_back(std::move(visual));
    }
    if (catalog.entries_.empty()) return std::nullopt;
    std::sort(catalog.entries_.begin(), catalog.entries_.end(),
              [](const auto& a, const auto& b) { return a.kind < b.kind; });
    return catalog;
}

std::optional<MonsterCatalog> MonsterCatalog::parse_bin(
    std::span<const std::uint8_t> bytes) {
    if (bytes.size() < sizeof(MhFileHeader)) return std::nullopt;
    MhFileHeader header{};
    std::memcpy(&header, bytes.data(), sizeof(header));
    if (header.file_size > 256u * 1024u * 1024u) return std::nullopt;
    std::size_t offset = sizeof(MhFileHeader) + 1;
    if (offset + header.file_size > bytes.size()) {
        offset = sizeof(MhFileHeader);
        if (offset + header.file_size > bytes.size()) return std::nullopt;
    }
    const auto decoded = decrypt_bin_payload(bytes.subspan(offset, header.file_size), header.type);
    return parse_text(decoded);
}

const MonsterVisual* MonsterCatalog::find(std::uint16_t kind) const noexcept {
    const auto it = std::lower_bound(entries_.begin(), entries_.end(), kind,
        [](const MonsterVisual& item, std::uint16_t value) { return item.kind < value; });
    return it != entries_.end() && it->kind == kind ? &*it : nullptr;
}
} // namespace mxh::compat
