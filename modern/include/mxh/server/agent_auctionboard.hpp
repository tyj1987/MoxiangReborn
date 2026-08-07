// agent_auctionboard.hpp - AgentAuctionBoard data plane (category=10, MP_MP_AUCTIONBOARD).
//
// 1:1 port of MP_MP_AUCTIONBOARDMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_MP_AUCTIONBOARD traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_MP_AUCTIONBOARD at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_MP_AUCTIONBOARD (10 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t mp_auctionboard_category = 10u;

// Legacy sub-protocol offsets within MP_PROTOCOL_MP_AUCTIONBOARD (0..18,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t mp_auctionboard_open_syn = 0u;
inline constexpr std::uint8_t mp_auctionboard_open_ack = 1u;
inline constexpr std::uint8_t mp_auctionboard_open_nack = 2u;
inline constexpr std::uint8_t mp_auctionboard_list_syn = 3u;
inline constexpr std::uint8_t mp_auctionboard_list_ack = 4u;
inline constexpr std::uint8_t mp_auctionboard_list_nack = 5u;
inline constexpr std::uint8_t mp_auctionboard_contents_syn = 6u;
inline constexpr std::uint8_t mp_auctionboard_contents_ack = 7u;
inline constexpr std::uint8_t mp_auctionboard_contents_nack = 8u;
inline constexpr std::uint8_t mp_auctionboard_write_syn = 9u;
inline constexpr std::uint8_t mp_auctionboard_write_ack = 10u;
inline constexpr std::uint8_t mp_auctionboard_write_nack = 11u;
inline constexpr std::uint8_t mp_auctionboard_delete_syn = 12u;
inline constexpr std::uint8_t mp_auctionboard_delete_ack = 13u;
inline constexpr std::uint8_t mp_auctionboard_delete_nack = 14u;
inline constexpr std::uint8_t mp_auctionboard_bid_syn = 15u;
inline constexpr std::uint8_t mp_auctionboard_bid_ack = 16u;
inline constexpr std::uint8_t mp_auctionboard_bid_nack = 17u;
inline constexpr std::uint8_t mp_auctionboard_closecontents = 18u;

enum class AgentAuctionBoardOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentAuctionBoardRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentAuctionBoardOutcome classify_agent_auctionboard(const AgentAuctionBoardRequest& r) noexcept {
    return r.user_found ? AgentAuctionBoardOutcome::ForwardToUser
                        : AgentAuctionBoardOutcome::DropNoUser;
}

}  // namespace mxh::server
