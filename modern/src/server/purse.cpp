// purse.cpp - Phase D6 Purse 1:1 port implementations.

#include "mxh/server/purse.hpp"

namespace mxh::server {

bool purse_init(PurseState& s, void* p_owner, MoneyType money, MoneyType max) {
    if (s.m_bInit) return false;
    // Legacy clamp: if (money > max) money = max;
    if (money > max) money = max;
    s.m_pOwner = p_owner;
    s.m_dwMoney = money;
    s.m_dwMaxMoney = max;
    s.m_bInit = true;
    return true;
}

void purse_release(PurseState& s) {
    s.m_pOwner = nullptr;
    s.m_bInit = false;
}

bool purse_is_addition_enough(const PurseState& s, MoneyType addition) {
    if (!s.m_bInit) return false;
    if (s.m_dwMaxMoney == PURSE_UNLIMITED) return true;
    return addition <= (s.m_dwMaxMoney - s.m_dwMoney);
}

bool purse_is_enough_money(const PurseState& s, MoneyType subtraction) {
    if (!s.m_bInit) return false;
    return s.m_dwMoney >= subtraction;
}

MoneyType purse_addition(PurseState& s, MoneyType add, std::uint8_t /*msg_flag*/) {
    if (!s.m_bInit) return 0;
    // Legacy: if (add > max - cur) add = max - cur;
    // (skips the legacy wrap-around edge case where add < cur - cur is unobservable
    //  when max is finite; modern treats max-unlimited as unbounded.)
    if (s.m_dwMaxMoney == PURSE_UNLIMITED) {
        s.m_dwMoney += add;
        return add;
    }
    const MoneyType room = (s.m_dwMoney >= s.m_dwMaxMoney) ? 0u : (s.m_dwMaxMoney - s.m_dwMoney);
    MoneyType actual = (add <= room) ? add : room;
    s.m_dwMoney += actual;
    return actual;
}

MoneyType purse_subtraction(PurseState& s, MoneyType sub, std::uint8_t /*msg_flag*/) {
    if (!s.m_bInit) return 0;
    if (!purse_is_enough_money(s, sub)) return 0;
    // Legacy: if (cur - sub > cur) return 0; -> underflow protection.
    if (sub > s.m_dwMoney) return 0;
    s.m_dwMoney -= sub;
    return sub;
}

bool purse_set_max_money(PurseState& s, MoneyType max) {
    if (!s.m_bInit) return false;
    // Legacy: if (cur > max) return FALSE;
    if (s.m_dwMoney > max) return false;
    s.m_dwMaxMoney = max;
    return true;
}

void purse_set_zero_money(PurseState& s) { s.m_dwMoney = 0; }

MoneyType purse_get_cur_money(const PurseState& s) { return s.m_dwMoney; }
MoneyType purse_get_max_money(const PurseState& s) { return s.m_dwMaxMoney; }

void purse_set_money(PurseState& s, MoneyType money) { s.m_dwMoney = money; }

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int purse_translation_unit_anchor = 0;
}
