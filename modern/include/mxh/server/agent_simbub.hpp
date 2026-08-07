// agent_simbub.hpp - AgentSimBub data plane (category=24, MP_SIMBUB).
//
// 1:1 port of MP_SimbubMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_SIMBUB traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_SIMBUB at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_SIMBUB (24 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t simbub_category = 24u;

// Legacy sub-protocol offsets within MP_PROTOCOL_SIMBUB (0..2,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t simbub_change_syn = 0u;
inline constexpr std::uint8_t simbub_change_ack = 1u;
inline constexpr std::uint8_t simbub_change_nack = 2u;

enum class AgentSimBubOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentSimBubRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentSimBubOutcome classify_agent_simbub(const AgentSimBubRequest& r) noexcept {
    return r.user_found ? AgentSimBubOutcome::ForwardToUser
                        : AgentSimBubOutcome::DropNoUser;
}

}  // namespace mxh::server
