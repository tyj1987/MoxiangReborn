#include "mxh/server/player.hpp"
#include "mxh/server/money_manager.hpp"

#include <algorithm>
#include <limits>

namespace mxh::server {

namespace {
constexpr std::uint16_t kInventorySlots = 80;
constexpr std::uint16_t kEquipmentBase = 80;
constexpr std::uint16_t kMaxLevel = 99;

bool valid_item(const mxh::game::ItemBase& item) noexcept {
    return item.dwDBIdx != 0 && item.wIconIdx != 0;
}

bool has_item_db_id(const InventorySlots& inventory, std::uint32_t db_idx) noexcept {
    for (const auto& item : inventory.items) {
        if (item.dwDBIdx == db_idx && db_idx != 0) return true;
    }
    return false;
}
}

bool Player::initialize(const PlayerSpawnInfo& info) noexcept {
    if (lifecycle_ != PlayerLifecycle::Disconnected) return false;
    state_ = make_player_state(info.player_id, info.user_id, info.level,
                               info.base, info.bonuses);
    state_.name = info.name;
    state_.map_num = info.map_num;
    state_.progress.max_exp = 0;
    lifecycle_ = PlayerLifecycle::Loading;
    return true;
}

bool Player::activate() noexcept {
    if (lifecycle_ != PlayerLifecycle::Loading) return false;
    lifecycle_ = PlayerLifecycle::Active;
    return true;
}

bool Player::begin_logout() noexcept {
    if (lifecycle_ != PlayerLifecycle::Active &&
        lifecycle_ != PlayerLifecycle::Dead) return false;
    lifecycle_ = PlayerLifecycle::LoggingOut;
    return true;
}

void Player::release() noexcept {
    state_ = PlayerState{};
    lifecycle_ = PlayerLifecycle::Disconnected;
}

bool Player::add_money(std::uint32_t amount) noexcept {
    if (!is_active() || amount == 0) return false;
    constexpr auto max_money = MXH_PLAYER_MAX_MONEY;
    const auto available = max_money - std::min(state_.progress.money, max_money);
    const auto applied = std::min(amount, available);
    state_.progress.money += applied;
    return applied != 0;
}

bool Player::spend_money(std::uint32_t amount) noexcept {
    if (!is_active() || amount == 0 || state_.progress.money < amount) return false;
    state_.progress.money -= amount;
    return true;
}

bool Player::set_money(std::uint32_t amount) noexcept {
    if (!is_active()) return false;
    state_.progress.money = std::min(amount, MXH_PLAYER_MAX_MONEY);
    return true;
}

std::uint32_t Player::add_experience(std::uint32_t amount,
                                     std::uint32_t next_level_exp) noexcept {
    if ((!is_active() && lifecycle_ != PlayerLifecycle::Loading) || amount == 0) {
        return 0;
    }
    state_.progress.max_exp = next_level_exp;
    const auto before = state_.progress.total_exp;
    const auto room = std::numeric_limits<std::uint32_t>::max() - before;
    const auto applied = std::min(amount, room);
    state_.progress.total_exp = before + applied;
    const auto level_room = std::numeric_limits<std::uint32_t>::max() -
                            state_.progress.level_exp;
    state_.progress.level_exp += std::min(applied, level_room);
    std::uint32_t level_ups = 0;
    while (state_.progress.level < kMaxLevel &&
           next_level_exp != 0 &&
           state_.progress.level_exp >= next_level_exp) {
        state_.progress.level_exp -= next_level_exp;
        ++state_.progress.level;
        ++level_ups;
    }
    return level_ups;
}

std::optional<std::uint16_t> Player::insert_inventory_item(mxh::game::ItemBase item) noexcept {
    if (!is_active() || !valid_item(item) || has_item_db_id(state_.inventory, item.dwDBIdx)) {
        return std::nullopt;
    }
    std::optional<std::uint16_t> slot;
    if (item.Position < kInventorySlots &&
        state_.inventory.items[item.Position].dwDBIdx == 0) {
        slot = item.Position;
    } else {
        for (std::uint16_t i = 0; i < kInventorySlots; ++i) {
            if (state_.inventory.items[i].dwDBIdx == 0) {
                slot = i;
                break;
            }
        }
    }
    if (!slot) return std::nullopt;
    item.Position = *slot;
    state_.inventory.items[*slot] = item;
    return slot;
}

std::optional<mxh::game::ItemBase> Player::remove_inventory_item(std::uint16_t slot) noexcept {
    if (!is_active() || slot >= kInventorySlots || state_.inventory.items[slot].dwDBIdx == 0) {
        return std::nullopt;
    }
    auto item = state_.inventory.items[slot];
    state_.inventory.items[slot] = mxh::game::make_empty_item();
    state_.inventory.items[slot].Position = slot;
    return item;
}

bool Player::equip_inventory_item(std::uint16_t inventory_slot,
                                  std::uint8_t equipment_slot) noexcept {
    if (!is_active() || inventory_slot >= kInventorySlots ||
        equipment_slot >= state_.equipment.items.size()) return false;
    auto& source = state_.inventory.items[inventory_slot];
    auto& target = state_.equipment.items[equipment_slot];
    if (!valid_item(source) || valid_item(target)) return false;
    target = source;
    target.Position = static_cast<std::uint16_t>(kEquipmentBase + equipment_slot);
    source = mxh::game::make_empty_item();
    source.Position = inventory_slot;
    state_.recompute_max_stats();
    return true;
}

bool Player::unequip_item(std::uint8_t equipment_slot,
                          std::uint16_t inventory_slot) noexcept {
    if (!is_active() || equipment_slot >= state_.equipment.items.size() ||
        inventory_slot >= kInventorySlots) return false;
    auto& source = state_.equipment.items[equipment_slot];
    auto& target = state_.inventory.items[inventory_slot];
    if (!valid_item(source) || valid_item(target)) return false;
    target = source;
    target.Position = inventory_slot;
    source = mxh::game::make_empty_item();
    source.Position = static_cast<std::uint16_t>(kEquipmentBase + equipment_slot);
    state_.recompute_max_stats();
    return true;
}

PlayerDamageResult Player::apply_damage(std::uint32_t amount) noexcept {
    PlayerDamageResult result;
    result.requested = amount;
    if (!is_alive() || amount == 0) {
        result.died = lifecycle_ == PlayerLifecycle::Dead;
        return result;
    }

    std::uint32_t real_shield_damage = amount;
    if (state_.mussang_mode) {
        real_shield_damage = static_cast<std::uint32_t>(amount * 0.7f);
    }
    const auto shield = state_.vitals.current_shield;
    std::uint32_t life_damage = 0;
    if (shield < real_shield_damage) {
        result.shield_damage = shield;
        life_damage = amount - shield;
    } else {
        result.shield_damage = real_shield_damage;
    }
    state_.vitals.current_shield -= result.shield_damage;
    result.life_damage = std::min(life_damage, state_.vitals.current_hp);
    state_.vitals.current_hp -= result.life_damage;
    if (state_.vitals.current_hp == 0) {
        lifecycle_ = PlayerLifecycle::Dead;
        result.died = true;
    }
    return result;
}

bool Player::revive() noexcept {
    if (lifecycle_ != PlayerLifecycle::Dead) return false;
    state_.vitals.current_hp = std::max<std::uint32_t>(1, state_.vitals.max_hp / 2);
    state_.vitals.current_shield = 0;
    lifecycle_ = PlayerLifecycle::Active;
    return true;
}

void Player::heal_full() noexcept {
    state_.vitals.current_hp = state_.vitals.max_hp;
    state_.vitals.current_shield = state_.vitals.max_shield;
    state_.vitals.current_mp = state_.vitals.max_mp;
}

}


