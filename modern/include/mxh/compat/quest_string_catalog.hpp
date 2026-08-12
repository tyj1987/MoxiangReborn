#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::compat {

struct QuestStringEntry {
    std::uint16_t quest_id = 0;
    std::uint16_t subquest_id = 0;
    std::string title;
    std::vector<std::string> description;
};

struct QuestStringCatalog {
    std::vector<QuestStringEntry> entries;
    std::string error_message;

    const QuestStringEntry* find(std::uint16_t quest_id,
                                 std::uint16_t subquest_id = 0) const noexcept;
    std::vector<const QuestStringEntry*> main_quests() const;
};

QuestStringCatalog parse_quest_string_text(std::string_view text) noexcept;
QuestStringCatalog load_quest_string_catalog(const std::filesystem::path& path);

}  // namespace mxh::compat
