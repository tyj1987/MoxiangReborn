// boss_monster.cpp - 1:1 port of legacy CBossMonster behavior.

#include "mxh/server/boss_monster.hpp"
#include <algorithm>
#include <cstring>

namespace mxh::server {

BossMonsterInstance create_boss_from_template(std::uint32_t monster_kind,
                                                const mxh::game::MonsterTemplate& tpl,
                                                const BossMonsterInfo& info,
                                                std::uint32_t object_id,
                                                std::int32_t spawn_x,
                                                std::int32_t spawn_y,
                                                std::int32_t spawn_z,
                                                std::uint16_t map_num) noexcept {
    BossMonsterInstance b;
    b.base.object_id      = object_id;
    b.base.monster_kind   = monster_kind;
    b.base.object_kind    = tpl.ObjectKind;
    std::memcpy(b.base.name, tpl.Name, sizeof(b.base.name));
    b.base.map_num        = map_num;
    b.base.pos_x          = spawn_x;
    b.base.pos_y          = spawn_y;
    b.base.pos_z          = spawn_z;
    b.base.spawn_x        = spawn_x;
    b.base.spawn_y        = spawn_y;
    b.base.spawn_z        = spawn_z;
    b.base.current_hp     = tpl.Life;
    b.base.max_hp         = tpl.Life;
    b.base.current_shield = tpl.Shield;
    b.base.max_shield     = tpl.Shield;
    b.base.exp_reward     = tpl.ExpPoint;
    b.base.ai_state       = AiState::Stand;
    b.base.behavior.monster_kind   = monster_kind;
    b.base.behavior.search_radius  = static_cast<std::uint16_t>(tpl.SearchRange / 50.0f);
    b.base.behavior.chase_radius   = static_cast<std::uint16_t>(tpl.DomainRange / 50.0f);
    b.base.behavior.attack_interval_ms = 2000;  // legacy default 2 s
    b.base.behavior.flee_hp_percent     = 0;   // bosses do not flee
    b.base.behavior.is_boss             = 1;
    b.base.group       = 0;
    b.base.sub_id      = 0;
    b.base.regen_num   = 0;
    b.base.drop_item_id      = 0;
    b.base.drop_item_ratio   = 100;
    b.base.suryun_group      = 0;
    b.base.event_mob         = false;

    b.is_field_boss    = info.is_field_boss;
    b.stage            = 0;
    b.speech_id        = info.speech_id_base;

    (void)info.time_limit_ms;     // tracked at manager level
    (void)info.killer_limit;      // tracked at manager level
    return b;
}

BossPhase apply_boss_damage(BossMonsterInstance& b,
                              std::uint32_t damage,
                              std::uint32_t attacker_player_id,
                              std::uint32_t now_ms) noexcept {
    if (b.base.ai_state == AiState::Die) {
        return BossPhase::Dead;
    }
    // Apply damage to HP.
    if (damage >= b.base.current_hp) {
        b.base.current_hp = 0;
        b.base.ai_state   = AiState::Die;
        b.base.state      = 2;
        b.base.state_entered_ms = now_ms;
        b.stage = 4;
        b.base.killer_player_id = attacker_player_id;
        return BossPhase::Dying;
    }
    b.base.current_hp -= damage;
    // Track last attacker for rage logic.
    if (attacker_player_id != 0) {
        b.base.last_attacker_id = attacker_player_id;
    }
    // Stage transition.
    auto next_phase = boss_phase_from_hp(b.base.current_hp, b.base.max_hp);
    auto cur_phase  = boss_phase_transition(BossPhase::Combat, b.base.current_hp, b.base.max_hp);
    (void)cur_phase;
    if (next_phase >= BossPhase::Enraged && b.stage < 1) {
        b.stage = 1;
        b.base.behavior.attack_interval_ms = 1500;
    }
    if (next_phase >= BossPhase::Phase2 && b.stage < 2) {
        b.stage = 2;
        b.base.behavior.attack_interval_ms = 1000;
    }
    if (next_phase >= BossPhase::Rage && b.stage < 3) {
        b.stage = 3;
        b.base.behavior.attack_interval_ms = 700;
    }
    return next_phase;
}

std::uint32_t pick_rage_target(const std::vector<std::pair<std::uint32_t, std::uint32_t>>& damage_by_player) noexcept {
    if (damage_by_player.empty()) return 0;
    std::uint32_t best_player = damage_by_player[0].first;
    std::uint32_t best_dmg    = damage_by_player[0].second;
    for (const auto& kv : damage_by_player) {
        if (kv.second > best_dmg || (kv.second == best_dmg && kv.first < best_player)) {
            best_player = kv.first;
            best_dmg    = kv.second;
        }
    }
    return best_player;
}

}  // namespace mxh::server