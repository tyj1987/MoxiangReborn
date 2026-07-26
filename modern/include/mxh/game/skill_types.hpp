// skill_types.hpp - Phase D1.3: 1:1 SKILLINFO port
//
// 1:1 port of the legacy SKILLINFO struct from
//   `墨香【源码】\[CC]Header\CommonStruct.h` lines 2690-2869.
//
// Phase 10d started with a simplified 14-field version; D1.3 expands
// it to the full legacy surface (60+ fields including 7 12-element
// level-scaled arrays).  Every field name and offset is a 1:1 match
// with the legacy struct; field naming preserves the legacy WORD/DWORD
// distinctions (e.g. `SkillIdx` is uint16_t not uint32_t, `Duration`
// is uint32_t not uint16_t).
//
// The legacy struct is read from SkillList.bin (a MHFile-packed text
// file).  See skill_list_parser.hpp for the loader.

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
// Legacy constants from CommonStruct.h / CommonGameDefine.h.
//
// MAX_NAME_LENGTH = 16, so SkillName is char[17] in the legacy struct.
// SKILL_MAX_LEVEL = 12 is the size of every level-scaled array.
// ============================================================================
inline constexpr std::uint16_t SKILL_MAX_NAME  = 17;  // MAX_NAME_LENGTH+1
inline constexpr std::uint16_t SKILL_MAX_LEVEL = 12;

// ============================================================================
// SkillInfo - 1:1 port of legacy SKILLINFO.
//
// Field names match the legacy struct exactly (CamelCase, e.g.
// `SkillIdx` not `skill_idx`).  Where the legacy struct is word-sized
// we use uint16_t; where it is DWORD-sized we use uint32_t.  Array
// fields use a fixed-size std::array<T, SKILL_MAX_LEVEL> for 1:1
// byte layout, so the size of the struct matches the legacy wire
// format (modulo the SkillName field which is a std::string of equal
// capacity -- the parser fills it with the NUL-padded 16-byte name).
//
// The legacy struct contains many "AdditiveAttr" segments that are
// only set by one of N branches in skillinfo.cpp (lines 129-228).
// Modern port uses fixed-size std::array<T, 12> for each so the
// "current" branch's values are kept and other branches' values stay 0.
// ============================================================================
struct SkillInfo {
    // --- identity (legacy offset 0..3) ---
    std::uint16_t SkillIdx = 0;                    // unique skill ID
    char          SkillName[SKILL_MAX_NAME] = {};  // 16+1 NUL-padded (EUC-KR)
    std::uint16_t SkillTooltipIdx = 0;
    std::uint16_t RestrictLevel = 0;               // legacy LEVELTYPE = WORD
    std::int32_t  LowImage = 0;
    std::int32_t  HighImage = 0;

    // --- classification (legacy offset ~21) ---
    std::uint16_t SkillKind = 0;                   // raw value, cast to SkillKind
    std::uint16_t WeaponKind = 0;
    std::uint16_t SkillRange = 0;

    // --- target geometry (legacy offset ~27) ---
    std::uint16_t TargetKind = 0;                  // 0: target, 1: self
    std::uint16_t TargetRange = 0;                 // 0 = single, else AoE
    std::uint16_t TargetAreaIdx = 0;
    std::uint16_t TargetAreaPivot = 0;             // 0: target, 1: self
    std::uint16_t TargetAreaFix = 0;

    std::uint16_t MoveTargetArea = 0;
    std::uint16_t MoveTargetAreaDirection = 0;
    float         MoveTargetAreaVelocity = 0.0f;

    // --- timing (legacy offset ~49) ---
    std::uint32_t Duration = 0;                    // legacy DWORD
    std::uint16_t Interval = 0;
    std::uint16_t DelaySingleEffect = 0;

    // --- combo / casting (legacy offset ~57) ---
    std::uint16_t ComboNum = 0;                    // 1..6 = combo step, 100 = mugong
    std::uint16_t Life = 0;                        // life-skill flag
    std::uint16_t BindOperator = 0;

    // --- effect references (legacy offset ~63) ---
    std::int32_t  EffectStartTime = 0;
    std::int32_t  EffectStart = 0;                 // -1 = none, else effect number
    std::int32_t  EffectUse = 0;
    std::int32_t  EffectSelf = 0;
    std::int32_t  EffectMapObjectCreate = 0;
    std::int32_t  EffectMineOperate = 0;

    // --- damage / cooldowns (legacy offset ~87) ---
    std::uint32_t DelayTime = 0;                   // legacy DWORD
    std::uint16_t FatalDamage = 0;

    // --- per-level arrays (legacy: 12 elements each) ---
    std::uint32_t NeedExp[SKILL_MAX_LEVEL]    = {}; // legacy DWORD[12]
    std::uint16_t NeedNaeRyuk[SKILL_MAX_LEVEL] = {};

    // --- attribute / targeting (legacy offset ~127) ---
    std::uint16_t Attrib = 0;                       // 0..6: none/fire/water/...
    std::uint16_t NegativeResultTargetType = 0;
    std::uint16_t TieUpType = 0;
    std::uint16_t ChangeSpeedType = 0;
    std::uint16_t ChangeSpeedRate = 0;
    std::uint16_t Dispel = 0;
    std::uint16_t PositiveResultTargetType = 0;
    std::uint16_t Immune = 0;
    std::uint16_t AIObject = 0;
    std::uint16_t MineCheckRange = 0;
    std::uint16_t MineCheckStartTime = 0;

    std::uint16_t CounterDodgeKind = 0;             // 0: defense, 1: counter, 2: dodge
    std::int32_t  CounterEffect = 0;
    std::uint16_t DamageDecreaseForDist = 0;        // 041213 KES

    // --- AdditiveAttr segment 0..N (legacy: 6 segments, 12 values each) ---
    // Each segment has a discriminator (AdditiveAttr) followed by 12
    // values.  The parser sets the relevant field; the others stay 0.
    // Naming uses PascalCase to match legacy struct members.
    float         CounterDodgeRate[SKILL_MAX_LEVEL]     = {};
    std::uint16_t FirstRecoverLife[SKILL_MAX_LEVEL]    = {};
    std::uint16_t FirstRecoverNaeRyuk[SKILL_MAX_LEVEL] = {};
    std::uint16_t ContinueRecoverLife[SKILL_MAX_LEVEL]   = {};
    std::uint16_t ContinueRecoverNaeRyuk[SKILL_MAX_LEVEL] = {};
    std::uint16_t ContinueRecoverShield[SKILL_MAX_LEVEL]  = {};
    float         CounterPhyAttack[SKILL_MAX_LEVEL]     = {};
    float         CounterAttAttack[SKILL_MAX_LEVEL]     = {};
    float         CriticalRate[SKILL_MAX_LEVEL]         = {};
    float         StunRate[SKILL_MAX_LEVEL]             = {};
    std::uint16_t StunTime[SKILL_MAX_LEVEL]             = {};
    float         FirstPhyAttack[SKILL_MAX_LEVEL]       = {};
    float         FirstAttAttack[SKILL_MAX_LEVEL]       = {};
    std::uint16_t FirstAttAttackMin[SKILL_MAX_LEVEL]   = {};
    std::uint16_t FirstAttAttackMax[SKILL_MAX_LEVEL]   = {};
    std::uint16_t ContinueAttAttack[SKILL_MAX_LEVEL]   = {};
    float         ContinueAttAttackRate[SKILL_MAX_LEVEL] = {};
    std::uint16_t AmplifiedPowerPhy[SKILL_MAX_LEVEL]   = {};
    std::uint16_t AmplifiedPowerAtt[SKILL_MAX_LEVEL]   = {};
    float         AmplifiedPowerAttRate[SKILL_MAX_LEVEL] = {};
    float         VampiricLife[SKILL_MAX_LEVEL]         = {};
    float         VampiricNaeryuk[SKILL_MAX_LEVEL]      = {};
    float         RecoverStateAbnormal[SKILL_MAX_LEVEL] = {};
    float         DispelAttackFeelRate[SKILL_MAX_LEVEL] = {};
    float         ChangeSpeedProbability[SKILL_MAX_LEVEL] = {};
    std::uint16_t UpMaxLife[SKILL_MAX_LEVEL]            = {};
    std::uint16_t UpMaxNaeRyuk[SKILL_MAX_LEVEL]         = {};
    std::uint16_t UpMaxShield[SKILL_MAX_LEVEL]          = {};
    float         UpPhyDefence[SKILL_MAX_LEVEL]         = {};
    float         UpAttDefence[SKILL_MAX_LEVEL]         = {};
    float         UpPhyAttack[SKILL_MAX_LEVEL]          = {};
    std::uint16_t DownMaxLife[SKILL_MAX_LEVEL]          = {};
    std::uint16_t DownMaxNaeRyuk[SKILL_MAX_LEVEL]       = {};
    std::uint16_t DownMaxShield[SKILL_MAX_LEVEL]        = {};
    float         DownPhyDefence[SKILL_MAX_LEVEL]       = {};
    float         DownAttDefence[SKILL_MAX_LEVEL]       = {};
    float         DownPhyAttack[SKILL_MAX_LEVEL]        = {};
    std::uint32_t SkillAdditionalTime[SKILL_MAX_LEVEL]  = {};  // legacy DWORD
    std::uint16_t UpAttAttack[SKILL_MAX_LEVEL]          = {};
    float         DamageRate[SKILL_MAX_LEVEL]            = {};
    float         AttackRate[SKILL_MAX_LEVEL]            = {};
    float         UpCriticalRate[SKILL_MAX_LEVEL]        = {};
    float         AttackLifeRate[SKILL_MAX_LEVEL]        = {};
    float         AttackShieldRate[SKILL_MAX_LEVEL]      = {};
    float         AttackSuccessRate[SKILL_MAX_LEVEL]     = {};
    float         VampiricReverseLife[SKILL_MAX_LEVEL]  = {};
    float         VampiricReverseNaeryuk[SKILL_MAX_LEVEL] = {};
    std::uint32_t AttackPhyLastUp[SKILL_MAX_LEVEL]      = {};  // legacy DWORD
    std::uint32_t AttackAttLastUp[SKILL_MAX_LEVEL]      = {};  // legacy DWORD

    // --- 2nd-class / magi82 ext (legacy offset ~end) ---
    std::uint16_t SkipEffect = 0;
    std::int32_t  CanSkipEffect = 0;                // legacy BOOL = int32_t
    std::uint16_t SpecialState = 0;
    std::uint16_t ChangeKind = 0;
    std::int32_t  AddDegree = 0;
    std::uint16_t SafeRange = 0;
    std::uint16_t LinkSkillIdx = 0;
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
// Simplified (legacy-compatible) SkillInfo builder.
//
// The D1.3 parser populates the full legacy struct, but the in-game
// combat path still uses the simplified fields.  This helper extracts
// the simplified view from a full SkillInfo at lookup time.  1:1
// with the legacy CSkillInfo::Get*() accessors.
// ============================================================================
struct SkillInfoSimple {
    std::uint32_t skill_idx = 0;
    std::string   name;
    SkillKind     skill_kind = SkillKind::Combo;
    std::uint16_t skill_range = 1;
    std::uint16_t target_range = 0;
    std::uint16_t delay_time = 1000;
    std::uint16_t duration = 0;
    std::uint8_t  weapon_kind = 0;
    std::uint8_t  attrib = 0;
    std::uint16_t phy_attack = 10;
    std::uint16_t att_attack = 0;
    std::uint16_t att_rate = 100;
    std::uint8_t  critical_rate = 5;
    std::uint8_t  stun_rate = 0;
    std::uint16_t need_nearyuk = 0;
};

inline SkillInfoSimple to_simple(const SkillInfo& s) {
    SkillInfoSimple out;
    out.skill_idx    = s.SkillIdx;
    // SkillName is 16+1 NUL-padded EUC-KR; copy up to first NUL.
    std::string name;
    for (std::uint16_t i = 0; i < SKILL_MAX_NAME; ++i) {
        if (s.SkillName[i] == '\0') break;
        name.push_back(s.SkillName[i]);
    }
    out.name         = std::move(name);
    out.skill_kind   = (s.SkillKind < static_cast<std::uint16_t>(SkillKind::Max))
                           ? static_cast<SkillKind>(s.SkillKind)
                           : SkillKind::Combo;
    out.skill_range  = s.SkillRange;
    out.target_range = s.TargetRange;
    // DelayTime is DWORD in legacy (ms).  Truncate to 16 bits for the
    // simple view; the full struct preserves the full DWORD.
    out.delay_time   = static_cast<std::uint16_t>(s.DelayTime & 0xFFFFu);
    out.duration     = static_cast<std::uint16_t>(s.Duration & 0xFFFFu);
    out.weapon_kind  = static_cast<std::uint8_t>(s.WeaponKind & 0xFFu);
    out.attrib       = static_cast<std::uint8_t>(s.Attrib & 0xFFu);
    // PhyAttack level 0 (== 1st level in legacy) -- 1:1 with
    // CSkillInfo::GetUpPhyAttack(1).
    out.phy_attack   = static_cast<std::uint16_t>(s.UpPhyAttack[0]);
    // FirstAttAttack level 1: legacy CSkillInfo::GetFirstAttAttack(1).
    out.att_attack   = static_cast<std::uint16_t>(s.FirstAttAttack[0]);
    // AttackSuccessRate level 1: legacy CSkillInfo::GetAttackSuccessRate(1)
    // stored as percentage (0..100).
    out.att_rate     = static_cast<std::uint16_t>(s.AttackSuccessRate[0]);
    out.critical_rate = static_cast<std::uint8_t>(s.CriticalRate[0]);
    out.stun_rate    = static_cast<std::uint8_t>(s.StunRate[0]);
    out.need_nearyuk = s.NeedNaeRyuk[0];
    return out;
}

// ============================================================================
// Default skill templates (Phase D1.1 placeholder, retained for tests
// and for the D1.3 fallback when SkillList.bin is unavailable).
// ============================================================================
inline std::vector<SkillInfo> get_default_skills() {
    std::vector<SkillInfo> skills;

    // Skill 1: basic physical attack (OuterMugong)
    {
        SkillInfo s{};
        s.SkillIdx   = 1;
        const char name1[] = "BasicStrike";
        for (std::size_t i = 0; i < sizeof(name1); ++i) s.SkillName[i] = name1[i];
        s.SkillKind    = static_cast<std::uint16_t>(SkillKind::OuterMugong);
        s.SkillRange   = 1;
        s.TargetRange  = 0;     // single target
        s.DelayTime    = 1000;  // 1s cooldown
        s.Duration     = 0;     // instant
        s.WeaponKind   = 0;     // any
        s.Attrib       = 0;     // none
        s.UpPhyAttack[0]   = 15;
        s.FirstAttAttack[0] = 0;
        s.AttackSuccessRate[0] = 100.0f;
        s.CriticalRate[0]   = 5.0f;
        s.StunRate[0]       = 0.0f;
        s.NeedNaeRyuk[0]    = 5;
        skills.push_back(s);
    }
    // Skill 2: ranged combo (Combo, multi-hit)
    {
        SkillInfo s{};
        s.SkillIdx   = 2;
        const char name2[] = "TripleCombo";
        for (std::size_t i = 0; i < sizeof(name2); ++i) s.SkillName[i] = name2[i];
        s.SkillKind    = static_cast<std::uint16_t>(SkillKind::Combo);
        s.SkillRange   = 2;
        s.TargetRange  = 0;
        s.DelayTime    = 2500;
        s.Duration     = 0;
        s.WeaponKind   = 0;
        s.Attrib       = 0;
        s.UpPhyAttack[0]   = 12;
        s.AttackSuccessRate[0] = 100.0f;
        s.CriticalRate[0]   = 3.0f;
        s.NeedNaeRyuk[0]    = 15;
        skills.push_back(s);
    }
    // Skill 3: self-heal (Simbub)
    {
        SkillInfo s{};
        s.SkillIdx   = 3;
        const char name3[] = "HealSelf";
        for (std::size_t i = 0; i < sizeof(name3); ++i) s.SkillName[i] = name3[i];
        s.SkillKind    = static_cast<std::uint16_t>(SkillKind::Simbub);
        s.SkillRange   = 0;     // self-cast
        s.TargetRange  = 0;
        s.DelayTime    = 5000;
        s.Duration     = 0;
        s.WeaponKind   = 0;
        s.Attrib       = 0;
        s.AttackSuccessRate[0] = 100.0f;
        s.NeedNaeRyuk[0]    = 20;
        skills.push_back(s);
    }
    // Skill 4: area-of-effect attack (OuterMugong, AoE)
    {
        SkillInfo s{};
        s.SkillIdx   = 4;
        const char name4[] = "Whirlwind";
        for (std::size_t i = 0; i < sizeof(name4); ++i) s.SkillName[i] = name4[i];
        s.SkillKind    = static_cast<std::uint16_t>(SkillKind::OuterMugong);
        s.SkillRange   = 3;
        s.TargetRange  = 200;
        s.DelayTime    = 4000;
        s.Duration     = 0;
        s.WeaponKind   = 0;
        s.Attrib       = 0;
        s.UpPhyAttack[0]   = 20;
        s.StunRate[0]      = 10.0f;
        s.CriticalRate[0]  = 4.0f;
        s.AttackSuccessRate[0] = 100.0f;
        s.NeedNaeRyuk[0]    = 30;
        skills.push_back(s);
    }
    return skills;
}

}  // namespace mxh::game
