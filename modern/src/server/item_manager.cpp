// item_manager.cpp

#include "mxh/server/item_manager.hpp"
#include <algorithm>

namespace mxh::server {

// MAX_MONEY == 2,000,000,000 (legacy 32-bit cap)
constexpr std::uint32_t kMaxMoney = 2000000000u;

static bool item_empty(const mxh::game::ItemBase& item) noexcept {
    return item.dwDBIdx == 0;
}

std::optional<std::uint16_t> find_free_inventory_slot(const InventorySlots& inv) noexcept {
    for (std::uint16_t i = 0; i < inv.items.size(); ++i) {
        if (item_empty(inv.items[i])) return i;
    }
    return std::nullopt;
}

std::uint16_t inventory_free_count(const InventorySlots& inv) noexcept {
    std::uint16_t n = 0;
    for (std::uint16_t i = 0; i < inv.items.size(); ++i) {
        if (item_empty(inv.items[i])) ++n;
    }
    return n;
}

ItemOpResult add_item(InventorySlots& inv, const mxh::game::ItemBase& item) noexcept {
    ItemOpResult r;
    auto pos = find_free_inventory_slot(inv);
    if (!pos.has_value()) {
        r.success = false;
        r.overflow = 1;
        return r;
    }
    inv.items[*pos] = item;
    r.success = true;
    return r;
}

ItemOpResult remove_item(InventorySlots& inv, std::uint16_t pos) noexcept {
    ItemOpResult r;
    if (pos >= inv.items.size() || item_empty(inv.items[pos])) {
        r.success = false;
        return r;
    }
    inv.items[pos] = mxh::game::ItemBase{};
    r.success = true;
    return r;
}

ItemOpResult equip_item(EquipSlots& slots, PlayerState& state,
                        const mxh::game::ItemBase& item,
                        std::uint8_t slot_idx) noexcept {
    ItemOpResult r;
    if (slot_idx >= slots.items.size()) {
        r.success = false;
        return r;
    }
    // Displaced item (if any) goes to inventory
    if (!item_empty(slots.items[slot_idx])) {
        auto displaced = slots.items[slot_idx];
        slots.items[slot_idx] = item;
        auto inv_result = add_item(state.inventory, displaced);
        if (!inv_result.success) {
            // Inventory full: rollback
            slots.items[slot_idx] = displaced;
            r.success = false;
            r.overflow = 1;
            return r;
        }
    } else {
        slots.items[slot_idx] = item;
    }
    r.success = true;
    state.recompute_max_stats();
    return r;
}

ItemOpResult unequip_item(EquipSlots& slots, InventorySlots& inv,
                          std::uint8_t slot_idx) noexcept {
    ItemOpResult r;
    if (slot_idx >= slots.items.size() || item_empty(slots.items[slot_idx])) {
        r.success = false;
        return r;
    }
    auto pos = find_free_inventory_slot(inv);
    if (!pos.has_value()) {
        r.success = false;
        return r;
    }
    inv.items[*pos] = slots.items[slot_idx];
    slots.items[slot_idx] = mxh::game::ItemBase{};
    r.success = true;
    return r;
}

ItemOpResult pyoguk_in(PyogukSlots& p, const mxh::game::ItemBase& item) noexcept {
    ItemOpResult r;
    for (std::uint16_t i = 0; i < p.items.size(); ++i) {
        if (item_empty(p.items[i])) {
            p.items[i] = item;
            r.success = true;
            return r;
        }
    }
    r.success = false;
    return r;
}

ItemOpResult pyoguk_out(PyogukSlots& p, std::uint16_t pos) noexcept {
    ItemOpResult r;
    if (pos >= p.items.size() || item_empty(p.items[pos])) {
        r.success = false;
        return r;
    }
    p.items[pos] = mxh::game::ItemBase{};
    r.success = true;
    return r;
}

std::optional<std::uint32_t> add_money(std::uint32_t purse, std::uint32_t delta) noexcept {
    std::uint64_t sum = static_cast<std::uint64_t>(purse) + delta;
    if (sum > kMaxMoney) return std::nullopt;
    return static_cast<std::uint32_t>(sum);
}

std::optional<std::uint32_t> spend_money(std::uint32_t purse, std::uint32_t cost) noexcept {
    if (cost > purse) return std::nullopt;
    return purse - cost;
}

}  // namespace mxh::server
