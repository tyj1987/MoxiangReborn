// wantregistdialog.hpp — modern port of 墨香
// CWantRegistDialog (wanted registration editor dialog:
// 1 cStatic + 1 cEditBox).
//
// 1:1 port of legacy `CWantRegistDialog` from
//   `墨香【源码】\[Client]MH\WantRegistDialog.h` (930 B) and
//   `墨香【源码】\[Client]MH\WantRegistDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_WANTREGISTDIALOG (legacy
//     cWindow type tag).
//   - Dtor: empty body.
//   - Linking: resolve 1 cStatic (m_WantedName by
//     WANTREG_WANTEDNAME) + 1 cEditBox
//     (m_PrizeEdit by WANTREG_PRIZEEDIT), call
//     SetValidCheck(VCM_NUMBER) on the cEditBox;
//     init state (m_bShow = FALSE, m_dwStartShowTime
//     = 0).
//   - SetWantedName(char* pName): call
//     m_WantedName->SetStaticText(pName); call
//     m_PrizeEdit->SetEditText("").
//   - SetActive override: if val == TRUE →
//     m_dwStartShowTime = gCurTime; else →
//     m_PrizeEdit->SetFocusEdit(FALSE) + send
//     MSGBASE MP_WANTED/MP_WANTED_REGIST_CANCEL
//     via NETWORK; m_bShow = FALSE; call base
//     cDialog::SetActive(val).
//   - ActionEvent: if !m_bShow && gCurTime -
//     m_dwStartShowTime >= 3000 → m_bShow = TRUE;
//     else return WE_NULL; then call base
//     cDialog::ActionEvent(mouseInfo).
//   - Render: if m_bShow, call cDialog::Render().
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type = WT_WANTREGISTDIALOG
//     drop, modern cWindow does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve 2 children by id,
//     call SetValidCheck(kVcmNumber = 1) on
//     cEditBox.
//   - SetWantedName: REAL with std::string (1:1
//     with legacy char* c-string). The modern
//     port uses null guard for pName (legacy
//     would crash on null).
//   - SetActive override: TODO (gCurTime + HERO
//     + NETWORK not ported, R-12.x deferred).
//     Modern port calls base SetActive + the
//     val == FALSE SetFocusEdit(false) call.
//     The MSGBASE NETWORK send is TODO.
//   - ActionEvent: TODO (CMouse + gCurTime not
//     ported, R-12.x deferred). Modern port
//     returns WE_NULL.
//   - Render: TODO (Phase 12.x minimal port; the
//     m_bShow gating is documented but not
//     implemented — the modern port always
//     renders if active, matching the legacy
//     behavior once m_bShow is set). The m_bShow
//     + m_dwStartShowTime state fields are
//     documented in the modern port for future
//     implementation.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 38th **Tier 2** dialog port (after
// cWantedDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — only gCurTime + CMouse + HERO +
// NETWORK singletons (R-12.x deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;
class cEditBox;

// Shared clock provider signature (replaces legacy gCurTime global).
using WgClockFn = std::uint32_t (*)(void* userData);

class cWantRegistDialog : public cDialog {
public:
    cWantRegistDialog();
    ~cWantRegistDialog() override;

    // ----- 1:1 with legacy CWantRegistDialog::Linking -----

    // 1:1 with legacy Linking. Resolve 1 cStatic
    // (m_WantedName by kIdWantedName) + 1 cEditBox
    // (m_PrizeEdit by kIdPrizeEdit), call
    // SetValidCheck(kVcmNumber = 1) on the cEditBox.
    void Linking();

    // ----- 1:1 with legacy CWantRegistDialog::SetWantedName -----

    // 1:1 with legacy SetWantedName(char* pName).
    // The modern port uses std::string for the
    // pName argument (1:1 with legacy char*
    // c-string, since the SetStaticText + SetEditText
    // calls are just simple setters). 1:1 quirk:
    // modern port guards null pName (legacy would
    // crash on null).
    void SetWantedName(const char* pName);

    // ----- 1:1 with legacy CWantRegistDialog::SetActive override -----

    // 1:1 with legacy SetActive override.
    // val==TRUE: stamp m_dwStartShowTime via
    // OPTIONAL host clock provider (legacy gCurTime).
    // val==FALSE: SetFocusEdit(false) is REAL;
    // the MSGBASE NETWORK send is TODO
    // (HERO + NETWORK singletons).
    void SetActive(bool val) noexcept override;

    // Replace the legacy gCurTime read for SetActive +
    // ActionEvent. A null provider preserves the
    // safe zero-clock fallback.
    void SetCurrentTimeProvider(WgClockFn getCurrentTime,
                                void* userData = nullptr) noexcept;

    // ----- 1:1 with legacy CWantRegistDialog::ActionEvent -----

    // 1:1 with legacy ActionEvent. The whole
    // method is TODO (CMouse + gCurTime not
    // ported, R-12.x deferred). Modern port returns
    // WE_NULL (matching the legacy "early return"
    // path when m_bShow is false).
    std::uint32_t ActionEvent();

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID
    // (WANTREG_WANTEDNAME / WANTREG_PRIZEEDIT).
    // Local 510-511 — distinct from 200-500 used
    // by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdWantedName = 510;
    static constexpr std::int32_t kIdPrizeEdit  = 511;

    // VCM_NUMBER = 1 (1:1 with legacy cEditBox
    // valid-check enum: digits-only valid check).
    static constexpr int kVcmNumber = 1;

    // 1:1 with legacy m_dwStartShowTime getter (test-only).
    std::uint32_t GetStartShowTime() const noexcept { return m_dwStartShowTime; }

    // 1:1 with legacy m_bShow getter (test-only).
    bool IsShow() const noexcept { return m_bShow; }

private:
    // 1:1 with legacy m_WantedName (resolved in
    // Linking by WANTREG_WANTEDNAME id).
    cStatic* m_WantedName = nullptr;

    // 1:1 with legacy m_PrizeEdit (resolved in
    // Linking by WANTREG_PRIZEEDIT id).
    cEditBox* m_PrizeEdit = nullptr;

    // 1:1 with legacy m_bShow gating state.
    bool m_bShow = false;

    // 1:1 with legacy m_dwStartShowTime.
    std::uint32_t m_dwStartShowTime = 0;

    WgClockFn m_getCurrentTimeFn = nullptr;
    void*     m_clockUserData    = nullptr;
};

}  // namespace mxh::ui
