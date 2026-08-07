// agent_bossmonster.hpp - AgentBossMonster data plane (category=34, MP_BOSSMONSTER).
//
// 1:1 port of MP_BossmonsterMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_BOSSMONSTER traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_BOSSMONSTER at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_BOSSMONSTER (34 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t bossmonster_category = 34u;

// Legacy sub-protocol offsets within MP_PROTOCOL_BOSSMONSTER (0..7,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t boss_rest_start_notify = 0u;
inline constexpr std::uint8_t boss_recall_notify = 1u;
inline constexpr std::uint8_t boss_life_notify = 2u;
inline constexpr std::uint8_t boss_shield_notify = 3u;
inline constexpr std::uint8_t boss_stand_notify = 4u;
inline constexpr std::uint8_t boss_stand_end_notify = 5u;
inline constexpr std::uint8_t field_life_notify = 6u;
inline constexpr std::uint8_t field_shield_notify = 7u;

enum class AgentBossMonsterOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentBossMonsterRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentBossMonsterOutcome classify_agent_bossmonster(const AgentBossMonsterRequest& r) noexcept {
    return r.user_found ? AgentBossMonsterOutcome::ForwardToUser
                        : AgentBossMonsterOutcome::DropNoUser;
}

}  // namespace mxh::server
