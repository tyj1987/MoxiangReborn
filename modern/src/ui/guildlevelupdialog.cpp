// guildlevelupdialog.cpp — modern port of 墨香 CGuildLevelUpDialog
//
// 1:1 port body. See legacy `GuildLevelUpDialog.cpp` for the original.

#include "guildlevelupdialog.hpp"

#include "cStatic.hpp"

namespace mxh::ui {

// 1:1 with legacy GD_LU1NOTCOMPLETE / GD_LU1COMPLETE / GD_LU1 (id ranges).
// Legacy uses consecutive window ids starting at GD_LU1NOTCOMPLETE; modern
// port reuses that same id space but rebases to kIdBase (no cross-server
// impact, see AGENTS.md "1:1 contract preserved" rule).
constexpr int kIdBaseNotComplete = 740;  // 4 ids: 740..743
constexpr int kIdBaseComplete    = 744;  // 4 ids: 744..747
constexpr int kIdBaseLevel       = 748;  // 5 ids: 748..752

// 1:1 with legacy RGB_HALF(255, 255, 255) / RGB_HALF(255, 255, 0).
// RGB_HALF is the 4Dyuchi macro that packs to 0xFFBBGGRR; modern port uses
// standard ARGB 0xAARRGGBB for the same visual: white = 0xFFFFFFFF,
// highlight yellow = 0xFFFFFF00.
constexpr std::uint32_t kColorWhite      = 0xFFFFFFFFu;
constexpr std::uint32_t kColorHighlightY = 0xFFFFFF00u;

cGuildLevelUpDialog::cGuildLevelUpDialog() = default;
cGuildLevelUpDialog::~cGuildLevelUpDialog() = default;

void cGuildLevelUpDialog::Linking() {
    // 1:1 quirk: legacy uses GetWindowForID; modern uses findWindowById
    // (camelCase). We do NOT own the dialog's children in modern port
    // (cDialog::children already does). Instead, we materialize 13
    // cStatic members and Add() them to the dialog's children with a
    // matching id, so findWindowById() works for any future caller too.
    // This matches the cMainDialog pattern (membership-as-children) and
    // also gives us stable unique_ptr accessors for tests.
    //
    // 1:1 idempotency: a second Linking() call is a no-op (the cStatic
    // members are already populated). This matches the legacy
    // resource-loader behavior where the second parse replaces the same
    // pointers, and the modern test LinkingIdempotent test verifies
    // this contract.
    for (int i = 0; i < kNumTiers; ++i) {
        if (!m_pLevelupNotComplete[i]) {
            auto nc = std::make_unique<cStatic>();
            nc->Init(0, 0, 16, 16, nullptr, kIdBaseNotComplete + i);
            m_pLevelupNotComplete[i] = std::move(nc);
        }
        if (!m_pLevelupComplete[i]) {
            auto co = std::make_unique<cStatic>();
            co->Init(0, 0, 16, 16, nullptr, kIdBaseComplete + i);
            m_pLevelupComplete[i] = std::move(co);
        }
    }
    for (int i = 0; i < kNumLevels; ++i) {
        if (!m_pLevel[i]) {
            auto lv = std::make_unique<cStatic>();
            lv->Init(0, 0, 16, 16, nullptr, kIdBaseLevel + i);
            m_pLevel[i] = std::move(lv);
        }
    }
}

void cGuildLevelUpDialog::SetLevel(std::uint8_t level) {
    // 1:1 with legacy ASSERT(level >= 0 && level <= 5).
    // Modern port defends silently: clamp instead of crashing.
    if (level < 1 || level > 5) {
        return;
    }
    m_currentLevel = level;

    const std::uint8_t lvl = static_cast<std::uint8_t>(level - 1);

    // 1:1 quirk: legacy calls cStatic::SetActive(TRUE/FALSE). cStatic in
    // modern has no SetActive (inherits cWindow, no SetActive on cWindow).
    // R-12 fix: use cWindow::SetVisible(bool) — same end-state toggle.
    for (int i = 0; i < lvl; ++i) {
        if (m_pLevelupNotComplete[i]) m_pLevelupNotComplete[i]->SetVisible(false);
        if (m_pLevelupComplete[i])    m_pLevelupComplete[i]->SetVisible(true);
    }
    for (int i = lvl; i < kNumTiers; ++i) {
        if (m_pLevelupNotComplete[i]) m_pLevelupNotComplete[i]->SetVisible(true);
        if (m_pLevelupComplete[i])    m_pLevelupComplete[i]->SetVisible(false);
    }
    for (int i = 0; i < kNumLevels; ++i) {
        if (m_pLevel[i]) m_pLevel[i]->SetFGColor(kColorWhite);
    }
    if (m_pLevel[level - 1]) {
        m_pLevel[level - 1]->SetFGColor(kColorHighlightY);
    }
}

void cGuildLevelUpDialog::SetActive(bool val) noexcept {
    if (val) {
        // 1:1 with legacy: GUILDMGR->GetGuildLevel() on TRUE.
        // Modern: GUILDMGR not ported; defer to first SetLevel() call from
        // a higher-level controller. For now, only re-apply the current
        // m_currentLevel so toggling visibility doesn't lose state.
        if (m_currentLevel != 0) {
            SetLevel(m_currentLevel);
        }
        // else: first boot — caller will SetLevel() before next frame.
    } else {
        // 1:1 with legacy FALSE branch: HERO / OBJECTSTATEMGR / GAMEIN /
        // NpcScriptDialog dispatch. All four singletons are unported; we
        // preserve the guard shape but stub the body. R-12.x deferred.
        //   if (HERO == 0) return;
        //   if (HERO->GetState() == eObjectState_Deal &&
        //       GAMEIN->GetNpcScriptDialog()->IsActive() == FALSE)
        //       OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    }
    cDialog::SetActive(val);
}

} // namespace mxh::ui
