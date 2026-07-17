// baildialog.hpp — modern port of 墨香 CBailDialog
// (bail dialog: enter amount of bad fame to pay off +
// show the bail cost + minimum required).
//
// 1:1 port of legacy `CBailDialog` from
//   `墨香【源码】\[Client]MH\BailDialog.h` (497 B) and
//   `墨香【源码】\[Client]MH\BailDialog.cpp`.
//
// What the legacy does:
//   - Ctor: 2 children null + m_BadFame = 0.
//   - Linking: resolve cEditBox m_pBailEdtBox by id +
//     SetValidCheck(VCM_NUMBER) + SetAlign(TXT_RIGHT).
//     Resolve cTextArea m_pBailText by id + set its
//     script text to a formatted string (CHATMGR msg
//     644 + the bail cost formatted with AddComma).
//   - Open: if HERO->GetBadFame() > MIN_BADFAME_FOR_BAIL,
//     set the edit text to "0" + activate the dialog.
//     Else: show a WINDOWMGR msg box with CHATMGR msg
//     659. 1:1 quirk: legacy has `SetActive(TRUE)` (not
//     `SetActive(true)`) — the modern SetActive
//     override matches the base noexcept spec.
//   - Close: SetDisable(FALSE) + SetActive(FALSE) +
//     OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal).
//   - SetFame: read the edit text as a number (with
//     AddComma handling) + 4-singleton check +
//     WINDOWMGR msg box. Returns void.
//   - SetBadFrameSync: send MSG_FAME network message +
//     Close.
//
// The modern port covers the public API:
//   - Linking REAL (resolve 2 children + SetValidCheck +
//     SetAlign + SetScriptText with placeholder text).
//   - Open / Close / SetFame / SetBadFrameSync: 1:1
//     wrapper signatures; the singleton dispatches are
//     documented as TODO.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 16th **Tier 2** dialog port. The dialog
// exercises cTextArea (already ported in 0.13.23) +
// cEditBox (already ported in 6.1).
//
// 1:1 quirks preserved:
//   - Ctor initializes m_pBailEdtBox = m_pBailText = NULL
//     + m_BadFame = 0 (modern port uses nullptr +
//     default member init).
//   - Linking calls SetValidCheck(VCM_NUMBER) +
//     SetAlign(TXT_RIGHT) on the cEditBox. The
//     SetScriptText call uses placeholder text
//     "BAIL_TEXT_PLACEHOLDER" (legacy uses
//     CHATMGR->GetChatMsg(644) + AddComma-formatted
//     bail cost).
//   - Open: legacy has `if (HERO->GetBadFame() > ...)`
//     with a 4-singleton dispatch. Modern port: TODO.
//   - Close: legacy has 3-singleton dispatch. Modern
//     port: TODO.
//   - SetFame: 4-singleton dispatch. Modern port: TODO.
//   - SetBadFrameSync: 3-singleton dispatch. Modern
//     port: TODO.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;
class cTextArea;

class cBailDialog : public cDialog {
public:
    cBailDialog();
    ~cBailDialog() override;

    // ----- 1:1 with legacy CBailDialog::Linking -----

    // Resolves 2 children by id (kBailEditBoxId=320,
    // kBailTextId=321) + SetValidCheck(VCM_NUMBER) +
    // SetAlign(TXT_RIGHT) on the cEditBox + SetScriptText
    // on the cTextArea (placeholder text until CHATMGR
    // is ported).
    void Linking();

    // ----- 1:1 with legacy CBailDialog::Open / Close -----

    // Open: 1:1 wrapper that conditionally activates
    // the dialog based on the hero's bad fame + shows
    // a msg box. The singleton dispatch is TODO.
    void Open();

    // Close: 1:1 wrapper that disables + deactivates
    // the dialog + ends the hero's deal object state.
    // The singleton dispatch is TODO.
    void Close();

    // ----- 1:1 with legacy CBailDialog::SetFame / SetBadFrameSync -----

    // SetFame: reads the edit text as a number + checks
    // the hero's bad fame + money + shows msg boxes.
    // The singleton dispatch is TODO.
    void SetFame();

    // SetBadFrameSync: sends MSG_FAME network message +
    // closes the dialog. The singleton dispatch is TODO.
    void SetBadFrameSync();

    // ----- Accessors (used by tests) -----

    cEditBox* GetBailEditBox() const noexcept { return m_pBailEdtBox; }
    cTextArea* GetBailText()    const noexcept { return m_pBailText; }
    std::uint32_t GetBadFame() const noexcept { return m_BadFame; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kBailEditBoxId = 320;  // was BAIL_BAILEDITBOX
    static constexpr std::int32_t kBailTextId    = 321;  // was BAIL_TEXTAREA

private:
    cEditBox*    m_pBailEdtBox = nullptr;
    cTextArea*   m_pBailText   = nullptr;
    std::uint32_t m_BadFame     = 0;
};

}  // namespace mxh::ui
