#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace mxh::server {

inline constexpr std::size_t PKLOOTING_ITEM_NUM = 20u;
inline constexpr std::uint32_t PKLOOTING_LIMIT_TIME = 10000u;
inline constexpr std::uint32_t PKLOOTING_DLG_DELAY_TIME = 2000u;
inline constexpr float PK_LOOTING_DISTANCE = 1000.0f;
inline constexpr std::uint32_t PKLOOTING_TIMEOUT_GRACE = 3000u;

enum class LootingItemKind : std::int32_t {
    None = 0,
    Item = 1,
    Money = 2,
    Exp = 3,
    Selected = 4,
    Max = 5,
};

enum class LootingError : std::uint32_t {
    Ok = 0,
    NoMoreChance = 1,
    NoMoreItemLootCount = 2,
    InvalidPosition = 3,
    AlreadySelected = 4,
    OverDistance = 5,
    NoLootingRoom = 6,
};

struct LootingItem {
    LootingItemKind nKind = LootingItemKind::None;
    std::uint32_t dwData = 0;
};

struct LootingRoom {
    std::uint32_t m_dwDiePlayer = 0;
    std::uint32_t m_dwAttacker = 0;
    std::array<LootingItem, PKLOOTING_ITEM_NUM> m_LootingItemArray{};
    int m_nItemLootCount = 0;
    int m_nChance = 0;
    std::uint32_t m_dwLootingStartTime = 0;
    int m_nLootedItemCount = 0;
};

struct LootingManagerState {
    std::unordered_map<std::uint32_t, LootingRoom> m_LootingRooms;
};

struct LootingAttemptResult {
    LootingError error = LootingError::Ok;
    LootingItem item{};
    bool consumedChance = false;
    bool looted = false;
};

inline int get_looting_chance(std::uint32_t badFame) {
    if (badFame < 100000u) return 3;
    if (badFame < 500000u) return 4;
    if (badFame < 1000000u) return 5;
    if (badFame < 5000000u) return 6;
    if (badFame < 10000000u) return 7;
    if (badFame < 50000000u) return 8;
    if (badFame < 100000000u) return 9;
    return 10;
}

inline int get_looting_item_num(std::uint32_t badFame) {
    if (badFame < 50u) return 0;
    if (badFame < 100000000u) return 1;
    if (badFame < 400000000u) return 2;
    if (badFame < 700000000u) return 3;
    if (badFame < 1000000000u) return 4;
    return 5;
}

inline int get_wear_item_looting_ratio(std::uint32_t badFame) {
    if (badFame == 0u) return 0;
    if (badFame < 50u) return 1;
    if (badFame < 4000u) return 10;
    if (badFame < 20000u) return 20;
    if (badFame < 80000u) return 30;
    if (badFame < 400000u) return 40;
    if (badFame < 1600000u) return 50;
    if (badFame < 8000000u) return 60;
    if (badFame < 32000000u) return 70;
    if (badFame < 100000000u) return 85;
    return 100;
}

inline bool is_looting_situation(bool attackerIsPlayer, bool attackerIsDead,
                                 bool attackerPkMode, bool victimPkMode,
                                 bool attackerOwnsWanted) {
    if (!attackerIsPlayer || attackerIsDead) return false;
    return attackerPkMode || victimPkMode || attackerOwnsWanted;
}

inline LootingRoom make_looting_room(std::uint32_t victimId, std::uint32_t attackerId,
                                     std::uint32_t attackerBadFame,
                                     std::uint32_t startTime) {
    LootingRoom room;
    room.m_dwDiePlayer = victimId;
    room.m_dwAttacker = attackerId;
    room.m_nChance = get_looting_chance(attackerBadFame);
    room.m_nItemLootCount = get_looting_item_num(attackerBadFame);
    room.m_dwLootingStartTime = startTime;
    return room;
}

inline void set_looting_item(LootingRoom& room, std::size_t slot,
                             LootingItemKind kind, std::uint32_t data) {
    if (slot < room.m_LootingItemArray.size()) room.m_LootingItemArray[slot] = {kind, data};
}

inline std::uint32_t calculate_loot_money(std::uint32_t money) {
    return money * 3u / 100u;
}

inline bool is_looting_room_timeout(const LootingRoom& room, std::uint32_t now) {
    return now - room.m_dwLootingStartTime >
           PKLOOTING_LIMIT_TIME + PKLOOTING_DLG_DELAY_TIME + PKLOOTING_TIMEOUT_GRACE;
}

inline LootingAttemptResult try_loot(LootingRoom& room, int slot, float distance,
                                     bool force = false) {
    if (room.m_nChance <= 0) return {LootingError::NoMoreChance};
    if (room.m_nItemLootCount <= 0) return {LootingError::NoMoreItemLootCount};
    if (slot < 0 || slot >= static_cast<int>(PKLOOTING_ITEM_NUM))
        return {LootingError::InvalidPosition};
    auto& selected = room.m_LootingItemArray[static_cast<std::size_t>(slot)];
    if (selected.nKind == LootingItemKind::Selected)
        return {LootingError::AlreadySelected};
    if (!force && distance > PK_LOOTING_DISTANCE + 500.0f)
        return {LootingError::OverDistance};

    --room.m_nChance;
    if (selected.nKind == LootingItemKind::None)
        return {LootingError::Ok, selected, true, false};

    LootingAttemptResult result{LootingError::Ok, selected, true, true};
    selected = {LootingItemKind::Selected, 0u};
    --room.m_nItemLootCount;
    ++room.m_nLootedItemCount;
    return result;
}

inline void create_looting_room(LootingManagerState& state, LootingRoom room) {
    state.m_LootingRooms.insert_or_assign(room.m_dwDiePlayer, room);
}

inline LootingRoom* get_looting_room(LootingManagerState& state, std::uint32_t victimId) {
    const auto it = state.m_LootingRooms.find(victimId);
    return it == state.m_LootingRooms.end() ? nullptr : &it->second;
}

inline bool is_looted_player(const LootingManagerState& state, std::uint32_t victimId) {
    return state.m_LootingRooms.find(victimId) != state.m_LootingRooms.end();
}

inline bool close_looting_room(LootingManagerState& state, std::uint32_t victimId) {
    return state.m_LootingRooms.erase(victimId) != 0u;
}

inline std::size_t cancel_looting_by_attacker(LootingManagerState& state,
                                              std::uint32_t attackerId) {
    std::vector<std::uint32_t> victims;
    for (const auto& [victimId, room] : state.m_LootingRooms)
        if (room.m_dwAttacker == attackerId) victims.push_back(victimId);
    for (const auto victimId : victims) state.m_LootingRooms.erase(victimId);
    return victims.size();
}

inline std::size_t process_looting_timeouts(LootingManagerState& state, std::uint32_t now) {
    std::vector<std::uint32_t> victims;
    for (const auto& [victimId, room] : state.m_LootingRooms)
        if (is_looting_room_timeout(room, now)) victims.push_back(victimId);
    for (const auto victimId : victims) state.m_LootingRooms.erase(victimId);
    return victims.size();
}

}
