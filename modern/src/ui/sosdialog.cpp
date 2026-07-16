// sosdialog.cpp — 1:1 port of 墨香 CSOSDlg (guild SOS
// dialog). See sosdialog.hpp for the data-model rationale
// + 1:1 quirks.

#include "sosdialog.hpp"
#include "clistdialog.hpp"
#include "cbutton.hpp"

namespace mxh::ui {

cSOSDialog::cSOSDialog() = default;

cSOSDialog::~cSOSDialog() {
    // 1:1 with legacy CSOSDlg::~CSOSDlg (m_pListDlg->RemoveAll()).
    // The legacy unconditionally dereferences; the modern
    // port is null-checked (1:1 quirk: nil-deref if Linking
    // never resolved m_pListDlg; modern port is more
    // defensive).
    if (m_pListDlg) m_pListDlg->RemoveAll();
}

void cSOSDialog::Linking() {
    // 1:1 with legacy CSOSDlg::Linking. REAL — no
    // singleton, all widget operations. Defensive
    // null-checks (the legacy unconditionally
    // dereferences m_pListDlg in SetShowSelect /
    // SetHeight).
    m_pListDlg  = static_cast<cListDialog*>(findWindowById(kMemberListId));
    m_pSOSOkBtn = static_cast<cButton*>(findWindowById(kOkBtnId));

    if (m_pListDlg) {
        m_pListDlg->SetShowSelect(true);
        // 1:1 quirk: legacy calls
        //   m_pListDlg->SetHeight(158)
        // to size the list to 158 px tall. The modern
        // cListDialog / cDialog API doesn't expose
        // SetHeight — cListDialog sizes itself via
        // InitList(clipH) and cDialog doesn't have a
        // SetHeight method (size is fixed at Init
        // time). The modern port drops the SetHeight
        // call (1:1 with the spirit of the legacy
        // behavior — the size is configured at Init
        // time, not in Linking). The cListDialog
        // member retains whatever height it was
        // Init'd with.
    }
}

void cSOSDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CSOSDlg::SetActive. The legacy is:
    //   SOSMemberInfo();
    //   cDialog::SetActive(val);
    //   if (!val) {
    //       MSGBASE msg;
    //       SetProtocol(&msg, MP_GUILD, MP_GUILD_SOS_SEND_CANCEL);
    //       msg.dwObjectID = HEROID;
    //       NETWORK->Send(&msg, sizeof(MSGBASE));
    //   }
    //
    // The modern port calls the base SetActive (so the
    // dialog becomes active/inactive correctly) but the
    // SOSMemberInfo fetch + cancel send are no-op stubs
    // until GUILDMGR + HEROID + NETWORK singletons are
    // ported. See the TODO below for the exact dispatch
    // logic.
    cDialog::SetActive(val);
    if (val) {
        // TODO: SOSMemberInfo() — fetch GUILDMGR member
        //       list + populate m_pListDlg. The body
        //       depends on GUILDMGR (Tier 3 port) +
        //       GUILDMEMBERINFO (game_types port) +
        //       cPtrList (legacy collection type, not
        //       ported).
    } else {
        // TODO: send MP_GUILD_SOS_SEND_CANCEL via NETWORK.
    }
}

std::uint32_t cSOSDialog::ActionEvent(std::int32_t mouseX,
                                      std::int32_t mouseY,
                                      std::uint32_t mouseFlags) {
    // 1:1 with legacy CSOSDlg::ActionEvent. The legacy
    // is:
    //   DWORD we = WE_NULL;
    //   if (!m_bActive) return we;
    //   we = cDialog::ActionEvent(mouseInfo);
    //   if (m_pListDlg->PtIdxInRow(x, y) != -1) {
    //       if (we & WE_LBTNCLICK) {
    //           if (WINDOWMGR->IsMouseDownUsed() == FALSE) {
    //               int Idx = m_pListDlg->GetCurSelectedRowIdx();
    //               if (Idx != -1) m_dwSelectIdx = Idx;
    //           }
    //       }
    //   }
    //   return we;
    //
    // The modern cDialog::ActionEvent already returns
    // the base we. The row-click tracking is deferred
    // until cListDialog::PtIdxInRow is fully ported
    // (it's in the header but the modern test surface
    // doesn't exercise it). The modern port just calls
    // the base ActionEvent.
    if (!isActive()) return 0;
    std::uint32_t we = cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
    // TODO: track m_dwSelectIdx from m_pListDlg
    //       row click (depends on cListDialog row API).
    return we;
}

void cSOSDialog::SOSMemberInfo() {
    // 1:1 with legacy CSOSDlg::SOSMemberInfo. The legacy
    // is:
    //   m_pListDlg->RemoveAll();
    //   cPtrList* pList = GUILDMGR->GetGuild()->GetMemberList();
    //   PTRLISTPOS pos = pList->GetHeadPosition();
    //   GUILDMEMBERINFO* pInfo;
    //   while (pos) {
    //       pInfo = (GUILDMEMBERINFO*)pList->GetNext(pos);
    //       if (pInfo) {
    //           // format name + rank + level, color by bLogged
    //           if (pInfo->bLogged) m_pListDlg->AddItem(buf, 0xffffffff);
    //           else                m_pListDlg->AddItem(buf, RGBA_MAKE(172,182,199,255));
    //       }
    //   }
    //
    // The modern port is a no-op until GUILDMGR +
    // GUILDMEMBERINFO + cPtrList are ported. The
    // m_pListDlg->RemoveAll() / AddItem() surface
    // (cListDialog API) is already in place.
    if (m_pListDlg) m_pListDlg->RemoveAll();
    // TODO: fetch GUILDMGR member list + populate
    //       m_pListDlg with formatted "name rank level"
    //       rows.
}

void cSOSDialog::OnActionEvent(std::int32_t lId, void* /*p*/,
                               std::uint32_t /*we*/) {
    // 1:1 with legacy CSOSDlg::OnActionEvent. The legacy
    // handles SOS_OKBTN:
    //   case SOS_OKBTN: {
    //       cPtrList* pList = GUILDMGR->GetGuild()->GetMemberList();
    //       PTRLISTPOS pos = pList->FindIndex(m_dwSelectIdx);
    //       if (pos) {
    //           GUILDMEMBERINFO* pInfo = (GUILDMEMBERINFO*)pList->GetAt(pos);
    //           if (pInfo->MemberIdx == HEROID) {
    //               CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1631));
    //               break;
    //           }
    //           if (pInfo->bLogged == FALSE) {
    //               CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1632));
    //               break;
    //           }
    //           MSG_DWORD4 msg;
    //           SetProtocol(&msg, MP_GUILD, MP_GUILD_SOS_SEND_SYN);
    //           msg.dwObjectID = HEROID;
    //           msg.dwData1 = pInfo->MemberIdx;
    //           msg.dwData2 = MAP->GetMapNum();
    //           VECTOR3 pos = HERO->GetCurPosition();
    //           stMOVEPOINT stMovePoint;
    //           stMovePoint.SetMovePoint((WORD)pos.x, (WORD)pos.z);
    //           msg.dwData3 = stMovePoint.value;
    //           msg.dwData4 = gChannelNum;
    //           NETWORK->Send(&msg, sizeof(MSG_DWORD4));
    //       }
    //   } break;
    //
    // The modern port is a no-op until GUILDMGR + HEROID
    // + MAP + CHATMGR + NETWORK singletons are ported.
    (void)lId;
    // TODO: dispatch to GUILDMGR + HEROID + MAP + CHATMGR
    //       + NETWORK once those singletons are ported.
}

}  // namespace mxh::ui
