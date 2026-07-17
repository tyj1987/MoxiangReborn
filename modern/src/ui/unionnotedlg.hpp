// unionnotedlg.hpp — modern port of 墨香
// CUnionNoteDialog (guild union note sender dialog:
// 1 cEditBox + 1 cTextArea + 1 CItem state +
// m_bUse flag).
//
// 1:1 port of legacy `CUnionNoteDlg` from
//   `墨香【源码】\[Client]MH\UnionNoteDlg.h` and
//   `墨香【源码】\[Client]MH\UnionNoteDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_bUse = FALSE.
//   - Dtor: empty body.
//   - Linking: resolve 1 cTextArea (m_pNoteText
//     by AN_TEXTREA); SetEnterAllow(FALSE);
//     SetScriptText("").
//   - Show(CItem* pItem): checks HERO guild idx +
//     rank + union idx + pItem + m_bUse (4-singleton
//     dispatch via CHATMGR); m_pItem = pItem;
//     SetActive(TRUE).
//   - Use(): m_bUse = FALSE; m_pNoteText->SetScriptText("");
//     send MSG_ITEM_USE_SYN via NETWORK; ITEMMGR++.
//   - OnActionEvnet (typo): 2 button case —
//     AN_SENDOKBTN → send MSG_GUILD_SEND_NOTE via
//     NETWORK; SetActive(FALSE). AN_CANCELBTN →
//     SetActive(FALSE).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_bUse default
//     = false via member init).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cTextArea by id,
//     SetEnterAllow(false), SetScriptText("").
//   - Show: TODO (HERO + CHATMGR singletons, R-12.x
//     deferred). Modern port stores m_pItem
//     forward-declared as void* + SetActive(true)
//     if all checks pass; but checks are TODO.
//   - Use: TODO (HERO + NETWORK + ITEMMGR singletons,
//     R-12.x deferred). Modern port clears
//     m_pNoteText + m_bUse.
//   - OnActionEvnet: TODO (HERO + NETWORK singletons,
//     R-12.x deferred). Modern port is empty
//     (no-op for now; the body becomes the legacy
//     code when CHATMGR + HERO + NETWORK are ported).
//   - 1:1 quirk: legacy ctor m_bUse = FALSE;
//     modern uses default member init (m_bUse = false).
//   - 1:1 quirk: m_pTitleEdit is declared in legacy
//     header but never used in cpp; modern port
//     preserves the declaration for 1:1 parity.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 44th **Tier 2** dialog port (after
// cMPGuageDialog). The dialog has 1 cTextArea
// (m_pNoteText) + 1 cEditBox (m_pTitleEdit,
// unused) + 1 CItem (forward-declared) +
// m_bUse flag. HERO + CHATMGR + NETWORK + ITEMMGR
// are R-12.x deferred.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;
class cTextArea;

class cUnionNoteDlg : public cDialog {
public:
    cUnionNoteDlg();
    ~cUnionNoteDlg() override;

    // ----- 1:1 with legacy CUnionNoteDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 1 cTextArea
    // (m_pNoteText by kIdNoteText), call
    // SetEnterAllow(false), SetScriptText("").
    void Linking();

    // ----- 1:1 with legacy CUnionNoteDlg::Show -----

    // 1:1 with legacy Show(CItem* pItem). The
    // HERO + CHATMGR 4-singleton dispatch is TODO
    // (R-12.x deferred). Modern port stores
    // pItem forward-declared as void* + sets
    // m_pItem; SetActive(true) is called only if
    // all checks pass (currently: always pass, TODO
    // make the checks real when CHATMGR is ported).
    void Show(void* pItem);

    // ----- 1:1 with legacy CUnionNoteDlg::Use -----

    // 1:1 with legacy Use(). The HERO + NETWORK +
    // ITEMMGR dispatch is TODO (R-12.x deferred).
    // Modern port clears m_pNoteText + m_bUse +
    // m_pItem.
    void Use();

    // ----- 1:1 with legacy CUnionNoteDlg::OnActionEvnet -----

    // 1:1 with legacy OnActionEvnet (typo'd).
    // 2 button case (AN_SENDOKBTN + AN_CANCELBTN).
    // The whole method is TODO (HERO + NETWORK
    // singletons, R-12.x deferred). Modern port is
    // a no-op for now.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy m_bUse getter -----

    // 1:1 with legacy m_bUse access (used by tests
    // to verify the flag transitions).
    bool IsUse() const noexcept { return m_bUse; }

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h AN_TEXTREA (620).
    // Local 620 — distinct from 200-620 used by
    // previous Tier 2 dialogs.
    static constexpr std::int32_t kIdNoteText = 620;

    // 1:1 with legacy AN_TITLEEDIT (declared in
    // header but unused in cpp). Local 621.
    static constexpr std::int32_t kIdTitleEdit = 621;

    // 1:1 with legacy AN_SENDOKBTN + AN_CANCELBTN.
    static constexpr std::int32_t kIdSendOkBtn   = 622;
    static constexpr std::int32_t kIdCancelBtn   = 623;

private:
    // 1:1 with legacy m_pTitleEdit (declared in
    // header but never used in cpp). Modern port
    // preserves the field for 1:1 parity.
    cEditBox* m_pTitleEdit = nullptr;

    // 1:1 with legacy m_pNoteText (resolved in
    // Linking by AN_TEXTREA id).
    cTextArea* m_pNoteText = nullptr;

    // 1:1 with legacy m_pItem. CItem is forward-
    // declared; modern port stores as void* (untyped
    // pointer, R-12.x deferred).
    void* m_pItem = nullptr;

    // 1:1 with legacy m_bUse (BOOL; init FALSE
    // in ctor). Modern uses bool (default member
    // init = false).
    bool m_bUse = false;
};

}  // namespace mxh::ui
