// player_state.cpp - implementation for PlayerState + helpers.

#include "mxh/server/player_state.hpp"
#include <algorithm>

namespace mxh::server {

CalcBaseStats PlayerState::base_stats() const noexcept {
    CalcBaseStats b;
    b.level   = progress.level;
    b.gengol  = attributes.gengol;
    b.simmek  = attributes.simmek;
    b.minchub = attributes.minchub;
    b.cheryuk = attributes.cheryuk;
    return b;
}

void PlayerState::recompute_max_stats() noexcept {
    auto b = base_stats();
    vitals.max_hp     = compute_max_life(b, bonuses);
    vitals.max_shield = compute_max_shield(b, bonuses);
    vitals.max_mp     = compute_max_naeryuk(b, bonuses);
}

PlayerState make_player_state(std::uint32_t player_id,
                              std::uint32_t user_id,
                              std::uint16_t level,
                              const CalcBaseStats& base,
                              const CalcEquipBonuses& bonuses) {
    PlayerState s;
    s.player_id = player_id;
    s.user_id   = user_id;
    s.progress.level = level;
    s.attributes.gengol  = base.gengol;
    s.attributes.simmek  = base.simmek;
    s.attributes.minchub = base.minchub;
    s.attributes.cheryuk = base.cheryuk;
    s.bonuses = bonuses;
    s.recompute_max_stats();
    // Legacy: spawn with full vitals
    s.vitals.current_hp     = s.vitals.max_hp;
    s.vitals.current_shield = s.vitals.max_shield;
    s.vitals.current_mp     = s.vitals.max_mp;
    return s;
}

std::int32_t apply_hp_delta(PlayerVitals& v, std::int32_t delta) noexcept {
    std::int64_t cur = static_cast<std::int64_t>(v.current_hp) + delta;
    if (cur < 0) { delta = -static_cast<std::int32_t>(v.current_hp); cur = 0; }
    if (cur > static_cast<std::int64_t>(v.max_hp)) {
        delta = static_cast<std::int32_t>(v.max_hp - v.current_hp);
        cur = v.max_hp;
    }
    v.current_hp = static_cast<std::uint32_t>(cur);
    return delta;
}

std::int32_t apply_shield_delta(PlayerVitals& v, std::int32_t delta) noexcept {
    std::int64_t cur = static_cast<std::int64_t>(v.current_shield) + delta;
    if (cur < 0) { delta = -static_cast<std::int32_t>(v.current_shield); cur = 0; }
    if (cur > static_cast<std::int64_t>(v.max_shield)) {
        delta = static_cast<std::int32_t>(v.max_shield - v.current_shield);
        cur = v.max_shield;
    }
    v.current_shield = static_cast<std::uint32_t>(cur);
    return delta;
}

std::int32_t apply_mp_delta(PlayerVitals& v, std::int32_t delta) noexcept {
    std::int64_t cur = static_cast<std::int64_t>(v.current_mp) + delta;
    if (cur < 0) { delta = -static_cast<std::int32_t>(v.current_mp); cur = 0; }
    if (cur > static_cast<std::int64_t>(v.max_mp)) {
        delta = static_cast<std::int32_t>(v.max_mp - v.current_mp);
        cur = v.max_mp;
    }
    v.current_mp = static_cast<std::uint32_t>(cur);
    return delta;
}

bool add_learned_skill(SkillBook& book, std::uint32_t mugong_idx, std::uint8_t level) noexcept {
    if (find_learned_skill(book, mugong_idx).has_value()) return false;
    if (book.count >= kLearnedSkillMax) return false;
    LearnedSkill s;
    s.mugong_idx = mugong_idx;
    s.level = level;
    book.skills[book.count++] = s;
    return true;
}

std::optional<LearnedSkill> find_learned_skill(const SkillBook& book, std::uint32_t mugong_idx) noexcept {
    for (std::uint32_t i = 0; i < book.count; ++i) {
        if (book.skills[i].mugong_idx == mugong_idx) return book.skills[i];
    }
    return std::nullopt;
}

bool remove_learned_skill(SkillBook& book, std::uint32_t mugong_idx) noexcept {
    for (std::uint32_t i = 0; i < book.count; ++i) {
        if (book.skills[i].mugong_idx == mugong_idx) {
            // shift remaining
            for (std::uint32_t j = i; j + 1 < book.count; ++j) {
                book.skills[j] = book.skills[j + 1];
            }
            book.skills[book.count - 1] = LearnedSkill{};
            --book.count;
            return true;
        }
    }
    return false;
}

static bool item_slot_empty(const mxh::game::ItemBase& item) noexcept {
    return item.dwDBIdx == 0;
}

std::optional<std::uint16_t> find_inventory_slot(const InventorySlots& inv, std::uint16_t wIconIdx) noexcept {
    for (std::uint16_t i = 0; i < inv.items.size(); ++i) {
        if (!item_slot_empty(inv.items[i]) && inv.items[i].wIconIdx == wIconIdx) {
            return i;
        }
    }
    return std::nullopt;
}

std::uint16_t inventory_occupied_count(const InventorySlots& inv) noexcept {
    std::uint16_t n = 0;
    for (std::uint16_t i = 0; i < inv.items.size(); ++i) {
        if (!item_slot_empty(inv.items[i])) ++n;
    }
    return n;
}

bool is_inventory_slot_empty(const InventorySlots& inv, std::uint16_t pos) noexcept {
    if (pos >= inv.items.size()) return true;
    return item_slot_empty(inv.items[pos]);
}

// ---- Quick slot helpers ----
// Find a skill_idx binding across all sheets. Returns pair
// (sheet_idx, item_idx) encoded as a single uint8: high nibble = sheet,
// low nibble = item. Caller compares against the encoded value.
std::optional<std::uint8_t> find_quick_slot_binding(const QuickBar& bar, std::uint32_t skill_idx) noexcept {
    for (std::uint16_t s = 0; s < bar.sheets.size(); ++s) {
        for (std::uint16_t i = 0; i < bar.sheets[s].size(); ++i) {
            if (bar.sheets[s][i].skill_idx == skill_idx && skill_idx != 0) {
                return static_cast<std::uint8_t>((s << 4) | (i & 0x0f));
            }
        }
    }
    return std::nullopt;
}

// ---- Membership helpers ----
bool is_in_guild(const GuildMembership& g) noexcept { return g.guild_id != 0; }
bool is_in_party(const PartyMembership& p) noexcept { return p.party_id != 0; }

}  // namespace mxh::server
