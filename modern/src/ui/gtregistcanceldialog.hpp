// gtregistcanceldialog.hpp — modern port of 墨香
// CGTRegistcancelDialog (guild tournament registration
// cancel dialog: 1 cButton).
//
// 1:1 port of legacy `CGTRegistcancelDialog` from
//   `墨香【源码】\[Client]MH\GTRegistcancelDialog.h` (795 B) and
//   `墨香【源码】\[Client]MH\GTRegistcancelDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_GTREGISTCANCEL_DLG (legacy
//     cWindow type tag).
//   - Dtor: empty body.
//   - Linking: resolve 1 cButton (m_pCancelBtn
//     by GDT_CANCELBTN id).
//   - SetActive override: call cDialog::SetActive,
//     then if val == FALSE and HERO->GetState() ==
//     eObjectState_Deal → OBJECTSTATEMGR->
//     EndObjectState(HERO, eObjectState_Deal).
//   - TournamentRegistCancelSyn: build MSGBASE,
//     set Category = MP_GTOURNAMENT, Protocol =
//     MP_GTOURNAMENT_REGISTCANCEL_SYN, dwObjectID
//     = HEROID, send via NETWORK.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type =
//     WT_GTREGISTCANCEL_DLG drop, modern cWindow
//     does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cButton child by id.
//   - SetActive override: 1:1 with legacy. The
//     HERO + OBJECTSTATEMGR dispatch is TODO
//     (R-12.x deferred). The base SetActive is
//     always called (matches legacy call order).
//   - TournamentRegistCancelSyn: TODO (2-singleton:
//     HERO + NETWORK not ported, R-12.x deferred).
//     Modern port is a no-op (no network send)
//     while singletons are unported. When ported,
//     the body becomes the legacy code.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 34th **Tier 2** dialog port (after
// cChangeJobDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — only HERO + OBJECTSTATEMGR + NETWORK
// singletons (R-12.x deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;

class cGTRegistcancelDialog : public cDialog {
public:
    cGTRegistcancelDialog();
    ~cGTRegistcancelDialog() override;

    // ----- 1:1 with legacy CGTRegistcancelDialog::Linking -----

    // 1:1 with legacy Linking. Resolve cButton
    // child (m_pCancelBtn by kIdCancelBtn) by id.
    void Linking();

    // ----- 1:1 with legacy CGTRegistcancelDialog::SetActive override -----

    // 1:1 with legacy SetActive override. Call base SetActive;
    // if val == FALSE, then if the hero-state provider reports the
    // hero is currently in eObjectState_Deal (legacy: HERO->GetState()
    // == eObjectState_Deal), the end-deal-state callback is invoked
    // (legacy: OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal)).
    // Both callbacks are OPTIONAL. A null check fn skips the
    // conditional; a null end fn leaves the deal state untouched
    // (preserves the legacy singleton-unavailable behavior). The base
    // SetActive is always called (matches legacy call order).
    void SetActive(bool val) noexcept override;

    // ----- Host-injected callbacks (legacy: HERO + OBJECTSTATEMGR singletons) -----

    // 1:1 with legacy HERO->GetState(). Returns the current hero state
    // as an ObjectState numeric value. Used by SetActive(val==FALSE) to
    // check whether the hero is in eObjectState_Deal before invoking the
    // end-deal-state callback.
    using GetHeroStateFn = std::int32_t (*)(void* userData);

    // 1:1 with legacy OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal).
    // Used by SetActive(val==FALSE) when the hero is currently in
    // eObjectState_Deal.
    using EndDealStateFn = void (*)(void* userData);

    // Wire the host callbacks + shared userData pointer used by all calls.
    // A null fn preserves the safe no-op behavior so the dialog can be
    // exercised in tests without wiring a full HERO + OBJECTSTATEMGR host.
    void SetCallbacks(GetHeroStateFn getHeroState,
                      EndDealStateFn endDealState,
                      void* userData = nullptr) noexcept;

    // ----- 1:1 with legacy eObjectState_Deal -----

    // 1:1 with legacy eObjectState_Deal from [Server]CommonServerDefine.h.
    // The host GetHeroStateFn returns this value when the hero is in a deal state.
    static constexpr std::int32_t kObjectStateDeal = 6;

    // ----- 1:1 with legacy CGTRegistcancelDialog::TournamentRegistCancelSyn -----

    // 1:1 with legacy TournamentRegistCancelSyn.
    // The whole method is TODO (2-singleton: HERO
    // + NETWORK not ported, R-12.x deferred).
    // Modern port is a no-op (does not call
    // NETWORK->Send) while singletons are unported.
    // When ported, the body becomes the legacy code.
    void TournamentRegistCancelSyn();

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID
    // (GDT_CANCELBTN). Local 460 — distinct
    // from 200-450 used by previous Tier 2
    // dialogs.
    static constexpr std::int32_t kIdCancelBtn = 460;

private:
    // 1:1 with legacy m_pCancelBtn (resolved in
    // Linking by GDT_CANCELBTN id).
    cButton* m_pCancelBtn = nullptr;

    // Host-injected callbacks (replaces HERO + OBJECTSTATEMGR singletons).
    // A null pointer preserves the safe no-op behavior so the dialog can
    // be exercised in tests without wiring the full host singletons.
    GetHeroStateFn m_getHeroStateFn  = nullptr;
    EndDealStateFn m_endDealStateFn  = nullptr;
    void*          m_callbackUserData = nullptr;
};

}  // namespace mxh::ui
