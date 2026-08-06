
//
// D4.109 -- AgentSiegeWarProfit side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_SIEGEWARPROFITUserMsgParser + MP_SIEGEWARPROFITServerMsgParser. The data plane
// (classify_siegewarprofit_user + classify_siegewarprofit_server) decides which action
// to take; this header captures the ordered side-effect list the orchestrator must execute.
//
// USER side-effects:
//   - forward_to_map: unconditional TransToMapServerMsgParser (raw forwarding).
//
// SERVER side-effects:
//   - broadcast_to_other_maps: raw Broadcast2MapServerExceptOne for
//     CHANGE_TEXRATE_NOTIFY_TO_MAP (7) + CHANGE_GUILD_NOTIFY_TO_MAP (11).
//   - forward_to_client: TransToClientMsgParser fallback.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_siegewarprofit.hpp"

namespace mxh::server {

// USER side-effect kinds the orchestrator must dispatch in order.
enum class SiegeWarProfitUserSideEffectKind : std::uint8_t {
    ForwardRawToMap,                       // unconditional TransToMapServerMsgParser
};

struct SiegeWarProfitUserSideEffect final {
    SiegeWarProfitUserSideEffectKind kind = SiegeWarProfitUserSideEffectKind::ForwardRawToMap;
};

struct SiegeWarProfitUserSideEffectPlan final {
    std::vector<SiegeWarProfitUserSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool siegewarprofit_user_effect_targets_map(const SiegeWarProfitUserSideEffect&) noexcept {
    return true;
}

inline SiegeWarProfitUserSideEffectPlan siegewarprofit_user_side_effect_plan(const SiegeWarProfitUserAction& a) {
    SiegeWarProfitUserSideEffectPlan plan;
    using K = SiegeWarProfitUserSideEffectKind;
    using A = SiegeWarProfitUserActionKind;
    switch (a.kind) {
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap});
            return plan;
    }
    return plan;
}

// SERVER side-effect kinds.
enum class SiegeWarProfitServerSideEffectKind : std::uint8_t {
    BroadcastToOtherMaps,                  // raw Broadcast2MapServerExceptOne
    ForwardRawToClient,                    // TransToClientMsgParser fallback
};

struct SiegeWarProfitServerSideEffect final {
    SiegeWarProfitServerSideEffectKind kind = SiegeWarProfitServerSideEffectKind::ForwardRawToClient;
    std::uint8_t protocol = 0u;
};

struct SiegeWarProfitServerSideEffectPlan final {
    std::vector<SiegeWarProfitServerSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool siegewarprofit_server_effect_targets_map(const SiegeWarProfitServerSideEffect& e) noexcept {
    return e.kind == SiegeWarProfitServerSideEffectKind::BroadcastToOtherMaps;
}

inline bool siegewarprofit_server_effect_targets_client(const SiegeWarProfitServerSideEffect& e) noexcept {
    return e.kind == SiegeWarProfitServerSideEffectKind::ForwardRawToClient;
}

inline SiegeWarProfitServerSideEffectPlan siegewarprofit_server_side_effect_plan(const SiegeWarProfitServerAction& a) {
    SiegeWarProfitServerSideEffectPlan plan;
    using K = SiegeWarProfitServerSideEffectKind;
    using A = SiegeWarProfitServerActionKind;
    switch (a.kind) {
        case A::broadcast_to_other_maps:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastToOtherMaps, a.protocol});
            return plan;
        case A::forward_to_client:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToClient, a.protocol});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server