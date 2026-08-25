#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::compat {

struct MapChangeEntry final {
    std::uint16_t kind = 0;
    std::string current_map_name;
    std::string object_name;
    std::uint16_t current_map_num = 0;
    std::uint16_t move_map_num = 0;
    float current_x = 0.0f;
    float current_z = 0.0f;
    float move_x = 0.0f;
    float move_z = 0.0f;
    std::uint16_t chx_num = 0;
};

struct MapChangeCatalog final {
    std::vector<MapChangeEntry> entries;

    [[nodiscard]] const MapChangeEntry* find_kind(
        std::uint16_t kind) const noexcept;
    [[nodiscard]] const MapChangeEntry* find_destination(
        std::uint16_t current_map, std::uint16_t destination_map) const noexcept;
};

[[nodiscard]] std::optional<MapChangeCatalog> parse_map_change_text(
    std::string_view text) noexcept;
[[nodiscard]] std::optional<MapChangeCatalog> load_map_change_bin(
    const std::filesystem::path& path) noexcept;

}  // namespace mxh::compat
