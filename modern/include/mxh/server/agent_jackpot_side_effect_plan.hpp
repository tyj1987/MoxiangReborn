//
// D4.167 -- AgentJackpot side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_JACKPOTUserMsgParser (lines 4616-4619, empty) and MP_JACKPOTServerMsgParser
// (lines 4621-4673). The data plane (classify_jackpot_user / classify_jackpot_server)
// decides the action; this header captures the ordered side-effect list the
// orchestrator must execute.
//
// Legacy semantics (preserved verbatim):
// USER side: empty handler -> every sub-protocol silently dropped.
// SERVER side:
//   MP_JACKPOT_PRIZE_NOTIFY (0):
//       JACKPOTMGR->SetTotalMoney(pmsg->dwRestTotalMoney);
//       broadcast pmsg verbatim to every USERINFO in g_pUserTable.
//   MP_JACKPOT_TOTALMONEY_NOTIFY_TO_AGENT (2):
//       JACKPOTMGR->SetTotalMoney(pmsg->dwData);
//       mutate pmsg->Protocol = MP_JACKPOT_TOTALMONEY_NOTIFY;
//       broadcast to every USERINFO with wUserMapNum != 0.
//   PRIZE_EFFECT, CHEAT_MAPTOTALMONEY, TOTALMONEY_NOTIFY: default TransToClientMsgParser fallback.
//   unknown protocol: silent drop.
//
// Side effects:
//   - SetJackpotTotalMoney: calls JACKPOTMGR->SetTotalMoney with the supplied value.
//   - BroadcastAllUsers: Send2User for every USERINFO slot, no map filter.
//   - BroadcastInMapUsers: Send2User for every USERINFO with wUserMapNum != 0,
//       rewritten protocol = jackpot_totalmoney_notify.
//   - ForwardToOriginatingClient: TransToClientMsgParser fallback.
//   - Drop: silent no-op (user side / unknown).

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "mxh/server/agent_jackpot.hpp"

namespace mxh::server {

// Side-effect kinds the AgentJackpot dispatcher must execute in order.
enum class JackpotSideEffectKind : std::uint8_t {
    Drop,
    SetJackpotTotalMoney,
    BroadcastAllUsers,
    BroadcastInMapUsers,
    ForwardToOriginatingClient,
};

struct JackpotSideEffect final {
    JackpotSideEffectKind kind = JackpotSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint8_t rewritten_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    std::uint32_t jackpot_total_money = 0u;
    bool rewrite_protocol = false;
    bool forward_payload = true;
};

struct JackpotSideEffectPlan final {
    std::vector<JackpotSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
    bool broadcast = false;
    bool broadcast_in_map = false;
    bool set_total_money = false;
    bool rewrite_protocol = false;
};

inline JackpotSideEffectPlan jackpot_user_side_effect_plan(const JackpotUserAction& a) {
    JackpotSideEffectPlan plan;
    using S = JackpotSideEffectKind;
    plan.drop = true;
    plan.effects.push_back({S::Drop, a.reply_protocol, 0u, a.dw_object_id, 0u, false, false});
    return plan;
}

inline JackpotSideEffectPlan jackpot_server_side_effect_plan(const JackpotServerAction& a) {
    JackpotSideEffectPlan plan;
    using K = JackpotServerActionKind;
    using S = JackpotSideEffectKind;
    switch (a.kind) {
        case K::broadcast_all_users: {
            plan.dispatched = true;
            plan.drop = false;
            plan.broadcast = true;
            plan.set_total_money = true;
            plan.effects.push_back({S::SetJackpotTotalMoney, a.reply_protocol, 0u, a.dw_object_id, a.jackpot_total_money, false, true});
            plan.effects.push_back({S::BroadcastAllUsers, a.reply_protocol, 0u, a.dw_object_id, a.jackpot_total_money, false, true});
            return plan;
        }
        case K::broadcast_in_map_users: {
            plan.dispatched = true;
            plan.drop = false;
            plan.broadcast_in_map = true;
            plan.set_total_money = true;
            plan.rewrite_protocol = true;
            plan.effects.push_back({S::SetJackpotTotalMoney, a.reply_protocol, 0u, a.dw_object_id, a.jackpot_total_money, false, true});
            plan.effects.push_back({S::BroadcastInMapUsers, a.reply_protocol, a.rewritten_protocol, a.dw_object_id, a.jackpot_total_money, a.rewrite_protocol, true});
            return plan;
        }
        case K::forward_to_originating_client: {
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({S::ForwardToOriginatingClient, a.reply_protocol, 0u, a.dw_object_id, 0u, false, true});
            return plan;
        }
        case K::drop_unknown_protocol: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.reply_protocol, 0u, a.dw_object_id, 0u, false, false});
            return plan;
    }
    }
    return plan;
}

}  // namespace mxh::server
