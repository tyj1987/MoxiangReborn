// exchange_manager.hpp - Phase D5 1:1 port of legacy [Server]Map/ExchangeManager.h + ExchangeRoom.h
// State machine for two-player item exchange (lock + accept flow). All fields
// mirror legacy CExchangeManager/CExchangeRoom members in CamelCase so a
// byte-level diff against the reference exe is possible when paired with the
// side-by-side harness.
//
// Reference: ??[Source]/[Server]Map/ExchangeManager.h, ExchangeRoom.h
//
// Limitations:
// - MAX_EXCHANGEITEM mirrored to 10.
// - eEXCHANGE_ERROR values mirror legacy enum exactly.
// - sEXCHANGEDATA POD uses std::array<ITEMBASE, MAX_EXCHANGEITEM> so the
//   container is trivially copyable. Full ITEMBASE lives in
//   [CC]Header/CommonStruct.h; we keep an opaque POD here.

#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace mxh::server {

inline constexpr int MAX_EXCHANGEITEM = 10;

enum class ExchangeError : std::uint8_t {
    OK              = 0,
    UserCancel      = 1,
    UserLogout      = 2,
    UserDie         = 3,
    Die             = 4,
    NotEnoughMoney  = 5,
    NotEnoughSpace  = 6,
    MaxMoney        = 7,
    NotMatchItem    = 8,
    Error           = 9,
};

enum class ExchangeState : std::uint8_t {
    Waiting = 0,
    Doing   = 1,
};

// Opaque POD standing in for legacy ITEMBASE in the exchange context.
struct ExchangeItemSlot {
    std::uint32_t dwDBIdx = 0;
    std::uint16_t wItemIdx = 0;
    std::uint16_t wPosition = 0;
    std::uint32_t dwDurability = 0;
    std::uint32_t dwRareIdx = 0;
    std::uint16_t wQuickPosition = 0;
    std::uint32_t dwItemParam = 0;
};

static_assert(sizeof(ExchangeItemSlot) == 24);

// Mirrors legacy sEXCHANGEDATA.
struct ExchangeData {
    void*                  pPlayer = nullptr;  // CPlayer* opaque
    int                    nAddItemNum = 0;
    std::array<ExchangeItemSlot, MAX_EXCHANGEITEM> ItemInfo{};
    std::uint32_t          dwMoney = 0;
    bool                   bLock = false;
    bool                   bExchange = false;
};

// Mirrors legacy CExchangeRoom.
struct ExchangeRoom {
    std::array<ExchangeData, 2> m_ExchangeData{};
    int                          m_nExchangeState = 0;

    inline void init_data(ExchangeData& slot, void* p) {
        slot.pPlayer = p;
        slot.nAddItemNum = 0;
        slot.ItemInfo.fill(ExchangeItemSlot{});
        slot.dwMoney = 0;
        slot.bLock = false;
        slot.bExchange = false;
    }
};

// Mirrors legacy sEXCHANGECONTAINER.
struct ExchangeContainer {
    void* pRoom     = nullptr;  // CExchangeRoom* opaque
    int   nMyIndex  = -1;
    void* pOpPlayer = nullptr;
};

// ---- Free functions (mirroring CExchangeRoom methods) ----

inline void init_exchange_room(ExchangeRoom& r, void* p1, void* p2) {
    r.m_nExchangeState = static_cast<int>(ExchangeState::Waiting);
    r.init_data(r.m_ExchangeData[0], p1);
    r.init_data(r.m_ExchangeData[1], p2);
}

inline void exit_exchange_room(ExchangeRoom& r) {
    r.m_nExchangeState = static_cast<int>(ExchangeState::Waiting);
    r.m_ExchangeData[0] = ExchangeData{};
    r.m_ExchangeData[1] = ExchangeData{};
}

// Lock / unlock by participant index.
inline void lock_slot(ExchangeRoom& r, int idx, bool lock_flag) {
    if (idx < 0 || idx > 1) return;
    if (lock_flag) {
        r.m_ExchangeData[idx].bLock = true;
        return;
    }
    for (auto& slot : r.m_ExchangeData) {
        slot.bLock = false;
        slot.bExchange = false;
    }
}

inline bool is_slot_locked(const ExchangeRoom& r, int idx) {
    if (idx < 0 || idx > 1) return false;
    return r.m_ExchangeData[idx].bLock;
}

inline bool is_all_locked(const ExchangeRoom& r) {
    return r.m_ExchangeData[0].bLock && r.m_ExchangeData[1].bLock;
}

// SetExchange toggles the per-player accept flag.
inline void set_exchange_accept(ExchangeRoom& r, int idx) {
    if (idx < 0 || idx > 1) return;
    r.m_ExchangeData[idx].bExchange = true;
}

inline bool is_all_accepted(const ExchangeRoom& r) {
    return r.m_ExchangeData[0].bExchange && r.m_ExchangeData[1].bExchange;
}

inline void set_exchange_state(ExchangeRoom& r, ExchangeState s) {
    r.m_nExchangeState = static_cast<int>(s);
}

// Add an item to a participant's slot list. Returns false on overflow.
inline bool add_exchange_item(ExchangeRoom& r, int idx, const ExchangeItemSlot& item) {
    if (idx < 0 || idx > 1) return false;
    auto& data = r.m_ExchangeData[idx];
    if (data.bLock || data.nAddItemNum >= MAX_EXCHANGEITEM) return false;
    if (item.wQuickPosition != 0) return false;
    data.ItemInfo[static_cast<std::size_t>(data.nAddItemNum)] = item;
    data.nAddItemNum += 1;
    return true;
}

inline bool del_exchange_item(ExchangeRoom& r, int idx, int pos) {
    if (idx < 0 || idx > 1) return false;
    auto& data = r.m_ExchangeData[idx];
    if (pos < 0 || pos >= data.nAddItemNum || data.bLock) return false;
    for (int i = pos; i < data.nAddItemNum - 1; ++i) {
        data.ItemInfo[static_cast<std::size_t>(i)] = data.ItemInfo[static_cast<std::size_t>(i + 1)];
    }
    data.ItemInfo[static_cast<std::size_t>(data.nAddItemNum - 1)] = ExchangeItemSlot{};
    data.nAddItemNum -= 1;
    return true;
}

// InputMoney replaces the participant offer, clamps it to current player money,
// and returns the actual stored amount (legacy CExchangeRoom::InputMoney).
inline std::uint32_t input_money(ExchangeRoom& r, int idx, std::uint32_t amount,
                                 std::uint32_t available_money = 0xFFFFFFFFu) {
    if (idx < 0 || idx > 1) return 0;
    const std::uint32_t actual = amount > available_money ? available_money : amount;
    r.m_ExchangeData[idx].dwMoney = actual;
    return actual;
}

// DoExchange succeeds only when both players locked AND both accepted. Returns
// ExchangeError::OK on success; Error otherwise.
inline ExchangeError do_exchange(const ExchangeRoom& r) {
    if (!is_all_locked(r))     return ExchangeError::Error;
    if (!is_all_accepted(r))   return ExchangeError::Error;
    return ExchangeError::OK;
}

// CancelExchange resets the room back to Waiting with cleared state.
inline void cancel_exchange(ExchangeRoom& r) {
    r.m_nExchangeState = static_cast<int>(ExchangeState::Waiting);
    for (auto& slot : r.m_ExchangeData) {
        slot.bLock = false;
        slot.bExchange = false;
    }
}

// Convenience: total item count across both slots.
inline int total_item_count(const ExchangeRoom& r) {
    return r.m_ExchangeData[0].nAddItemNum + r.m_ExchangeData[1].nAddItemNum;
}

} // namespace mxh::server
