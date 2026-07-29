// item_container.cpp - slot operations.

#include "mxh/server/item_container.hpp"

namespace mxh::server {

std::size_t InventoryContainer::first_empty_slot() const noexcept {
    for (std::size_t i = 0; i < slots.size(); ++i) {
        if (mxh::game::is_empty_slot(slots[i])) return i;
    }
    return slots.size();  // sentinel = full
}

bool InventoryContainer::insert(const mxh::game::ItemBase& item) noexcept {
    auto pos = first_empty_slot();
    if (pos >= slots.size()) return false;
    slots[pos] = item;
    slots[pos].Position = static_cast<std::uint16_t>(pos);
    return true;
}

bool InventoryContainer::remove(std::uint16_t position) noexcept {
    if (position >= slots.size()) return false;
    if (mxh::game::is_empty_slot(slots[position])) return false;
    slots[position] = make_empty_slot();
    return true;
}

mxh::game::ItemBase* InventoryContainer::find_by_dbidx(std::uint32_t db_idx) noexcept {
    if (db_idx == 0) return nullptr;
    for (auto& s : slots) {
        if (!mxh::game::is_empty_slot(s) && s.dwDBIdx == db_idx) return &s;
    }
    return nullptr;
}

bool WearContainer::equip(std::uint8_t position, const mxh::game::ItemBase& item) noexcept {
    if (position >= slots.size()) return false;
    if (!mxh::game::is_empty_slot(slots[position])) return false;  // legacy refuses double-equip
    slots[position] = item;
    slots[position].Position = position;
    return true;
}

bool WearContainer::unequip(std::uint8_t position) noexcept {
    if (position >= slots.size()) return false;
    if (mxh::game::is_empty_slot(slots[position])) return false;
    slots[position] = make_empty_slot();
    return true;
}

bool ShopInven::add(const mxh::game::ItemBase& item) noexcept {
    for (std::size_t i = 0; i < slots.size(); ++i) {
        if (mxh::game::is_empty_slot(slots[i])) {
            slots[i] = item;
            slots[i].Position = static_cast<std::uint16_t>(i);
            return true;
        }
    }
    return false;
}

bool ShopInven::remove(std::uint16_t position) noexcept {
    if (position >= slots.size()) return false;
    if (mxh::game::is_empty_slot(slots[position])) return false;
    slots[position] = make_empty_slot();
    return true;
}

bool PyogukContainer::deposit(const mxh::game::ItemBase& item) noexcept {
    for (std::size_t i = 0; i < slots.size(); ++i) {
        if (mxh::game::is_empty_slot(slots[i])) {
            slots[i] = item;
            slots[i].Position = static_cast<std::uint16_t>(i);
            return true;
        }
    }
    return false;
}

bool PyogukContainer::withdraw(std::uint16_t position) noexcept {
    if (position >= slots.size()) return false;
    if (mxh::game::is_empty_slot(slots[position])) return false;
    slots[position] = make_empty_slot();
    return true;
}

}  // namespace mxh::server
