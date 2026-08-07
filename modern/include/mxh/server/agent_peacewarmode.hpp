// agent_peacewarmode.hpp - AgentPeaceWarMode data plane (category=15, MP_MP_PEACEWARMODE).
//
// 1:1 port of MP_MP_PEACEWARMODEMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_MP_PEACEWARMODE traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_MP_PEACEWARMODE at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_MP_PEACEWARMODE (15 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t mp_peacewarmode_category = 15u;

// Legacy sub-protocol offsets within MP_PROTOCOL_MP_PEACEWARMODE (0..1,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t mp_peacewarmode_peace = 0u;
inline constexpr std::uint8_t mp_peacewarmode_war = 1u;

enum class AgentPeaceWarModeOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentPeaceWarModeRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentPeaceWarModeOutcome classify_agent_peacewarmode(const AgentPeaceWarModeRequest& r) noexcept {
    return r.user_found ? AgentPeaceWarModeOutcome::ForwardToUser
                        : AgentPeaceWarModeOutcome::DropNoUser;
}

}  // namespace mxh::server
