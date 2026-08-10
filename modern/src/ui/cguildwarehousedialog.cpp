#include "cguildwarehousedialog.hpp"

namespace mxh::ui {
namespace {
bool valid(WarehouseItem item) { return item.item_id != 0 && item.quantity != 0; }
}

bool cGuildWarehouseDialog::Store(std::size_t slot, WarehouseItem item) {
    if (m_locked || !m_can_store || slot >= kSlots || m_items[slot] || !valid(item)) return false;
    if (m_inventory_service && !m_inventory_service->hasItem(item.item_id)) return false;
    m_items[slot] = item;
    return true;
}

std::optional<WarehouseItem> cGuildWarehouseDialog::Take(std::size_t slot) {
    if (m_locked || !m_can_take || slot >= kSlots || !m_items[slot]) return std::nullopt;
    auto item = m_items[slot];
    m_items[slot].reset();
    return item;
}

bool cGuildWarehouseDialog::Move(std::size_t from, std::size_t to) {
    if (m_locked || !m_can_store || !m_can_take || from >= kSlots || to >= kSlots || from == to || !m_items[from] || m_items[to]) return false;
    std::swap(m_items[from], m_items[to]);
    return true;
}
}
