// agent_quick.hpp - AgentQuick data plane (category=12, MP_MP_QUICK).
//
// 1:1 port of MP_MP_QUICKMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_MP_QUICK traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_MP_QUICK at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_MP_QUICK (12 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t mp_quick_category = 12u;

// Legacy sub-protocol offsets within MP_PROTOCOL_MP_QUICK (0..14,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t mp_quick_add_syn = 0u;
inline constexpr std::uint8_t mp_quick_add_ack = 1u;
inline constexpr std::uint8_t mp_quick_add_nack = 2u;
inline constexpr std::uint8_t mp_quick_use_syn = 3u;
inline constexpr std::uint8_t mp_quick_use_ack = 4u;
inline constexpr std::uint8_t mp_quick_use_nack = 5u;
inline constexpr std::uint8_t mp_quick_move_syn = 6u;
inline constexpr std::uint8_t mp_quick_move_ack = 7u;
inline constexpr std::uint8_t mp_quick_move_nack = 8u;
inline constexpr std::uint8_t mp_quick_rem_syn = 9u;
inline constexpr std::uint8_t mp_quick_rem_ack = 10u;
inline constexpr std::uint8_t mp_quick_rem_nack = 11u;
inline constexpr std::uint8_t mp_quick_set_syn = 12u;
inline constexpr std::uint8_t mp_quick_set_ack = 13u;
inline constexpr std::uint8_t mp_quick_set_nack = 14u;

enum class AgentQuickOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentQuickRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentQuickOutcome classify_agent_quick(const AgentQuickRequest& r) noexcept {
    return r.user_found ? AgentQuickOutcome::ForwardToUser
                        : AgentQuickOutcome::DropNoUser;
}

}  // namespace mxh::server
