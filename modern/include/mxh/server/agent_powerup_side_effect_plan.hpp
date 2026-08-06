
//
// D4.116 -- AgentPowerUp side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_POWERUPMsgParser (lines 206-208) + ServerSystem self-init (lines 309-318) +
// MP_AGENTSERVERMsgParser (lines 230-280) + MP_SERVER_REGISTMAP_ACK (265-280).
// The data plane (classify_powerup + classify_powerup_self_init +
// classify_powerup_server_kind_dispatch + classify_powerup_registmap_ack) decides
// which action to take; this header captures the ordered side-effect list the
// orchestrator must execute.
//
// Side effects (legacy BOOTMNGR + ServerTable + dispatch):
//   - forward_to_boot_manager: BOOTMNGR->NetworkMsgParse(pMsg, dwLength).
//   - add_self_boot_list: BOOTMNGR->AddSelfBootList(ServerNum, ServerPort).
//   - start_server: BOOTMNGR->StartServer(ServerPort) (legacy ASSERT(0) on fail).
//   - connect_to_ms: BOOTMNGR->ConnectToMS().
//   - send_registmap_ack_to_ms: build MSG_WORD2 (Category=SERVER, Protocol=REGISTMAP_ACK,
//     wData1=mapServerPort, wData2=loadAgentNum) to MS connection.
//   - send_user_count_to_distribute: build MSG_WORD3 (Category=SERVER, Protocol=USERCNT,
//     wPortForServer, dwUserCount=agent_user_count) to distribute.
//   - send_registmap_syn_to_map: build MSG_WORD (Category=SERVER, Protocol=REGISTMAP_SYN,
//     wData=map_num) to map server connection.
//   - set_map_regist: ServerTable->SetMapRegist(wHaveMapNum, port).
//   - map_user_unregist_login: ServerTable->MapUserUnRegistLoginMapInfo(port).
//   - drop_unknown_server_kind: silent drop.
//   - drop_unreachable_ms: trigger OnConnectServerFail(MONITOR_SERVER stub) and shutdown.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_powerup.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must dispatch in order.
enum class PowerUpSideEffectKind : std::uint8_t {
    Drop,                                  // unknown_server_kind or unreachable_ms
    ForwardToBootManager,                  // BOOTMNGR->NetworkMsgParse raw forward
    AddSelfBootList,                       // BOOTMNGR->AddSelfBootList
    StartServer,                           // BOOTMNGR->StartServer (ASSERT(0) on fail)
    ConnectToMS,                           // BOOTMNGR->ConnectToMS
    SendRegistMapAckToMS,                  // MSG_WORD2 (REGISTMAP_ACK) to MS
    SendUserCountToDistribute,             // MSG_WORD3 (USERCNT) to distribute
    SendRegistMapSynToMap,                 // MSG_WORD (REGISTMAP_SYN) to map
    SetMapRegist,                          // ServerTable->SetMapRegist
    MapUserUnRegistLogin,                  // ServerTable->MapUserUnRegistLoginMapInfo
};

struct PowerUpSideEffect final {
    PowerUpSideEffectKind kind = PowerUpSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t target_port = 0u;
    std::uint16_t map_num = 0u;
    std::uint16_t user_count = 0u;
    bool forward_payload = false;
    bool need_assert = false;
};

struct PowerUpSideEffectPlan final {
    std::vector<PowerUpSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool powerup_effect_targets_boot_manager(const PowerUpSideEffect& e) noexcept {
    return e.kind == PowerUpSideEffectKind::ForwardToBootManager ||
           e.kind == PowerUpSideEffectKind::AddSelfBootList ||
           e.kind == PowerUpSideEffectKind::StartServer ||
           e.kind == PowerUpSideEffectKind::ConnectToMS;
}

inline bool powerup_effect_targets_server_table(const PowerUpSideEffect& e) noexcept {
    return e.kind == PowerUpSideEffectKind::SetMapRegist ||
           e.kind == PowerUpSideEffectKind::MapUserUnRegistLogin;
}

inline bool powerup_effect_targets_remote(const PowerUpSideEffect& e) noexcept {
    return e.kind == PowerUpSideEffectKind::SendRegistMapAckToMS ||
           e.kind == PowerUpSideEffectKind::SendUserCountToDistribute ||
           e.kind == PowerUpSideEffectKind::SendRegistMapSynToMap;
}

inline PowerUpSideEffectPlan powerup_side_effect_plan(const PowerUpAction& a) {
    PowerUpSideEffectPlan plan;
    using K = PowerUpSideEffectKind;
    using A = PowerUpActionKind;
    switch (a.kind) {
        case A::forward_to_boot_manager:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardToBootManager, a.reply_protocol, a.target_port, a.map_num, a.user_count, a.forward_payload, a.need_assert});
            return plan;
        case A::add_self_boot_list:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::AddSelfBootList, a.reply_protocol, a.target_port, a.map_num, a.user_count, false, false});
            return plan;
        case A::start_server:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::StartServer, a.reply_protocol, a.target_port, a.map_num, a.user_count, false, a.need_assert});
            return plan;
        case A::connect_to_ms:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ConnectToMS, a.reply_protocol, a.target_port, a.map_num, a.user_count, false, false});
            return plan;
        case A::send_registmap_ack_to_ms:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendRegistMapAckToMS, a.reply_protocol, a.target_port, a.map_num, a.user_count, a.forward_payload, false});
            return plan;
        case A::send_user_count_to_distribute:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendUserCountToDistribute, a.reply_protocol, a.target_port, a.map_num, a.user_count, a.forward_payload, false});
            return plan;
        case A::send_registmap_syn_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendRegistMapSynToMap, a.reply_protocol, a.target_port, a.map_num, a.user_count, a.forward_payload, false});
            return plan;
        case A::set_map_regist:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SetMapRegist, a.reply_protocol, a.target_port, a.map_num, a.user_count, false, false});
            return plan;
        case A::map_user_unregist_login:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::MapUserUnRegistLogin, a.reply_protocol, a.target_port, a.map_num, a.user_count, false, false});
            return plan;
        case A::drop_unknown_server_kind:
        case A::drop_unreachable_ms:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.reply_protocol, a.target_port, a.map_num, a.user_count, false, a.need_assert});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server