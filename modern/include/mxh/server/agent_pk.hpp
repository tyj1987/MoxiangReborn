// agent_pk.hpp - AgentPk data plane (category=41, MP_PK).
//
// 1:1 port of MP_PKMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol PK handler in legacy; the
// default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_PK traffic is forwarded verbatim to the connected
// user whose object_id matches the packet header. If the user is
// not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_PK at the agent is a pure object-id
// forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_PK (41 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t pk_category = 41u;

// Legacy sub-protocol offsets within MP_PROTOCOL_PK (0..22,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t pk_pkon_syn                = 0u;
inline constexpr std::uint8_t pk_pkon_ack                = 1u;
inline constexpr std::uint8_t pk_pkon_nack               = 2u;
inline constexpr std::uint8_t pk_pkoff_syn               = 3u;
inline constexpr std::uint8_t pk_pkoff_ack               = 4u;
inline constexpr std::uint8_t pk_pkoff_nack              = 5u;
inline constexpr std::uint8_t pk_looting_start           = 6u;
inline constexpr std::uint8_t pk_looting_beinglooted     = 7u;
inline constexpr std::uint8_t pk_looting_select_syn      = 8u;
inline constexpr std::uint8_t pk_looting_select_ack      = 9u;
inline constexpr std::uint8_t pk_looting_select_nack     = 10u;
inline constexpr std::uint8_t pk_looting_itemlooting     = 11u;
inline constexpr std::uint8_t pk_looting_itemlooted      = 12u;
inline constexpr std::uint8_t pk_looting_moenylooting    = 13u;
inline constexpr std::uint8_t pk_looting_moenylooted     = 14u;
inline constexpr std::uint8_t pk_looting_explooting      = 15u;
inline constexpr std::uint8_t pk_looting_explooted       = 16u;
inline constexpr std::uint8_t pk_looting_nolooting       = 17u;
inline constexpr std::uint8_t pk_looting_noinvenspace    = 18u;
inline constexpr std::uint8_t pk_looting_endlooting      = 19u;
inline constexpr std::uint8_t pk_destroy_item            = 20u;
inline constexpr std::uint8_t pk_looting_error           = 21u;

enum class AgentPkOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentPkRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentPkOutcome classify_agent_pk(const AgentPkRequest& r) noexcept {
    return r.user_found ? AgentPkOutcome::ForwardToUser
                        : AgentPkOutcome::DropNoUser;
}

}  // namespace mxh::server
