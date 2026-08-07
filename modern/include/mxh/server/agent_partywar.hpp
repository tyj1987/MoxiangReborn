// agent_partywar.hpp - AgentPartyWar data plane (category=59, MP_PARTYWAR).
//
// 1:1 port of MP_PartywarMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_PARTYWAR traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_PARTYWAR at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_PARTYWAR (59 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t partywar_category = 59u;

// Legacy sub-protocol offsets within MP_PROTOCOL_PARTYWAR (0..18,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t partywar_nack = 0u;
inline constexpr std::uint8_t partywar_suggest = 1u;
inline constexpr std::uint8_t partywar_suggest_wait = 2u;
inline constexpr std::uint8_t partywar_suggest_accept = 3u;
inline constexpr std::uint8_t partywar_suggest_deny = 4u;
inline constexpr std::uint8_t partywar_addmember_syn = 5u;
inline constexpr std::uint8_t partywar_addmember_ack = 6u;
inline constexpr std::uint8_t partywar_addmember_nack = 7u;
inline constexpr std::uint8_t partywar_removemember_syn = 8u;
inline constexpr std::uint8_t partywar_removemember_ack = 9u;
inline constexpr std::uint8_t partywar_removemember_nack = 10u;
inline constexpr std::uint8_t partywar_lock = 11u;
inline constexpr std::uint8_t partywar_unlock = 12u;
inline constexpr std::uint8_t partywar_start = 13u;
inline constexpr std::uint8_t partywar_cancel = 14u;
inline constexpr std::uint8_t partywar_ready = 15u;
inline constexpr std::uint8_t partywar_fight = 16u;
inline constexpr std::uint8_t partywar_result = 17u;
inline constexpr std::uint8_t partywar_end = 18u;

enum class AgentPartyWarOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentPartyWarRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentPartyWarOutcome classify_agent_partywar(const AgentPartyWarRequest& r) noexcept {
    return r.user_found ? AgentPartyWarOutcome::ForwardToUser
                        : AgentPartyWarOutcome::DropNoUser;
}

}  // namespace mxh::server
