#pragma once

// ============================================================================
// Monster/NPC data structures â€” 1:1 with original CommonStruct.h
//
// MONSTER_TOTALINFO layout (14 bytes, packed):
//   Life(4) Shield(4) MonsterKind(2) Group(2) MapNum(2)
//
// SEND_MONSTER_TOTALINFO is sent via UserConn::MonsterAdd (proto=37):
//   BASEOBJECT_INFO(35B) + MONSTER_TOTALINFO(14B) + SEND_MOVEINFO(14B)
//   + bLogin(1B) + AddableInfoList(variable)
//
// NPC_REGEN (spawn point data, ~44 bytes):
//   dwObjectID(4) MapNum(2) NpcKind(2) Name[17] NpcIndex(2)
//   Pos(12) Angle(4)
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <array>

namespace mxh::game {

// Monster/NPC name length limits (from CommonGameDefine.h).
inline constexpr std::size_t MAX_MONSTER_NAME_LENGTH = 60;  // 60+NUL, legacy CommonGameDefine.h L1819
inline constexpr std::size_t MAX_CHXNAME_LENGTH      = 24;  // chx file name (legacy GameResourceStruct.h L123)
inline constexpr std::size_t MAX_MONSTER_SPEECH_LEN  = 128; // chat line, legacy cMonsterSpeechManager

// Object kind constants (from CommonGameDefine.h eObjectKind)
constexpr std::uint8_t OBJECTKIND_MONSTER      = 32;
constexpr std::uint8_t OBJECTKIND_BOSS_MONSTER = 33;
constexpr std::uint8_t OBJECTKIND_SPECIAL_MONSTER = 34;
constexpr std::uint8_t OBJECTKIND_FIELD_BOSS   = 35;
constexpr std::uint8_t OBJECTKIND_FIELD_SUB    = 36;
constexpr std::uint8_t OBJECTKIND_TOGETHER_PLAY= 37;
constexpr std::uint8_t OBJECTKIND_TITAN        = 41;

// Monster group limits (from CommonGameDefine.h)
constexpr int MAX_MONSTER_GROUPNUM  = 200;
constexpr int MAX_MONSTER_REGEN_NUM = 100;

// Random spawn offset range (from original RegenManager)
constexpr float MONSTER_REGEN_RANDOM_RANGE = 1500.0f;

// AI state machine states
enum class MonsterAIState : std::uint8_t {
    Idle       = 0,
    Patrol     = 1,
    Chase      = 2,
    Attack     = 3,
    Return     = 4,
    Dead       = 5,
};

#pragma pack(push, 1)

// MONSTER_TOTALINFO (14 bytes) â€” from CommonStruct.h
struct MonsterTotalInfo {
    std::uint32_t Life;          // current HP
    std::uint32_t Shield;        // current shield/MP
    std::uint16_t MonsterKind;   // monster type index
    std::uint16_t Group;         // group number
    std::uint16_t MapNum;        // map number
};
static_assert(sizeof(MonsterTotalInfo) == 14, "MonsterTotalInfo must be 14 bytes");

// NPC_REGEN (44 bytes) â€” spawn point configuration from GameResourceStruct.h
struct NpcRegen {
    std::uint32_t dwObjectID;    // unique object ID
    std::uint16_t MapNum;        // map number
    std::uint16_t NpcKind;       // monster kind index
    char          Name[17];      // monster name
    std::uint16_t NpcIndex;      // unique spawn index
    float         PosX;          // spawn X
    float         PosY;          // spawn Y
    float         PosZ;          // spawn Z
    float         Angle;         // facing angle
};
static_assert(sizeof(NpcRegen) == 43, "NpcRegen must be 43 bytes");

// Monster template stats (simplified from BASE_MONSTER_LIST)
struct MonsterTemplate {
    std::uint16_t MonsterKind = 0;
    std::uint8_t  ObjectKind  = OBJECTKIND_MONSTER;
    char          Name[17]    = {};
    std::uint8_t  Level       = 1;
    std::uint32_t Life        = 100;
    std::uint32_t Shield      = 0;
    std::uint32_t ExpPoint    = 10;
    std::uint16_t AttackMin   = 5;
    std::uint16_t AttackMax   = 15;
    std::uint16_t Defense     = 3;
    float         WalkSpeed   = 50.0f;
    float         RunSpeed    = 100.0f;
    float         SearchRange = 500.0f;
    float         DomainRange = 1000.0f;
    bool          Aggressive  = false;
};

#pragma pack(pop)

// Runtime monster instance (server-side only, not sent over wire)
struct MonsterInstance {
    std::uint32_t object_id    = 0;      // unique runtime ID
    std::uint16_t monster_kind = 0;      // template index
    std::uint8_t  object_kind  = OBJECTKIND_MONSTER;
    char          name[17]     = {};
    std::uint8_t  level        = 1;
    std::uint16_t group        = 0;
    std::uint16_t map_num      = 0;

    // Current state
    std::uint32_t current_life = 0;
    std::uint32_t max_life     = 0;
    std::uint32_t current_shield = 0;
    std::uint32_t max_shield   = 0;

    // Position
    float pos_x = 0, pos_y = 0, pos_z = 0;
    float spawn_x = 0, spawn_y = 0, spawn_z = 0;  // original spawn point
    float angle   = 0;

    // AI state
    MonsterAIState ai_state = MonsterAIState::Idle;
    std::uint32_t  target_object_id = 0;  // current aggro target
    float          patrol_target_x = 0;
    float          patrol_target_z = 0;
    std::uint64_t  last_ai_tick_ms = 0;
    std::uint64_t  respawn_time_ms = 0;   // 0 = alive
    bool           is_dead = false;

    // Template stats (copied from MonsterTemplate at spawn)
    std::uint32_t exp_reward  = 10;
    std::uint16_t attack_min  = 5;
    std::uint16_t attack_max  = 15;
    std::uint16_t defense     = 3;
    float         walk_speed  = 50.0f;
    float         run_speed   = 100.0f;
    float         search_range= 500.0f;
    float         domain_range= 1000.0f;
    bool          aggressive  = false;
};

// Helper: compute distance squared between two 2D points (XZ plane)
inline float distance_sq_2d(float x1, float z1, float x2, float z2) {
    float dx = x2 - x1;
    float dz = z2 - z1;
    return dx * dx + dz * dz;
}

// Helper: build a MonsterTotalInfo wire payload (14 bytes)
inline MonsterTotalInfo make_monster_totalinfo(const MonsterInstance& m) {
    MonsterTotalInfo info{};
    info.Life = m.current_life;
    info.Shield = m.current_shield;
    info.MonsterKind = m.monster_kind;
    info.Group = m.group;
    info.MapNum = m.map_num;
    return info;
}

// Default monster templates for testing (Phase 10c P0)
// In the future these will be loaded from MonsterList.bin
inline std::vector<MonsterTemplate> get_default_templates() {
    std::vector<MonsterTemplate> templates;

    // Template 0: weak training dummy monster
    {
        MonsterTemplate t;
        t.MonsterKind = 0;
        t.ObjectKind  = OBJECTKIND_MONSTER;
        std::strncpy(t.Name, "Doksa", 16);
        t.Level       = 1;
        t.Life        = 80;
        t.Shield      = 0;
        t.ExpPoint    = 5;
        t.AttackMin   = 3;
        t.AttackMax   = 8;
        t.Defense     = 2;
        t.WalkSpeed   = 40.0f;
        t.RunSpeed    = 80.0f;
        t.SearchRange = 300.0f;
        t.DomainRange = 800.0f;
        t.Aggressive  = false;
        templates.push_back(t);
    }
    // Template 1: aggressive wild animal
    {
        MonsterTemplate t;
        t.MonsterKind = 1;
        t.ObjectKind  = OBJECTKIND_MONSTER;
        std::strncpy(t.Name, "Langdu", 16);
        t.Level       = 3;
        t.Life        = 150;
        t.Shield      = 10;
        t.ExpPoint    = 15;
        t.AttackMin   = 8;
        t.AttackMax   = 20;
        t.Defense     = 5;
        t.WalkSpeed   = 50.0f;
        t.RunSpeed    = 120.0f;
        t.SearchRange = 600.0f;
        t.DomainRange = 1200.0f;
        t.Aggressive  = true;
        templates.push_back(t);
    }
    // Template 2: strong elite monster
    {
        MonsterTemplate t;
        t.MonsterKind = 2;
        t.ObjectKind  = OBJECTKIND_MONSTER;
        std::strncpy(t.Name, "Heifeng", 16);
        t.Level       = 10;
        t.Life        = 500;
        t.Shield      = 50;
        t.ExpPoint    = 80;
        t.AttackMin   = 25;
        t.AttackMax   = 60;
        t.Defense     = 15;
        t.WalkSpeed   = 45.0f;
        t.RunSpeed    = 110.0f;
        t.SearchRange = 800.0f;
        t.DomainRange = 1500.0f;
        t.Aggressive  = true;
        templates.push_back(t);
    }
    return templates;
}

// Default spawn points for testing (Phase 10c P0)
// In the future these will be loaded from map regen files
inline std::vector<NpcRegen> get_default_spawn_points(std::uint16_t map_num) {
    std::vector<NpcRegen> spawns;
    // Spawn 5 monsters around the map center
    const float cx = 150.0f, cz = 150.0f;
    const float radius = 30.0f;
    for (int i = 0; i < 5; ++i) {
        NpcRegen r{};
        r.dwObjectID = 50000 + i;  // reserved range for monsters
        r.MapNum     = map_num;
        r.NpcKind    = static_cast<std::uint16_t>(i % 3);  // cycle templates
        std::snprintf(r.Name, sizeof(r.Name), "Monster%d", i);
        r.NpcIndex   = static_cast<std::uint16_t>(i);
        float ang = static_cast<float>(i) * 2.0f * 3.14159f / 5.0f;
        r.PosX = cx + radius * std::cos(ang);
        r.PosY = 0.0f;
        r.PosZ = cz + radius * std::sin(ang);
        r.Angle = ang;
        spawns.push_back(r);
    }
    return spawns;
}

}  // namespace mxh::game
