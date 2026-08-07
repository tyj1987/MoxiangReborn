// agent_autopatch.hpp - AgentAutoPatch data plane (category=18, MP_MP_AUTOPATCH).
//
// 1:1 port of MP_MP_AUTOPATCHMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_MP_AUTOPATCH traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_MP_AUTOPATCH at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_MP_AUTOPATCH (18 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t mp_autopatch_category = 18u;

// Legacy sub-protocol offsets within MP_PROTOCOL_MP_AUTOPATCH (0..2,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t mp_autopatch_traffic_syn = 0u;
inline constexpr std::uint8_t mp_autopatch_traffic_ack = 1u;
inline constexpr std::uint8_t mp_autopatch_traffic_nack = 2u;

enum class AgentAutoPatchOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentAutoPatchRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentAutoPatchOutcome classify_agent_autopatch(const AgentAutoPatchRequest& r) noexcept {
    return r.user_found ? AgentAutoPatchOutcome::ForwardToUser
                        : AgentAutoPatchOutcome::DropNoUser;
}

}  // namespace mxh::server
