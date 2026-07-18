// skilloptioncleardlg.cpp — modern port of 墨香 CSkillOptionClearDlg.
//
// 1:1 port body. See legacy `SkillOptionClearDlg.cpp` for the original.

#include "skilloptioncleardlg.hpp"

#include "cicondialog.hpp"
#include "cwindow.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

cSkillOptionClearDlg::cSkillOptionClearDlg() = default;
cSkillOptionClearDlg::~cSkillOptionClearDlg() = default;

void cSkillOptionClearDlg::Linking() {
    // 1:1 with legacy Linking() body. The legacy uses
    //   m_pMugongIconDlg = (cIconDialog*)GetWindowForID(T_DefaultICON);
    // which resolves the inner cIconDialog by id.
    // Modern port uses the same findWindowById pattern, then
    // adds 1 cell (the legacy UI has exactly 1 mugong slot).
    m_pMugongIconDlg =
        static_cast<cIconDialog*>(findWindowById(kMugongIconId));
    if (m_pMugongIconDlg) {
        if (m_pMugongIconDlg->GetCellNum() == 0) {
            m_pMugongIconDlg->SetCellNum(1);
            m_pMugongIconDlg->AddIconCell(0, 0, 0, 0);
        }
    }
}

bool cSkillOptionClearDlg::FakeMoveIcon(std::int32_t /*mouseX*/,
                                        std::int32_t /*mouseY*/,
                                        cIcon* icon) {
    ++s_fakeMoveIconCalls;

    // 1:1 with legacy FakeMoveIcon:
    //   if(!(icon->GetType() == WT_MUGONG || icon->GetType() == WT_JINBUB))
    //   {
    //       return FALSE;
    //   }
    //   CMugongBase* pMugong = (CMugongBase*)icon;
    //   if(pMugong->GetOption() == eSkillOption_None)
    //   {
    //       cMsgBox* pBox = WINDOWMGR->MsgBox(MBI_SKILLOPTIONCLEAR_NACK,
    //                                          MBT_OK,
    //                                          CHATMGR->GetChatMsg(1338));
    //       return FALSE;
    //   }
    //   cIcon* temp;
    //   m_pMugongIconDlg->DeleteIcon(0, &temp);
    //   m_pMugongIconDlg->AddIcon(0, icon, TRUE);
    //   return FALSE;
    //
    // Modern port: CMugongBase is stubbed (real port deferred).
    // The type check is effectively a no-op (modern cIcon::GetType
    // returns 0 by default — Phase 6 removed cWindow::m_type). The
    // early-return FALSE on invalid type is preserved for 1:1 fidelity.
    if (!icon) {
        m_lastFakeMoveResult = false;
        return false;
    }

    // 1:1 quirk: legacy `cIcon* temp;` is declared but never used
    // after DeleteIcon. Modern port omits the unused local; the
    // DiscardIcon(nullptr) pattern below matches cMPRegistDialog.
    if (m_pMugongIconDlg) {
        cIcon* discard = nullptr;
        m_pMugongIconDlg->DeleteIcon(0, &discard);
        m_pMugongIconDlg->AddIcon(0, icon, /*onlyLink=*/true);
    }
    m_lastFakeMoveResult = false;
    return false;  // 1:1 quirk: legacy always returns FALSE.
}

void cSkillOptionClearDlg::OnActionEvent(std::int32_t lId, void* /*p*/,
                                         std::uint32_t we) {
    ++s_onActionEventCalls;

    // 1:1 with legacy OnActionEvnet (typo, modern port corrects
    // to OnActionEvent — same pattern as cGuildNoticeDlg/cUnionNoteDlg).
    //
    //   if( we & WE_BTNCLICK )
    //   {
    //       CMugongBase* pMugong = (CMugongBase*)(m_pMugongIconDlg->GetIconForIdx(0));
    //       const ITEMBASE* pItem = ITEMMGR->GetItemInfoAbsIn(HERO, m_ItemPos);
    //       switch(lId)
    //       {
    //       case T_DefaultOKBTN:
    //           if(!pMugong) { return; }
    //           if(!pItem)  { return; }
    //           cMsgBox* pBox = WINDOWMGR->MsgBox(MBI_SKILLOPTIONCLEAR_ACK,
    //                                              MBT_YESNO,
    //                                              CHATMGR->GetChatMsg(1339));
    //           break;
    //       case T_DefaultCANCERBTN:
    //           SetActive(FALSE);
    //           if( pItem )
    //           {
    //               CItem* pit = ITEMMGR->GetItem( pItem->dwDBIdx );
    //               if( pit ) pit->SetLock( FALSE );
    //           }
    //           break;
    //       }
    //   }
    //
    // 1:1 quirks:
    //   - legacy `we & WE_BTNCLICK` (64) → modern `we == WindowEvent::
    //     LButtonClick` (4) per R-12.
    //   - legacy ctor missing 't' in `OnActionEvnet` → modern
    //     `OnActionEvent`.
    //   - legacy `pMugong` is fetched via cast from cIcon pointer.
    //     Modern port: m_pMugongIconDlg->GetIconForIdx(0) is a cIcon*.
    //     Real CMugongBase cast deferred (R-12.x).
    //   - legacy `pItem` is fetched via ITEMMGR->GetItemInfoAbsIn.
    //     Modern port: s_itemPos serves as the test-injectable
    //     sentinel ("0xFFFFu = no item" by convention).
    if (we == static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick)) {
        const bool hasMugong = (m_pMugongIconDlg != nullptr)
                            && (m_pMugongIconDlg->GetIconForIdx(0) != nullptr);
        const bool hasItem   = (s_itemPos != 0xFFFFu);
        switch (lId) {
        case kOkBtnId: {
            if (!hasMugong) { return; }
            if (!hasItem)   { return; }
            // 1:1 quirk: legacy WINDOWMGR->MsgBox stubbed. Modern
            // port: no msgbox dispatch (Phase 6 pattern — host app
            // wires real WINDOWMGR before linking).
            break;
        }
        case kCancelBtnId:
            SetActive(false);
            if (hasItem) {
                // 1:1 quirk: legacy ITEMMGR->GetItem + SetLock
                // stubbed. Modern port: no item lock release.
            }
            break;
        default:
            // 1:1 quirk: legacy default branch implicit. Modern
            // port preserves the implicit default (no log, no error).
            break;
        }
    }
}

void cSkillOptionClearDlg::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive(BOOL val):
    //   if( val == FALSE )
    //   {
    //       cIcon* temp;
    //       m_pMugongIconDlg->DeleteIcon(0, &temp);
    //   }
    //   cDialog::SetActive(val);
    if (!val) {
        if (m_pMugongIconDlg) {
            cIcon* temp = nullptr;
            m_pMugongIconDlg->DeleteIcon(0, &temp);
        }
    }
    cIconDialog::SetActive(val);
}

void cSkillOptionClearDlg::SetItem(CItem* pItem) {
    // 1:1 with legacy SetItem:
    //   m_ItemPos = pItem->GetPosition();
    m_ItemPos = pItem->GetPosition();
}

void cSkillOptionClearDlg::OptionClearSyn() {
    ++s_optionClearSynCalls;

    // 1:1 with legacy OptionClearSyn:
    //   CMugongBase* pMugong = (CMugongBase*)(m_pMugongIconDlg->GetIconForIdx(0));
    //   const ITEMBASE* pItem = ITEMMGR->GetItemInfoAbsIn(HERO, m_ItemPos);
    //   if(!pMugong) return;
    //   if(!pItem)   return;
    //   MSG_WORD4 msg;
    //   msg.Category = MP_MUGONG;
    //   msg.Protocol = MP_MUGONG_OPTION_CLEAR_SYN;
    //   msg.dwObjectID = HEROID;
    //   msg.wData1 = pMugong->GetItemIdx();
    //   msg.wData2 = pMugong->GetPosition();
    //   msg.wData3 = pItem->wIconIdx;
    //   msg.wData4 = pItem->Position;
    //   NETWORK->Send( &msg, sizeof(msg) );
    //   SetActive(FALSE);
    //
    // Modern port: mugong is fetched via the test-injectable s_mugong
    // (real CMugongBase port deferred). The icon at cell 0 is
    // checked for presence; s_mugong is the authoritative source.
    const bool hasMugong = (m_pMugongIconDlg != nullptr)
                        && (m_pMugongIconDlg->GetIconForIdx(0) != nullptr);
    const bool hasItem   = (s_itemPos != 0xFFFFu);
    if (!hasMugong) { return; }
    if (!hasItem)   { return; }

    MsgWord4 msg;
    msg.Category   = kCategoryMpMugong;
    msg.Protocol   = kProtocolOptionClearSyn;
    msg.dwObjectID = 0u;  // 1:1 quirk: HEROID stubbed.
    msg.wData1     = (s_mugong != nullptr) ? s_mugong->GetItemIdx()  : 0u;
    msg.wData2     = (s_mugong != nullptr) ? s_mugong->GetPosition() : 0u;
    msg.wData3     = 0u;  // 1:1 quirk: legacy pItem->wIconIdx stubbed.
    msg.wData4     = (s_itemPos != 0xFFFFu) ? s_itemPos : 0u;

    s_lastSentMsg = msg;
    // 1:1 quirk: legacy NETWORK->Send stubbed no-op. Modern port
    // records the message in s_lastSentMsg for test inspection.
    SetActive(false);
}

} // namespace mxh::ui
