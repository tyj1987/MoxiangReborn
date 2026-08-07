// agent_itemext.hpp - AgentItemExt data plane (category=73, MP_ITEMEXT).
//
// 1:1 port of MP_ItemextMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_ITEMEXT traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_ITEMEXT at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_ITEMEXT (73 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t itemext_category = 73u;

// Legacy sub-protocol offsets within MP_PROTOCOL_ITEMEXT (0..20,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t itemext_shopitem_curse_cancellation_additem_syn = 0u;
inline constexpr std::uint8_t itemext_shopitem_curse_cancellation_additem_ack = 1u;
inline constexpr std::uint8_t itemext_shopitem_curse_cancellation_additem_nack = 2u;
inline constexpr std::uint8_t itemext_shopitem_curse_cancellation_release = 3u;
inline constexpr std::uint8_t itemext_shopitem_curse_cancellation_deleteitem = 4u;
inline constexpr std::uint8_t itemext_shopitem_curse_cancellation_syn = 5u;
inline constexpr std::uint8_t itemext_shopitem_curse_cancellation_ack = 6u;
inline constexpr std::uint8_t itemext_shopitem_curse_cancellation_nack = 7u;
inline constexpr std::uint8_t itemext_uniqueitem_mix_additem_syn = 8u;
inline constexpr std::uint8_t itemext_uniqueitem_mix_additem_ack = 9u;
inline constexpr std::uint8_t itemext_uniqueitem_mix_additem_nack = 10u;
inline constexpr std::uint8_t itemext_uniqueitem_mix_release = 11u;
inline constexpr std::uint8_t itemext_uniqueitem_mix_deleteitem = 12u;
inline constexpr std::uint8_t itemext_uniqueitem_mix_syn = 13u;
inline constexpr std::uint8_t itemext_uniqueitem_mix_ack = 14u;
inline constexpr std::uint8_t itemext_uniqueitem_mix_nack = 15u;
inline constexpr std::uint8_t itemext_shopitem_decoration_on = 16u;
inline constexpr std::uint8_t itemext_skinitem_select_syn = 17u;
inline constexpr std::uint8_t itemext_skinitem_select_ack = 18u;
inline constexpr std::uint8_t itemext_skinitem_select_nack = 19u;
inline constexpr std::uint8_t itemext_skinitem_discard_ack = 20u;

enum class AgentItemExtOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentItemExtRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentItemExtOutcome classify_agent_itemext(const AgentItemExtRequest& r) noexcept {
    return r.user_found ? AgentItemExtOutcome::ForwardToUser
                        : AgentItemExtOutcome::DropNoUser;
}

}  // namespace mxh::server
