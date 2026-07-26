#pragma once
#include <cstdint>
#include "mxh/game/battle_factory.hpp"
namespace mxh::game {
struct BattleContext {
    std::uint32_t attacker_min_damage{};
    std::uint32_t attacker_max_damage{};
    double attack_rate{1.0};
    double base_attack_bonus{};
    std::uint32_t mana_cost{};
    std::uint32_t attacker_mana{};
    bool critical{};
    int random_gap{};
};
struct DamageResult {
    std::uint32_t damage{};
    std::uint32_t mana_before{};
    std::uint32_t mana_after{};
    bool executed{};
};
DamageResult resolve_physical_attack(const BattleContext& context);
}
