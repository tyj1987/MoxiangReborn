#pragma once
#include <cstdint>

namespace mxh::server {
// Resolved server facts, never fields supplied by the requesting client.
// MapNetworkMsgParser.cpp:3836 and Player.cpp:2162 in the original Map server.
struct PresentReviveContext {
    bool event_map;
    bool dead;
    bool looted;
    bool exiting;
    bool penalty_exempt_map; // siege, tournament, event, quest room, survival
    bool battle_channel;
    bool died_for_battle_channel;
    bool died_for_guild_field_war;
    bool korean_first_action_penalty; // false outside the KOR build
    std::uint16_t level;
};
enum class PresentReviveDecision {
    IgnoreEventMap, RejectNotDead, RejectLooted, RejectExiting,
    RecoverWithoutLoss, RecoverWithLoss
};
inline PresentReviveDecision decide_present_revive(const PresentReviveContext& c) noexcept {
    if (c.event_map) return PresentReviveDecision::IgnoreEventMap;
    if (!c.dead) return PresentReviveDecision::RejectNotDead;
    if (c.looted) return PresentReviveDecision::RejectLooted;
    if (c.exiting) return PresentReviveDecision::RejectExiting;
    if (c.korean_first_action_penalty || c.penalty_exempt_map ||
        (c.battle_channel && c.died_for_battle_channel) ||
        c.level < 5 || c.died_for_guild_field_war)
        return PresentReviveDecision::RecoverWithoutLoss;
    return PresentReviveDecision::RecoverWithLoss;
}
} // namespace mxh::server
