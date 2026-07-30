#pragma once

#include "mxh/server/player_state.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace mxh::server {

enum class PlayerLifecycle : std::uint8_t {
    Disconnected = 0,
    Loading = 1,
    Active = 2,
    Dead = 3,
    LoggingOut = 4,
};

struct PlayerSpawnInfo final {
    std::uint32_t player_id = 0;
    std::uint32_t user_id = 0;
    std::uint16_t level = 1;
    std::uint16_t map_num = 0;
    std::string name;
    CalcBaseStats base;
    CalcEquipBonuses bonuses;
};

struct PlayerDamageResult final {
    std::uint32_t requested = 0;
    std::uint32_t shield_damage = 0;
    std::uint32_t life_damage = 0;
    bool died = false;
};

class Player final {
public:
    Player() = default;

    bool initialize(const PlayerSpawnInfo& info) noexcept;
    bool activate() noexcept;
    bool begin_logout() noexcept;
    void release() noexcept;

    PlayerLifecycle lifecycle() const noexcept { return lifecycle_; }
    bool is_active() const noexcept { return lifecycle_ == PlayerLifecycle::Active; }
    bool is_alive() const noexcept {
        return lifecycle_ != PlayerLifecycle::Dead && state_.vitals.current_hp != 0;
    }

    PlayerState& state() noexcept { return state_; }
    const PlayerState& state() const noexcept { return state_; }

    bool add_money(std::uint32_t amount) noexcept;
    bool spend_money(std::uint32_t amount) noexcept;
    bool set_money(std::uint32_t amount) noexcept;

    std::uint32_t add_experience(std::uint32_t amount,
                                 std::uint32_t next_level_exp) noexcept;

    std::optional<std::uint16_t> insert_inventory_item(mxh::game::ItemBase item) noexcept;
    std::optional<mxh::game::ItemBase> remove_inventory_item(std::uint16_t slot) noexcept;
    bool equip_inventory_item(std::uint16_t inventory_slot,
                              std::uint8_t equipment_slot) noexcept;
    bool unequip_item(std::uint8_t equipment_slot,
                      std::uint16_t inventory_slot) noexcept;

    PlayerDamageResult apply_damage(std::uint32_t amount) noexcept;
    bool revive() noexcept;
    void heal_full() noexcept;

private:
    PlayerState state_{};
    PlayerLifecycle lifecycle_ = PlayerLifecycle::Disconnected;
};

}  // namespace mxh::server
