// mxh/src/services/PlayerStatsServiceImpl.hpp
// Phase 13.2: Real IPlayerStatsService implementation backed by
// mxh::game::PlayerCombatStats (level/HP/MP) plus the player_id
// (for level lookups in the future) and the equipped-item derived
// stat bonuses (deferred).
//
// Architecture (Phase 13 service model):
//   See InventoryServiceImpl.hpp for the service-binding pattern.
//   The dialog holds a reference to the service; the MapHandler
//   owns the PlayerCombatStats via PlayerInfo.
//
// This implementation currently exposes the *base* stats only
// (level, HP, MP). The five core attributes (str/agi/int/wis/dex)
// are derived from the character's equipped items + learned
// skills + level-up allocations. The legacy engine computes
// these via StatsCalcManager (see 墨香【源码】\[CC]Ability/) which
// has not been ported to modern/ yet — when it is, the dialog
// will pass it to the constructor alongside the base stats.
// For now, the five core attributes return 0 (the default for
// a freshly-spawned character with no items equipped).
//
// Threading: read-only from the dialog; mutations happen under
// PlayerInfo's player_mu_ on the server side.

#pragma once

#include "mxh/services/IPlayerStatsService.hpp"

#include "mxh/game/skill_types.hpp"

namespace mxh::services {

class PlayerStatsServiceImpl final : public IPlayerStatsService {
public:
    // Bind the service to a specific player's combat stats. The
    // reference must remain valid for the lifetime of the service.
    explicit PlayerStatsServiceImpl(const mxh::game::PlayerCombatStats& combat) noexcept
        : m_combat(combat) {}

    // ----- Core attributes -----
    //
    // 1:1 quirk: the legacy engine does not persist the five
    // core attributes directly — they're computed on-the-fly
    // from `equipped items + skill bonuses + level-up points`.
    // The modern port doesn't have the equivalent StatsCalcManager
    // yet, so these return 0 for the "no items, no points"
    // baseline. When StatsCalcManager lands, the constructor
    // will gain a second argument and these getters will read
    // from the calculated values.

    std::uint16_t getStr() const noexcept override { return 0; }
    std::uint16_t getAgi() const noexcept override { return 0; }
    std::uint16_t getInt() const noexcept override { return 0; }
    std::uint16_t getWis() const noexcept override { return 0; }
    std::uint16_t getDex() const noexcept override { return 0; }

    // ----- Level / experience -----
    //
    // The legacy engine splits level/exp into a separate
    // "character progress" struct (BaseObject.cpp, ExpPoint
    // field). PlayerCombatStats only carries `level`; the
    // service treats `level_exp` as 0 and `exp_for_next_level`
    // as a hardcoded estimate (100 per level, matching the
    // legacy's early-game curve). When the character progress
    // struct is ported, the constructor will gain a second
    // argument and these getters will read from it.

    std::uint16_t getLevel() const noexcept override { return m_combat.level; }
    std::uint32_t getLevelExp() const noexcept override { return 0; }

    std::uint32_t getExpForNextLevel() const noexcept override {
        // 1:1 quirk (legacy): the level-to-exp table is read
        // from `character_exp.bin` at server startup. For the
        // modern port without the binary loader, we use a
        // hardcoded 100-per-level baseline. Update when
        // character_exp.bin is ported.
        return 100u * static_cast<std::uint32_t>(m_combat.level);
    }

    // ----- Health / mana -----

    std::uint32_t getCurrentHp() const noexcept override { return m_combat.current_hp; }
    std::uint32_t getMaxHp()     const noexcept override { return m_combat.max_hp; }
    std::uint32_t getCurrentMp() const noexcept override { return m_combat.current_mp; }
    std::uint32_t getMaxMp()     const noexcept override { return m_combat.max_mp; }

    // ----- Derived (convenience) -----

    float getHpFraction() const noexcept override {
        return m_combat.max_hp == 0 ? 0.0f
            : static_cast<float>(m_combat.current_hp) /
              static_cast<float>(m_combat.max_hp);
    }
    float getMpFraction() const noexcept override {
        return m_combat.max_mp == 0 ? 0.0f
            : static_cast<float>(m_combat.current_mp) /
              static_cast<float>(m_combat.max_mp);
    }

private:
    const mxh::game::PlayerCombatStats& m_combat;
};

}  // namespace mxh::services
