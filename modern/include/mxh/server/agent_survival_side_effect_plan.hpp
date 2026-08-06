
//
// D4.105 -- AgentSurvival side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_SURVIVALUserMsgParser (lines 5094-5105) and MP_SURVIVALServerMsgParser
// (lines 5105-5158). The data plane (classify_survival_user + classify_survival_server)
// decides which action to take; this header captures the ordered side-effect
// list the orchestrator must execute.
//
// USER side-effects:
//   - send_leave_syn_to_map: build MSG_DWORD3 (Category=SURVIVAL, Protocol=LEAVE_SYN,
//     dwObjectID, dwData1=UniqueConnectIdx, dwData2=UserLevel, dwData3=wChannel)
//     send to user->dwMapServerConnectionIndex.
//   - gm_protected_forward_to_map: forward raw MSG_DWORD to map server (GM tools).
//   - default_forward_to_map: TransToMapServerMsgParser fallback.
//
// SERVER side-effects:
//   - update_user_map_and_forward_to_client: update wUserMapNum+dwMapServerConnectionIndex
//     to target_map, then raw forward MSG_DWORD to client (TransToClientMsgParser).
//   - default_forward_to_client: TransToClientMsgParser fallback.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_survival.hpp"

namespace mxh::server {

// USER side-effect kinds the orchestrator must dispatch in order.
enum class SurvivalUserSideEffectKind : std::uint8_t {
    ForwardRawToMap,                      // TransToMapServerMsgParser default or GM-protected raw forward
    SendLeaveSynToMap,                    // MSG_DWORD3 with unique_connect_idx/user_level/channel to user->dwMapServerConnectionIndex
};

struct SurvivalUserSideEffect final {
    SurvivalUserSideEffectKind kind = SurvivalUserSideEffectKind::ForwardRawToMap;
    std::uint32_t object_id = 0u;
    std::uint32_t unique_connect_idx = 0u;
    std::uint8_t user_level = 0u;
    std::uint8_t channel = 0u;
};

struct SurvivalUserSideEffectPlan final {
    std::vector<SurvivalUserSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool survival_user_effect_targets_map(const SurvivalUserSideEffect& e) noexcept {
    return e.kind == SurvivalUserSideEffectKind::ForwardRawToMap ||
           e.kind == SurvivalUserSideEffectKind::SendLeaveSynToMap;
}

inline SurvivalUserSideEffectPlan survival_user_side_effect_plan(const SurvivalUserAction& a) {
    SurvivalUserSideEffectPlan plan;
    using K = SurvivalUserSideEffectKind;
    using A = SurvivalUserActionKind;
    switch (a.kind) {
        case A::send_leave_syn_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendLeaveSynToMap, a.object_id, a.unique_connect_idx, a.user_level, a.channel});
            return plan;
        case A::gm_protected_forward_to_map:
        case A::default_forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.object_id, 0u, 0u, 0u});
            return plan;
    }
    return plan;
}

// SERVER side-effect kinds.
enum class SurvivalServerSideEffectKind : std::uint8_t {
    ForwardRawToClient,                   // TransToClientMsgParser default
    UpdateUserMapAndForwardToClient,       // update wUserMapNum+dwMapServerConnectionIndex then raw forward
};

struct SurvivalServerSideEffect final {
    SurvivalServerSideEffectKind kind = SurvivalServerSideEffectKind::ForwardRawToClient;
    std::uint32_t object_id = 0u;
    std::uint32_t target_map = 0u;
    bool update_user_state = false;
};

struct SurvivalServerSideEffectPlan final {
    std::vector<SurvivalServerSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool survival_server_effect_targets_client(const SurvivalServerSideEffect& e) noexcept {
    return e.kind == SurvivalServerSideEffectKind::ForwardRawToClient ||
           e.kind == SurvivalServerSideEffectKind::UpdateUserMapAndForwardToClient;
}

inline SurvivalServerSideEffectPlan survival_server_side_effect_plan(const SurvivalServerAction& a) {
    SurvivalServerSideEffectPlan plan;
    using K = SurvivalServerSideEffectKind;
    using A = SurvivalServerActionKind;
    switch (a.kind) {
        case A::update_user_map_and_forward_to_client:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::UpdateUserMapAndForwardToClient, a.object_id, a.target_map, a.update_user_state});
            return plan;
        case A::default_forward_to_client:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToClient, a.object_id, 0u, false});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server