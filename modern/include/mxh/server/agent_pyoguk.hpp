// agent_pyoguk.hpp - AgentPyoguk data plane (category=30, MP_PYOGUK).
//
// 1:1 port of MP_PyogukMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_PYOGUK traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_PYOGUK at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_PYOGUK (30 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t pyoguk_category = 30u;

// Legacy sub-protocol offsets within MP_PROTOCOL_PYOGUK (0..15,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t pyoguk_listinfo_syn = 0u;
inline constexpr std::uint8_t pyoguk_listinfo_ack = 1u;
inline constexpr std::uint8_t pyoguk_listinfo_nack = 2u;
inline constexpr std::uint8_t pyoguk_buy_syn = 3u;
inline constexpr std::uint8_t pyoguk_buy_ack = 4u;
inline constexpr std::uint8_t pyoguk_buy_nack = 5u;
inline constexpr std::uint8_t pyoguk_del_syn = 6u;
inline constexpr std::uint8_t pyoguk_del_ack = 7u;
inline constexpr std::uint8_t pyoguk_del_nack = 8u;
inline constexpr std::uint8_t pyoguk_putin_money_syn = 9u;
inline constexpr std::uint8_t pyoguk_putin_money_ack = 10u;
inline constexpr std::uint8_t pyoguk_putin_money_nack = 11u;
inline constexpr std::uint8_t pyoguk_putout_money_syn = 12u;
inline constexpr std::uint8_t pyoguk_putout_money_ack = 13u;
inline constexpr std::uint8_t pyoguk_putout_money_nack = 14u;
inline constexpr std::uint8_t pyoguk_info = 15u;

enum class AgentPyogukOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentPyogukRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentPyogukOutcome classify_agent_pyoguk(const AgentPyogukRequest& r) noexcept {
    return r.user_found ? AgentPyogukOutcome::ForwardToUser
                        : AgentPyogukOutcome::DropNoUser;
}

}  // namespace mxh::server
