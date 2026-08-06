
//
// D4.113 -- AgentHackCheck side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_HACKCHECKMsgParser (lines 3683-3740). The data plane (classify_hackcheck) decides
// which action to take; this header captures the ordered side-effect list the
// orchestrator must execute.
//
// Legacy branches:
//   - speedhack (0): if user not found -> drop. Else if (server_time >= client_time AND
//     server_time - client_time < (SPEEDHACK_CHECKTIME - 3000ms)) -> detect_speedhack_and_ban.
//     Otherwise -> ignore (legitimate client).
//   - ban_user_toagent (2): if user not found -> drop. Else -> ban_user_to_agent_always.
//   - default (1): ignore.
//
// Side effects:
//   - detect_speedhack_and_ban: rewrite Protocol to BAN_USER (1), write data=delta into dwData,
//     and Send2User(user->dwConnectionIndex, sizeof(MSG_DWORD)).
//   - ban_user_to_agent_always: rewrite Protocol to BAN_USER (1) and Send2User.
//   - drop_no_user: silent drop.
//   - ignore: silent drop (legitimate client or default protocol).
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_hackcheck.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must dispatch in order.
enum class HackCheckSideEffectKind : std::uint8_t {
    Drop,                                  // no_user or ignore (no side effect)
    SendBanUserToUser,                     // rewrite Protocol=BAN_USER and Send2User (size MSG_DWORD)
};

struct HackCheckSideEffect final {
    HackCheckSideEffectKind kind = HackCheckSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint32_t data = 0u;
};

struct HackCheckSideEffectPlan final {
    std::vector<HackCheckSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool hackcheck_effect_targets_user(const HackCheckSideEffect& e) noexcept {
    return e.kind == HackCheckSideEffectKind::SendBanUserToUser;
}

inline HackCheckSideEffectPlan hackcheck_side_effect_plan(const HackCheckAction& a) {
    HackCheckSideEffectPlan plan;
    using K = HackCheckSideEffectKind;
    using A = HackCheckActionKind;
    switch (a.kind) {
        case A::drop_no_user:
        case A::ignore:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.object_id, 0u});
            return plan;
        case A::detect_speedhack_and_ban:
        case A::ban_user_to_agent_always:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendBanUserToUser, a.protocol, a.object_id, a.data});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server