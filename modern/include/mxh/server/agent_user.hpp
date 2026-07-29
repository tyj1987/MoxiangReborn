#pragma once
#include <cstdint>
#include <string>
namespace mxh::server {
// agent_user.hpp - Phase 6.3 AgentUser 1:1 port (thin layer over UserTable).
//
// Source-of-truth: legacy [Server]Agent/AgentServer.cpp + AgentNetworkMsgParser.cpp.
// agent_user provides agent-side user lifecycle glue: login bookkeeping,
// per-user state on the agent (object_id <-> map channel), per-key routing.
//
// Locked invariants (1:1 with legacy):
//   - login order: insert-by-key FIRST, then validate-id, then insert-by-id.
//     legacy uses ASSERT(!FindUserById(dwUserID)) on duplicate insert.
//   - logout order: remove-by-id FIRST, then remove-by-key. The disconnect
//     hooks (DisconnectUser / Free) fire in the same order as legacy.
//   - map channel assignment: 0 means 'no map assigned'; legacy uses 0 as
//     a sentinel for an agent-side user that has not entered a map yet.
//   - AgentUserInfo::bForceMove is the legacy 'teleport-to-map' flag;
//     forced moves bypass the user-requested map number but still record it.
struct AgentUserInfo {
    std::uint32_t dwObjectID=0;
    std::uint32_t dwUserID=0;
    std::uint32_t dwAuthKey=0;
    std::uint32_t dwMapChannel=0;
    std::uint16_t wMapNum=0;
    std::uint8_t bForceMove=0;
    char name[17]={};
};
struct AgentUserRecord { AgentUserInfo info{}; bool in_use=false; };
// Insert helper. Returns true if the record was added; false if key or id
// already mapped (legacy ASSERT path).
bool insert_agent_user(AgentUserRecord& r,std::uint32_t key,std::uint32_t object_id);
// Remove helper. Returns true if the record was removed; false if it was empty.
bool remove_agent_user(AgentUserRecord& r);
// Map channel assignment. Returns true if assigned; false if no map (dwMapChannel==0).
bool assign_agent_user_map(AgentUserRecord& r,std::uint32_t channel);
// Force-move flag toggling. Returns the new flag value.
bool toggle_agent_user_force_move(AgentUserRecord& r);
}
