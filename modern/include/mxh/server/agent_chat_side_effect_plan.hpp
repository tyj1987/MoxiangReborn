#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/agent_chat.hpp>

namespace mxh::server {

// Side-effect intents for MP_CHAT server-side handlers at
// [Server]Agent/AgentNetworkMsgParser.cpp lines 1561-1880.
enum class ChatSideEffectKind : std::uint8_t {
    Drop,
    ForwardToClient,
    SendWhisperNackToServer,
    SendWhisperAckToServer,
    SendWhisperToReceiverUser,
    SendWhisperGmToReceiverUser,
    ForwardAckToUser,
    ForwardNackToUser,
    SendPartyChatToMember,
    BroadcastPartyChatToOtherAgents,
    BroadcastGuildChatToAllMaps,
    BroadcastGuildUnionChatToAllMaps,
};

struct ChatSideEffect final {
    ChatSideEffectKind kind = ChatSideEffectKind::Drop;
    std::uint32_t receiver_id = 0u;
    std::uint8_t nack_code = 0u;
    bool forward_payload = false;
};

struct ChatSideEffectPlan final {
    std::vector<ChatSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

struct ChatRequest final {
    std::uint8_t protocol = 0u;
    bool receiver_found = true;
    bool receiver_blocks_whisper = false;
    bool sender_found = true;
    bool msg_table_insert_ok = true;
    bool target_name_too_short = false;
};

inline ChatSideEffectPlan chat_side_effect_plan(
    const ChatRequest& request) {
    ChatSideEffectPlan plan;
    using K = ChatSideEffectKind;

    switch (request.protocol) {
        case chat_all:
        case chat_smallshout:
        case chat_gm_smallshout:
        case chat_monster_speech:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardToClient, 0u, 0u, true});
            return plan;

        case chat_whisper_syn:
            if (!request.receiver_found) {
                return plan;
            }
            plan.dispatched = true;
            plan.drop = false;
            if (request.receiver_blocks_whisper) {
                plan.effects.push_back({K::SendWhisperNackToServer,
                    0u, 2u, false});
                return plan;
            }
            plan.effects.push_back({K::SendWhisperAckToServer,
                0u, 0u, true});
            plan.effects.push_back({K::SendWhisperToReceiverUser,
                0u, 0u, true});
            return plan;

        case chat_whisper_gm_syn:
            if (!request.receiver_found) {
                return plan;
            }
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendWhisperAckToServer,
                0u, 0u, true});
            plan.effects.push_back({K::SendWhisperGmToReceiverUser,
                0u, 0u, true});
            return plan;

        case chat_whisper_ack:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardAckToUser,
                0u, 0u, true});
            return plan;

        case chat_whisper_nack:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardNackToUser,
                0u, 0u, true});
            return plan;

        case chat_party:
            if (!request.sender_found) {
                return plan;
            }
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendPartyChatToMember,
                0u, 0u, true});
            plan.effects.push_back({K::BroadcastPartyChatToOtherAgents,
                0u, 0u, true});
            return plan;

        case chat_guild:
            if (!request.sender_found) {
                return plan;
            }
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastGuildChatToAllMaps,
                0u, 0u, true});
            return plan;

        case chat_guild_union:
            if (!request.sender_found) {
                return plan;
            }
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastGuildUnionChatToAllMaps,
                0u, 0u, true});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server
