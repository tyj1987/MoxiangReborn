// chaseinputdialog.hpp — modern port of 墨香 CChaseinputDialog
// (chase input dialog: enter target player name for wanted
// chase item).
//
// 1:1 port of legacy `CChaseinputDialog` from
//   `墨香【源码】\[Client]MH\ChaseinputDialog.h` (497 B) and
//   `墨香【源码】\[Client]MH\ChaseinputDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_CHASEINPUT_DLG (legacy cWindow
//     type tag; modern cWindow / cDialog don't have
//     m_type, so modern port drops the ctor body).
//     m_LastChktime = 0.
//   - Linking: resolve cEditBox m_pEditName by id +
//     SetValidCheck(VCM_CHARNAME) (1:1 quirk: legacy
//     uses cIMEex VCM_CHARNAME; modern cEditBox supports
//     0/1/2/3 modes, closest is mode 2 = alpha only).
//   - SetActive override: 1:1 with base noexcept. Body:
//     base SetActive + if val clear edit text + reset
//     m_dwItemIdx to 0.
//   - SetItemIdx: 1:1 wrapper that sets m_dwItemIdx.
//   - WantedChaseSyn: 6-singleton dispatch
//     (gCurTime/CHATMGR/HERO/FILTERTABLE/WANTEDMGR/
//     NETWORK). Modern port: TODO.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 14th **Tier 2** dialog port. The dialog is
// the simplest Tier 2 in P2-12 (1 child, 4 methods,
// 0 cTextArea dependencies — only cEditBox).
//
// 1:1 quirks preserved:
//   - Ctor drops m_type = WT_CHASEINPUT_DLG (legacy
//     cWindow type tag removed in Phase 6).
//   - Linking calls SetValidCheck(VCM_CHARNAME alias = 2)
//     — closest modern equivalent for the legacy
//     cIMEex character-name validator (same as
//     cMiniFriendDialog).
//   - SetActive matches base noexcept (R-12 polymorphic
//     virtual required).
//   - SetItemIdx is a 1:1 wrapper.
//   - WantedChaseSyn is documented as TODO (6-singleton
//     dispatch deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;

class cChaseInputDialog : public cDialog {
public:
    cChaseInputDialog();
    ~cChaseInputDialog() override;

    // ----- 1:1 with legacy CChaseinputDialog::Linking -----

    // Resolves cEditBox m_pEditName by id (kEditNameId=300)
    // + SetValidCheck(VCM_CHARNAME alias = 2).
    void Linking();

    // ----- 1:1 with legacy CChaseinputDialog::SetActive -----

    // 1:1 override: calls base SetActive + if val
    // clears the edit text + resets m_dwItemIdx.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CChaseinputDialog::SetItemIdx -----

    // 1:1 wrapper that sets m_dwItemIdx.
    void SetItemIdx(std::uint32_t dwItem) noexcept {
        m_dwItemIdx = dwItem;
    }

    // ----- 1:1 with legacy CChaseinputDialog::WantedChaseSyn -----

    // Dispatch the chase syn. The 6-singleton dispatch
    // is TODO until CHATMGR + HERO + FILTERTABLE +
    // WANTEDMGR + NETWORK + gCurTime are ported.
    void WantedChaseSyn();

    // ----- Accessors (used by tests) -----

    cEditBox* GetEditName() const noexcept { return m_pEditName; }
    std::uint32_t GetItemIdx() const noexcept { return m_dwItemIdx; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kEditNameId = 300;  // was CHASE_EDITBOX

    // 1:1 quirk: legacy VCM_CHARNAME (from cIMEex.h) is
    // a character-name validator. The modern cEditBox
    // supports 0=none, 1=digits only, 2=alpha only,
    // 3=alnum. The closest modern equivalent for
    // VCM_CHARNAME is mode 2 (alpha only).
    static constexpr int kVcmCharnameAlias = 2;

private:
    cEditBox*    m_pEditName  = nullptr;
    std::uint32_t m_LastChktime = 0;  // 1:1 with legacy
    std::uint32_t m_dwItemIdx   = 0;
};

}  // namespace mxh::ui
