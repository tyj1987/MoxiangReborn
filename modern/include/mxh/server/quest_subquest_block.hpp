#pragma once

#include "mxh/server/quest_execute.hpp"
#include "mxh/server/quest_script_line.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace mxh::server {

struct QuestSubquestEntry final {
    std::uint32_t subquest_idx = 0;
    std::vector<QuestLimitSpec> limits;
    std::vector<QuestScriptLine> triggers;
};

std::optional<std::vector<QuestLimitSpec>> parse_quest_subquest_limit_line(
    std::string_view line) noexcept;

std::optional<QuestSubquestEntry> parse_quest_subquest_block(
    std::string_view block, std::uint32_t quest_idx,
    std::uint32_t subquest_idx) noexcept;

std::optional<QuestSubquestEntry> parse_quest_subquest_stanza(
    std::string_view stanza, std::uint32_t quest_idx) noexcept;

}  // namespace mxh::server
