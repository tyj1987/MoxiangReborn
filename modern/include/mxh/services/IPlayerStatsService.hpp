// mxh/services/IPlayerStatsService.hpp
// Phase 13 service interface for player stats access.
//
// Tier 3 dialog (CharacterDialog, StatsCalcManager consumers,
// StatusIconDlg, MPGuageDialog, ChaseDialog, etc.) currently
// reads player stats via legacy OBJECTMGR->GetHero() chains
// or GAMEIN->GetCharacterDialog() back-pointers. This service
// is the modern replacement: dialog code takes an
// `IPlayerStatsService*` and queries the player's current
// state through it.
//
// The interface is deliberately minimal — only the read paths
// the Tier 2/3 dialogs need. Write paths (AddExp, LevelUp,
// TakeDamage) belong to the network/AI layer; dialogs should
// never mutate stats directly.
//
// Usage pattern (from a future CharacterDialog::Refresh):
//   void CharacterDialog::refresh() {
//     const auto* stats = m_playerStatsService;  // injected
//     if (!stats) return;
//     m_strStatic->SetStaticText(std::to_string(stats->getStr()));
//     m_agiStatic->SetStaticText(std::to_string(stats->getAgi()));
//     m_hpGuagen->SetValue(static_cast<float>(stats->getCurrentHp())
//                          / static_cast<float>(stats->getMaxHp()));
//   }

#pragma once

#include <cstdint>

namespace mxh::services {

class IPlayerStatsService {
public:
    virtual ~IPlayerStatsService() = default;

    // ----- Core attributes -----

    virtual std::uint16_t getStr() const noexcept = 0;
    virtual std::uint16_t getAgi() const noexcept = 0;
    virtual std::uint16_t getInt() const noexcept = 0;
    virtual std::uint16_t getWis() const noexcept = 0;
    virtual std::uint16_t getDex() const noexcept = 0;

    // ----- Level / experience -----

    virtual std::uint16_t getLevel() const noexcept = 0;

    // Current exp within the level. Cumulative exp across all
    // levels is `getLevelExp() + level_to_exp(getLevel())`
    // (the service may compute the level-to-exp table
    // internally).
    virtual std::uint32_t getLevelExp() const noexcept = 0;

    // Exp needed to advance from the current level to the
    // next. For max level this returns 0.
    virtual std::uint32_t getExpForNextLevel() const noexcept = 0;

    // ----- Health / mana -----

    virtual std::uint32_t getCurrentHp() const noexcept = 0;
    virtual std::uint32_t getMaxHp() const noexcept = 0;
    virtual std::uint32_t getCurrentMp() const noexcept = 0;
    virtual std::uint32_t getMaxMp() const noexcept = 0;

    // ----- Derived (convenience) -----

    // Hp as a 0..1 fraction for cGuagen::SetValue. The
    // service may compute it on the fly; if `getMaxHp()` is
    // 0 the result is 0 (avoids div-by-zero in the dialog).
    virtual float getHpFraction() const noexcept = 0;
    virtual float getMpFraction() const noexcept = 0;
};

}  // namespace mxh::services
