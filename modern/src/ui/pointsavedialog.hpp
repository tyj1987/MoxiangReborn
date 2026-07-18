// pointsavedialog.hpp — modern port of 墨香
// CPointSaveDialog (map save-point name editor
// dialog: 1 cEditBox + 1 cTextArea + ItemPos/ItemIdx
// state + m_bNewPoint flag).
//
// 1:1 port of legacy `CPointSaveDialog` from
//   `墨香【源码】\[Client]MH\PointSaveDialog.h`
//   and `墨香【源码】\[Client]MH\PointSaveDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_bNewPoint = TRUE; m_ItemIdx = 0;
//     m_ItemPos = 0.
//   - Dtor: empty body.
//   - Linking: resolve 1 cEditBox (m_pNameEdtBox by
//     CHA_NAMEEDITBOX); SetValidCheck(VCM_CHARNAME).
//   - SetActive(BOOL val) override: cDialog::SetActive
//     + m_pNameEdtBox->SetFocusEdit(val);
//     if (val) m_pNameEdtBox->SetEditText("").
//   - SetItemToMapServer: inline setter for m_ItemIdx
//     + m_ItemPos.
//   - ChangePointName: 4-singleton dispatch via
//     ITEMMGR + GAMEIN + HERO + CHATMGR + MAP +
//     NETWORK (sends SEND_MOVEDATA_WITHITEM or
//     SEND_MOVEDATA_SIMPLE).
//   - CancelPointName: 4-singleton dispatch via
//     ITEMMGR + HERO + OBJECTSTATEMGR.
//   - SetDialogStatus: inline setter for m_bNewPoint.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_bNewPoint = true
//     + m_ItemIdx/m_ItemPos init via default
//     member init).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cEditBox by id +
//     SetValidCheck(VCM_CHARNAME=2).
//   - SetActive override: REAL — cDialog::SetActive
//     + SetFocusEdit + SetEditText (if val).
//   - SetItemToMapServer: REAL inline setter.
//   - ChangePointName: TODO (5-singleton dispatch,
//     R-12.x deferred). Modern port is empty.
//   - CancelPointName: TODO (3-singleton dispatch).
//     Modern port is empty.
//   - SetDialogStatus: REAL inline setter.
//   - State accessors: IsNewPoint + GetItemPos +
//     GetItemIdx for tests.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;
class cTextArea;

class cPointSaveDialog : public cDialog {
public:
    cPointSaveDialog();
    ~cPointSaveDialog() override;

    // ----- 1:1 with legacy CPointSaveDialog::Linking -----

    // 1:1 with legacy Linking. Resolve 1 cEditBox
    // (m_pNameEdtBox by kIdNameEditBox) +
    // SetValidCheck(VCM_CHARNAME=2).
    void Linking();

    // ----- 1:1 with legacy CPointSaveDialog::SetActive override -----

    // 1:1 with legacy SetActive override. Always
    // base + SetFocusEdit(val). If val, SetEditText("").
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CPointSaveDialog::SetItemToMapServer -----

    // 1:1 with legacy SetItemToMapServer(DWORD,
    // DWORD) inline setter.
    void SetItemToMapServer(std::uint32_t itemIdx,
                             std::uint32_t itemPos) noexcept;

    // ----- 1:1 with legacy CPointSaveDialog::ChangePointName -----

    // 1:1 with legacy ChangePointName. The 5-singleton
    // dispatch (ITEMMGR + GAMEIN + HERO + CHATMGR + MAP
    // + NETWORK) is TODO (R-12.x deferred). Modern
    // port is empty.
    void ChangePointName() {}

    // ----- 1:1 with legacy CPointSaveDialog::CancelPointName -----

    // 1:1 with legacy CancelPointName. The 3-singleton
    // dispatch (ITEMMGR + HERO + OBJECTSTATEMGR) is
    // TODO (R-12.x deferred). Modern port is empty.
    void CancelPointName() {}

    // ----- 1:1 with legacy CPointSaveDialog::SetDialogStatus -----

    // 1:1 with legacy SetDialogStatus(BOOL bNewPoint)
    // inline setter.
    void SetDialogStatus(bool bNewPoint) noexcept {
        m_bNewPoint = bNewPoint;
    }

    // ----- 1:1 with legacy state accessors -----

    // 1:1 with legacy m_bNewPoint getter.
    bool IsNewPoint() const noexcept { return m_bNewPoint; }
    // 1:1 with legacy m_ItemIdx getter.
    std::uint32_t GetItemIdx() const noexcept { return m_ItemIdx; }
    // 1:1 with legacy m_ItemPos getter.
    std::uint32_t GetItemPos() const noexcept { return m_ItemPos; }

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h CHA_NAMEEDITBOX.
    // Local 710 — distinct from 200-700 used by
    // previous Tier 2 dialogs.
    static constexpr std::int32_t kIdNameEditBox = 710;

    // 1:1 with legacy VCM_CHARNAME = 2 (char-name
    // valid check).
    static constexpr int kVcmCharName = 2;

private:
    // 1:1 with legacy m_pNameEdtBox (resolved in
    // Linking by CHA_NAMEEDITBOX id).
    cEditBox* m_pNameEdtBox = nullptr;

    // 1:1 with legacy m_pText (declared in header
    // but never used in legacy cpp body; modern
    // port preserves for 1:1 parity).
    cTextArea* m_pText = nullptr;

    // 1:1 with legacy m_bNewPoint (BOOL; init TRUE).
    bool m_bNewPoint = true;

    // 1:1 with legacy m_ItemPos (DWORD; init 0).
    std::uint32_t m_ItemPos = 0;

    // 1:1 with legacy m_ItemIdx (DWORD; init 0).
    std::uint32_t m_ItemIdx = 0;
};

}  // namespace mxh::ui
