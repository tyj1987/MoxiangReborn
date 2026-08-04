// sosdialog.cpp — 1:1 port of 墨香 CSOSDlg (guild SOS
// dialog). See sosdialog.hpp for the data-model rationale
// + 1:1 quirks.

#include "sosdialog.hpp"
#include "clistdialog.hpp"
#include "cbutton.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

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

void cSOSDialog::SetCallbacks(
    GetMemberCountFn getMemberCount, GetMemberFn getMember,
    GetHeroObjectIdFn getHeroObjectId, GetDwordFn getMapNum,
    GetDwordFn getChannelNum, GetPositionFn getHeroPosition,
    AddSystemMessageFn addSystemMessage, SendCancelFn sendCancel,
    SendSOSFn sendSOS, IsMouseDownUsedFn isMouseDownUsed,
    void* userData) noexcept {
    m_getMemberCount = getMemberCount;
    m_getMember = getMember;
    m_getHeroObjectId = getHeroObjectId;
    m_getMapNum = getMapNum;
    m_getChannelNum = getChannelNum;
    m_getHeroPosition = getHeroPosition;
    m_addSystemMessage = addSystemMessage;
    m_sendCancel = sendCancel;
    m_sendSOS = sendSOS;
    m_isMouseDownUsed = isMouseDownUsed;
    m_callbackUserData = userData;
}

void cSOSDialog::SetActive(bool val) noexcept {
    SOSMemberInfo();
    cDialog::SetActive(val);
    if (!val && m_getHeroObjectId && m_sendCancel) {
        m_sendCancel(m_getHeroObjectId(m_callbackUserData), m_callbackUserData);
    }
}

std::uint32_t cSOSDialog::ActionEvent(std::int32_t mouseX,
                                      std::int32_t mouseY,
                                      std::uint32_t mouseFlags) {
    if (!isActive()) return 0;
    const std::uint32_t we = cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
    if (m_pListDlg && m_pListDlg->PtIdxInRow(mouseX, mouseY) != -1 &&
        (we & kWeLeftButtonClick) != 0u &&
        (!m_isMouseDownUsed || !m_isMouseDownUsed(m_callbackUserData))) {
        const int selected = m_pListDlg->GetCurSelectedRowIdx();
        if (selected != -1) m_dwSelectIdx = static_cast<std::uint32_t>(selected);
    }
    return we;
}

void cSOSDialog::SOSMemberInfo() {
    if (!m_pListDlg) return;
    m_pListDlg->RemoveAll();
    if (!m_getMemberCount || !m_getMember) return;

    const auto count = m_getMemberCount(m_callbackUserData);
    for (std::size_t index = 0; index < count; ++index) {
        SOSGuildMember member;
        if (!m_getMember(index, &member, m_callbackUserData)) continue;
        char name[17]{};
        char rank[7]{};
        std::snprintf(name, sizeof(name), "%.16s", member.memberName ? member.memberName : "");
        std::snprintf(rank, sizeof(rank), "%.6s", member.rankName ? member.rankName : "");
        std::fill(name + std::strlen(name), name + 16, ' ');
        std::fill(rank + std::strlen(rank), rank + 6, ' ');
        char row[64]{};
        std::snprintf(row, sizeof(row), "%s %10s %4d", name, rank, member.level);
        m_pListDlg->AddItem(row, member.logged ? kOnlineColor : kOfflineColor);
    }
}

void cSOSDialog::OnActionEvent(std::int32_t lId, void* p,
                               std::uint32_t we) {
    (void)p;
    (void)we;
    if (lId != kOkBtnId || !m_getMember) return;

    SOSGuildMember member;
    if (!m_getMember(m_dwSelectIdx, &member, m_callbackUserData)) return;
    const auto heroObjectId = m_getHeroObjectId
        ? m_getHeroObjectId(m_callbackUserData)
        : 0u;
    if (member.memberIdx == heroObjectId) {
        if (m_addSystemMessage) m_addSystemMessage(kSelfTargetMessageId, m_callbackUserData);
        return;
    }
    if (!member.logged) {
        if (m_addSystemMessage) m_addSystemMessage(kOfflineTargetMessageId, m_callbackUserData);
        return;
    }
    if (!m_getMapNum || !m_getChannelNum || !m_getHeroPosition || !m_sendSOS) return;

    float x = 0.0f;
    float z = 0.0f;
    m_getHeroPosition(&x, &z, m_callbackUserData);
    const auto movePoint = static_cast<std::uint32_t>(static_cast<std::uint16_t>(x)) |
        (static_cast<std::uint32_t>(static_cast<std::uint16_t>(z)) << 16u);
    m_sendSOS(heroObjectId, member.memberIdx, m_getMapNum(m_callbackUserData),
              movePoint, m_getChannelNum(m_callbackUserData), m_callbackUserData);
}


}  // namespace mxh::ui
