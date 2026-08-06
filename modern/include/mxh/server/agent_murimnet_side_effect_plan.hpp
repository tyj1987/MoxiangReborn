
//
// D4.108 -- AgentMurimNet side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/MNNetworkMsgParser.cpp
// (legacy classify_murimnet_user + classify_murimnet_server). The data plane
// decides which action to take; this header captures the ordered side-effect
// list the orchestrator must execute.
//
// Side effects:
//   - forward_to_map: TransToMapServerMsgParser (raw forwarding).
//   - send_ack: build reply (ChangetoMurimNet_Ack / ReturntoMurimNet_Ack) and send to
//     the target map server at the looked-up port.
//   - send_nack: build reply (ChangetoMurimNet_Nack / ReturntoMurimNet_Nack) and send
//     to the target map server at the looked-up port.
//   - drop_unknown: silent drop (legacy unknown protocol behavior).
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_murimnet.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must dispatch in order.
enum class MurimNetSideEffectKind : std::uint8_t {
    Drop,                                  // unknown protocol
    ForwardRawToMap,                       // TransToMapServerMsgParser
    SendAckToMap,                          // reply ACK (ChangetoMurimNet_Ack / ReturntoMurimNet_Ack)
    SendNackToMap,                         // reply NACK (ChangetoMurimNet_Nack / ReturntoMurimNet_Nack)
};

struct MurimNetSideEffect final {
    MurimNetSideEffectKind kind = MurimNetSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t character_id = 0u;
};

struct MurimNetSideEffectPlan final {
    std::vector<MurimNetSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool murimnet_effect_targets_map(const MurimNetSideEffect& e) noexcept {
    return e.kind != MurimNetSideEffectKind::Drop;
}

inline MurimNetSideEffectPlan murimnet_side_effect_plan(const MurimNetAction& a) {
    MurimNetSideEffectPlan plan;
    using K = MurimNetSideEffectKind;
    using A = MurimNetActionKind;
    switch (a.kind) {
        case A::drop_unknown:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.character_id});
            return plan;
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.character_id});
            return plan;
        case A::send_ack:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendAckToMap, a.protocol, a.character_id});
            return plan;
        case A::send_nack:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendNackToMap, a.protocol, a.character_id});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server