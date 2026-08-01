#include "mxh/server/pyo_guk_manager.hpp"

#include <algorithm>

namespace mxh::server {
namespace {

std::size_t locale_page_limit(PyoGukLocale locale, std::uint32_t extra_slot_count) {
    switch (locale) {
    case PyoGukLocale::Japan:
        return 3u + extra_slot_count;
    case PyoGukLocale::HongKong:
    case PyoGukLocale::Taiwan:
        return 2u + extra_slot_count;
    case PyoGukLocale::Korea:
    case PyoGukLocale::China:
        return PYOGUK_LIST_COUNT;
    }
    return PYOGUK_LIST_COUNT;
}

MoneyType remaining_capacity(MoneyType current, MoneyType maximum) {
    return current < maximum ? maximum - current : 0;
}

} // namespace

PyoGukManager::PyoGukManager() {
    for (std::size_t index = 0; index < m_levels.size(); ++index) {
        m_levels[index].max_cell_num =
            static_cast<std::uint8_t>(PYOGUK_CELLS_PER_PAGE * (index + 1));
    }
}

bool PyoGukManager::configure_level(
    std::size_t level,
    MoneyType max_money,
    MoneyType buy_price) {
    if (level == 0 || level > m_levels.size()) {
        return false;
    }

    auto& info = m_levels[level - 1];
    info.max_cell_num = static_cast<std::uint8_t>(PYOGUK_CELLS_PER_PAGE * level);
    info.max_money = max_money;
    info.buy_price = buy_price;
    return true;
}

const PyoGukListInfo* PyoGukManager::info_for(std::size_t level) const noexcept {
    if (level == 0 || level > m_levels.size()) {
        return nullptr;
    }
    return &m_levels[level - 1];
}

PyoGukPurchaseResult PyoGukManager::buy(
    PyoGukAccountState& state,
    PyoGukLocale locale) const {
    if (state.page_count >= PYOGUK_LIST_COUNT ||
        state.page_count >= locale_page_limit(locale, state.extra_slot_count)) {
        return {PyoGukPurchaseStatus::Nack, state.page_count, 0};
    }

    const auto& info = m_levels[state.page_count];
    if (state.inventory_money < info.buy_price) {
        return {PyoGukPurchaseStatus::Nack, state.page_count, 0};
    }

    if (state.page_count == 0) {
        state.item_info_initialized = true;
    }
    state.inventory_money -= info.buy_price;
    ++state.page_count;
    state.warehouse_max_money = m_levels[state.page_count - 1].max_money;
    return {PyoGukPurchaseStatus::Ack, state.page_count, info.buy_price};
}

PyoGukTransferResult PyoGukManager::deposit(
    PyoGukAccountState& state,
    MoneyType requested) const {
    const auto amount = std::min({
        requested,
        state.inventory_money,
        remaining_capacity(state.warehouse_money, state.warehouse_max_money),
    });
    if (amount == 0) {
        return {PyoGukTransferStatus::Nack, 0, state.warehouse_money};
    }

    state.inventory_money -= amount;
    state.warehouse_money += amount;
    return {PyoGukTransferStatus::Ack, amount, state.warehouse_money};
}

PyoGukTransferResult PyoGukManager::withdraw(
    PyoGukAccountState& state,
    MoneyType requested) const {
    const auto amount = std::min({
        requested,
        state.warehouse_money,
        remaining_capacity(state.inventory_money, state.inventory_max_money),
    });
    if (amount == 0) {
        return {PyoGukTransferStatus::NoResponse, 0, state.warehouse_money};
    }

    state.warehouse_money -= amount;
    state.inventory_money += amount;
    return {PyoGukTransferStatus::Ack, amount, state.warehouse_money};
}

bool PyoGukManager::check_access(
    bool has_show_pyoguk_item,
    bool is_near_warehouse_npc) noexcept {
    return has_show_pyoguk_item || is_near_warehouse_npc;
}

} // namespace mxh::server
