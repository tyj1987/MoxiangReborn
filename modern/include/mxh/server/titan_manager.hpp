// titan_manager.hpp - Phase D5 1:1 port of legacy [Server]Map/TitanManager.h
// State machine for the player Titan system (summon, ride-in, upgrade,
// endurance, recall timer). Mirrors legacy CTitanManager fields in CamelCase
// so byte-level diff against the reference exe is possible.
//
// Reference: ??[Source]/[Server]Map/TitanManager.h + TitanItemManager.h

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::server {

// ---- Constants 1:1 ----

inline constexpr std::uint32_t MAX_TITANGRADE = 3u;
inline constexpr std::uint32_t TITAN_EQUIPITEM_ENDURANCE_MAX = 10000000u;
// TITAN_STATE_CHECKTIME (from Titan.h)
inline constexpr std::uint32_t TITAN_STATE_CHECKTIME = 10000u;

// ---- Enumerations ----

enum class GetOffReason : std::uint8_t {
    Normal        = 0,
    FromUser      = 1,
    MasterLifeRate = 2,
    ExhaustFuel   = 3,
    ExhaustSpell  = 4,
};

enum class EnduranceException : std::uint8_t {
    None     = 0,
    Inven    = 1,
    Pyoguk   = 2,
};

enum class EnduranceCalcPoint : std::uint8_t {
    WhenTitanAttack  = 0,
    WhenTitanDefense = 1,
};

// Mirrors legacy eTitanWearedItem_Max.
inline constexpr int kTitanWearedItemMax = 7;  // helmet/armor/gloves/legs/cloak/shield/weapon

// ---- POD structs ----

// Mirrors legacy TitanWearedInfo.
struct TitanWearedInfo {
    std::uint16_t TitanEquipItemIdx = 0;
    std::uint32_t TitanEquipItemDBIdx = 0;
};

// Opaque POD standing in for legacy TITAN_TOTALINFO.
struct TitanTotalInfo {
    std::uint32_t TitanCallItemDBIdx = 0;
    std::uint16_t TitanKind          = 0;
    std::uint16_t TitanGrade         = 1;
    std::uint32_t TitanFuel          = 0;
    std::uint32_t TitanSpell         = 0;
    std::uint8_t  bRiding            = 0;
    std::uint8_t  bRegistered        = 0;
};

// Opaque POD for legacy TITAN_ENDURANCE_ITEMINFO.
struct TitanEnduranceItemInfo {
    std::uint32_t equipItemDBIdx = 0;
    std::uint32_t Endurance      = 0;
};

// Opaque POD for legacy titan_calc_stats.
struct TitanCalcStats {
    std::uint32_t phyAttackMin  = 0;
    std::uint32_t phyAttackMax  = 0;
    std::uint32_t phyDefense    = 0;
    std::uint32_t maxLife       = 0;
    std::uint32_t maxSpell      = 0;
    float        moveSpeed      = 0.0f;
};

// Opaque POD for legacy TITAN_SHOPITEM_OPTION.
struct TitanShopitemOption {
    std::uint32_t fuelRecoveryBoost = 0;
    std::uint32_t spellRecoveryBoost = 0;
};

// Mirrors legacy CTitanManager.
struct TitanManagerState {
    std::optional<std::uint32_t> m_pCurRidingTitan;  // call item db idx
    std::uint32_t m_dwCurRegistTitanCallItemDBIdx = 0;
    std::uint32_t m_dwRecallCheckTime = 0;
    std::uint16_t TitanScaleForNewOne = 100;  // 100 => 1.0f scale
    std::array<TitanWearedInfo, kTitanWearedItemMax> m_TitanWearedInfo{};
    bool              m_bAvaliableEndurance = true;
    TitanShopitemOption m_TitanShopitemOption{};
    TitanCalcStats    m_titanStats{};
    TitanCalcStats    m_titanItemStats{};
    std::vector<TitanTotalInfo>         m_TitanInfoList;
    std::vector<TitanEnduranceItemInfo> m_ItemEnduranceList;
    std::vector<TitanEnduranceItemInfo> m_ItemUsingEnduranceList;

    // Recall timer (2007.09.14 CBH).
    std::uint32_t m_dwTitanRecallProcessTime = 0;
    std::uint32_t m_dwCurrentTime = 0;
    bool          m_bTitanRecall = false;
    bool          m_bTitanRecallClient = false;
    // Titan EP / Maintain timers.
    std::uint32_t m_dwTitanEPTime = 0;
    std::uint32_t m_dwTitanMaintainTime = 0;
};

// ---- Free functions ----

inline TitanManagerState make_titan_manager() {
    return TitanManagerState{};
}

inline void init_titan_manager(TitanManagerState& s) {
    s.m_pCurRidingTitan.reset();
    s.m_dwCurRegistTitanCallItemDBIdx = 0;
    s.m_dwRecallCheckTime = 0;
    s.TitanScaleForNewOne = 100;
    for (auto& w : s.m_TitanWearedInfo) w = TitanWearedInfo{};
    s.m_bAvaliableEndurance = true;
    s.m_TitanShopitemOption = TitanShopitemOption{};
    s.m_titanStats = TitanCalcStats{};
    s.m_titanItemStats = TitanCalcStats{};
    s.m_TitanInfoList.clear();
    s.m_ItemEnduranceList.clear();
    s.m_ItemUsingEnduranceList.clear();
    s.m_dwTitanRecallProcessTime = 0;
    s.m_dwCurrentTime = 0;
    s.m_bTitanRecall = false;
    s.m_bTitanRecallClient = false;
    s.m_dwTitanEPTime = 0;
    s.m_dwTitanMaintainTime = 0;
}

// Add / Remove titan total info (1:1 to legacy CTitanManager methods).
inline void add_titan_total_info(TitanManagerState& s, const TitanTotalInfo& info) {
    s.m_TitanInfoList.push_back(info);
}

inline void remove_titan_total_info(TitanManagerState& s, std::uint32_t call_item_db_idx) {
    for (auto it = s.m_TitanInfoList.begin(); it != s.m_TitanInfoList.end(); ++it) {
        if (it->TitanCallItemDBIdx == call_item_db_idx) {
            s.m_TitanInfoList.erase(it);
            if (s.m_pCurRidingTitan && *s.m_pCurRidingTitan == call_item_db_idx) {
                s.m_pCurRidingTitan.reset();
            }
            return;
        }
    }
}

inline TitanTotalInfo* find_titan_total_info(TitanManagerState& s, std::uint32_t call_item_db_idx) {
    for (auto& t : s.m_TitanInfoList) {
        if (t.TitanCallItemDBIdx == call_item_db_idx) return &t;
    }
    return nullptr;
}

// Grade up: bumps grade up to MAX_TITANGRADE; returns false if at max.
inline bool upgrade_titan(TitanTotalInfo& titan) {
    if (titan.TitanGrade >= MAX_TITANGRADE) return false;
    titan.TitanGrade += 1;
    return true;
}

// Recall state machine.
inline void start_titan_recall(TitanManagerState& s, std::uint32_t now_ms) {
    s.m_bTitanRecall = true;
    s.m_bTitanRecallClient = true;
    s.m_dwTitanRecallProcessTime = now_ms;
}

inline bool is_titan_recall_active(const TitanManagerState& s) {
    return s.m_bTitanRecall;
}

inline void init_titan_recall(TitanManagerState& s) {
    s.m_bTitanRecall = false;
    s.m_bTitanRecallClient = false;
    s.m_dwTitanRecallProcessTime = 0;
}

inline void set_recall_check_time(TitanManagerState& s, std::uint32_t t) {
    s.m_dwRecallCheckTime = t;
}

inline bool check_recall_available(const TitanManagerState& s, std::uint32_t now_ms) {
    if (s.m_dwRecallCheckTime == 0u) return true;
    return (now_ms - s.m_dwRecallCheckTime) >= 30000u;  // 30s minimum cooldown
}

// Endurance helpers.
inline void add_endurance(TitanManagerState& s, std::uint32_t equip_item_db_idx, std::uint32_t initial) {
    TitanEnduranceItemInfo e;
    e.equipItemDBIdx = equip_item_db_idx;
    e.Endurance = initial;
    s.m_ItemEnduranceList.push_back(e);
}

inline void plus_item_endurance(TitanManagerState& s, std::uint32_t equip_item_db_idx, std::uint32_t delta) {
    for (auto& e : s.m_ItemEnduranceList) {
        if (e.equipItemDBIdx == equip_item_db_idx) {
            const std::uint64_t v = static_cast<std::uint64_t>(e.Endurance) + delta;
            e.Endurance = v > TITAN_EQUIPITEM_ENDURANCE_MAX ? TITAN_EQUIPITEM_ENDURANCE_MAX
                                                            : static_cast<std::uint32_t>(v);
            return;
        }
    }
}

inline void minus_item_endurance(TitanManagerState& s, std::uint32_t equip_item_db_idx, std::uint32_t delta) {
    for (auto& e : s.m_ItemEnduranceList) {
        if (e.equipItemDBIdx == equip_item_db_idx) {
            const std::int64_t v = static_cast<std::int64_t>(e.Endurance) - static_cast<std::int64_t>(delta);
            e.Endurance = v < 0 ? 0u : static_cast<std::uint32_t>(v);
            return;
        }
    }
}

// WearedInfo accessors.
inline void set_weared_info(TitanManagerState& s, int slot, std::uint16_t item_idx, std::uint32_t item_db_idx) {
    if (slot < 0 || slot >= kTitanWearedItemMax) return;
    s.m_TitanWearedInfo[static_cast<std::size_t>(slot)].TitanEquipItemIdx = item_idx;
    s.m_TitanWearedInfo[static_cast<std::size_t>(slot)].TitanEquipItemDBIdx = item_db_idx;
}

inline void clear_weared_info(TitanManagerState& s, int slot) {
    if (slot < 0 || slot >= kTitanWearedItemMax) return;
    s.m_TitanWearedInfo[static_cast<std::size_t>(slot)] = TitanWearedInfo{};
}

// Titan stats helpers (TITAN_EQUIPITEM_ENDURANCE_MAX-bound adders).
inline void add_cur_titan_fuel(TitanTotalInfo& t, std::uint16_t amount, std::uint32_t max_fuel) {
    const std::uint64_t v = static_cast<std::uint64_t>(t.TitanFuel) + amount;
    t.TitanFuel = v > max_fuel ? max_fuel : static_cast<std::uint32_t>(v);
}

inline void add_cur_titan_spell(TitanTotalInfo& t, std::uint16_t amount, std::uint32_t max_spell) {
    const std::uint64_t v = static_cast<std::uint64_t>(t.TitanSpell) + amount;
    t.TitanSpell = v > max_spell ? max_spell : static_cast<std::uint32_t>(v);
}

inline void add_cur_titan_fuel_as_rate(TitanTotalInfo& t, float rate, std::uint32_t max_fuel) {
    if (rate <= 0.0f) return;
    const std::uint32_t amount = static_cast<std::uint32_t>(static_cast<float>(max_fuel) * rate);
    add_cur_titan_fuel(t, static_cast<std::uint16_t>(amount), max_fuel);
}

inline void add_cur_titan_spell_as_rate(TitanTotalInfo& t, float rate, std::uint32_t max_spell) {
    if (rate <= 0.0f) return;
    const std::uint32_t amount = static_cast<std::uint32_t>(static_cast<float>(max_spell) * rate);
    add_cur_titan_spell(t, static_cast<std::uint16_t>(amount), max_spell);
}

// SetWearedInfo equivalent for both add and remove paths.
inline void set_weared_info_in(TitanManagerState& s, int slot, std::uint16_t item_idx, std::uint32_t item_db_idx) {
    set_weared_info(s, slot, item_idx, item_db_idx);
}

} // namespace mxh::server
