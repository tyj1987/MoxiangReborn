// agent_chat_dispatch.hpp
//
// Runtime executor for MP_CHAT side-effect plans produced by
// chat_side_effect_plan() in agent_chat_side_effect_plan.hpp. Like
// agent_dispatch.hpp (D4.R1) it splits data plane (in the plan header)
// from runtime wire-layer dispatch (this header).
//
// Wire-layer abstraction (IChatWireSink) is a small interface that
// captures each of the 12 ChatSideEffectKind values. Production wires
// it to the modern TcpServer.Send / Broadcast / SendToGuild helpers;
// tests wire it to a recording mock.
//
// 1:1 invariants:
//   - dispatch_agent_chat_plan walks effects in plan.effects order.
//   - For every effect, exactly one sink method is called.
//   - Returns the count of effects dispatched (== plan.effects.size()
//     when the plan is fully applied; 0 if sink is null).
//   - Sink methods receive the ChatSideEffect fields verbatim so the
//     caller can decide what to do with the nack_code / receiver_id.

#pragma once

#include <cstddef>
#include <cstdint>

#include <mxh/server/agent_chat_side_effect_plan.hpp>

namespace mxh::server {

// Wire-layer abstraction for chat dispatch.
// One method per ChatSideEffectKind. Tests inject a recording mock;
// production wires this to the agent network layer.
struct IChatWireSink {
    virtual ~IChatWireSink() = default;

    // Drop the protocol/object_id pair (1:1 with legacy discarded msg).
    virtual void drop(std::uint8_t /*protocol*/, std::uint32_t /*object_id*/) {}

    // Forward the (protocol, object_id, payload) to the user that sent
    // the original message; used for ForwardToClient / ForwardAckToUser /
    // ForwardNackToUser.
    virtual void send2user(std::uint32_t /*receiver_id*/,
                            std::uint8_t /*protocol*/,
                            std::uint32_t /*object_id*/,
                            bool /*forward_payload*/) {}

    // Whisper-specific forwards back to the agent that hosts the sender.
    // Used by SendWhisperNackToServer / SendWhisperAckToServer.
    virtual void send_whisper_ack_to_server(std::uint8_t /*protocol*/,
                                            std::uint32_t /*object_id*/,
                                            bool /*forward_payload*/) {}

    // Whisper delivery to the receiver user. The GM variant sets the
    // gm_only flag at the sink layer so the wire layer can append the
    // legacy GM marker.
    virtual void send_whisper_to_user(std::uint32_t /*receiver_id*/,
                                      std::uint8_t /*protocol*/,
                                      std::uint32_t /*object_id*/,
                                      bool /*forward_payload*/,
                                      bool /*gm_only*/) {}

    // Party chat: deliver to local member then broadcast across all
    // other agent servers so the party members on different maps see
    // the message too.
    virtual void send_party_chat_to_member(std::uint32_t /*receiver_id*/,
                                           std::uint8_t /*protocol*/,
                                           std::uint32_t /*object_id*/,
                                           bool /*forward_payload*/) {}
    virtual void broadcast_party_chat_to_other_agents(std::uint8_t /*protocol*/,
                                                     std::uint32_t /*object_id*/,
                                                     bool /*forward_payload*/) {}

    // Guild / guild-union chat: broadcast to all members across all maps.
    virtual void broadcast_guild_chat_to_all_maps(std::uint8_t /*protocol*/,
                                                  std::uint32_t /*object_id*/,
                                                  bool /*forward_payload*/) {}
    virtual void broadcast_guild_union_chat_to_all_maps(std::uint8_t /*protocol*/,
                                                       std::uint32_t /*object_id*/,
                                                       bool /*forward_payload*/) {}
};

// Walk the plan.effects list and call the matching sink method for each.
// Returns the number of effects dispatched.
inline std::size_t dispatch_agent_chat_plan(
    const ChatSideEffectPlan& plan, IChatWireSink* sink) noexcept {
    if (sink == nullptr) return 0u;
    std::size_t n = 0u;
    for (const auto& e : plan.effects) {
        switch (e.kind) {
            case ChatSideEffectKind::Drop:
                sink->drop(0u, 0u);
                break;
            case ChatSideEffectKind::ForwardToClient:
                sink->send2user(e.receiver_id, 0u, 0u, e.forward_payload);
                break;
            case ChatSideEffectKind::SendWhisperNackToServer:
                // nack_code is the legacy 0/1/2 byte; passed via protocol
                // arg so production can serialize without changing the
                // sink interface.
                sink->send_whisper_ack_to_server(
                    static_cast<std::uint8_t>(e.nack_code), 0u, false);
                break;
            case ChatSideEffectKind::SendWhisperAckToServer:
                sink->send_whisper_ack_to_server(0u, 0u, e.forward_payload);
                break;
            case ChatSideEffectKind::SendWhisperToReceiverUser:
                sink->send_whisper_to_user(
                    e.receiver_id, 0u, 0u, e.forward_payload, false);
                break;
            case ChatSideEffectKind::SendWhisperGmToReceiverUser:
                sink->send_whisper_to_user(
                    e.receiver_id, 0u, 0u, e.forward_payload, true);
                break;
            case ChatSideEffectKind::ForwardAckToUser:
                sink->send2user(e.receiver_id, 0u, 0u, e.forward_payload);
                break;
            case ChatSideEffectKind::ForwardNackToUser:
                sink->send2user(e.receiver_id, 0u, 0u, e.forward_payload);
                break;
            case ChatSideEffectKind::SendPartyChatToMember:
                sink->send_party_chat_to_member(
                    e.receiver_id, 0u, 0u, e.forward_payload);
                break;
            case ChatSideEffectKind::BroadcastPartyChatToOtherAgents:
                sink->broadcast_party_chat_to_other_agents(
                    0u, 0u, e.forward_payload);
                break;
            case ChatSideEffectKind::BroadcastGuildChatToAllMaps:
                sink->broadcast_guild_chat_to_all_maps(
                    0u, 0u, e.forward_payload);
                break;
            case ChatSideEffectKind::BroadcastGuildUnionChatToAllMaps:
                sink->broadcast_guild_union_chat_to_all_maps(
                    0u, 0u, e.forward_payload);
                break;
        }
        ++n;
    }
    return n;
}

}  // namespace mxh::server