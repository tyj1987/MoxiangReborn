#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat {

struct CharacterModList {
    std::string base_object;
    std::vector<std::string> mod_files;
};

struct CharacterAppearanceCatalog {
    CharacterModList body;
    std::vector<std::string> faces;
    std::vector<std::string> hairs;

    [[nodiscard]] static std::optional<CharacterModList> parse_mod_list_bin(
        std::span<const std::uint8_t> bytes);
    [[nodiscard]] static std::optional<std::vector<std::string>> parse_part_list_bin(
        std::span<const std::uint8_t> bytes);
};

} // namespace mxh::compat
