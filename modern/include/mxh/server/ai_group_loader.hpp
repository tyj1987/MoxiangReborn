#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::server {

struct AiGroupCondition final {
    std::uint32_t target_group_id = 0;
    float remainder_ratio = 0.0f;
    std::uint32_t regen_delay = 0;
    bool regen = false;
};

struct AiSpawnDefinition final {
    std::uint8_t object_kind = 0;
    std::uint32_t source_object_id = 0;
    std::uint16_t monster_kind = 0;
    float pos_x = 0.0f;
    float pos_z = 0.0f;
    bool initially_dead = false;
};

enum class AiRegenDelayKind : std::uint8_t {
    None = 0,
    FixedUnique = 1,
    RandomGroup = 2,
    RandomUnique = 3,
    RandomUniqueRange = 4,
};

struct AiRegenDelaySpec final {
    AiRegenDelayKind kind = AiRegenDelayKind::None;
    std::uint32_t min_minutes = 0;
    std::uint32_t max_minutes = 0;
};

struct AiGroupDefinition final {
    std::uint32_t group_id = 0;
    bool unique = false;
    std::uint32_t max_object = 0;
    std::uint16_t property = 0;
    std::string group_name;
    std::vector<AiGroupCondition> conditions;
    std::vector<AiSpawnDefinition> spawns;
    AiRegenDelaySpec regen_delay;
};

struct AiFieldBossPosition final {
    float x = 0.0f;
    float z = 0.0f;
};

struct AiGroupList final {
    std::vector<AiGroupDefinition> groups;
    std::vector<AiFieldBossPosition> field_boss_positions;

    std::size_t spawn_count() const noexcept;
    const AiGroupDefinition* find_group(std::uint32_t group_id) const noexcept;
};

std::optional<AiGroupList> parse_ai_group_list(std::string_view text) noexcept;
std::optional<AiGroupList> load_ai_group_list_bin(
    const std::filesystem::path& path) noexcept;

}  // namespace mxh::server
