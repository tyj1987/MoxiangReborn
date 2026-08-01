#pragma once

#include "mxh/server/purse.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace mxh::server {

inline constexpr std::size_t PYOGUK_LIST_COUNT = 5;
inline constexpr std::size_t PYOGUK_CELLS_PER_PAGE = 30;
inline constexpr std::size_t PYOGUK_SLOT_COUNT = 150;

enum class PyoGukLocale {
    Korea,
    China,
    Japan,
    HongKong,
    Taiwan,
};

struct PyoGukListInfo {
    std::uint8_t max_cell_num = 0;
    MoneyType max_money = 0;
    MoneyType buy_price = 0;
};

static_assert(sizeof(PyoGukListInfo) == 12);

struct PyoGukAccountState {
    std::uint8_t page_count = 0;
    MoneyType inventory_money = 0;
    MoneyType warehouse_money = 0;
    MoneyType inventory_max_money = PURSE_UNLIMITED;
    MoneyType warehouse_max_money = 0;
    std::uint32_t extra_slot_count = 0;
    bool item_info_initialized = false;
};

enum class PyoGukPurchaseStatus { Ack, Nack };

struct PyoGukPurchaseResult {
    PyoGukPurchaseStatus status = PyoGukPurchaseStatus::Nack;
    std::uint8_t page_count = 0;
    MoneyType charged = 0;
};

enum class PyoGukTransferStatus { Ack, Nack, NoResponse };

struct PyoGukTransferResult {
    PyoGukTransferStatus status = PyoGukTransferStatus::NoResponse;
    MoneyType transferred = 0;
    MoneyType warehouse_money = 0;
};

class PyoGukManager {
public:
    PyoGukManager();

    bool configure_level(std::size_t level, MoneyType max_money, MoneyType buy_price);
    const PyoGukListInfo* info_for(std::size_t level) const noexcept;
    PyoGukPurchaseResult buy(PyoGukAccountState& state, PyoGukLocale locale) const;
    PyoGukTransferResult deposit(PyoGukAccountState& state, MoneyType requested) const;
    PyoGukTransferResult withdraw(PyoGukAccountState& state, MoneyType requested) const;
    static bool check_access(bool has_show_pyoguk_item, bool is_near_warehouse_npc) noexcept;

private:
    std::array<PyoGukListInfo, PYOGUK_LIST_COUNT> m_levels{};
};

} // namespace mxh::server
