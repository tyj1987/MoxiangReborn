
//
// D4.106 -- AgentAutonote side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_AUTONOTEUserMsgParser (lines 5236-5252) and MP_AUTONOTEServerMsgParser
// (lines 5252-5290). The data plane (classify_autonote_user + classify_autonote_server)
// decides which action to take; this header captures the ordered side-effect
// list the orchestrator must execute.
//
// USER side-effects:
//   - drop_no_user: user not in objectid table.
//   - send_punish_to_user: Send2User(connection_index, MSG_DWORD (Protocol=PUNISH, dwData=punish_seconds)).
//   - forward_to_map: TransToMapServerMsgParser (raw forward).
//
// SERVER side-effects (each gated by user_object_found/user_id_found/user_has_character):
//   - asktoauto_ack: send ASKTOAUTO_ACK to user + add 120s punish.
//   - notauto: add auto_note_use_minutes*60s punish + send NOTAUTO if user_has_character.
//   - answer_ack: add auto_note_use_minutes*60s punish to other (the other party).
//   - answer_fail: increment punish count.
//   - answer_logout: increment punish count.
//   - answer_timeout (with user): increment punish count + send ANSWER_TIMEOUT.
//   - answer_timeout (no user): increment punish count (treat as fail).
//   - killauto: send KILLAUTO if user_has_character.
//   - disconnect: disconnect user if user_id_found.
//   - forward_to_user: TransToClientMsgParser if user_object_found.
//   - drop_no_user: silent drop.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_autonote.hpp"

namespace mxh::server {

// USER side-effect kinds the orchestrator must dispatch in order.
enum class AutonoteUserSideEffectKind : std::uint8_t {
    Drop,                                  // no_user
    SendPunishToUser,                      // MSG_DWORD Protocol=PUNISH dwData=punish_seconds to connection_index
    ForwardRawToMap,                       // TransToMapServerMsgParser
};

struct AutonoteUserSideEffect final {
    AutonoteUserSideEffectKind kind = AutonoteUserSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t connection_index = 0u;
    std::uint32_t punish_seconds = 0u;
};

struct AutonoteUserSideEffectPlan final {
    std::vector<AutonoteUserSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool autonote_user_effect_targets_user(const AutonoteUserSideEffect& e) noexcept {
    return e.kind == AutonoteUserSideEffectKind::SendPunishToUser;
}

inline bool autonote_user_effect_targets_map(const AutonoteUserSideEffect& e) noexcept {
    return e.kind == AutonoteUserSideEffectKind::ForwardRawToMap;
}

inline AutonoteUserSideEffectPlan autonote_user_side_effect_plan(const AutonoteUserAction& a) {
    AutonoteUserSideEffectPlan plan;
    using K = AutonoteUserSideEffectKind;
    using A = AutonoteUserActionKind;
    switch (a.kind) {
        case A::drop_no_user:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.connection_index, 0u});
            return plan;
        case A::send_punish_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendPunishToUser, a.protocol, a.connection_index, a.punish_seconds});
            return plan;
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.connection_index, 0u});
            return plan;
    }
    return plan;
}

// SERVER side-effect kinds.
enum class AutonoteServerSideEffectKind : std::uint8_t {
    Drop,                                  // user not found / not eligible
    SendAskToAutoAckAndPunish,             // ASKTOAUTO_ACK + 120s punish
    PunishOtherAndSendNotAuto,             // auto_note_use_minutes*60s punish + NOTAUTO if has_character
    PunishOther,                           // auto_note_use_minutes*60s punish for other party
    IncrementPunishCount,                  // ANSWER_FAIL/LOGOUT/TIMEOUT(no-user) punish count++
    IncrementPunishCountAndSendTimeout,    // ANSWER_TIMEOUT (user found) punish count++ + send
    SendKillAutoIfCharacter,               // KILLAUTO if has_character
    DisconnectUser,                        // DISCONNECT user
    ForwardRawToUser,                      // TransToClientMsgParser fallback
};

struct AutonoteServerSideEffect final {
    AutonoteServerSideEffectKind kind = AutonoteServerSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint32_t user_id = 0u;
    std::uint32_t punish_seconds = 0u;
    bool disconnect = false;
};

struct AutonoteServerSideEffectPlan final {
    std::vector<AutonoteServerSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool autonote_server_effect_targets_user(const AutonoteServerSideEffect& e) noexcept {
    return e.kind != AutonoteServerSideEffectKind::Drop &&
           e.kind != AutonoteServerSideEffectKind::PunishOther &&
           e.kind != AutonoteServerSideEffectKind::IncrementPunishCount;
}

inline AutonoteServerSideEffectPlan autonote_server_side_effect_plan(const AutonoteServerAction& a) {
    AutonoteServerSideEffectPlan plan;
    using K = AutonoteServerSideEffectKind;
    using A = AutonoteServerActionKind;
    switch (a.kind) {
        case A::asktoauto_ack_send_and_punish:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendAskToAutoAckAndPunish, a.protocol, a.object_id, a.user_id, a.punish_seconds, false});
            return plan;
        case A::notauto_punish_and_send_to_user_if_character:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::PunishOtherAndSendNotAuto, a.protocol, a.object_id, a.user_id, a.punish_seconds, false});
            return plan;
        case A::answer_ack_punish_other:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::PunishOther, a.protocol, a.object_id, a.user_id, a.punish_seconds, false});
            return plan;
        case A::answer_fail_punish_count:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::IncrementPunishCount, a.protocol, a.object_id, a.user_id, 0u, false});
            return plan;
        case A::answer_logout_punish_count:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::IncrementPunishCount, a.protocol, a.object_id, a.user_id, 0u, false});
            return plan;
        case A::answer_timeout_punish_count_and_send:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::IncrementPunishCountAndSendTimeout, a.protocol, a.object_id, a.user_id, 0u, false});
            return plan;
        case A::killauto_send_if_character:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendKillAutoIfCharacter, a.protocol, a.object_id, a.user_id, 0u, false});
            return plan;
        case A::disconnect_if_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::DisconnectUser, a.protocol, a.object_id, a.user_id, 0u, true});
            return plan;
        case A::forward_to_user_if_found:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToUser, a.protocol, a.object_id, a.user_id, 0u, false});
            return plan;
        case A::drop_no_user:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.object_id, a.user_id, 0u, false});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server