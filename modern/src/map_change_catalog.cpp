#include "mxh/compat/map_change_catalog.hpp"

#include "mxh/compat/mh_file_ex.hpp"

#include <charconv>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string_view>

namespace mxh::compat {
namespace {

std::vector<std::string_view> fields(std::string_view line) {
    std::vector<std::string_view> result;
    while (true) {
        const auto tab = line.find('\t');
        result.push_back(line.substr(0, tab));
        if (tab == std::string_view::npos) break;
        line.remove_prefix(tab + 1);
    }
    return result;
}

bool parse_u16(std::string_view value, std::uint16_t& out) noexcept {
    std::uint32_t parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
        parsed > std::numeric_limits<std::uint16_t>::max()) return false;
    out = static_cast<std::uint16_t>(parsed);
    return true;
}

bool parse_float(std::string_view value, float& out) noexcept {
    std::string copy(value);
    char* end = nullptr;
    const float parsed = std::strtof(copy.c_str(), &end);
    if (end != copy.c_str() + copy.size()) return false;
    out = parsed;
    return true;
}

}  // namespace

const MapChangeEntry* MapChangeCatalog::find_kind(std::uint16_t kind) const noexcept {
    for (const auto& entry : entries) if (entry.kind == kind) return &entry;
    return nullptr;
}

const MapChangeEntry* MapChangeCatalog::find_destination(
    std::uint16_t current_map, std::uint16_t destination_map) const noexcept {
    for (const auto& entry : entries) {
        if (entry.current_map_num == current_map &&
            entry.move_map_num == destination_map) return &entry;
    }
    return nullptr;
}

const MapChangeEntry* MapChangeCatalog::find_object_destination(
    std::uint16_t current_map, std::string_view object_name) const noexcept {
    if (object_name.empty()) return nullptr;
    for (const auto& entry : entries) {
        if (entry.current_map_num == current_map &&
            entry.object_name == object_name && entry.move_map_num != 0) {
            return &entry;
        }
    }
    return nullptr;
}

std::optional<MapChangeCatalog> parse_map_change_text(
    std::string_view text) noexcept {
    MapChangeCatalog result;
    std::size_t cursor = 0;
    while (cursor < text.size()) {
        const auto line_end = text.find('\n', cursor);
        auto line = text.substr(cursor,
            line_end == std::string_view::npos ? text.size() - cursor
                                               : line_end - cursor);
        cursor = line_end == std::string_view::npos ? text.size() : line_end + 1;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.empty()) continue;
        const auto values = fields(line);
        // One shipped legacy row omits CurMapName, leaving nine tab fields;
        // CMHFile's sequential GetString/GetWord reader still consumes it as
        // an empty field. Normalize that exact representation here.
        std::vector<std::string_view> normalized;
        if (values.size() == 9) {
            normalized.reserve(10);
            normalized.push_back(values[0]);
            normalized.emplace_back();
            normalized.insert(normalized.end(), values.begin() + 1, values.end());
        } else if (values.size() == 10) {
            normalized = values;
        } else {
            return std::nullopt;
        }
        MapChangeEntry entry;
        if (!parse_u16(normalized[0], entry.kind) ||
            !parse_u16(normalized[3], entry.current_map_num) ||
            !parse_u16(normalized[4], entry.move_map_num) ||
            !parse_float(normalized[5], entry.current_x) ||
            !parse_float(normalized[6], entry.current_z) ||
            !parse_float(normalized[7], entry.move_x) ||
            !parse_float(normalized[8], entry.move_z) ||
            !parse_u16(normalized[9], entry.chx_num)) return std::nullopt;
        entry.current_map_name = std::string(normalized[1]);
        entry.object_name = std::string(normalized[2]);
        result.entries.push_back(std::move(entry));
    }
    return result;
}

std::optional<MapChangeCatalog> load_map_change_bin(
    const std::filesystem::path& path) noexcept {
    const auto file = read_mh_bin(path);
    if (!file.ok()) return std::nullopt;
    const auto* data = reinterpret_cast<const char*>(file.value.data.data());
    return parse_map_change_text({data, file.value.data.size()});
}

}  // namespace mxh::compat
