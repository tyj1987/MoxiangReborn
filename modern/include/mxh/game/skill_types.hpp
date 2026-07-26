// skill_types.hpp - Phase 10d: Skill and battle data structures.
//
// Simplified skill system for the modern server. The original system
// ([CC]Skill/) has a complex component-based architecture with
// CSkillObject, CSkillInfo, SkillObjectFactory, etc. This is a
// minimal version that supports basic skill usage and damage.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::game {

// ============================================================================
// SkillKind - 1:1 with the legacy SKILLKIND enum in
//   `墨香【源码】\[CC]Header\CommonStruct.h` lines 2676-2688.
// Stored as std::uint8_t to match the original wire format (see
// SKILLINFO::SkillKind in the legacy struct).  Max is a sentinel
// count (== 9), NOT a valid skill-kind value.
// ============================================================================
enum class SkillKind : std::uint8_t {
    Combo       = 0,   // SKILLKIND_COMBO      = 连击
    OuterMugong = 1,   // SKILLKIND_OUTERMUGONG = 外功
    InnerMugong = 2,   // SKILLKIND_INNERMUGONG = 内功
    Simbub      = 3,   // SKILLKIND_SIMBUB      = 心法
    Jinbub      = 4,   // SKILLKIND_JINBUB      = 真法
    Mining      = 5,   // SKILLKIND_MINING      = 采矿
    Collection  = 6,   // SKILLKIND_COLLECTION  = 采集
    Hunt        = 7,   // SKILLKIND_HUNT        = 狩猎
    Titan       = 8,   // SKILLKIND_TITAN       = 泰坦 (magi82)
    Max         = 9,   // SKILLKIND_MAX         = count sentinel
};

// ============================================================================
// SkillInfo - Static skill data (equivalent to CSkillInfo in original).
//
// In the original, this is loaded from SkillList.bin. For the modern server,
// we use a simplified hardcoded table or load from a simple config.
// ============================================================================
struct SkillInfo {
    std::uint32_t skill_idx = 0;       // unique skill ID
    std::string   name;                // skill name (for logging)
    SkillKind    skill_kind = SkillKind::Combo;
    std::uint16_t skill_range = 1;     // max range in tiles
    std::uint16_t target_range = 0;    // AoE radius (0 = single target)
    std::uint16_t delay_time = 1000;   // cooldown in ms
    std::uint16_t duration = 0;        // duration in ms (0 = instant)
    std::uint8_t  weapon_kind = 0;     // required weapon type (0=any)
    std::uint8_t  attrib = 0;          // element attribute (0=none)

    // Level-scaled values (simplified: single value, not 12-level array)
    std::uint16_t phy_attack = 10;     // physical attack power
    std::uint16_t att_attack = 0;      // attribute attack power
    std::uint16_t att_rate = 100;      // attribute attack success rate (%)
    std::uint8_t  critical_rate = 5;   // critical hit chance (%)
    std::uint8_t  stun_rate = 0;       // stun chance (%)
    std::uint16_t need_nearyuk = 0;    // MP cost (内力)
};

// ============================================================================
// SkillInstance - Active skill on the map (equivalent to CSkillObject).
//
// Created when a player uses a skill. Tracks the skill's lifetime,
// targets, and damage results.
// ============================================================================
struct SkillInstance {
    std::uint32_t skill_object_id = 0;  // unique instance ID
    std::uint32_t skill_idx = 0;        // which skill (→ SkillInfo)
    std::uint32_t caster_id = 0;        // who cast it (player/monster ID)
    std::uint32_t main_target_id = 0;   // primary target
    float pos_x = 0, pos_z = 0;        // skill position
    std::uint16_t direction = 0;        // facing direction
    std::uint64_t start_time = 0;       // when it started (ms since epoch)
    std::uint16_t duration = 0;         // how long it lasts (ms)
    bool is_active = true;
};

// ============================================================================
// DamageResult - Result of a damage calculation.
// ============================================================================
struct DamageResult {
    std::uint32_t target_id = 0;
    std::int32_t  damage = 0;           // negative = heal
    bool is_critical = false;
    bool is_miss = false;
    std::uint8_t  hit_result = 0;       // 0=miss, 1=hit, 2=critical
};

// ============================================================================
// PlayerCombatStats - Combat-relevant player stats.
//
// Extracted from PlayerInfo for damage calculation. In the original,
// these come from the character's equipped items, skills, buffs, etc.
// ============================================================================
struct PlayerCombatStats {
    std::uint16_t level = 1;
    std::uint32_t max_hp = 100;
    std::uint32_t current_hp = 100;
    std::uint32_t max_mp = 50;
    std::uint32_t current_mp = 50;
    std::uint16_t phy_attack = 10;      // physical attack
    std::uint16_t phy_defence = 5;      // physical defence
    std::uint16_t att_attack = 0;       // attribute attack
    std::uint16_t att_defence = 0;      // attribute defence
    std::uint8_t  critical_rate = 5;    // critical chance (%)
    std::uint8_t  dodge_rate = 5;       // dodge chance (%)
};

// ============================================================================
// Default skill templates (Phase D1.1 placeholder).
//
// Symmetric to monster_types.hpp::get_default_templates() — a small
// hardcoded table covering the most common skill archetypes.  Will
// be replaced by SkillList.bin parsing in D1.3 once the legacy
// format is documented / ported.
// ============================================================================
inline std::vector<SkillInfo> get_default_skills() {
    std::vector<SkillInfo> skills;

    // Skill 1: basic physical attack (OuterMugong)
    {
        SkillInfo s{};
        s.skill_idx   = 1;
        s.name        = "BasicStrike";
        s.skill_kind  = SkillKind::OuterMugong;
        s.skill_range = 1;
        s.target_range = 0;     // single target
        s.delay_time  = 1000;   // 1s cooldown
        s.duration    = 0;      // instant
        s.weapon_kind = 0;      // any
        s.attrib      = 0;      // none
        s.phy_attack  = 15;
        s.att_attack  = 0;
        s.att_rate    = 100;
        s.critical_rate = 5;
        s.stun_rate   = 0;
        s.need_nearyuk = 5;     // 5 内力
        skills.push_back(s);
    }
    // Skill 2: ranged combo (Combo, multi-hit)
    {
        SkillInfo s{};
        s.skill_idx   = 2;
        s.name        = "TripleCombo";
        s.skill_kind  = SkillKind::Combo;
        s.skill_range = 2;
        s.target_range = 0;
        s.delay_time  = 2500;
        s.duration    = 0;
        s.weapon_kind = 0;
        s.attrib      = 0;
        s.phy_attack  = 12;     // per-hit damage
        s.att_attack  = 0;
        s.att_rate    = 100;
        s.critical_rate = 3;
        s.stun_rate   = 0;
        s.need_nearyuk = 15;
        skills.push_back(s);
    }
    // Skill 3: self-heal (Simbub)
    {
        SkillInfo s{};
        s.skill_idx   = 3;
        s.name        = "HealSelf";
        s.skill_kind  = SkillKind::Simbub;
        s.skill_range = 0;      // self-cast
        s.target_range = 0;
        s.delay_time  = 5000;
        s.duration    = 0;
        s.weapon_kind = 0;
        s.attrib      = 0;
        s.phy_attack  = 0;
        s.att_attack  = 0;
        s.att_rate    = 100;
        s.critical_rate = 0;
        s.stun_rate   = 0;
        s.need_nearyuk = 20;
        skills.push_back(s);
    }
    // Skill 4: area-of-effect attack (OuterMugong, AoE)
    {
        SkillInfo s{};
        s.skill_idx   = 4;
        s.name        = "Whirlwind";
        s.skill_kind  = SkillKind::OuterMugong;
        s.skill_range = 3;
        s.target_range = 200;   // 2-tile AoE radius
        s.delay_time  = 4000;
        s.duration    = 0;
        s.weapon_kind = 0;
        s.attrib      = 0;
        s.phy_attack  = 20;
        s.att_attack  = 0;
        s.att_rate    = 100;
        s.critical_rate = 4;
        s.stun_rate   = 10;
        s.need_nearyuk = 30;
        skills.push_back(s);
    }
    return skills;
}

}  // namespace mxh::game
