// guildnotedlg.hpp — modern port of 墨香
// CGuildNoteDialog (guild note sender dialog:
// 1 cEditBox + 1 cTextArea + 1 CItem state +
// m_bUse flag).
//
// 1:1 port of legacy `CGuildNoteDlg` from
//   `墨香【源码】\[Client]MH\GuildNoteDlg.h` and
//   `墨香【源码】\[Client]MH\GuildNoteDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_bUse = FALSE.
//   - Dtor: empty body.
//   - Linking: resolve 1 cTextArea (m_pNoteText
//     by GN_TEXTREA); SetEnterAllow(FALSE);
//     SetScriptText("").
//   - Show(CItem* pItem): check pItem + m_bUse
//     (3-singleton dispatch via CHATMGR); m_pItem
//     = pItem; SetActive(TRUE).
//   - Use(): m_bUse = FALSE; m_pNoteText->SetScriptText("");
//     send MSG_ITEM_USE_SYN via NETWORK; ITEMMGR++.
//   - OnActionEvnet (typo): 2 button case —
//     GN_SENDOKBTN → send MSG_GUILD_SEND_NOTE via
//     NETWORK; SetActive(FALSE). GN_CANCELBTN →
//     SetActive(FALSE).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_bUse default
//     = false via member init).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cTextArea by id,
//     SetEnterAllow(false), SetScriptText("").
//   - Show: TODO (CHATMGR 3-singleton dispatch,
//     R-12.x deferred). Modern port stores m_pItem
//     + SetActive(true) without the checks.
//   - Use: TODO (NETWORK + ITEMMGR singletons,
//     R-12.x deferred). Modern port clears
//     m_pNoteText + m_bUse.
//   - OnActionEvent: TODO (NETWORK singletons,
//     R-12.x deferred). Modern port is no-op.
//   - 1:1 quirks: m_pTitleEdit declared but unused
//     in legacy cpp; modern port preserves.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 51st **Tier 2** dialog port (after
// cUnionNoteDlg). The dialog has 1 cTextArea
// (m_pNoteText) + 1 cEditBox (m_pTitleEdit,
// unused) + 1 CItem (forward-declared) +
// m_bUse flag. CHATMGR + NETWORK + ITEMMGR are
// R-12.x deferred.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;
class cTextArea;

class cGuildNoteDlg : public cDialog {
public:
    cGuildNoteDlg();
    ~cGuildNoteDlg() override;

    // ----- 1:1 with legacy CGuildNoteDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 1 cTextArea
    // (m_pNoteText by kIdNoteText), call
    // SetEnterAllow(false), SetScriptText("").
    void Linking();

    // ----- 1:1 with legacy CGuildNoteDlg::Show -----

    // 1:1 with legacy Show(CItem* pItem). The
    // CHATMGR 3-singleton dispatch is TODO (R-12.x
    // deferred). Modern port stores pItem +
    // SetActive(true) without the checks.
    void Show(void* pItem);

    // ----- 1:1 with legacy CGuildNoteDlg::Use -----

    // 1:1 with legacy Use(). The NETWORK + ITEMMGR
    // dispatch is TODO (R-12.x deferred). Modern
    // port clears m_pNoteText + m_bUse + m_pItem.
    void Use();

    // ----- 1:1 with legacy CGuildNoteDlg::OnActionEvnet -----

    // 1:1 with legacy OnActionEvnet (typo'd).
    // 2 button case (GN_SENDOKBTN + GN_CANCELBTN).
    // The whole method is TODO (NETWORK singletons,
    // R-12.x deferred). Modern port is no-op.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy m_bUse getter -----

    // 1:1 with legacy m_bUse access.
    bool IsUse() const noexcept { return m_bUse; }

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h GN_TEXTREA (700).
    // Local 700 — distinct from 200-700 used by
    // previous Tier 2 dialogs.
    static constexpr std::int32_t kIdNoteText = 700;

    // 1:1 with legacy GN_TITLEEDIT (declared in
    // header but unused in cpp). Local 701.
    static constexpr std::int32_t kIdTitleEdit = 701;

    // 1:1 with legacy GN_SENDOKBTN + GN_CANCELBTN.
    static constexpr std::int32_t kIdSendOkBtn   = 702;
    static constexpr std::int32_t kIdCancelBtn   = 703;

private:
    // 1:1 with legacy m_pTitleEdit (declared in
    // header but never used in cpp).
    cEditBox* m_pTitleEdit = nullptr;

    // 1:1 with legacy m_pNoteText (resolved in
    // Linking by GN_TEXTREA id).
    cTextArea* m_pNoteText = nullptr;

    // 1:1 with legacy m_pItem. CItem is forward-
    // declared; modern port stores as void* (untyped
    // pointer, R-12.x deferred).
    void* m_pItem = nullptr;

    // 1:1 with legacy m_bUse (BOOL; init FALSE).
    bool m_bUse = false;
};

}  // namespace mxh::ui
