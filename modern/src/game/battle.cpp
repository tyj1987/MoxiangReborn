#include "mxh/game/battle.hpp"
namespace mxh::game {
DamageResult resolve_physical_attack(const BattleContext& context) {
    DamageResult result{};
    result.mana_before = context.attacker_mana;
    if (context.attacker_mana < context.mana_cost) {
        result.mana_after = context.attacker_mana;
        return result;
    }
    result.damage = compute_player_physical_attack(context.attacker_min_damage,
        context.attacker_max_damage, context.base_attack_bonus,
        context.attack_rate, context.critical, context.random_gap);
    result.mana_after = context.attacker_mana - context.mana_cost;
    result.executed = true;
    return result;
}
}
