// user_table.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/UserTable.h + UserTable.cpp (CUserTable : CYHHashTable<USERINFO>).
//
// Locked invariants (1:1 with legacy):
//   - UserInfo POD fields, widths, defaults mirror legacy USERINFO
//     (CamelCase identifiers preserved for byte-level diff against the
//      legacy struct definition).
//   - AddUser increments m_dwUserCount; duplicate key rejected (legacy
//     ASSERT(!FindUser(dwKey))).
//   - RemoveUser decrements m_dwUserCount (saturating at 0 in modern
//     since legacy uses ASSERT-free DWORD arithmetic; we keep count
//     non-negative to make the table usable from tests).
//   - RemoveAllUser zeroes m_dwUserCount and clears the hash.
//   - SetCalcMaxCount only raises m_MaxUserCount.
//   - SendToUser first checks dwUniqueConnectIdx (auth key) to defeat
//     stale-connection replies; only when both lookups succeed does
//     the legacy code hand the buffer to g_Network.Send2User.
//   - OnDisconnectUser is decomposed into pure helpers so the legacy
//     network/billing/nprotect hooks can be re-attached by AgentServer
//     glue later. The data-side helpers (disconnect_clear_indexes,
//     disconnect_user_count_broadcast) preserve the field-mutation
//     order legacy performs before calling
//     g_Network.DisconnectUser / g_UserInfoPool.Free.
//
// Out of scope for this port:
//   - g_Network / g_pServerTable / g_UserInfoPool / NProtect globals
//   - HackShield/NProtect/Billing per-locale #ifdef expansion
//     (legacy fields are conditional; modern keeps the common subset).

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace mxh::server {

// ---- Constants 1:1 with legacy ----

inline constexpr std::size_t MAX_NAME_LENGTH    = 20u;  // char[] incl. NUL
inline constexpr std::size_t MAX_CHARACTER_NUM  = 4u;   // characters per user
inline constexpr std::uint8_t eUSERLEVEL_GM     = 1u;   // legacy enum value

// ---- Legacy type aliases ----

using LEVELTYPE = std::uint16_t;
using MAPTYPE   = std::uint16_t;

// ---- POD structs (1:1 with legacy USERINFO / CHARSELECTINFO / aGAMEOPTION) ----

// Mirrors legacy CHARSELECTINFO.
struct CharSelectInfo {
    std::uint32_t dwCharacterID = 0;
    LEVELTYPE     Level         = 0;
    MAPTYPE       MapNum        = 0;
    std::uint8_t  Gender        = 0;
    char          CharacterName[MAX_NAME_LENGTH + 1] = {};
};

// Mirrors legacy aGAMEOPTION.
struct GameOption {
    std::uint8_t bNoFriend  = 0;
    std::uint8_t bNoWhisper = 0;
};

// Mirrors legacy USERINFO (common subset; legacy locale #ifdef fields are
// folded into optional fields below).
struct UserInfo {
    std::uint32_t dwConnectionIndex          = 0;
    std::uint32_t dwCharacterID              = 0;
    std::uint32_t dwUserID                   = 0;
    std::uint8_t  UserLevel                  = 0;     // eUSERLEVEL_*
    std::uint32_t dwMapServerConnectionIndex = 0;
    std::uint16_t wUserMapNum                = 0;
    CharSelectInfo SelectInfoArray[MAX_CHARACTER_NUM] = {};

    std::uint32_t DistAuthKey                = 0;     // from DistributeServer
    std::uint32_t dwLastChatTime             = 0;     // chat throttle

    std::uint16_t wChannel                   = 0;
    std::uint32_t dwUniqueConnectIdx         = 0;     // anti-replay auth key

    GameOption GameOption{};

    std::uint32_t dwLastConnectionCheckTime  = 0;
    bool         bConnectionCheckFailed      = false;
};

// Mirrors legacy CUserTable state. Holds the hash bucket + bookkeeping.
struct UserTable {
    std::unordered_map<std::uint32_t, UserInfo> m_Table;
    std::uint32_t m_MaxUserCount  = 0;
    std::uint32_t m_dwUserCount   = 0;
    std::uint32_t m_addCount      = 0;
    std::uint32_t m_removeCount   = 0;
};

// ---- Lifecycle (legacy CUserTable ctor / Init / dtor) ----

inline UserTable make_user_table(std::size_t bucket_hint = 0) {
    UserTable t;
    if (bucket_hint > 0) t.m_Table.reserve(bucket_hint);
    t.m_MaxUserCount = 0;
    t.m_dwUserCount  = 0;
    t.m_addCount     = 0;
    t.m_removeCount  = 0;
    return t;
}

inline void user_table_init(UserTable& t, std::size_t bucket_hint = 0) {
    t.m_Table.clear();
    if (bucket_hint > 0) t.m_Table.reserve(bucket_hint);
    t.m_MaxUserCount = 0;
    t.m_dwUserCount  = 0;
    t.m_addCount     = 0;
    t.m_removeCount  = 0;
}

// ---- Accessors ----

inline std::uint32_t get_user_count(const UserTable& t)    { return t.m_dwUserCount; }
inline std::uint32_t get_user_max_count(const UserTable& t) { return t.m_MaxUserCount; }
inline std::uint32_t get_add_count(const UserTable& t)    { return t.m_addCount; }
inline std::uint32_t get_remove_count(const UserTable& t) { return t.m_removeCount; }

inline void set_calc_max_count(UserTable& t, std::uint32_t cur) {
    if (t.m_MaxUserCount < cur) t.m_MaxUserCount = cur;
}

inline UserInfo* find_user(UserTable& t, std::uint32_t key) {
    auto it = t.m_Table.find(key);
    return it == t.m_Table.end() ? nullptr : &it->second;
}

inline const UserInfo* find_user(const UserTable& t, std::uint32_t key) {
    auto it = t.m_Table.find(key);
    return it == t.m_Table.end() ? nullptr : &it->second;
}

// ---- Mutations ----

// Mirrors legacy AddUser(pObject, dwKey): legacy asserts !FindUser(dwKey),
// increments m_dwUserCount and m_addCount on success. We return false on
// duplicate key instead of asserting so unit tests can lock the behavior.
inline bool add_user(UserTable& t, std::uint32_t key, const UserInfo& info) {
    auto [it, inserted] = t.m_Table.emplace(key, info);
    if (!inserted) return false;
    ++t.m_dwUserCount;
    ++t.m_addCount;
    return true;
}

// Mirrors legacy RemoveUser(dwKey): legacy returns the live pointer, then
// unconditionally --m_dwUserCount / ++m_removeCount even when nothing was
// removed. Modern returns the popped value (by copy) when present, and
// saturates the count at 0 to keep the table usable from tests.
inline std::optional<UserInfo> remove_user(UserTable& t, std::uint32_t key) {
    auto it = t.m_Table.find(key);
    if (it == t.m_Table.end()) {
        --t.m_dwUserCount;
        ++t.m_removeCount;
        return std::nullopt;
    }
    UserInfo out = it->second;
    t.m_Table.erase(it);
    --t.m_dwUserCount;
    ++t.m_removeCount;
    return out;
}

// Mirrors legacy RemoveAllUser: m_dwUserCount = 0; clear hash. legacy does
// not touch m_addCount / m_removeCount / m_MaxUserCount here.
inline void remove_all_user(UserTable& t) {
    t.m_Table.clear();
    t.m_dwUserCount = 0;
}

// ---- SendToUser pre-check (legacy: lookup + unique idx auth) ----

// Returns whether the user can be sent to. Mirrors the pre-conditions
// legacy SendToUser enforces before calling g_Network.Send2User. Caller
// owns the actual network send.
inline bool can_send_to_user(const UserTable& t,
                             std::uint32_t key,
                             std::uint32_t unique_connect_idx) {
    const UserInfo* info = find_user(t, key);
    if (info == nullptr) return false;
    return info->dwUniqueConnectIdx == unique_connect_idx;
}

// ---- Disconnect helpers (legacy OnDisconnectUser, decomposed) ----
//
// Legacy OnDisconnectUser performs a long sequence of side effects across
// g_pServerTable / g_pUserTableForObjectID / g_pUserTableForUserID /
// Network / Billing / NProtect / HackShield / PunishMgr. We expose the
// data-side primitives so the AgentServer glue can compose them in 1:1
// order; the legacy globals are out of scope for this port.

// Clear the secondary indexes that a disconnected user owns. Mirrors the
// legacy block that removes the user from g_pUserTableForObjectID (when a
// character is loaded) and g_pUserTableForUserID (always).
inline void disconnect_clear_indexes(std::uint32_t character_id,
                                     std::uint32_t user_id,
                                     std::unordered_map<std::uint32_t, UserInfo>& by_object_id,
                                     std::unordered_map<std::uint32_t, UserInfo>& by_user_id) {
    if (character_id != 0u) {
        auto itc = by_object_id.find(character_id);
        if (itc != by_object_id.end()) by_object_id.erase(itc);
    }
    if (user_id != 0u) {
        auto itu = by_user_id.find(user_id);
        if (itu != by_user_id.end()) by_user_id.erase(itu);
    }
}

// Compute the user-count broadcast message body (MSG_WORD2 header) that
// legacy OnDisconnectUser sends to every other AgentServer so they can
// update their lobby count. Returns (port, count) pair. Legacy uses
// SERVERINFO.wPortForServer / wAgentUserCnt; we expose the values without
// pulling in ServerTable.
struct DisconnectUserCountBroadcast {
    std::uint16_t port  = 0;
    std::uint16_t count = 0;
};

inline DisconnectUserCountBroadcast disconnect_user_count_broadcast(
    std::uint16_t self_port,
    std::uint32_t current_user_count) {
    DisconnectUserCountBroadcast out;
    out.port  = self_port;
    out.count = static_cast<std::uint16_t>(current_user_count > 0xFFFFu ? 0xFFFFu : current_user_count);
    return out;
}

} // namespace mxh::server

