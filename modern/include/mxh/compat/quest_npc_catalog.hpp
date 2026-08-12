#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::compat {

struct QuestNpcEntry {
    std::uint16_t map_num = 0;
    std::uint16_t npc_kind = 0;
    std::string name;
    std::uint16_t npc_index = 0;
    std::uint16_t position_x = 0;
    std::uint16_t position_z = 0;
    float direction = 0;
};

struct QuestNpcCatalog {
    std::vector<QuestNpcEntry> entries;
    std::uint32_t parse_errors = 0;
    std::string error_message;
};

std::size_t quest_npc_count_for_map(const QuestNpcCatalog& catalog,
                                    std::uint16_t map_num) noexcept;

QuestNpcCatalog parse_quest_npc_text(std::string_view text) noexcept;
QuestNpcCatalog load_quest_npc_catalog(const std::filesystem::path& path);

}  // namespace mxh::compat
