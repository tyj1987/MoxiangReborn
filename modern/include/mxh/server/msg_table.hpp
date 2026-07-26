// msg_table.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/MsgTable.h + MsgTable.cpp (CMsgTable).
//
// MsgTable is a tiny key-allocated chat-message buffer: clients push a
// pending chat (TESTMSG or MSG_CHAT) with a key, the AgentServer replies
// to that key later, and the AgentServer GCs the buffer when done.
//
// Locked invariants (1:1 with legacy):
//   - AddMsg returns TRUE on success and writes the assigned key via the
//     out-parameter; returns FALSE when the input is null or the table is
//     exhausted.
//   - AddMsg(TESTMSG*) sets Name[0] = 0 (no Name in TESTMSG).
//   - AddMsg(MSG_CHAT*) copies the Name field as well as Msg.
//   - RemoveMsg(dwKey) frees the slot and recycles the key.
//   - GetMsg(dwKey) returns the stored MSG_CHAT (or nullptr if missing).
//   - The MAX_MSGTABLE = 500 cap matches legacy CMsgTable::CMsgTable()
//     (m_htMsg.Initialize(MAX_MSGTABLE)). Once 500 entries are live, the
//     next AddMsg returns FALSE.
//
// Out of scope for this port:
//   - Legacy CMemoryPoolTempl<MSG_CHAT> / CYHHashTable / INDEXCR_HANDLE:
//     modern owns the storage directly via std::vector<std::unique_ptr>
//     + std::unordered_map<key, MsgChat>. Key allocation is a monotonic
//     counter that wraps to 0 when the table is full (no real reuse).

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>



namespace mxh::server {

// ---- Constants 1:1 with legacy ----

inline constexpr std::size_t MAX_CHAT_LENGTH  = 127u;
inline constexpr std::size_t MAX_NAME_LENGTH  = 16u;   // mirror user_table.hpp
inline constexpr std::size_t MAX_MSGTABLE     = 500u;  // legacy cap

// ---- POD (mirrors legacy MSGBASE / MSG_CHAT / TESTMSG enough for the
// MsgTable surface; legacy MSGBASE has additional pad fields but we only
// expose what AddMsg / GetMsg touch) ----

struct MsgBase {
    std::uint8_t  Category   = 0;
    std::uint8_t  Protocol   = 0;
    std::uint32_t dwObjectID = 0;
};

struct MsgChat {
    std::uint8_t  Category   = 0;
    std::uint8_t  Protocol   = 0;
    std::uint32_t dwObjectID = 0;
    char          Name[MAX_NAME_LENGTH + 1]  = {};
    char          Msg [MAX_CHAT_LENGTH + 1]  = {};
};

struct TestMsg {
    std::uint8_t  Category   = 0;
    std::uint8_t  Protocol   = 0;
    std::uint32_t dwObjectID = 0;
    char          Msg [MAX_CHAT_LENGTH + 1]  = {};
};

// Mirrors legacy CMsgTable state.
struct MsgTable {
    std::unordered_map<std::uint32_t, std::unique_ptr<MsgChat>> m_Table;
    std::uint32_t m_NextKey = 1;  // legacy ICAllocIndex returns non-zero keys
    std::size_t   m_Capacity = MAX_MSGTABLE;
};

// ---- Lifecycle ----

inline MsgTable make_msg_table(std::size_t capacity = MAX_MSGTABLE) {
    MsgTable t;
    t.m_Capacity = capacity;
    if (capacity > 0) t.m_Table.reserve(capacity);
    return t;
}

inline void msg_table_clear(MsgTable& t) {
    t.m_Table.clear();
    t.m_NextKey = 1;
}



// ---- Add / Remove / Lookup ----

inline bool add_msg(MsgTable& t, const MsgChat& src, std::uint32_t& key_out) {
    if (t.m_Table.size() >= t.m_Capacity) return false;
    auto chat = std::make_unique<MsgChat>();
    chat->Category   = src.Category;
    chat->Protocol   = src.Protocol;
    chat->dwObjectID = src.dwObjectID;
    std::strncpy(chat->Name, src.Name, MAX_NAME_LENGTH);
    chat->Name[MAX_NAME_LENGTH] = 0;
    std::strncpy(chat->Msg,  src.Msg,  MAX_CHAT_LENGTH);
    chat->Msg[MAX_CHAT_LENGTH] = 0;

    std::uint32_t key = t.m_NextKey++;
    if (t.m_NextKey == 0) t.m_NextKey = 1;  // skip 0 sentinel
    t.m_Table.emplace(key, std::move(chat));
    key_out = key;
    return true;
}

inline bool add_msg(MsgTable& t, const TestMsg& src, std::uint32_t& key_out) {
    if (t.m_Table.size() >= t.m_Capacity) return false;
    auto chat = std::make_unique<MsgChat>();
    chat->Category   = src.Category;
    chat->Protocol   = src.Protocol;
    chat->dwObjectID = src.dwObjectID;
    chat->Name[0]    = 0;  // legacy: TESTMSG path leaves Name empty.
    std::strncpy(chat->Msg, src.Msg, MAX_CHAT_LENGTH);
    chat->Msg[MAX_CHAT_LENGTH] = 0;

    std::uint32_t key = t.m_NextKey++;
    if (t.m_NextKey == 0) t.m_NextKey = 1;
    t.m_Table.emplace(key, std::move(chat));
    key_out = key;
    return true;
}

inline MsgChat* get_msg(MsgTable& t, std::uint32_t key) {
    auto it = t.m_Table.find(key);
    return it == t.m_Table.end() ? nullptr : it->second.get();
}

inline const MsgChat* get_msg(const MsgTable& t, std::uint32_t key) {
    auto it = t.m_Table.find(key);
    return it == t.m_Table.end() ? nullptr : it->second.get();
}

inline bool remove_msg(MsgTable& t, std::uint32_t key) {
    auto it = t.m_Table.find(key);
    if (it == t.m_Table.end()) return false;
    t.m_Table.erase(it);
    return true;
}

inline std::size_t msg_table_size(const MsgTable& t) {
    return t.m_Table.size();
}

} // namespace mxh::server

