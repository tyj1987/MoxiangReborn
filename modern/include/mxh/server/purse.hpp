// purse.hpp - Phase D6 Purse 1:1 port.
//
// Source-of-truth: legacy [Server]Map/Purse.h + .cpp.
// Mirrors legacy CPurse state machine (init, addition, subtraction,
// max-money cap, set-zero).  CInventoryPurse / CPyogukPurse /
// CMunpaWarePurse downcasts are left to integration; they only
// override virtual SetMoney / SendMoneyMsg which we leave as hooks.

#pragma once

#include <cstdint>

namespace mxh::server {

// MONEYTYPE alias (mirror legacy typedef).
using MoneyType = std::uint32_t;

// Sentinel for unbounded money (used when max is not enforced).
inline constexpr MoneyType PURSE_UNLIMITED = 0xFFFFFFFFu;

struct PurseState {
    void* m_pOwner = nullptr;
    MoneyType m_dwMoney   = 0;
    MoneyType m_dwMaxMoney = PURSE_UNLIMITED;
    bool m_bInit = false;
};

// Init: legacy ASSERT if already init; clamp money to max if money>max.
// Returns false if already initialised.  modern softens the duplicate
// init condition (no fatal assert) to keep unit tests deterministic.
bool purse_init(PurseState& s, void* p_owner, MoneyType money, MoneyType max);

// Release: clears m_bInit and p_owner pointer.
void purse_release(PurseState& s);

// Addition: cap to (max - cur), return what was actually added.
// Returns 0 if not init.
MoneyType purse_addition(PurseState& s, MoneyType add, std::uint8_t msg_flag = 0);

// Subtraction: refuses if not enough, returns 0.
// Otherwise subtracts and returns the actual subtracted amount.
MoneyType purse_subtraction(PurseState& s, MoneyType sub, std::uint8_t msg_flag = 0);

bool purse_is_addition_enough(const PurseState& s, MoneyType addition);
bool purse_is_enough_money(const PurseState& s, MoneyType subtraction);

// SetMaxMoney refuses if cur > max (returns false).
bool purse_set_max_money(PurseState& s, MoneyType max);

// SetZeroMoney clamps to 0.
void purse_set_zero_money(PurseState& s);

MoneyType purse_get_cur_money(const PurseState& s);
MoneyType purse_get_max_money(const PurseState& s);

// Setters for integration (SetMoney hook).
void purse_set_money(PurseState& s, MoneyType money);

}  // namespace mxh::server
