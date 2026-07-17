// mpregistdialog.cpp — 1:1 port of 墨香 CMPRegistDialog
// (MP practice registration dialog). See mpregistdialog.hpp
// for the data-model rationale + 1:1 quirks.

#include "mpregistdialog.hpp"

#include "ctextarea.hpp"
#include "cstatic.hpp"
#include "cicondialog.hpp"

#include <cstdio>
#include <cstring>

namespace mxh::ui {

// Forward declarations for non-ported singletons. Each
// is a TODO marker; the real impl will replace these
// forward decls with the actual port when SURYUNMGR /
// CHATMGR / WINDOWMGR / OBJECTSTATEMGR / HERO get their
// modern equivalents.
class CMugongBase;
class CSkillInfo;

cMPRegistDialog::cMPRegistDialog() = default;

cMPRegistDialog::~cMPRegistDialog() = default;

void cMPRegistDialog::Linking() {
    // 1:1 with legacy CMPRegistDialog::Linking. REAL —
    // resolves 4 children and stores them as members.
    // The legacy also calls
    //   m_pMugongIconDlg->SetDragOverIconType(WT_MUGONG)
    // which is a 1:1 quirk drop in the modern port
    // (modern cIconDialog has no such API; cPetWearedExDialog
    // and cWearedExDialog also drop this at port time).
    m_MugongInfo    = static_cast<cTextArea*>(findWindowById(kMugongInfoId));
    m_PracticeInfo  = static_cast<cTextArea*>(findWindowById(kPracticeInfoId));
    m_Fee           = static_cast<cStatic*>(findWindowById(kFeeId));
    m_pMugongIconDlg = static_cast<cIconDialog*>(findWindowById(kMugongIconId));

    // AddIconCell(0, 0, default_w, default_h) to make
    // cell 0 addable. The legacy cIconDialog implicitly
    // configures a single full-size cell via the
    // resource loader; the modern port must do it
    // explicitly because there's no resource loader
    // hook in the modern UI framework.
    if (m_pMugongIconDlg) {
        if (m_pMugongIconDlg->GetCellNum() == 0) {
            m_pMugongIconDlg->SetCellNum(1);
            m_pMugongIconDlg->AddIconCell(0, 0, 0, 0);
        }
    }
}

void cMPRegistDialog::SetActive(bool val) noexcept {
    if (!val) {
        // 1:1 quirk: legacy resets 4 things on
        // val == FALSE, then calls base SetActive.
        if (m_MugongInfo) {
            // 1:1 quirk: legacy uses
            //   m_MugongInfo->SetScriptText(CHATMGR->GetChatMsg(662))
            // Modern port uses a placeholder string.
            m_MugongInfo->SetScriptText("MP_REGIST_CLEAR");
        }
        if (m_pMugongIconDlg) {
            cIcon* pIcon = nullptr;
            m_pMugongIconDlg->DeleteIcon(0, &pIcon);
        }
        if (m_PracticeInfo) {
            m_PracticeInfo->SetScriptText("");
        }
        if (m_Fee) {
            m_Fee->SetStaticValue(0);
        }
        // 1:1 quirk: legacy dismisses MBI_MPNOTICE_NOTFIT
        // msgbox via WINDOWMGR. WINDOWMGR is not yet
        // ported; modern port is a TODO marker. When
        // WINDOWMGR is ported, this becomes:
        //   if (auto* pMsgBox = WINDOWMGR->GetWindowForID(MBI_MPNOTICE_NOTFIT)) {
        //       pMsgBox->SetActive(FALSE);
        //   }
        // 1:1 quirk: legacy ends OBJECTSTATEMGR
        // eObjectState_Deal. OBJECTSTATEMGR is not
        // yet ported; modern port is a TODO marker.
        // When OBJECTSTATEMGR is ported, this becomes:
        //   OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    }
    cDialog::SetActive(val);
}

bool cMPRegistDialog::FakeMoveIcon(std::int32_t /*mouseX*/,
                                    std::int32_t /*mouseY*/,
                                    cIcon* /*icon*/) {
    // 1:1 with legacy CMPRegistDialog::FakeMoveIcon.
    // The legacy body has 14 lines of singleton-dispatch
    // logic against SURYUNMGR / HERO / WINDOWMGR /
    // CHATMGR / OBJECTSTATEMGR + reads
    // CMugongBase::GetSung / GetItemIdx / m_pSkillInfo
    // + cSkillInfo::GetSkillKind / GetWeaponType. None
    // of these are ported yet. The modern port returns
    // FALSE unconditionally (matching the legacy's
    // "invalid drop → return FALSE" branch at the very
    // end of the body), with a TODO marker explaining
    // what needs to be ported for the real impl.
    //
    // When CMugongBase + cSkillInfo + the 5 singletons
    // get their modern equivalents, this becomes:
    //   if (!icon) return FALSE;
    //   if (icon->GetType() != WT_MUGONG && icon->GetType() != WT_JINBUB) return FALSE;
    //   auto* pMugong = static_cast<CMugongBase*>(icon);
    //   BYTE sung = pMugong->GetSung();
    //   if (!SURYUNMGR->NeedSuryun(pMugong->GetItemIdx(), sung, pMugong->GetExpPoint())) return FALSE;
    //   if (pMugong->m_pSkillInfo->GetSkillKind() == SKILLKIND_TITAN) {
    //       WINDOWMGR->MsgBox(MBI_MPNOTICE_NOTFIT, MBT_OK, CHATMGR->GetChatMsg(1659));
    //       return FALSE;
    //   }
    //   if (HERO->GetWeaponEquipType() != pMugong->m_pSkillInfo->GetWeaponType() &&
    //       pMugong->m_pSkillInfo->GetSkillKind() == SKILLKIND_OUTERMUGONG) {
    //       WINDOWMGR->MsgBox(MBI_MPNOTICE_NOTFIT, MBT_OK, CHATMGR->GetChatMsg(151));
    //       return FALSE;
    //   }
    //   auto* pInfo = SURYUNMGR->GetMissionInfo(pMugong->GetItemIdx());
    //   if (!pInfo) return FALSE;
    //   auto* pSInfo = pInfo->GetSuryunInfo(sung);
    //   SetSuryunMugongInfo(pMugong->m_pSkillInfo->GetSkillName(), sung);
    //   SetPracticeInfo(pSInfo->AimSung, pSInfo->LimitTime, pSInfo->MonKind, pSInfo->MonNum,
    //                    SURYUNMGR->GetSuryunFee(pSInfo));
    //   AddLink(icon);
    //   return FALSE;  // legacy always returns FALSE
    return false;
}

void cMPRegistDialog::SetSuryunMugongInfo(const char* mugongName,
                                           std::uint8_t sung) {
    // 1:1 with legacy SetSuryunMugongInfo. The legacy
    // sprintf's a CHATMGR-formatted string with
    // mugongName + sung, then SetScriptText's it on
    // m_MugongInfo. The modern port uses a placeholder
    // format string to keep the sprintf signature
    // exercised end-to-end without depending on CHATMGR.
    if (!m_MugongInfo) return;
    char buf[128];
    std::snprintf(buf, sizeof(buf),
                  "Mugong: %s (Sung %u)",
                  mugongName ? mugongName : "(null)",
                  static_cast<unsigned>(sung));
    m_MugongInfo->SetScriptText(buf);
}

void cMPRegistDialog::SetPracticeInfo(std::uint8_t sung,
                                       std::uint32_t limitime,
                                       int kind, int num,
                                       std::uint64_t fee) {
    // 1:1 with legacy SetPracticeInfo. Computes
    // LTime = limitime / 60000 (ms → min), sprintf's
    // a CHATMGR-formatted string with all 4 fields, then
    // SetScriptText's it on m_PracticeInfo and
    // SetStaticValue's on m_Fee.
    if (!m_PracticeInfo) return;
    const std::uint32_t lTime = limitime / 60000u;
    char buf[128];
    std::snprintf(buf, sizeof(buf),
                  "Practice: Sung %u, %u min, Kind %d, Num %d",
                  static_cast<unsigned>(sung),
                  static_cast<unsigned>(lTime),
                  kind, num);
    m_PracticeInfo->SetScriptText(buf);
    if (m_Fee) {
        m_Fee->SetStaticValue(static_cast<std::int32_t>(fee));
    }
}

void cMPRegistDialog::AddLink(cIcon* picon) {
    if (!m_pMugongIconDlg) return;
    // 1:1 with legacy AddLink. If cell 0 is not
    // addable (occupied), DeleteIcon(0) first. Then
    // AddIcon(0, picon, TRUE) (legacy bOnlyLink=TRUE).
    if (!m_pMugongIconDlg->IsAddable(0)) {
        m_pMugongIconDlg->DeleteIcon(0);
    }
    m_pMugongIconDlg->AddIcon(0, picon, /*onlyLink=*/true);
}

class CMugongBase* cMPRegistDialog::GetMugong() const {
    // 1:1 with legacy GetMugong. Casts cell 0 to
    // CMugongBase*. CMugongBase is not yet ported
    // (R-12.x deferred, same constraint as
    // cPetWearedExDialog::CheckDuplication and
    // cWearedExDialog's Titan-vs-normal branch), so
    // the modern port returns nullptr unconditionally
    // with a TODO marker. When CMugongBase is ported,
    // this becomes:
    //   if (!m_pMugongIconDlg) return nullptr;
    //   cIcon* pIcon = m_pMugongIconDlg->GetIconForIdx(0);
    //   if (!pIcon) return nullptr;
    //   return static_cast<CMugongBase*>(pIcon);
    return nullptr;
}

}  // namespace mxh::ui
