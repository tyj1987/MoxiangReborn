// agent_auction.hpp - AgentAuction data plane (category=17, MP_MP_AUCTION).
//
// 1:1 port of MP_MP_AUCTIONMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_MP_AUCTION traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_MP_AUCTION at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_MP_AUCTION (17 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t mp_auction_category = 17u;

// Legacy sub-protocol offsets within MP_PROTOCOL_MP_AUCTION (0..20,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t mp_auction_success_syn = 0u;
inline constexpr std::uint8_t mp_auction_success_ack = 1u;
inline constexpr std::uint8_t mp_auction_success_nack = 2u;
inline constexpr std::uint8_t mp_auction_search_syn = 3u;
inline constexpr std::uint8_t mp_auction_search_ack = 4u;
inline constexpr std::uint8_t mp_auction_search_nack = 5u;
inline constexpr std::uint8_t mp_auction_sort_syn = 6u;
inline constexpr std::uint8_t mp_auction_sort_ack = 7u;
inline constexpr std::uint8_t mp_auction_sort_nack = 8u;
inline constexpr std::uint8_t mp_auction_register_ok_syn = 9u;
inline constexpr std::uint8_t mp_auction_register_ok_ack = 10u;
inline constexpr std::uint8_t mp_auction_register_ok_nack = 11u;
inline constexpr std::uint8_t mp_auction_registser_cancel_syn = 12u;
inline constexpr std::uint8_t mp_auction_registser_cancel_ack = 13u;
inline constexpr std::uint8_t mp_auction_registser_cancel_nack = 14u;
inline constexpr std::uint8_t mp_auction_join_ok_syn = 15u;
inline constexpr std::uint8_t mp_auction_join_ok_ack = 16u;
inline constexpr std::uint8_t mp_auction_join_ok_nack = 17u;
inline constexpr std::uint8_t mp_auction_cancel_syn = 18u;
inline constexpr std::uint8_t mp_auction_cancel_ack = 19u;
inline constexpr std::uint8_t mp_auction_cancel_nack = 20u;

enum class AgentAuctionOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentAuctionRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentAuctionOutcome classify_agent_auction(const AgentAuctionRequest& r) noexcept {
    return r.user_found ? AgentAuctionOutcome::ForwardToUser
                        : AgentAuctionOutcome::DropNoUser;
}

}  // namespace mxh::server
