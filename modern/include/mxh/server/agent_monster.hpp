// agent_monster.hpp - AgentMonster data plane (category=35, MP_MONSTER).
//
// 1:1 port of MP_MonsterMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_MONSTER traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_MONSTER at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_MONSTER (35 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t monster_category = 35u;

// Legacy sub-protocol offsets within MP_PROTOCOL_MONSTER (0..3,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t monster_life_notify = 0u;
inline constexpr std::uint8_t monster_reststart_notify = 1u;
inline constexpr std::uint8_t monster_restend_notify = 2u;
inline constexpr std::uint8_t monster_recall_notify = 3u;

enum class AgentMonsterOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentMonsterRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentMonsterOutcome classify_agent_monster(const AgentMonsterRequest& r) noexcept {
    return r.user_found ? AgentMonsterOutcome::ForwardToUser
                        : AgentMonsterOutcome::DropNoUser;
}

}  // namespace mxh::server
