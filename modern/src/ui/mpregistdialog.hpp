// mpregistdialog.hpp — modern port of 墨香 CMPRegistDialog
// (MP practice registration dialog: 2 cTextArea + 1 cStatic
// + 1 cIconDialog children).
//
// 1:1 port of legacy `CMPRegistDialog` from
//   `墨香【源码】\[Client]MH\MPRegistDialog.h` (1098 B) and
//   `墨香【源码】\[Client]MH\MPRegistDialog.cpp` (3860 B).
//
// What the legacy does:
//   - Ctor: m_type = WT_MPREGISTDIALOG (legacy cWindow
//     type tag; modern cWindow / cDialog don't have
//     m_type, so modern port drops the ctor body).
//   - Linking: resolve 4 children by id —
//     m_MugongInfo (cTextArea id MP_RMUGONGINFO),
//     m_PracticeInfo (cTextArea id MP_RPRACTICEINFO),
//     m_Fee (cStatic id MP_RFEE),
//     m_pMugongIconDlg (cIconDialog id MP_RMUGONGICON).
//     Then call SetDragOverIconType(WT_MUGONG) on the
//     icon dialog child. The modern cIconDialog API
//     does not expose SetDragOverIconType (it was a
//     legacy interface method that cPetWearedExDialog
//     and cWearedExDialog also dropped at port time);
//     the modern port is a 1:1 quirk drop — when
//     drag-over UI is wired in a follow-up phase, add
//     the API to cIconDialog + thread it through here.
//   - FakeMoveIcon(mouseX, mouseY, cIcon* icon): the
//     meat of the dialog. 14 lines of legacy logic
//     dispatching to 5 singletons (SURYUNMGR, HERO,
//     WINDOWMGR, CHATMGR, OBJECTSTATEMGR) and reading
//     icon-derived state. The modern port keeps the
//     signature + ctor body shape (returns FALSE in
//     all branches, like the legacy), but every
//     singleton dispatch is TODO and the icon-state
//     read is no-op (CMugongBase / cSkillInfo not
//     ported). This is the same TODO pattern as
//     cAlertDlg::ActionEvent (0.13.41) and
//     cGuildNoticeDlg::OnActionEvent (0.13.30).
//   - SetSuryunMugongInfo: sprintf with CHATMGR
//     placeholder + SetScriptText on m_MugongInfo.
//   - SetPracticeInfo: sprintf with CHATMGR
//     placeholder + SetScriptText on m_PracticeInfo +
//     SetStaticValue on m_Fee.
//   - SetActive override: 1:1 with legacy. On
//     val == FALSE, reset 4 children (3 text + 1 fee +
//     1 icon delete + 1 OBJECTSTATEMGR call), then
//     call cDialog::SetActive(val).
//   - AddLink: 1:1 with legacy — DeleteIcon(0) if not
//     addable, then AddIcon(0, picon, TRUE).
//   - GetMugong: 1:1 — GetIconForIdx(0) cast to
//     CMugongBase*. CMugongBase not ported; modern
//     returns nullptr with a TODO marker.
//
// The modern port covers everything 1:1 — ctor no-op,
// Linking REAL (4 children resolved + SetDragOverIconType
// call dropped per the 1:1 quirk), SetActive override
// 1:1 with legacy (4-state reset on val == FALSE + base
// call), SetSuryunMugongInfo/SetPracticeInfo placeholders
// (CHATMGR->GetChatMsg(660/661) are TODO), AddLink REAL
// wrap, GetMugong returns nullptr TODO.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 44th **Tier 2** dialog port (after cMPGuageDialog
// 0.13.43). The dialog has no service dependency on the
// modern service interface (Phase 13) — all state lives
// in cIconDialog's cell array (1 cell, MP practice slot
// holds 1 cIcon at a time) and the 4 children fields.

#pragma once

#include "cIconDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cTextArea;
class cStatic;
class cIcon;

class cMPRegistDialog : public cIconDialog {
public:
    cMPRegistDialog();
    ~cMPRegistDialog() override;

    // ----- 1:1 with legacy CMPRegistDialog::Linking -----

    // Resolves 4 children by id. 1:1 with legacy
    // Linking(). SetDragOverIconType(WT_MUGONG) call
    // is a 1:1 quirk drop — modern cIconDialog has no
    // such API (cPetWearedExDialog / cWearedExDialog
    // also drop this). The drag-over UI wiring is
    // deferred to the same follow-up phase that adds
    // SetDragOverIconType to cIconDialog.
    void Linking();

    // ----- 1:1 with legacy CMPRegistDialog::SetActive -----

    // 1:1 with legacy SetActive(BOOL val). On val == FALSE,
    // resets m_MugongInfo (CHATMGR msg 662 placeholder),
    // m_pMugongIconDlg (DeleteIcon(0, &pIcon)),
    // m_PracticeInfo (empty), m_Fee (SetStaticValue(0)),
    // optionally dismisses MBI_MPNOTICE_NOTFIT msgbox
    // (WINDOWMGR TODO), and ends OBJECTSTATEMGR
    // eObjectState_Deal (TODO). Then calls
    // cDialog::SetActive(val).
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CMPRegistDialog::FakeMoveIcon -----

    // 1:1 with legacy FakeMoveIcon(LONG mouseX, LONG
    // mouseY, cIcon* icon). The legacy body has 14
    // lines of singleton-dispatch logic against
    // SURYUNMGR / HERO / WINDOWMGR / CHATMGR / OBJECTSTATEMGR
    // (none ported yet). The modern port keeps the
    // signature + unconditional FALSE return (matching
    // legacy's "valid drop → register; invalid → return
    // FALSE" branch at the very end), with all
    // singleton dispatch TODO. This locks the
    // signature so a follow-up port of CMugongBase +
    // cSkillInfo + the 5 singletons can fill in the
    // body without breaking the dialog shape.
    bool FakeMoveIcon(std::int32_t mouseX, std::int32_t mouseY, cIcon* icon);

    // ----- 1:1 with legacy CMPRegistDialog::SetSuryunMugongInfo -----

    // 1:1 with legacy SetSuryunMugongInfo(char* MugongName,
    // BYTE Sung). sprintf via CHATMGR->GetChatMsg(661)
    // placeholder + SetScriptText on m_MugongInfo. The
    // CHATMGR msg 661 format string is "Mugong %s at
    // grade %d" (Korean-localized in the legacy); the
    // modern port uses a static placeholder format
    // string to keep the sprintf signature exercised
    // end-to-end without depending on CHATMGR.
    void SetSuryunMugongInfo(const char* mugongName, std::uint8_t sung);

    // ----- 1:1 with legacy CMPRegistDialog::SetPracticeInfo -----

    // 1:1 with legacy SetPracticeInfo(BYTE Sung, DWORD
    // limitime, int Kind, int num, MONEYTYPE Fee).
    // Computes LTime = limitime / 60000 (ms → min),
    // sprintf via CHATMGR->GetChatMsg(660) placeholder
    // + SetScriptText on m_PracticeInfo, and
    // SetStaticValue on m_Fee.
    void SetPracticeInfo(std::uint8_t sung, std::uint32_t limitime,
                         int kind, int num, std::uint64_t fee);

    // ----- 1:1 with legacy CMPRegistDialog::AddLink -----

    // 1:1 with legacy AddLink(cIcon* picon). If
    // m_pMugongIconDlg cell 0 is not addable, delete
    // it first; then AddIcon(0, picon, TRUE) (the
    // legacy bOnlyLink=TRUE path).
    void AddLink(cIcon* picon);

    // ----- 1:1 with legacy CMPRegistDialog::GetMugong -----

    // 1:1 with legacy GetMugong(). Casts
    // m_pMugongIconDlg->GetIconForIdx(0) to CMugongBase*.
    // CMugongBase is not yet ported (R-12.x deferred,
    // same constraint as cPetWearedExDialog::CheckDuplication
    // and cWearedExDialog's Titan-vs-normal branch), so
    // the modern port returns nullptr unconditionally
    // with a TODO marker. When CMugongBase is ported,
    // this becomes:
    //   cIcon* pIcon = m_pMugongIconDlg->GetIconForIdx(0);
    //   if (!pIcon) return nullptr;
    //   return static_cast<CMugongBase*>(pIcon);
    class CMugongBase* GetMugong() const;

    // ----- Accessors (used by tests) -----

    cTextArea* GetMugongInfo()   const noexcept { return m_MugongInfo; }
    cTextArea* GetPracticeInfo() const noexcept { return m_PracticeInfo; }
    cStatic*   GetFee()          const noexcept { return m_Fee; }
    cIconDialog* GetMugongIconDlg() const noexcept { return m_pMugongIconDlg; }

    // ----- Local id range (1:1 with legacy WindowIDs.h) -----

    // Local id range is 1:1 with legacy enum values
    // (562-568 per the 0.13.x batch port's reverse
    // mapping of MP_MISSION=570 / MP_MCAUTION=571 in
    // mpmissiondialog.hpp).
    static constexpr std::int32_t kRegistDlgId     = 562;  // was MP_REGISTDLG
    static constexpr std::int32_t kMugongIconId    = 563;  // was MP_RMUGONGICON
    static constexpr std::int32_t kMugongInfoId    = 564;  // was MP_RMUGONGINFO
    static constexpr std::int32_t kPracticeInfoId  = 565;  // was MP_RPRACTICEINFO
    static constexpr std::int32_t kFeeId           = 566;  // was MP_RFEE
    static constexpr std::int32_t kOkBtnId         = 567;  // was MP_ROKBTN
    static constexpr std::int32_t kCancelBtnId     = 568;  // was MP_RCANCELBTN

    // 1:1 quirk: legacy CHATMGR->GetChatMsg(660) +
    // CHATMGR->GetChatMsg(661) + CHATMGR->GetChatMsg(662).
    // The actual msg strings are localized. The modern
    // port uses placeholder text + format strings until
    // CHATMGR is ported.
    static constexpr int kSuryunMugongInfoChatMsgId   = 661;
    static constexpr int kPracticeInfoChatMsgId        = 660;
    static constexpr int kClearInfoChatMsgId           = 662;

private:
    cTextArea*  m_MugongInfo    = nullptr;  // MP_RMUGONGINFO
    cTextArea*  m_PracticeInfo  = nullptr;  // MP_RPRACTICEINFO
    cStatic*    m_Fee           = nullptr;  // MP_RFEE
    cIconDialog* m_pMugongIconDlg = nullptr;  // MP_RMUGONGICON (sub-dialog of this)
};

}  // namespace mxh::ui
