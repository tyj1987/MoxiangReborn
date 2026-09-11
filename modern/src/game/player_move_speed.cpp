#include "mxh/game/player_move_speed.hpp"
#include <cmath>

namespace mxh::game {
std::optional<float> player_move_speed(const PlayerMoveSpeedInput& p,
    std::span<const MoveSpeedStatus> statuses) noexcept {
    float base = 0;
    if ((p.kyunggong_id || (p.in_titan && p.run)) && !p.special_resources_resolved) return std::nullopt;
    if (p.in_titan) {
        if (p.kyunggong_id) {
            if (!p.titan_grade_lightness) base = 300; // Original missing-grade fallback, no bonuses.
            else {
                const auto level = p.kyunggong_id == 2602 ? 1 : p.kyunggong_id == 2604 ? 2 : 0;
                base = (*p.titan_grade_lightness)[level];
                base += p.avatar_lightness;
                base += p.shop_lightness;
            }
        } else if (p.run) {
            if (!p.titan_run_speed) return std::nullopt;
            base = *p.titan_run_speed;
        } else base = 300;
    } else if (p.kyunggong_id) {
        if (!p.lightness_base) base = 0; // Original missing-skill return, no bonuses.
        else {
            base = *p.lightness_base + p.ability_lightness;
            base += p.avatar_lightness;
            base += p.shop_lightness;
        }
    } else base = p.run ? 400.0f : 200.0f;
    if (!std::isfinite(base)) return std::nullopt;
    float up = 0, down = 0;
    for (const auto& status : statuses) {
        if (status.up_percent) up = base * (status.up_percent * 0.01f);
        if (status.down_percent) down = base * (status.down_percent * 0.01f);
    }
    const float result = (base + up) - down;
    if (!std::isfinite(result)) return std::nullopt;
    return result;
}
}
