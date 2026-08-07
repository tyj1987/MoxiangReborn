// agent_titan.hpp - AgentTitan data plane (category=72, MP_TITAN).
//
// 1:1 port of MP_TitanMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_TITAN traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_TITAN at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_TITAN (72 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t titan_category = 72u;

// Legacy sub-protocol offsets within MP_PROTOCOL_TITAN (0..18,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t titan_valueinfo = 0u;
inline constexpr std::uint8_t titan_fuel_ack = 1u;
inline constexpr std::uint8_t titan_spell_ack = 2u;
inline constexpr std::uint8_t titan_recall_syn = 3u;
inline constexpr std::uint8_t titan_recall_ack = 4u;
inline constexpr std::uint8_t titan_recall_nack = 5u;
inline constexpr std::uint8_t titan_recall_cancel_syn = 6u;
inline constexpr std::uint8_t titan_recall_cancel_ack = 7u;
inline constexpr std::uint8_t titan_recall_cancel_nack = 8u;
inline constexpr std::uint8_t titan_ridein_syn = 9u;
inline constexpr std::uint8_t titan_ridein_ack = 10u;
inline constexpr std::uint8_t titan_getoff_ack = 11u;
inline constexpr std::uint8_t titan_make_syn = 12u;
inline constexpr std::uint8_t titan_make_ack = 13u;
inline constexpr std::uint8_t titan_make_nack = 14u;
inline constexpr std::uint8_t titan_addnew_fromitem = 15u;
inline constexpr std::uint8_t titan_addnew_equip_fromitem = 16u;
inline constexpr std::uint8_t titan_statinfo = 17u;
inline constexpr std::uint8_t titan_endurance_update = 18u;

enum class AgentTitanOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentTitanRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentTitanOutcome classify_agent_titan(const AgentTitanRequest& r) noexcept {
    return r.user_found ? AgentTitanOutcome::ForwardToUser
                        : AgentTitanOutcome::DropNoUser;
}

}  // namespace mxh::server
