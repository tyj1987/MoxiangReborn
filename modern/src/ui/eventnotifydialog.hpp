// eventnotifydialog.hpp — modern port of 墨香 CEventNotifyDialog
// (GM event notification dialog: title + context text area).
//
// 1:1 port of legacy `CEventNotifyDialog` from
//   `墨香【源码】\[Client]MH\EventNotifyDialog.h` (793 B) and
//   `墨香【源码】\[Client]MH\EventNotifyDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_pStcTitle = m_pTAContext = NULL.
//   - Linking: resolve 2 children (cStatic m_pStcTitle +
//     cTextArea m_pTAContext). The SetToolTip call is
//     commented out (1:1 quirk: legacy's tool tip
//     requires SCRIPTMGR singleton which is not ported).
//   - SetActive override: calls base SetActive, then if
//     !val clears m_pTAContext's script text. The
//     NOTIFYMGR->SetNotifyActive call is commented out.
//   - ActionEvent override: calls base ActionEvent. The
//     click-to-close logic is commented out.
//   - SetTitle / SetContext: 1:1 wrappers that set the
//     static text + script text on the linked children.
//
// The modern port covers everything: ctor + Linking +
// SetActive override + ActionEvent override + SetTitle +
// SetContext are all REAL (or near-REAL — the ctor is
// no-op, the Linking SetToolTip / NOTIFYMGR calls are
// documented as TODO). The dialog is the second Tier 2
// port that uses cTextArea (after cMPNoticeDialog).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 11th **Tier 2** dialog port (after cExitDialog,
// cMacroDialog, cCharMakeDlg, cGuildJoinDialog,
// cCharStateDialog, cSOSDialog, cWearedExDialog,
// cMiniFriendDialog, cReviveDialog, cMPNoticeDialog).
// The dialog has no service dependency on the modern
// service interface (Phase 13) — all state lives in 3
// global singletons (SCRIPTMGR / CHATMGR / NOTIFYMGR),
// none of which are ported yet. The dialog exercises
// cStatic + cTextArea (both already ported) + the
// commented-out singleton calls are TODOs for the
// singleton port.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;
class cTextArea;

class cEventNotifyDialog : public cDialog {
public:
    cEventNotifyDialog();
    ~cEventNotifyDialog() override;

    // ----- 1:1 with legacy CEventNotifyDialog::Linking -----

    // Resolves 2 children by id (kStcTitleId=270, kTAContextId=271).
    // REAL — pure widget ops. The SetToolTip call from the
    // legacy is documented as TODO (SCRIPTMGR singleton).
    void Linking();

    // ----- 1:1 with legacy CEventNotifyDialog::SetActive -----

    // 1:1 override: calls base SetActive, then if !val
    // clears the context text area. The NOTIFYMGR call
    // is documented as TODO.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CEventNotifyDialog::ActionEvent -----

    // 1:1 override: calls base ActionEvent. The
    // click-to-close logic is commented out in the
    // legacy (TODO when needed).
    std::uint32_t ActionEvent(std::int32_t mouseX,
                              std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // ----- 1:1 with legacy CEventNotifyDialog::SetTitle / SetContext -----

    // 1:1 wrappers: set the static text + script text
    // on the linked children. Defensive null-checks.
    void SetTitle(const char* title);
    void SetContext(const char* context);

    // ----- 1:1 with legacy CEventNotifyDialog::SetEventCount -----

    // 1:1 quirk: legacy SetEventCount(bool bAdd) was
    // a state-machine method (probably used to update
    // the unread count). The modern port keeps the
    // method signature for API compatibility but the
    // body is a no-op until the underlying state is
    // ported.
    void SetEventCount(bool bAdd) noexcept;

    // ----- Accessors (used by tests) -----

    cStatic*  GetStcTitle()  const noexcept { return m_pStcTitle; }
    cTextArea* GetTAContext() const noexcept { return m_pTAContext; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kStcTitleId  = 270;  // was GMOT_STC_TITLE
    static constexpr std::int32_t kTAContextId = 271;  // was GMOT_TA_CONTEXT

private:
    cStatic*  m_pStcTitle  = nullptr;
    cTextArea* m_pTAContext = nullptr;
};

}  // namespace mxh::ui
