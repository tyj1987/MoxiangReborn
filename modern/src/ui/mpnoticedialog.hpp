// mpnoticedialog.hpp — modern port of 墨香 CMPNoticeDialog
// (MP notice dialog: 2 text areas showing caution + red
// caution messages).
//
// 1:1 port of legacy `CMPNoticeDialog` from
//   `墨香【源码】\[Client]MH\MPNoticeDialog.h` (695 B) and
//   `墨香【源码】\[Client]MH\MPNoticeDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_MPNOTICEDIALOG (legacy cWindow
//     type tag; modern cWindow / cDialog don't have
//     m_type, so modern port drops the ctor body).
//   - Linking: resolve 2 cTextArea children (m_pNCaution
//     + m_pNRedCaution), call SetScriptText on each
//     with a localized chat msg from CHATMGR (msg 667
//     + 668). The SetScriptText call is a cTextArea
//     API (already ported as Tier 1.5 sub-widget).
//
// The modern port covers everything: ctor no-op,
// Linking REAL (2 cTextArea resolutions + SetScriptText
// calls). The CHATMGR singleton is TODO (the chat
// msg IDs 667 + 668 are deferred; the modern port
// uses placeholder text "MP_NCAUTION" + "MP_NREDCAUTION"
// to keep the SetScriptText signature exercised
// end-to-end without depending on CHATMGR).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 10th **Tier 2** dialog port (after cExitDialog,
// cMacroDialog, cCharMakeDlg, cGuildJoinDialog,
// cCharStateDialog, cSOSDialog, cWearedExDialog,
// cMiniFriendDialog, cReviveDialog). The dialog has no
// service dependency on the modern service interface
// (Phase 13) — all state lives in 1 global singleton
// (CHATMGR), not ported yet. The cTextArea sub-widget
// IS ported (this commit's cTextArea addition is the
// prerequisite).

#pragma once

#include "cdialog.hpp"

namespace mxh::ui {

class cTextArea;

class cMPNoticeDialog : public cDialog {
public:
    cMPNoticeDialog();
    ~cMPNoticeDialog() override;

    // ----- 1:1 with legacy CMPNoticeDialog::Linking -----

    // Resolves 2 cTextArea children by id (kNCautionId=260,
    // kNRedCautionId=261) and calls SetScriptText on each
    // with the localized chat msg. Modern port uses
    // placeholder text since CHATMGR is not ported.
    void Linking();

    // ----- Accessors (used by tests) -----

    cTextArea* GetNCaution()   const noexcept { return m_pNCaution; }
    cTextArea* GetNRedCaution() const noexcept { return m_pNRedCaution; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kNCautionId   = 260;  // was MP_NCAUTION
    static constexpr std::int32_t kNRedCautionId = 261;  // was MP_NREDCAUTION

    // 1:1 quirk: legacy CHATMGR->GetChatMsg(667) +
    // CHATMGR->GetChatMsg(668). The actual msg strings
    // are localized. The modern port uses placeholder
    // text until CHATMGR is ported.
    static constexpr int kNCautionChatMsgId   = 667;
    static constexpr int kNRedCautionChatMsgId = 668;

private:
    cTextArea* m_pNCaution   = nullptr;
    cTextArea* m_pNRedCaution = nullptr;
};

}  // namespace mxh::ui
