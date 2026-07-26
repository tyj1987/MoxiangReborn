// quest_regen_mgr.hpp - Phase D5 1:1 port of legacy [Server]Map/QuestRegenMgr.h.
// Manages per-quest monster regen templates: each template specifies a
// monster kind, count, and (single or multi) positions with radius. Mirrors
// legacy CQuestRegenInfo + CQuestRegenMgr fields.

#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>

namespace mxh::server {

// ---- POD structs ----

// Opaque legacy VECTOR3.
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// Mirrors legacy QRPOS.
struct QRPos {
    Vec3          vPos{};
    std::uint16_t wRadius = 0;
};

// Mirrors legacy CQuestRegenInfo.
struct QuestRegenInfo {
    std::uint8_t  m_bCondition    = 0;
    std::uint16_t m_wMonsterCount = 0;
    std::uint16_t m_wMonsterKind  = 0;
    std::uint16_t m_wRadius       = 0;
    Vec3          m_vOnePos{};
    std::vector<QRPos> m_pPos;
};

// Mirrors legacy CQuestRegenMgr.
struct QuestRegenMgrState {
    std::unordered_map<std::uint32_t, QuestRegenInfo> m_QRITable;
};

// ---- Free functions ----

inline QuestRegenMgrState make_quest_regen_mgr() {
    return QuestRegenMgrState{};
}

inline void quest_regen_mgr_init(QuestRegenMgrState& s) {
    s.m_QRITable.clear();
}

inline void quest_regen_mgr_release(QuestRegenMgrState& s) {
    quest_regen_mgr_init(s);
}

// AddRegenInfo registers a template under a quest regen id.
inline void add_regen_info(QuestRegenMgrState& s, std::uint32_t regen_id, const QuestRegenInfo& info) {
    s.m_QRITable[regen_id] = info;
}

inline QuestRegenInfo* find_regen_info(QuestRegenMgrState& s, std::uint32_t regen_id) {
    auto it = s.m_QRITable.find(regen_id);
    return (it == s.m_QRITable.end()) ? nullptr : &it->second;
}

inline std::size_t regen_info_count(const QuestRegenMgrState& s) {
    return s.m_QRITable.size();
}

// Total monsters this regen template will spawn (= count).
inline std::uint32_t regen_info_total_monsters(const QuestRegenInfo& info) {
    return info.m_wMonsterCount;
}

// Spawn position pick: legacy chooses one of the multi-pos entries by
// hashing the player id, or returns m_vOnePos if only one is registered.
inline Vec3 pick_regen_pos(const QuestRegenInfo& info, std::uint32_t player_id) {
    if (info.m_pPos.empty()) {
        return info.m_vOnePos;
    }
    const std::size_t idx = static_cast<std::size_t>(player_id % info.m_pPos.size());
    return info.m_pPos[idx].vPos;
}

// Radius pick: legacy returns m_wRadius for single-pos or per-slot radius.
inline std::uint16_t pick_regen_radius(const QuestRegenInfo& info, std::uint32_t player_id) {
    if (info.m_pPos.empty()) {
        return info.m_wRadius;
    }
    const std::size_t idx = static_cast<std::size_t>(player_id % info.m_pPos.size());
    return info.m_pPos[idx].wRadius;
}

} // namespace mxh::server
