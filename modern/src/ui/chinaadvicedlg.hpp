// chinaadvicedlg.hpp — modern port of 墨香 CChinaAdviceDlg
// (China-region advice / T&C dialog: 1 cTextArea + 1 button).
//
// 1:1 port of legacy `CChinaAdviceDlg` from
//   `墨香【源码】\[Client]MH\ChinaAdviceDlg.h` (677 B) and
//   `墨香【源码】\[Client]MH\ChinaAdviceDlg.cpp`.
//
// What the legacy does:
//   - Ctor / dtor: empty bodies (no state init).
//   - Linking: resolve 1 cTextArea child
//     (CNA_TEXTAREA), call SetScriptText with
//     CHATMGR->GetChatMsg(30) — the "China advice"
//     T&C text (a long string in the chat message
//     table). The CNA_BTN_OK button is NOT resolved
//     in Linking (no .cpp code touches it; presumably
//     closed by clicking outside the dialog or by a
//     future button handler that was never written).
//   - OnActionEvent: empty body. No button dispatch.
//     The dialog relies on the auto-close / outside-
//     click dismissal path.
//
// The modern port covers:
//   - Ctor / dtor: empty (no-op) — 1:1 with legacy.
//   - Linking: REAL — resolve cTextArea child by id,
//     call SetScriptText with placeholder string
//     "CHINA_ADVICE_TEXT" (replacing
//     CHATMGR->GetChatMsg(30) — same pattern as
//     cMPNoticeDialog's "MP_NCAUTION" placeholder).
//   - OnActionEvent: empty no-op (1:1 with legacy
//     empty body).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 19th **Tier 2** dialog port (after
// cGuildNoticeDlg). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — only CHATMGR singleton (R-12.x
// deferred, but the placeholder pattern works
// around it without the singleton).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cTextArea;

class cChinaAdviceDlg : public cDialog {
public:
    cChinaAdviceDlg();
    ~cChinaAdviceDlg() override;

    // ----- 1:1 with legacy CChinaAdviceDlg::Linking -----

    // 1:1 with legacy Linking. Resolve cTextArea
    // child (m_pTextArea) by id, call SetScriptText
    // with placeholder "CHINA_ADVICE_TEXT"
    // (replacing CHATMGR->GetChatMsg(30)). The CNA
    // _BTN_OK button is NOT resolved (no legacy code
    // touches it).
    void Linking();

    // ----- 1:1 with legacy CChinaAdviceDlg::OnActionEvent -----

    // 1:1 with legacy: empty body. No button dispatch.
    // The dialog relies on the auto-close / outside-
    // click dismissal path.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID values
    // (CNA_TEXTAREA). Local 360 — distinct from
    // 200-352 used by previous Tier 2 dialogs. The
    // CNA_BTN_OK enum exists in WindowIDs.h but the
    // legacy .cpp does NOT resolve or use it
    // (presumably dead code or future hook), so the
    // modern port also does not include it.
    static constexpr std::int32_t kIdTextArea = 360;

private:
    // 1:1 with legacy m_pTextArea (resolved in
    // Linking by CNA_TEXTAREA id). Modern port
    // stores raw pointer (not owned; cDialog owns
    // the child).
    cTextArea* m_pTextArea = nullptr;
};

}  // namespace mxh::ui
