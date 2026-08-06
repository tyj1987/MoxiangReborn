
//
// D4.112 -- AgentGuild USER side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_GUILDUserMsgParser. The data plane (classify_guild_user + classify_guild_server_default)
// decides which action to take; this header captures the ordered side-effect list
// the orchestrator must execute.
//
// USER side-effects:
//   - send_nack: build MSG_DWORD (Protocol=GUILD_CREATE_NACK or GUILD_GIVENICKNAME_NACK,
//     dwData=guild_err_create_name or guild_err_nick_filter) to user->dwConnectionIndex.
//   - forward: TransToMapServerMsgParser (raw forward CREATE_SYN or GIVENICKNAME_SYN).
//
// SERVER side-effects (no state, simple pass-through):
//   - forward: unconditional TransToClientMsgParser fallback.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_guild.hpp"

namespace mxh::server {

// USER side-effect kinds the orchestrator must dispatch in order.
enum class GuildUserSideEffectKind : std::uint8_t {
    ForwardRawToMap,                       // CREATE_SYN or GIVENICKNAME_SYN
    SendNackToUser,                        // CREATE_NACK (err=4) or GIVENICKNAME_NACK (err=1)
};

struct GuildUserSideEffect final {
    GuildUserSideEffectKind kind = GuildUserSideEffectKind::ForwardRawToMap;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint8_t error_code = 0u;
};

struct GuildUserSideEffectPlan final {
    std::vector<GuildUserSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool guild_user_effect_targets_map(const GuildUserSideEffect& e) noexcept {
    return e.kind == GuildUserSideEffectKind::ForwardRawToMap;
}

inline bool guild_user_effect_targets_user(const GuildUserSideEffect& e) noexcept {
    return e.kind == GuildUserSideEffectKind::SendNackToUser;
}

inline GuildUserSideEffectPlan guild_user_side_effect_plan(const GuildAction& a) {
    GuildUserSideEffectPlan plan;
    using K = GuildUserSideEffectKind;
    using A = GuildActionKind;
    switch (a.kind) {
        case A::forward:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.object_id, 0u});
            return plan;
        case A::send_nack:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendNackToUser, a.protocol, a.object_id, a.error_code});
            return plan;
    }
    return plan;
}

// SERVER side-effect kinds (simple pass-through).
enum class GuildServerSideEffectKind : std::uint8_t {
    ForwardRawToClient,                    // TransToClientMsgParser fallback
};

struct GuildServerSideEffect final {
    GuildServerSideEffectKind kind = GuildServerSideEffectKind::ForwardRawToClient;
    std::uint8_t protocol = 0u;
};

struct GuildServerSideEffectPlan final {
    std::vector<GuildServerSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool guild_server_effect_targets_client(const GuildServerSideEffect&) noexcept {
    return true;
}

inline GuildServerSideEffectPlan guild_server_side_effect_plan(const GuildAction& a) {
    GuildServerSideEffectPlan plan;
    using K = GuildServerSideEffectKind;
    using A = GuildActionKind;
    switch (a.kind) {
        case A::forward:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToClient, a.protocol});
            return plan;
        case A::send_nack:
            // server-side classifier only emits forward; treat as drop
            plan.drop = true;
            plan.effects.push_back({K::ForwardRawToClient, a.protocol});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server