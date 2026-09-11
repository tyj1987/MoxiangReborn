#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace mxh::game {
struct MoveSpeedStatus { std::uint16_t up_percent = 0, down_percent = 0; };
struct PlayerMoveSpeedInput {
    bool run = true, in_titan = false;
    std::uint16_t kyunggong_id = 0;
    // Required for resource-dependent Titan/run or lightness branches. A resolved missing resource is
    // distinct from a modern caller that has not loaded the authoritative data.
    bool special_resources_resolved = false;
    std::optional<float> lightness_base;
    std::optional<float> titan_run_speed;
    std::optional<std::array<float,3>> titan_grade_lightness;
    float ability_lightness = 0, avatar_lightness = 0, shop_lightness = 0;
};
// Original Player::DoGetMoveSpeed + Object::GET_STATUS. Status order matters:
// each nonzero modifier overwrites its direction's prior value, not sums it.
// A negative source result is retained; movement admission must not run backwards.
std::optional<float> player_move_speed(const PlayerMoveSpeedInput& input,
    std::span<const MoveSpeedStatus> statuses = {}) noexcept;
}
