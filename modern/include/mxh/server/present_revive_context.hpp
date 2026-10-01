#pragma once
#include "mxh/server/player.hpp"
#include "mxh/server/looting_manager.hpp"
#include "mxh/server/present_revive_policy.hpp"
#include "mxh/game/map_kind.hpp"

namespace mxh::server {
// The remaining environment facts come from their owning server systems.
// Unknown map semantics must not silently select ordinary-map penalties.
inline std::optional<PresentReviveContext> resolve_present_revive_context(
    const Player& player,const LootingManagerState& looting,
    const mxh::game::MapKindTable& maps,bool battle_channel,
    bool exit_started,bool korean_first_action_penalty) {
    const auto& state=player.state();
    const auto flags=maps.resolved_flags(state.map_num);
    if(!flags && state.map_num!=58) return std::nullopt;
    // EventMapMgr::IsEventMap compares EVENTMAPNUM (58), not the EVENT flag.
    return PresentReviveContext{state.map_num==58,player.lifecycle()==PlayerLifecycle::Dead,
        is_looted_player(looting,state.player_id),
        exit_started || player.lifecycle()==PlayerLifecycle::LoggingOut,
        (flags.value_or(0) & mxh::game::present_revive_exempt_map_mask)!=0,battle_channel,
        state.death_flags.battle_channel,state.death_flags.guild_field_war,
        korean_first_action_penalty,state.progress.level};
}
} // namespace mxh::server
