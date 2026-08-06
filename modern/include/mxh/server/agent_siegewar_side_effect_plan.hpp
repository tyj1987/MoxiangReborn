#pragma once
//
// D4.100 -- AgentSiegeWar side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_SIEGEWARUserMsgParser (lines 4788-4920) and MP_SIEGEWARServerMsgParser
// (lines 4923-5010). The data plane (classify_siegewar_user + classify_siegewar_server)
// decides which action to take; this header captures the ordered side-effect
// list the orchestrator must execute.
//
// USER side-effects (per legacy body lines):
//   - cheat (61): fanout the raw MSG_DWORD4 to BOTH (dwData2, dwData3) map servers,
//     each independently gated by GetServerPort + FindServer. Up to 2 fanouts.
//   - movein_syn (1): forward raw MSG_DWORD2 to user->dwMapServerConnectionIndex.
//   - battlejoin_syn (7) / observerjoin_syn (10) with target_map_found:
//     update wUserMapNum+dwMapServerConnectionIndex, build SEND_SIEGEWAR_JOININFO
//     (Category, Protocol, dwObjectID, AgentIdx, UserLevel, GuildIdx, ReturnMapNum,
//     bObserver), send to target map connection. NO user-side reply.
//   - battlejoin_syn / observerjoin_syn without target_map_found: send raw
//     BATTLEJOIN_NACK (cat=SIEGEWAR proto=9) to user. NO map-side forward.
//   - leave_syn (12): build MSG_DWORD3 (Category=SIEGEWAR, Protocol=LEAVE_SYN,
//     dwObjectID, dwData1=UniqueConnectIdx, dwData2=UserLevel, dwData3=wChannel),
//     send to user->dwMapServerConnectionIndex.
//   - default: TransToMapServerMsgParser (forward raw to map).
//   - no user: drop.
//
// SERVER side-effects (per legacy body lines):
//   - taxrate (60): build MSG_DWORD (Category=SIEGEWAR, Protocol=TAXRATE,
//     dwData=Param), then for each of the affected_count entries lookup the
//     map server port + connection; if both found, send the MSG_DWORD to that
//     connection. Skip silently when port or connection is missing.
//   - returntomap (50): when user_found, lookup target_map server; if found,
//     update wUserMapNum + dwMapServerConnectionIndex; ALWAYS forward raw
//     MSG_DWORD to client (TransToClientMsgParser).
//   - flagchange (62): iterate ALL user table entries; for each, send raw
//     MSG_DWORD to user->dwConnectionIndex.
//   - default: TransToClientMsgParser (forward raw to client).
//   - no user (returntomap only): drop.

#include <cstdint>
#include <vector>

#include "mxh/server/agent_siegewar.hpp"
#include "mxh/server/agent_siegewar_server.hpp"

namespace mxh::server {
// USER side-effect kinds the orchestrator must dispatch in order.
enum class SiegeWarUserSideEffectKind : std::uint8_t {
    Drop,
    FanoutToMapServers,        // raw MSG_DWORD4 to up to 2 map connection indices (Data2, Data3)
    ForwardRawToUserMap,       // raw message to user->dwMapServerConnectionIndex
    ForwardBattleJoinToMap,    // SEND_SIEGEWAR_JOININFO to target_map server + update wUserMapNum + dwMapServerConnectionIndex
    SendBattleJoinNackToUser,  // raw BATTLEJOIN_NACK to user
    ForwardLeaveSynToUserMap,  // MSG_DWORD3 with unique_connect_idx/user_level/channel to user->dwMapServerConnectionIndex
    ForwardRawToMap,           // generic TransToMapServerMsgParser
};

struct SiegeWarUserSideEffect final {
    SiegeWarUserSideEffectKind kind = SiegeWarUserSideEffectKind::Drop;
    std::uint32_t data2_target_map = 0u;
    std::uint32_t data3_target_map = 0u;
    bool data2_map_found = false;
    bool data3_map_found = false;
    bool target_map_found = false;
    std::uint32_t guild_idx = 0u;
    std::uint32_t return_map_num = 0u;
    std::uint8_t observer_flag = 0u;
    std::uint32_t unique_connect_idx = 0u;
    std::uint8_t user_level = 0u;
    std::uint8_t channel = 0u;
};

struct SiegeWarUserSideEffectPlan final {
    std::vector<SiegeWarUserSideEffect> effects;
    bool dispatched = false;
    bool update_user_map_slot = false;
    bool drop = true;
};
inline bool siegewar_user_effect_targets_map(const SiegeWarUserSideEffect& e) noexcept {
    return e.kind != SiegeWarUserSideEffectKind::Drop &&
           e.kind != SiegeWarUserSideEffectKind::SendBattleJoinNackToUser;
}

inline bool siegewar_user_effect_targets_user(const SiegeWarUserSideEffect& e) noexcept {
    return e.kind == SiegeWarUserSideEffectKind::SendBattleJoinNackToUser;
}
inline SiegeWarUserSideEffectPlan siegewar_user_side_effect_plan(
        const SiegeWarUserAction& action,
        bool data2_map_found,
        bool data3_map_found) {
    SiegeWarUserSideEffectPlan plan;
    using K = SiegeWarUserSideEffectKind;
    using A = SiegeWarUserActionKind;
    switch (action.kind) {
        case A::drop_no_user:
            plan.drop = true;
            plan.effects.push_back({K::Drop, 0u, 0u, false, false, false, 0u, 0u, 0u,
                                    action.unique_connect_idx, action.user_level, action.channel});
            return plan;
        case A::cheat_fanout_to_map_servers:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::FanoutToMapServers, action.data2_target_map,
                                    action.data3_target_map, data2_map_found,
                                    data3_map_found, false, 0u, 0u, 0u,
                                    action.unique_connect_idx, action.user_level, action.channel});
            return plan;
        case A::movein_to_user_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToUserMap, 0u, 0u, false, false,
                                    false, 0u, 0u, 0u, action.unique_connect_idx,
                                    action.user_level, action.channel});
            return plan;
        case A::battlejoin_to_target_map_or_nack:
            plan.dispatched = true;
            plan.drop = false;
            if (action.protocol == siegewar_battlejoin_nack) {
                plan.effects.push_back({K::SendBattleJoinNackToUser, 0u, 0u, false,
                                        false, false, 0u, 0u, 0u,
                                        action.unique_connect_idx, action.user_level,
                                        action.channel});
            } else {
                plan.update_user_map_slot = true;
                plan.effects.push_back({K::ForwardBattleJoinToMap, 0u, 0u, false,
                                        false, true, action.guild_idx,
                                        action.return_map_num,
                                        action.observer_flag,
                                        action.unique_connect_idx, action.user_level,
                                        action.channel});
            }
            return plan;
        case A::leave_syn_to_user_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardLeaveSynToUserMap, 0u, 0u, false, false,
                                    false, 0u, 0u, 0u, action.unique_connect_idx,
                                    action.user_level, action.channel});
            return plan;
        case A::default_forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, 0u, 0u, false, false, false,
                                    0u, 0u, 0u, action.unique_connect_idx,
                                    action.user_level, action.channel});
            return plan;
    }
    return plan;
}

// SERVER side-effect kinds.
enum class SiegeWarServerSideEffectKind : std::uint8_t {
    Drop,
    BroadcastTaxrateToAffectedMaps,    // for each affected[i] with valid port+connection, send MSG_DWORD
    UpdateUserMapAndForwardToClient,  // update wUserMapNum+dwMapServerConnectionIndex + TransToClient
    BroadcastToAllUsers,              // iterate user table, send raw to each
    ForwardRawToClient,               // TransToClientMsgParser
};

struct SiegeWarServerSideEffect final {
    SiegeWarServerSideEffectKind kind = SiegeWarServerSideEffectKind::Drop;
    std::uint32_t taxrate_param = 0u;
    std::uint32_t target_map = 0u;
    bool target_map_found = false;
    std::uint16_t affected_count = 0u;
    bool update_user_map_slot = false;
};

struct SiegeWarServerSideEffectPlan final {
    std::vector<SiegeWarServerSideEffect> effects;
    bool dispatched = false;
    bool forward_to_client = false;
    bool drop = true;
};

inline bool siegewar_server_effect_targets_map(const SiegeWarServerSideEffect& e) noexcept {
    return e.kind == SiegeWarServerSideEffectKind::BroadcastTaxrateToAffectedMaps;
}

inline bool siegewar_server_effect_targets_client(const SiegeWarServerSideEffect& e) noexcept {
    return e.kind == SiegeWarServerSideEffectKind::UpdateUserMapAndForwardToClient ||
           e.kind == SiegeWarServerSideEffectKind::ForwardRawToClient;
}

inline SiegeWarServerSideEffectPlan siegewar_server_side_effect_plan(
        const SiegeWarServerAction& action,
        std::uint32_t taxrate_param,
        std::uint16_t affected_count) {
    SiegeWarServerSideEffectPlan plan;
    using K = SiegeWarServerSideEffectKind;
    using A = SiegeWarServerActionKind;
    switch (action.kind) {
        case A::drop_no_user:
            plan.drop = true;
            plan.effects.push_back({K::Drop, 0u, 0u, false, 0u, false});
            return plan;
        case A::broadcast_taxrate_to_affected_maps:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastTaxrateToAffectedMaps,
                                    taxrate_param, 0u, false, affected_count, false});
            return plan;
        case A::update_user_map_and_forward_to_client:
            plan.dispatched = true;
            plan.forward_to_client = true;
            plan.drop = false;
            plan.effects.push_back({K::UpdateUserMapAndForwardToClient, 0u,
                                    action.target_map, action.target_map > 0u, 0u, true});
            return plan;
        case A::broadcast_to_all_users:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastToAllUsers, 0u, 0u, false, 0u, false});
            return plan;
        case A::default_forward_to_client:
            plan.dispatched = true;
            plan.forward_to_client = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToClient, 0u, 0u, false, 0u, false});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server
