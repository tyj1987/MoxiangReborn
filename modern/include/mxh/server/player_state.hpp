// player_state.hpp - Server-side player state container with full
// character progression data, derived from the legacy [Server]Map/Player.h
// (CPlayer) and the character_calc_manager pure-function port.
//
// 1:1 port: all fields match the legacy struct member names (CamelCase)
// so that the test suite can compare struct offsets byte-for-byte when
// needed. This is the in-memory representation only; the wire-format
// struct (CHARACTER_TOTALINFO etc.) lives in mxh::game for protocol use.
//
// Used by MapHandler as the per-player state object (PlayerInfo is
// embedded in map_handler.cpp today; player_state replaces it as a
// first-class type).

#pragma once

#include "mxh/game/skill_types.hpp"
#include "mxh/game/item_types.hpp"
#include "mxh/game/monster_types.hpp"
#include "mxh/server/character_calc_manager.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mxh::server {

inline constexpr std::uint16_t kQuickSheetCount        = 10;  // MAX_QUICKSHEET_NUM
inline constexpr std::uint16_t kQuickItemPerSheetCount = 8;   // MAX_QUICKITEMPERSHEET_NUM
inline constexpr std::uint16_t kLearnedSkillMax       = 100; // legacy mugong slot count

// ---- Base attributes (from CharacterCalcManager, computed) ----
struct PlayerAttributes final {
    std::uint16_t gengol  = 0;   // GenGol
    std::uint16_t simmek  = 0;   // SimMek
    std::uint16_t minchub = 0;   // MinChub
    std::uint16_t cheryuk = 0;   // CheRyuk
};

// ---- Quick-slot bar entry (legacy QUICKITEM) ----
struct QuickSlot final {
    std::uint32_t skill_idx  = 0;  // 0 = empty slot
    std::uint8_t  slot_pos   = 0;
    std::uint8_t  reserved0  = 0;
    std::uint16_t reserved1  = 0;
};

// ---- Learned-skill entry (legacy MUGONGBASE) ----
struct LearnedSkill final {
    std::uint32_t mugong_idx  = 0;
    std::uint8_t  level       = 0;  // 0..12
    std::uint8_t  experience  = 0;  // 0..100
    std::uint16_t reserved    = 0;
};

// ---- Player progression (legacy BaseObjectInfo + HeroInfo subset) ----
struct PlayerProgress final {
    std::uint32_t player_id  = 0;
    std::uint16_t level      = 1;
    std::uint32_t level_exp  = 0;       // current exp within level
    std::uint32_t total_exp  = 0;       // cumulative exp
    std::uint32_t money      = 0;
    std::uint32_t max_exp    = 0;       // exp needed to next level
};

// ---- Vitals (HP / Shield / NaeRyuk = å†…åŠ›) ----
struct PlayerVitals final {
    std::uint32_t current_hp     = 0;
    std::uint32_t max_hp         = 1;
    std::uint32_t current_shield = 0;
    std::uint32_t max_shield     = 1;
    std::uint32_t current_mp     = 0;
    std::uint32_t max_mp         = 1;

    // Recovery timing state (legacy RECOVER_TIME / YYRECOVER_TIME)
    std::uint32_t last_life_check_ms    = 0;
    std::uint32_t last_shield_check_ms  = 0;
    std::uint32_t last_mp_check_ms      = 0;
    std::uint32_t last_ungi_check_ms    = 0;

    // Per-state recovery snapshots (StartUpdateLife stores
    // recoverUnitAmout + recoverDelayTime; we keep the modern
    // equivalent as recoverable deltas)
    std::uint32_t pending_life_unit    = 0;
    std::uint8_t  pending_life_count   = 0;
    std::uint32_t pending_life_delay   = 0;
    bool          life_recover_active  = false;

    std::uint32_t pending_shield_unit  = 0;
    std::uint8_t  pending_shield_count = 0;
    std::uint32_t pending_shield_delay = 0;
    bool          shield_recover_active = false;

    std::uint32_t pending_mp_unit      = 0;
    std::uint8_t  pending_mp_count     = 0;
    std::uint32_t pending_mp_delay     = 0;
    bool          mp_recover_active    = false;
};

// ---- Guild membership (legacy GuildIdx + GuildMemberRank) ----
struct GuildMembership final {
    std::uint32_t guild_id    = 0;   // 0 = not in guild
    std::uint8_t  member_rank = 0;
    std::uint8_t  reserved0   = 0;
    std::uint16_t reserved1   = 0;
};

// ---- Party membership (legacy PartyIdx + PartyMemberIdx) ----
struct PartyMembership final {
    std::uint32_t party_id    = 0;   // 0 = not in party
    std::uint8_t  member_idx  = 0;
    std::uint8_t  reserved0   = 0;
    std::uint16_t reserved1   = 0;
};

// ---- Inventory slot (legacy ITEMBASE, mirrors mxh::game::ItemBase) ----
// Inventory is the 80-slot grid the player carries around.
struct InventorySlots final {
    std::array<mxh::game::ItemBase, 80> items{};  // SLOT_INVENTORY_NUM = 80
};

// ---- Pyoguk (warehouse) slot ----
struct PyogukSlots final {
    std::array<mxh::game::ItemBase, 80> items{};
};

// ---- Equipment slot (legacy WearSlot = WEARED_ITEM_MAX = 10) ----
struct EquipSlots final {
    std::array<mxh::game::ItemBase, 10> items{};
};

// ---- Learned mugong (skill) list ----
struct SkillBook final {
    std::array<LearnedSkill, kLearnedSkillMax> skills{};
    std::uint32_t count = 0;
};

// ---- Quick-slot bar (10 sheets x 8 items) ----
struct QuickBar final {
    std::array<std::array<QuickSlot, kQuickItemPerSheetCount>, kQuickSheetCount> sheets{{}};
};

// ---- Comprehensive player state ----
// 1:1 port of the legacy CPlayer runtime state. Holds progression, vitals,
// attributes, learned skills, quick-slot bar, party/guild membership,
// inventory/equipment/warehouse and the bonus table (CalcEquipBonuses) for
// compute_max_* derivation. Used by MapHandler as the single source of truth.
struct PlayerState final {
    // Identity
    std::uint32_t player_id  = 0;
    std::uint32_t user_id    = 0;     // MHAccount index
    std::string   name;              // CHARACTER_TOTALINFO.CharacterName
    std::uint8_t  gender    = 0;
    std::uint8_t  face_type = 0;
    std::uint8_t  hair_type = 0;
    std::uint16_t map_num   = 0;
    float pos_x = 0.0f;
    float pos_z = 0.0f;

    // Character state
    PlayerProgress  progress;
    PlayerVitals    vitals;
    PlayerAttributes attributes;
    GuildMembership guild;
    PartyMembership party;

    // Equipment / inventory / warehouse
    EquipSlots    equipment;
    InventorySlots inventory;
    PyogukSlots   pyoguk;

    // Skill / quick-slot UI
    SkillBook     skills;
    QuickBar      quick;

    // Equipment-derived bonus table (legacy GetItemStats / GetSetItemStats /
    // GetAbilityStats / GetShopItemStats / GetAvatarOption /
    // GetSkillStatsOption / GetUniqueItemStats). Callers populate these
    // before invoking compute_max_* / tick_* helpers.
    CalcEquipBonuses bonuses;

    // Mussang-mode metadata (legacy IsMussangMode / GetStage). Set by the
    // battle handler when entering mussang mode; passed to recovery ticks.
    bool          mussang_mode = false;
    MussangStage  mussang_stage = MussangStage::Normal;

    // ---- Derived stats (call after mutating attributes / bonuses) ----

    // Recompute max_hp / max_shield / max_mp from attributes + bonuses.
    // Caller must hold players_mu_ (or run on the single-player thread).
    void recompute_max_stats() noexcept;

    // Convenience: build the CalcBaseStats snapshot from attributes.
    CalcBaseStats base_stats() const noexcept;
};

// ---- PlayerState factory helpers ----
PlayerState make_player_state(std::uint32_t player_id,
                              std::uint32_t user_id,
                              std::uint16_t level,
                              const CalcBaseStats& base,
                              const CalcEquipBonuses& bonuses);

// ---- Vitals delta helpers (HP / Shield / MP add / sub with clamping) ----
// Apply a damage delta (negative = damage, positive = heal).
// Returns the actual delta applied after clamping to [0, max].
std::int32_t apply_hp_delta(PlayerVitals& v, std::int32_t delta) noexcept;
std::int32_t apply_shield_delta(PlayerVitals& v, std::int32_t delta) noexcept;
std::int32_t apply_mp_delta(PlayerVitals& v, std::int32_t delta) noexcept;

// ---- Skill book helpers ----
bool add_learned_skill(SkillBook& book, std::uint32_t mugong_idx, std::uint8_t level) noexcept;
std::optional<LearnedSkill> find_learned_skill(const SkillBook& book, std::uint32_t mugong_idx) noexcept;
bool remove_learned_skill(SkillBook& book, std::uint32_t mugong_idx) noexcept;

// ---- Inventory slot helpers ----
std::optional<std::uint16_t> find_inventory_slot(const InventorySlots& inv,
                                                std::uint16_t wIconIdx) noexcept;
std::uint16_t inventory_occupied_count(const InventorySlots& inv) noexcept;
bool is_inventory_slot_empty(const InventorySlots& inv, std::uint16_t pos) noexcept;

// ---- Quick-slot helpers ----
std::optional<std::uint8_t> find_quick_slot_binding(const QuickBar& bar,
                                                    std::uint32_t skill_idx) noexcept;

// ---- Membership helpers ----
bool is_in_guild(const GuildMembership& g) noexcept;
bool is_in_party(const PartyMembership& p) noexcept;

}  // namespace mxh::server
