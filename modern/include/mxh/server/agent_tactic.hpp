// agent_tactic.hpp - AgentTactic data plane (category=20, MP_MP_TACTIC).
//
// 1:1 port of MP_MP_TACTICMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_MP_TACTIC traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_MP_TACTIC at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_MP_TACTIC (20 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t mp_tactic_category = 20u;

// Legacy sub-protocol offsets within MP_PROTOCOL_MP_TACTIC (0..8,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t mp_tactic_start_syn = 0u;
inline constexpr std::uint8_t mp_tactic_start_ack = 1u;
inline constexpr std::uint8_t mp_tactic_start_nack = 2u;
inline constexpr std::uint8_t mp_tactic_join_syn = 3u;
inline constexpr std::uint8_t mp_tactic_join_ack = 4u;
inline constexpr std::uint8_t mp_tactic_join_nack = 5u;
inline constexpr std::uint8_t mp_tactic_object_add = 6u;
inline constexpr std::uint8_t mp_tactic_fail = 7u;
inline constexpr std::uint8_t mp_tactic_execute = 8u;

enum class AgentTacticOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentTacticRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentTacticOutcome classify_agent_tactic(const AgentTacticRequest& r) noexcept {
    return r.user_found ? AgentTacticOutcome::ForwardToUser
                        : AgentTacticOutcome::DropNoUser;
}

}  // namespace mxh::server
