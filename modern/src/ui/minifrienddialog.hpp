// minifrienddialog.hpp — modern port of 墨香 CMiniFriendDialog
// (mini friend-add dialog: enter a character name to add
// as friend).
//
// 1:1 port of legacy `CMiniFriendDialog` from
//   `墨香【源码】\[Client]MH\MiniFriendDialog.h` (930 B) and
//   `墨香【源码】\[Client]MH\MiniFriendDialog.cpp`.
//
// What the legacy does:
//   - Ctor + Init: m_type = WT_MINIFRIENDDLG (legacy
//     cWindow type tag; modern cWindow / cDialog don't
//     have m_type field, so modern port drops the ctor
//     body and Init override).
//   - Linking: resolve 4 children by id (cStatic m_pName
//     + cEditBox m_pNameEdit + cButton m_pAddOkBtn +
//     cButton m_pAddCancelBtn), call
//     m_pNameEdit->SetValidCheck(VCM_CHARNAME) (legacy
//     cIMEex validator enum) + m_pNameEdit->SetEditText("").
//   - SetActive: 1:1 override — if m_bDisable return; if
//     val m_pNameEdit->SetEditText("") (clear the field
//     on show); cDialog::SetActiveRecursive(val).
//   - SetName(char*): m_pNameEdit->SetEditText(Name).
//
// The modern port covers everything: ctor + Init are
// no-op (m_type field doesn't exist), Linking REAL
// (4 children + SetValidCheck + clear text), SetActive
// REAL override, SetName REAL. All 4 children (cStatic +
// cEditBox + cButton × 2) are already ported.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 8th **Tier 2** dialog port (after cExitDialog,
// cMacroDialog, cCharMakeDlg, cGuildJoinDialog,
// cCharStateDialog, cSOSDialog, cWearedExDialog). This
// is the first Tier 2 port where ALL methods are REAL
// (no singleton dependencies). The dialog is the smallest
// "end-to-end testable" Tier 2 — no TODO, no deferred
// dispatch, all 4 methods verifiable end-to-end.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;
class cEditBox;
class cButton;

class cMiniFriendDialog : public cDialog {
public:
    cMiniFriendDialog();
    ~cMiniFriendDialog() override;

    // ----- 1:1 with legacy CMiniFriendDialog::Init -----

    // 1:1 quirk: legacy Init() calls cDialog::Init +
    // m_type = WT_MINIFRIENDDLG. Modern cDialog::Init is
    // the same signature, but m_type doesn't exist on
    // modern cWindow. The modern port keeps the Init
    // override (1:1 API shape) but the body just calls
    // base Init — the m_type assignment is dropped.
    void Init(std::int32_t x, std::int32_t y,
              std::uint16_t wid, std::uint16_t hei,
              void* basicImage, std::int32_t id = 0);

    // ----- 1:1 with legacy CMiniFriendDialog::Linking -----

    // REAL — resolves 4 children by id and configures
    // m_pNameEdit (SetValidCheck + clear text). Pure
    // widget ops, no singleton.
    void Linking();

    // ----- 1:1 with legacy CMiniFriendDialog::SetActive -----

    // 1:1 override — if m_bDisable return; if val clear
    // m_pNameEdit text; cDialog::SetActiveRecursive(val).
    // REAL — no singleton.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CMiniFriendDialog::SetName -----

    // REAL — m_pNameEdit->SetEditText(Name).
    void SetName(const char* name);

    // ----- Accessors (used by tests) -----

    cStatic*  GetNameStatic()    const noexcept { return m_pName; }
    cEditBox* GetNameEdit()      const noexcept { return m_pNameEdit; }
    cButton*  GetAddOkButton()   const noexcept { return m_pAddOkBtn; }
    cButton*  GetAddCancelButton() const noexcept { return m_pAddCancelBtn; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kNameId         = 240;  // was FRI_NAME
    static constexpr std::int32_t kNameEditId     = 241;  // was FRI_NAMEEDIT
    static constexpr std::int32_t kAddOkBtnId     = 242;  // was FRI_ADDOKBTN
    static constexpr std::int32_t kAddCancelBtnId = 243;  // was FRI_ADDCANCELBTN

    // 1:1 quirk: legacy VCM_CHARNAME (from cIMEex.h) is
    // a character-name validator. The modern cEditBox
    // supports 0=none, 1=digits only, 2=alpha only,
    // 3=alnum. The closest modern equivalent for
    // VCM_CHARNAME is mode 2 (alpha only). The legacy
    // VCM_CHARNAME includes cIMEex integration (which
    // isn't ported) but the validation mode matches
    // modern mode 2. Modern port uses 2 as the alias.
    static constexpr int kVcmCharnameAlias = 2;

private:
    cStatic*  m_pName         = nullptr;
    cEditBox* m_pNameEdit     = nullptr;
    cButton*  m_pAddOkBtn     = nullptr;
    cButton*  m_pAddCancelBtn = nullptr;
};

}  // namespace mxh::ui
