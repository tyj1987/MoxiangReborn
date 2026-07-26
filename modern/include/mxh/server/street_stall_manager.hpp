// street_stall_manager.hpp - Phase D5 1:1 port of legacy [Server]Map/StreetStallManager.h
// + StreetStall.h. State machine for player street stalls (sell/buy lists of
// up to MAX_STREETSTALL_CELLNUM cells). Mirrors legacy cStreetStall +
// cStreetStallManager fields in CamelCase so byte-level diff against the
// reference exe is possible.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::server {

// ---- Constants 1:1 ----

inline constexpr int MAX_STREETSTALL_CELLNUM  = 25;
inline constexpr int MAX_STREETBUYSTALL_CELLNUM = 5;
inline constexpr std::uint32_t STALL_SEARCH_DELAY_TIME = 3000u;
inline constexpr std::uint32_t ITEM_VIEW_DELAY_TIME    = 1000u;

// ---- Enumerations ----

enum class StallKind : std::uint16_t {
    Null = 0,
    Sell = 1,
    Buy  = 2,
};

enum class StreetStallFindState : std::uint8_t {
    FindSell = 0,
    FindBuy  = 1,
    FindMax,
};

enum class StreetStallPriceState : std::uint8_t {
    PriceMin = 0,
    PriceMax = 1,
};

enum class StreetStallDelayState : std::uint8_t {
    DelayStallSearch = 0,
    DelayItemView    = 1,
    DelayMax,
};

// ---- POD structs ----

// Mirrors legacy sCELLINFO. ItemBase is opaque POD (legacy ITEMBASE).
struct StallCellInfo {
    std::array<std::uint8_t, 64> sItemBase{};  // opaque legacy ITEMBASE blob
    std::uint32_t dwMoney = 0;
    std::uint16_t wVolume = 0;
    bool          bLock   = false;
    bool          bFill   = false;

    inline void init() {
        sItemBase.fill(0);
        dwMoney = 0;
        wVolume = 0;
        bLock = false;
        bFill = false;
    }
};

// Mirrors legacy cStreetStall.
struct StreetStall {
    std::array<StallCellInfo, MAX_STREETSTALL_CELLNUM> m_sArticles{};
    std::optional<std::uint32_t> m_pOwner;  // CPlayer* opaque
    std::vector<std::uint32_t>    m_GuestList;  // opaque CPlayer* id
    int             m_nCurRegistItemNum = 0;
    std::uint16_t   m_wStallKind        = 0;
    std::uint32_t   m_nTotalMoney       = 0;
};

// Mirrors legacy cStreetStallManager state. We expose just the parts that
// matter for the 1:1 test surface.
struct StreetStallManagerState {
    std::vector<StreetStall> m_StallTable;
    std::array<std::uint32_t, 2> m_dwSearchDelayTime{};
};

// ---- Free functions ----

inline StreetStallManagerState make_street_stall_manager() {
    return StreetStallManagerState{};
}

// Stall lifecycle.
inline void init_street_stall(StreetStall& s, std::uint32_t owner_id = 0u) {
    for (auto& cell : s.m_sArticles) cell.init();
    s.m_pOwner = (owner_id == 0u) ? std::optional<std::uint32_t>{} : std::optional<std::uint32_t>{owner_id};
    s.m_GuestList.clear();
    s.m_nCurRegistItemNum = 0;
    s.m_wStallKind = static_cast<std::uint16_t>(StallKind::Null);
    s.m_nTotalMoney = 0;
}

// Add a stall cell. Returns false if stall full.
inline bool fill_cell(StreetStall& s, std::uint32_t money, std::uint16_t volume,
                      bool bLock = false, std::uint16_t wAbsPosition = 0xFFFFu) {
    if (s.m_nCurRegistItemNum >= MAX_STREETSTALL_CELLNUM) return false;
    const std::uint16_t pos = (wAbsPosition == 0xFFFFu)
                                  ? static_cast<std::uint16_t>(s.m_nCurRegistItemNum)
                                  : wAbsPosition;
    if (pos >= MAX_STREETSTALL_CELLNUM) return false;
    auto& cell = s.m_sArticles[pos];
    cell.init();
    cell.dwMoney = money;
    cell.wVolume = volume;
    cell.bLock   = bLock;
    cell.bFill   = true;
    s.m_nCurRegistItemNum += 1;
    s.m_nTotalMoney += money;
    return true;
}

inline bool empty_cell(StreetStall& s, std::uint16_t pos) {
    if (pos >= MAX_STREETSTALL_CELLNUM) return false;
    if (!s.m_sArticles[pos].bFill) return false;
    s.m_nTotalMoney -= s.m_sArticles[pos].dwMoney;
    s.m_sArticles[pos].init();
    s.m_nCurRegistItemNum -= 1;
    return true;
}

inline void empty_cell_all(StreetStall& s) {
    for (auto& cell : s.m_sArticles) cell.init();
    s.m_nCurRegistItemNum = 0;
    s.m_nTotalMoney = 0;
}

inline void change_cell_state(StreetStall& s, std::uint16_t pos, bool bLock) {
    if (pos >= MAX_STREETSTALL_CELLNUM) return;
    s.m_sArticles[pos].bLock = bLock;
}

inline void set_cell_money(StreetStall& s, std::uint16_t pos, std::uint32_t money) {
    if (pos >= MAX_STREETSTALL_CELLNUM) return;
    if (s.m_sArticles[pos].bFill) {
        s.m_nTotalMoney -= s.m_sArticles[pos].dwMoney;
        s.m_nTotalMoney += money;
    }
    s.m_sArticles[pos].dwMoney = money;
}

inline void set_cell_volume(StreetStall& s, std::uint16_t pos, std::uint16_t volume) {
    if (pos >= MAX_STREETSTALL_CELLNUM) return;
    s.m_sArticles[pos].wVolume = volume;
}

// Stall kind accessors.
inline StallKind get_stall_kind(const StreetStall& s) {
    return static_cast<StallKind>(s.m_wStallKind);
}

inline void set_stall_kind(StreetStall& s, StallKind kind) {
    s.m_wStallKind = static_cast<std::uint16_t>(kind);
}

// Guest tracking.
inline void add_guest(StreetStall& s, std::uint32_t guest_id) {
    s.m_GuestList.push_back(guest_id);
}

inline void delete_guest(StreetStall& s, std::uint32_t guest_id) {
    for (auto it = s.m_GuestList.begin(); it != s.m_GuestList.end(); ++it) {
        if (*it == guest_id) {
            s.m_GuestList.erase(it);
            return;
        }
    }
}

inline void delete_guest_all(StreetStall& s) {
    s.m_GuestList.clear();
}

// Capacity queries.
inline bool is_stall_full(const StreetStall& s) {
    return s.m_nCurRegistItemNum >= MAX_STREETSTALL_CELLNUM;
}

inline int get_cur_regist_item_num(const StreetStall& s) {
    return s.m_nCurRegistItemNum;
}

inline std::uint32_t get_total_money(const StreetStall& s) {
    return s.m_nTotalMoney;
}

// Manager-level: register stall into table; return index or SIZE_MAX.
inline std::size_t register_stall(StreetStallManagerState& m, const StreetStall& s) {
    m.m_StallTable.push_back(s);
    return m.m_StallTable.size() - 1u;
}

inline StreetStall* find_stall_by_owner(StreetStallManagerState& m, std::uint32_t owner_id) {
    for (auto& s : m.m_StallTable) {
        if (s.m_pOwner && *s.m_pOwner == owner_id) return &s;
    }
    return nullptr;
}

inline bool delete_stall_by_owner(StreetStallManagerState& m, std::uint32_t owner_id) {
    for (auto it = m.m_StallTable.begin(); it != m.m_StallTable.end(); ++it) {
        if (it->m_pOwner && *it->m_pOwner == owner_id) {
            m.m_StallTable.erase(it);
            return true;
        }
    }
    return false;
}

// Delay state.
inline void set_search_delay(StreetStallManagerState& m, StreetStallDelayState slot, std::uint32_t ms) {
    if (static_cast<int>(slot) >= static_cast<int>(StreetStallDelayState::DelayMax)) return;
    m.m_dwSearchDelayTime[static_cast<std::size_t>(slot)] = ms;
}

inline bool check_delay_elapsed(const StreetStallManagerState& m, StreetStallDelayState slot,
                                std::uint32_t now_ms, std::uint32_t required_ms) {
    if (static_cast<int>(slot) >= static_cast<int>(StreetStallDelayState::DelayMax)) return true;
    const std::uint32_t t = m.m_dwSearchDelayTime[static_cast<std::size_t>(slot)];
    if (t == 0u) return true;
    return (now_ms - t) >= required_ms;
}

} // namespace mxh::server
