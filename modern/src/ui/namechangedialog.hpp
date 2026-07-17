// namechangedialog.hpp — modern port of 墨香 CNameChangeDialog
// (name change editor dialog: 1 cEditBox + 1 OK button +
// item DB idx state).
//
// 1:1 port of legacy `CNameChangeDialog` from
//   `墨香【源码】\[Client]MH\NameChangeDialog.h` (877 B) and
//   `墨香【源码】\[Client]MH\NameChangeDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_NAMECHANGE_DLG (legacy
//     cWindow type tag); m_dwDBIdx = 0.
//   - Dtor: empty body.
//   - Linking: resolve 1 cEditBox child (m_pNameBox
//     by CH_NAME_CHANGE_EDITBOX id), call
//     SetValidCheck(VCM_CHARNAME) to enforce the
//     character-name valid check.
//   - SetActive override: call cDialog::SetActive,
//     then if val == TRUE, clear edit text via
//     SetEditText("").
//   - NameChangeSyn: get edit text into local
//     char buf[20]; validate length (0 → chat msg
//     11 + return; < 4 → chat msg 19 + return; >
//     MAX_NAME_LENGTH → return); check duplicate
//     with hero name (return if same); check
//     FILTERTABLE for invalid char / usable name
//     (chat msg 14 if either fails + return);
//     check m_dwDBIdx == 0 (return if 0); then
//     build SEND_CHANGENAMEBASE msg + NETWORK
//     send + SetActive(FALSE).
//   - SetItemDBIdx(DWORD): inline setter for
//     m_dwDBIdx.
//   - GetItemDBIdx(): inline getter for m_dwDBIdx.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type =
//     WT_NAMECHANGE_DLG drop, modern cWindow
//     does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cEditBox child by
//     id, call SetValidCheck(kVcmCharname = 2).
//   - SetActive override: 1:1 with legacy
//     (call base SetActive + clear edit text on
//     val=true).
//   - NameChangeSyn: TODO (4-singleton: CHATMGR +
//     FILTERTABLE + HERO + NETWORK not ported,
//     R-12.x deferred). Modern port returns
//     immediately (no-op) while singletons are
//     unported. When ported, the body becomes the
//     legacy code.
//   - SetItemDBIdx / GetItemDBIdx: REAL inline
//     setter / getter.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 32nd **Tier 2** dialog port (after
// cPartyInviteDlg). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — only CHATMGR + FILTERTABLE + HERO +
// NETWORK singletons (R-12.x deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;

class cNameChangeDialog : public cDialog {
public:
    cNameChangeDialog();
    ~cNameChangeDialog() override;

    // ----- 1:1 with legacy CNameChangeDialog::Linking -----

    // 1:1 with legacy Linking. Resolve cEditBox
    // child (m_pNameBox by kIdNameBox) by id, then
    // call SetValidCheck(kVcmCharname = 2) on
    // it to enforce the character-name valid
    // check.
    void Linking();

    // ----- 1:1 with legacy CNameChangeDialog::SetActive override -----

    // 1:1 with legacy SetActive override. Call
    // base SetActive; if val == TRUE, clear edit
    // text via SetEditText("").
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CNameChangeDialog::NameChangeSyn -----

    // 1:1 with legacy NameChangeSyn. The whole
    // method is TODO (4-singleton: CHATMGR +
    // FILTERTABLE + HERO + NETWORK not ported,
    // R-12.x deferred). Modern port returns
    // immediately (no-op) while singletons are
    // unported. When ported, the body becomes the
    // legacy code.
    void NameChangeSyn();

    // ----- 1:1 with legacy CNameChangeDialog::SetItemDBIdx / GetItemDBIdx -----

    void SetItemDBIdx(std::uint32_t dbIdx) noexcept { m_dwDBIdx = dbIdx; }
    std::uint32_t GetItemDBIdx() const noexcept     { return m_dwDBIdx; }

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID
    // (CH_NAME_CHANGE_EDITBOX). Local 450 —
    // distinct from 200-443 used by previous
    // Tier 2 dialogs.
    static constexpr std::int32_t kIdNameBox = 450;

    // VCM_CHARNAME = 2 (1:1 with legacy cEditBox
    // valid-check enum: character-name valid
    // check).
    static constexpr int kVcmCharname = 2;

private:
    // 1:1 with legacy m_pNameBox (resolved in
    // Linking by CH_NAME_CHANGE_EDITBOX id).
    cEditBox* m_pNameBox = nullptr;

    // 1:1 with legacy m_dwDBIdx (item context
    // for the name change).
    std::uint32_t m_dwDBIdx = 0;
};

}  // namespace mxh::ui
