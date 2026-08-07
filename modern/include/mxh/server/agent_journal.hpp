// agent_journal.hpp - AgentJournal data plane (category=53, MP_JOURNAL).
//
// 1:1 port of MP_JournalMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_JOURNAL traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_JOURNAL at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_JOURNAL (53 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t journal_category = 53u;

// Legacy sub-protocol offsets within MP_PROTOCOL_JOURNAL (0..6,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t journal_getlist_syn = 0u;
inline constexpr std::uint8_t journal_getlist_ack = 1u;
inline constexpr std::uint8_t journal_getlist_nack = 2u;
inline constexpr std::uint8_t journal_add = 3u;
inline constexpr std::uint8_t journal_update = 4u;
inline constexpr std::uint8_t journal_delete = 5u;
inline constexpr std::uint8_t journal_levelup = 6u;

enum class AgentJournalOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentJournalRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentJournalOutcome classify_agent_journal(const AgentJournalRequest& r) noexcept {
    return r.user_found ? AgentJournalOutcome::ForwardToUser
                        : AgentJournalOutcome::DropNoUser;
}

}  // namespace mxh::server
