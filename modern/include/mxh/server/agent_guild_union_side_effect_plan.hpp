#pragma once

//
// D4.101 -- AgentGuildUnion side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_GUILD_UNIONUserMsgParser (lines 4735-4763) and MP_GUILD_UNIONServerMsgParser
// (lines 4765-4790). The data plane (classify_guild_union_user +
// classify_guild_union_server) decides which action to take; this header captures
// the ordered side-effect list the orchestrator must execute.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_guild_union.hpp"

namespace mxh::server {


// USER side-effect kinds.
enum class GuildUnionUserSideEffectKind : std::uint8_t {
    Drop,
    SendCreateNackToUser,        // MSG_DWORD2 (Category, Protocol=CREATE_NACK, dwData1=NotValidName, dwData2=0)
    ForwardRawToMap,             // TransToMapServerMsgParser (legacy forward path)
};

struct GuildUnionUserSideEffect final {
    GuildUnionUserSideEffectKind kind = GuildUnionUserSideEffectKind::Drop;
    std::uint32_t error_code = 0u;
};

struct GuildUnionUserSideEffectPlan final {
    std::vector<GuildUnionUserSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool guild_union_user_effect_targets_map(const GuildUnionUserSideEffect& e) noexcept {
    return e.kind == GuildUnionUserSideEffectKind::ForwardRawToMap;
}

inline bool guild_union_user_effect_targets_user(const GuildUnionUserSideEffect& e) noexcept {
    return e.kind == GuildUnionUserSideEffectKind::SendCreateNackToUser;
}

inline GuildUnionUserSideEffectPlan guild_union_user_side_effect_plan(const GuildUnionAction& action) {
    GuildUnionUserSideEffectPlan plan;
    using K = GuildUnionUserSideEffectKind;
    using A = GuildUnionActionKind;
    switch (action.kind) {
        case A::drop_no_user:
            plan.drop = true;
            plan.effects.push_back({K::Drop, 0u});
            return plan;
        case A::send_create_nack_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendCreateNackToUser, action.error_code});
            return plan;
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, 0u});
            return plan;
    }
    return plan;
}

// SERVER side-effect kinds.
enum class GuildUnionServerSideEffectKind : std::uint8_t {
    Drop,
    BroadcastToOtherMaps,        // Broadcast2MapServerExceptOne (raw forwarding to all maps except sender)
};

struct GuildUnionServerSideEffect final {
    GuildUnionServerSideEffectKind kind = GuildUnionServerSideEffectKind::Drop;
};

struct GuildUnionServerSideEffectPlan final {
    std::vector<GuildUnionServerSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool guild_union_server_effect_targets_map(const GuildUnionServerSideEffect& e) noexcept {
    return e.kind == GuildUnionServerSideEffectKind::BroadcastToOtherMaps;
}

inline GuildUnionServerSideEffectPlan guild_union_server_side_effect_plan(const GuildUnionServerAction& action) {
    GuildUnionServerSideEffectPlan plan;
    using K = GuildUnionServerSideEffectKind;
    using A = GuildUnionServerActionKind;
    switch (action.kind) {
        case A::broadcast_to_other_maps:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastToOtherMaps});
            return plan;
        case A::default_forward_to_client:
        case A::drop_unknown:
            plan.drop = true;
            plan.effects.push_back({K::Drop});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server
