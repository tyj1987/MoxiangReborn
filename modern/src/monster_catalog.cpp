#include "mxh/compat/monster_catalog.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
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

template <typename T>
bool parseInteger(std::string_view field, T& value) {
    unsigned long long parsed = 0;
    const auto result = std::from_chars(field.data(), field.data() + field.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != field.data() + field.size() ||
        parsed > static_cast<unsigned long long>(std::numeric_limits<T>::max())) return false;
    value = static_cast<T>(parsed);
    return true;
}

bool parseFloat(std::string_view field, float& value) {
    const auto result = std::from_chars(field.data(), field.data() + field.size(), value);
    return result.ec == std::errc{} && result.ptr == field.data() + field.size() &&
        std::isfinite(value);
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
        if (fields.size() < 48 || fields[6].empty()) continue;
        unsigned kind = 0;
        const auto parsed = std::from_chars(fields[0].data(), fields[0].data() + fields[0].size(), kind);
        if (parsed.ec != std::errc{} || kind == 0 || kind > 65535u) continue;
        MonsterVisual visual;
        visual.kind = static_cast<std::uint16_t>(kind);
        visual.name.assign(fields[1]);
        visual.chx_name.assign(fields[6]);
        try {
            float parsed_scale = 1.0f;
            const auto* first = fields[7].data();
            const auto* last = first + fields[7].size();
            const auto fc = std::from_chars(first, last, parsed_scale);
            if (fc.ec == std::errc{} && fc.ptr == last) {
                visual.scale = parsed_scale;
            } else {
                visual.scale = 1.0f;
            }
        } catch (...) {
            visual.scale = 1.0f;
        }
        if (!(visual.scale > 0.0f && visual.scale < 100.0f)) visual.scale = 1.0f;
        std::uint8_t fore_attack = 0;
        if (!parseInteger(fields[3], visual.level) ||
            !parseInteger(fields[11], visual.life) ||
            !parseInteger(fields[12], visual.shield) ||
            !parseInteger(fields[13], visual.exp) ||
            !parseInteger(fields[15], visual.attack_min) ||
            !parseInteger(fields[16], visual.attack_max) ||
            !parseInteger(fields[18], visual.defense) ||
            !parseFloat(fields[24], visual.walk_speed) ||
            !parseFloat(fields[25], visual.run_speed) ||
            !parseFloat(fields[34], visual.domain_range) ||
            !parseInteger(fields[37], fore_attack) || fore_attack > 1 ||
            !parseFloat(fields[38], visual.search_period_ms) ||
            !parseFloat(fields[40], visual.search_range) ||
            !parseInteger(fields[43], visual.attack_count) || visual.attack_count > 2 ||
            !parseInteger(fields[44], visual.attack_skills[0]) ||
            !parseInteger(fields[45], visual.attack_skills[1]) ||
            !parseInteger(fields[46], visual.attack_rates[0]) ||
            !parseInteger(fields[47], visual.attack_rates[1]) ||
            visual.life == 0 || visual.attack_min > visual.attack_max ||
            visual.walk_speed < 0.0f || visual.run_speed < 0.0f ||
            visual.domain_range < 0.0f || visual.search_period_ms < 0.0f ||
            visual.search_range < 0.0f) continue;
        visual.aggressive = fore_attack != 0;
        bool invalid_attack = false;
        for (std::size_t i = 0; i < visual.attack_count; ++i)
            invalid_attack = invalid_attack || visual.attack_skills[i] == 0;
        if (invalid_attack) continue;
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
