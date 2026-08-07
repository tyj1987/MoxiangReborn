// agent_kyunggong.hpp - AgentKyungGong data plane (category=23, MP_KYUNGGONG).
//
// 1:1 port of MP_KyunggongMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_KYUNGGONG traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_KYUNGGONG at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_KYUNGGONG (23 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t kyunggong_category = 23u;

// Legacy sub-protocol offsets within MP_PROTOCOL_KYUNGGONG (0..1,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t kyunggong_change_notify = 0u;
inline constexpr std::uint8_t kyunggong_ability_change_notify = 1u;

enum class AgentKyungGongOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentKyungGongRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentKyungGongOutcome classify_agent_kyunggong(const AgentKyungGongRequest& r) noexcept {
    return r.user_found ? AgentKyungGongOutcome::ForwardToUser
                        : AgentKyungGongOutcome::DropNoUser;
}

}  // namespace mxh::server
