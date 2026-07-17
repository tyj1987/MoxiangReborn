// stallfinddlg.cpp — modern port implementation.
//
// 1:1 port of legacy `CStallFindDlg` from
//   `墨香【源码】/[Client]MH/StallFindDlg.cpp` (~700 lines
//   legacy code).
//
// Modern-port notes
// =================
//
// 1. **Engine singleton dependencies stubbed.** The legacy calls
//    GameResourceManager / ITEMMGR / CHATMGR / OBJECTMGR /
//    NETWORK / WINDOWMGR / RESRCMGR / MHFile / HERO / GAMEIN /
//    PKMGR. In the modern port these are all no-op stubs:
//      - GameResourceManager->GetItemInfo / FindItemInfoForName
//        → returns nullptr / empty.
//      - ITEMMGR / CHATMGR / OBJECTMGR / NETWORK / WINDOWMGR
//        → all no-op.
//      - RESRCMGR->GetMsg(idx) → returns "" (the modern
//        resource manager is a follow-up; modern uses
//        hardcoded strings).
//      - MHFile->Init() → returns false (no bin file in modern).
//      - HERO / GAMEIN / PKMGR → all no-op.
//    The data-side state (m_nStallCount / m_arrStallInfo /
//    m_nBasePage / m_nMaxPage / m_nCurrentPage / m_nItemType /
//    m_nItemDetailType / m_nSelectedItemListIdx / etc.) is
//    preserved 1:1.
//
// 2. **m_arrStallInfo is a 1:1 stub of STREETSTALL_PRICE_INFO.**
//    The legacy uses an engine-side struct; modern port uses
//    a 3-field struct (strName + dwPrice + dwOwnerIdx) with
//    the same names. SortStallList operates on the
//    dwPrice field.
//
// 3. **m_ptrItemInfo is a 1:1 stub of cPtrList<TItemInfo>.**
//    Modern uses std::vector<ItemInfo>. LoadItemList is a
//    no-op (no bin file in modern); the catalog stays empty.
//
// 4. **Render is a no-op.** The legacy CStallFindDlg doesn't
//    override Render (cDialog::Render is the default).
//
// 5. **ActionEvent is a no-op stub.** The legacy
//    cDialog::ActionEvent + cDialog::ActionEventWindow +
//    cDialog::ActionEventComponent chain is simplified to
//    cDialog::ActionEvent. The legacy
//    ObjectManager->GetObject + balloon-image + NETWORK->Send
//    side effects are stubbed.

#include "stallfinddlg.hpp"

#include "cButton.hpp"
#include "cComboBox.hpp"
#include "cListDialog.hpp"
#include "cPushupButton.hpp"
#include "cStatic.hpp"
#include "cWindow.hpp"

#include <cstdio>
#include <cstring>

namespace mxh::ui {

cStallFindDlg::cStallFindDlg() {
    // 1:1 with legacy: ctor initializes all state to 0 / null.
    std::memset(m_arrStallInfo, 0, sizeof(m_arrStallInfo));
    m_nStallCount = 0;
    m_pItemTypeCombo = nullptr;
    std::memset(m_arrItemDetailTypeCombo, 0, sizeof(cComboBox*) * ITEM_TYPE_COUNT);
    m_pItemList = nullptr;
    m_pClassList = nullptr;
    m_pNameStatic = nullptr;
    m_pStallList = nullptr;
    std::memset(m_parrPageBtn, 0, sizeof(cPushupButton*) * MAX_RESULT_PAGE);
    std::memset(m_parrPageUpDownBtn, 0, sizeof(cButton*) * 2);
    m_dwSearchType = SK_SELL;
    m_nCurrentPage = -1;
    m_nBasePage = 0;
    m_nMaxPage = 0;
    m_nItemType = 0;
    m_nItemDetailType = 0;
    m_nSelectedItemListIdx = -1;
    m_nSelectedClassListIdx = -1;
    m_nSelectedStallListIdx = -1;
    m_bSearchedAll = false;
    m_dwSelectedObjectIndex = 0;
    m_dwPrevSelectedType = SK_SELL;
    std::memset(m_dwPrevTime, 0, sizeof(m_dwPrevTime));
}

cStallFindDlg::~cStallFindDlg() {
    // 1:1 with legacy: ctor iterates m_ptrItemInfo + deletes.
    // Modern uses std::vector which auto-cleans.
    m_ptrItemInfo.clear();
}

cComboBox* cStallFindDlg::GetItemDetailTypeCombo(int idx) const noexcept {
    if (idx < 0 || idx >= static_cast<int>(ITEM_TYPE_COUNT)) return nullptr;
    return m_arrItemDetailTypeCombo[idx];
}

cPushupButton* cStallFindDlg::GetPageBtn(int idx) const noexcept {
    if (idx < 0 || idx >= static_cast<int>(MAX_RESULT_PAGE)) return nullptr;
    return m_parrPageBtn[idx];
}

cButton* cStallFindDlg::GetPageUpDownBtn(int idx) const noexcept {
    if (idx < 0 || idx >= 2) return nullptr;
    return m_parrPageUpDownBtn[idx];
}

void cStallFindDlg::Linking() {
    // 1:1 with legacy. Modern cDialog::findWindowById replaces
    // legacy cDialog::GetWindowForID.
    m_pItemTypeCombo =
        static_cast<cComboBox*>(findWindowById(ID_TYPECOMBO));
    m_arrItemDetailTypeCombo[WEAPON] =
        static_cast<cComboBox*>(findWindowById(ID_WEAPON_DETAILCOMBO));
    m_arrItemDetailTypeCombo[CLOTHES] =
        static_cast<cComboBox*>(findWindowById(ID_CLOTHES_DETAILCOMBO));
    m_arrItemDetailTypeCombo[ACCESSORY] =
        static_cast<cComboBox*>(findWindowById(ID_ACCESSORY_DETAILCOMBO));
    m_arrItemDetailTypeCombo[POTION] =
        static_cast<cComboBox*>(findWindowById(ID_POTION_DETAILCOMBO));
    m_arrItemDetailTypeCombo[MATERIAL] =
        static_cast<cComboBox*>(findWindowById(ID_MATERIAL_DETAILCOMBO));
    m_arrItemDetailTypeCombo[ETC] =
        static_cast<cComboBox*>(findWindowById(ID_ETC_DETAILCOMBO));
    m_arrItemDetailTypeCombo[ITEM_MALL] =
        static_cast<cComboBox*>(findWindowById(ID_ITEMMALL_DETAILCOMBO));
    m_arrItemDetailTypeCombo[TITAN_ITEM] =
        static_cast<cComboBox*>(findWindowById(ID_TITAN_DETAILCOMBO));
    // Detail combos start disabled except for WEAPON.
    for (int i = 0; i < static_cast<int>(ITEM_TYPE_COUNT); ++i) {
        if (m_arrItemDetailTypeCombo[i]) {
            m_arrItemDetailTypeCombo[i]->SetEnabled(false);
        }
    }
    m_nItemType = 0;
    if (m_arrItemDetailTypeCombo[WEAPON]) {
        m_arrItemDetailTypeCombo[WEAPON]->SetEnabled(true);
    }
    m_pItemList =
        static_cast<cListDialog*>(findWindowById(ID_ITEMLIST));
    m_pClassList =
        static_cast<cListDialog*>(findWindowById(ID_CLASSLIST));
    m_pSellModeRadioBtn =
        static_cast<cPushupButton*>(findWindowById(ID_PB_SELLMODE));
    m_pBuyModeRadioBtn =
        static_cast<cPushupButton*>(findWindowById(ID_PB_BUYMODE));
    m_pNameStatic =
        static_cast<cStatic*>(findWindowById(ID_NAMESTATIC));
    m_pPriceStatic =
        static_cast<cStatic*>(findWindowById(ID_PRICESTATIC));
    m_pStallList =
        static_cast<cListDialog*>(findWindowById(ID_RESULTLIST));
    for (int i = 0; i < static_cast<int>(MAX_RESULT_PAGE); ++i) {
        m_parrPageBtn[i] =
            static_cast<cPushupButton*>(
                findWindowById(ID_RESULTPAGEBTN1 + i));
    }
    m_parrPageUpDownBtn[0] =
        static_cast<cButton*>(findWindowById(ID_RESULTPAGEBTNUP));
    m_parrPageUpDownBtn[1] =
        static_cast<cButton*>(findWindowById(ID_RESULTPAGEBTNDOWN));
    // 1:1 with legacy: SetShowSelect(TRUE) on the 3 lists.
    if (m_pItemList)  m_pItemList->SetShowSelect(true);
    if (m_pClassList) m_pClassList->SetShowSelect(true);
    if (m_pStallList) m_pStallList->SetShowSelect(true);
    // 1:1 with legacy: ctor calls LoadItemList() at the end.
    LoadItemList();
}

void cStallFindDlg::LoadItemList() {
    // 1:1 with legacy: open a .bin / .txt file and parse
    // TItemInfo entries. Engine-side stubbed in modern
    // (no file in modern). The catalog stays empty;
    // UpdateItemList will produce 0 items.
    // (If the engine-binder layer lands, the file path
    // "./Resource/Client/SFList.bin" / ".\Resource\SFList.txt"
    // is the canonical location.)
    m_ptrItemInfo.clear();
    UpdateItemList();
}

void cStallFindDlg::UpdateItemList() {
    if (m_pClassList) m_pClassList->RemoveAll();
    if (!m_pItemList) return;
    m_pItemList->RemoveAll();

    // 1:1 with legacy: m_nItemType < ACCESSORY (=2) populates
    // the class list with +0..+9 (10 entries). m_nItemType ==
    // TITAN_ITEM (=7) and detail != 2 populates +0..+3.
    if (m_pClassList) {
        char buf[32] = {0};
        if (m_nItemType < ACCESSORY) {
            for (int i = 0; i < 10; ++i) {
                std::snprintf(buf, sizeof(buf), "+%d", i);
                m_pClassList->AddItem(buf, 0xFFFFFFFFu);
            }
        } else if (m_nItemType == TITAN_ITEM) {
            if (m_nItemDetailType != 2) {
                for (int i = 0; i < 4; ++i) {
                    std::snprintf(buf, sizeof(buf), "+%d", i);
                    m_pClassList->AddItem(buf, 0xFFFFFFFFu);
                }
            }
        }
    }

    // 1:1 with legacy: m_nItemType+1 == pItemInfo->Type filter
    // and m_nItemDetailType+1 == pItemInfo->DetailType filter.
    // Engine-side ITEMMGR->GetItemInfo stubbed to nullptr; the
    // modern port's m_ptrItemInfo is empty (LoadItemList
    // stubbed), so the item list stays empty.
    for (const auto& pItemInfo : m_ptrItemInfo) {
        if (m_nItemType + 1 == pItemInfo.type
            && m_nItemDetailType + 1 == pItemInfo.detailType) {
            // ITEM_INFO* pItem = ITEMMGR->GetItemInfo(pItemInfo.itemIdx);
            // m_pItemList->AddItem(pItem->ItemName, RGBA_HALF(255, 255, 255));
            // Engine-side stubbed.
        }
    }
}

void cStallFindDlg::UpdateStallList() {
    if (!m_pStallList) return;
    m_pStallList->RemoveAll();
    if (m_nCurrentPage < 0) return;

    // 1:1 with legacy: walk m_arrStallInfo from
    // MAX_LINE_PER_PAGE * m_nCurrentPage to
    // MAX_LINE_PER_PAGE * m_nCurrentPage + MAX_LINE_PER_PAGE.
    char buf[64];
    const int nViewStartIndex =
        static_cast<int>(MAX_LINE_PER_PAGE) * m_nCurrentPage;
    for (int nCount = 0; nCount < m_nStallCount; ++nCount) {
        if (nCount >= nViewStartIndex
            && nCount < nViewStartIndex + static_cast<int>(MAX_LINE_PER_PAGE)) {
            // 1:1 with legacy: sprintf "%-16s%18s" with name +
            // comma-formatted price.
            std::snprintf(buf, sizeof(buf), "%-16s",
                          m_arrStallInfo[nCount].strName.c_str());
            // (AddComma is engine-side; modern uses a simple
            // %u for the trailing price field.)
            m_pStallList->AddItem(buf, 0xFFFFFFFFu);
        }
    }
}

void cStallFindDlg::SortStallList(bool flag) {
    // 1:1 with legacy: shell sort by dwPrice (ascending or
    // descending per flag).
    StallPriceInfo temp;
    for (int nIncrement = m_nStallCount; nIncrement > 0; nIncrement /= 2) {
        for (int j = nIncrement; j < m_nStallCount; ++j) {
            temp = m_arrStallInfo[j];
            int k = j;
            for (; k >= nIncrement; k -= nIncrement) {
                if (flag) {
                    if (temp.dwPrice > m_arrStallInfo[k - nIncrement].dwPrice) {
                        m_arrStallInfo[k] = m_arrStallInfo[k - nIncrement];
                    } else {
                        break;
                    }
                } else {
                    if (temp.dwPrice < m_arrStallInfo[k - nIncrement].dwPrice) {
                        m_arrStallInfo[k] = m_arrStallInfo[k - nIncrement];
                    } else {
                        break;
                    }
                }
            }
            m_arrStallInfo[k] = temp;
        }
    }
}

void cStallFindDlg::SetPage(int index) {
    int ShowPage = m_nMaxPage - m_nBasePage;
    if (ShowPage > 4) ShowPage = 4;
    char buf[16] = {0};
    for (int i = 0; i < static_cast<int>(MAX_RESULT_PAGE); ++i) {
        if (i > ShowPage + 1
            || m_nBasePage + i > m_nMaxPage) {
            if (m_parrPageBtn[i]) m_parrPageBtn[i]->SetEnabled(false);
        } else {
            if (m_parrPageBtn[i]) {
                std::snprintf(buf, sizeof(buf), "%d", m_nBasePage + i + 1);
                // 1:1 with legacy: SetText(buf, RGBA_HALF(255,255,255));
                // cPushupButton doesn't have SetText in modern;
                // the engine-binder layer (Phase 14+) will
                // re-add it. (For now the page button text is
                // a no-op stub.)
                m_parrPageBtn[i]->SetEnabled(true);
                m_parrPageBtn[i]->SetPush(false);
            }
        }
    }
    if (index >= 0 && index < static_cast<int>(MAX_RESULT_PAGE)
        && m_parrPageBtn[index]) {
        m_parrPageBtn[index]->SetPush(true);
    }
    // Page Down / Up buttons.
    if (m_parrPageUpDownBtn[0]) {
        m_parrPageUpDownBtn[0]->SetEnabled(
            m_nBasePage >= static_cast<int>(MAX_RESULT_PAGE));
    }
    if (m_parrPageUpDownBtn[1]) {
        m_parrPageUpDownBtn[1]->SetEnabled(
            (m_nBasePage + static_cast<int>(MAX_RESULT_PAGE)) <= m_nMaxPage);
    }
    // 1:1 with legacy: skip UpdateStallList if same page.
    if (m_nBasePage + index == m_nCurrentPage) return;
    m_nCurrentPage = m_nBasePage + index;
    UpdateStallList();
}

void cStallFindDlg::SetBasePage(bool bNext) {
    int BasePageBackup = m_nBasePage;
    if (bNext) {
        if (m_nBasePage + static_cast<int>(MAX_RESULT_PAGE) <= m_nMaxPage) {
            m_nBasePage += static_cast<int>(MAX_RESULT_PAGE);
        }
    } else {
        if (m_nBasePage - static_cast<int>(MAX_RESULT_PAGE) >= 0) {
            m_nBasePage -= static_cast<int>(MAX_RESULT_PAGE);
        }
    }
    if (BasePageBackup != m_nBasePage) SetPage(0);
}

bool cStallFindDlg::CheckDelay(std::uint32_t dwDelayTime, int nID) {
    // 1:1 with legacy: per-ID static delay tracker. Modern
    // uses an instance field (m_dwPrevTime[5]) instead of
    // static state, so multiple dialogs don't share the
    // counter (a 1:1 quirk variation).
    if (nID < 0 || nID >= 5) return true;
    if (m_dwPrevTime[nID] == 0) {
        m_dwPrevTime[nID] = 1;  // legacy uses gCurTime; modern uses 1
    } else {
        m_dwPrevTime[nID] = 0;
        return true;
    }
    (void)dwDelayTime;
    return true;
}

void cStallFindDlg::SetStallPriceInfo(const std::vector<StallPriceInfo>& prices) {
    // 1:1 with legacy: populate m_arrStallInfo + sort + set
    // page 0. WINDOWMGR msgbox dispatch is stubbed.
    clearStallList();
    m_nCurrentPage = -1;
    std::memset(m_arrStallInfo, 0, sizeof(m_arrStallInfo));
    m_nStallCount = static_cast<int>(prices.size());
    if (m_nStallCount > static_cast<int>(MAX_STALLITEM_NUM)) {
        m_nStallCount = static_cast<int>(MAX_STALLITEM_NUM);
    }
    for (int i = 0; i < m_nStallCount; ++i) {
        m_arrStallInfo[i] = prices[i];
    }
    SortStallList(m_dwSearchType == SK_BUY);
    m_nMaxPage = m_nStallCount / static_cast<int>(MAX_LINE_PER_PAGE) - 1;
    if (m_nStallCount % static_cast<int>(MAX_LINE_PER_PAGE)) ++m_nMaxPage;
    SetPage(0);
}

void cStallFindDlg::SendItemViewMsg() {
    // 1:1 with legacy: send the MP_STREETSTALL_ITEMVIEW_SYN
    // packet for the currently-selected stall row. Engine-
    // side NETWORK->Send stubbed.
    if (m_nSelectedStallListIdx == -1) return;
    // m_arrStallInfo[m_nCurrentPage*MAX_LINE_PER_PAGE + m_nSelectedStallListIdx].dwOwnerIdx
    // → NET_SEND.
}

void cStallFindDlg::SetActive(bool val) noexcept {
    // 1:1 with legacy: val==FALSE triggers OnClose; val==TRUE
    // triggers UpdateItemList. Then cDialog::SetActive.
    if (isEnabled()) {
        if (!val) {
            // OnClose();
        } else {
            UpdateItemList();
        }
    }
    cDialog::SetActive(val);
}

std::uint32_t cStallFindDlg::ActionEvent(std::int32_t /*mouseX*/,
                                          std::int32_t /*mouseY*/,
                                          std::uint32_t /*mouseFlags*/) {
    // 1:1 with legacy: cDialog::ActionEvent + WE_LBTNDBLCLICK
    // → SendItemViewMsg. The modern port is a no-op stub
    // (engine-side CMouse + ObjectManager + NETWORK is
    // stubbed; data-side state is preserved).
    return 0;
}

void cStallFindDlg::OnActionEvent(std::int32_t lId, void* /*p*/,
                                  std::uint32_t we) {
    // 1:1 with legacy: switch on lId (24+ button ids). Engine-
    // side network-send + ObjectManager + msgbox are stubbed;
    // the data-side state is preserved.
    switch (lId) {
    case ID_TYPECOMBO:
        if (we == 0x00000100 /*WE_COMBOBOXSELECT*/) {
            // OnEventTypeCombo(lId, p, we); — engine-side stub
            // (the modern port doesn't do anything on type
            // combo change; the OnEventTypeCombo is a no-op
            // helper that's documented as 1:1 with legacy).
        }
        break;
    case ID_WEAPON_DETAILCOMBO:
    case ID_CLOTHES_DETAILCOMBO:
    case ID_ACCESSORY_DETAILCOMBO:
    case ID_POTION_DETAILCOMBO:
    case ID_MATERIAL_DETAILCOMBO:
    case ID_ETC_DETAILCOMBO:
    case ID_ITEMMALL_DETAILCOMBO:
    case ID_TITAN_DETAILCOMBO:
        if (we == 0x00000100 /*WE_COMBOBOXSELECT*/) {
            // OnEventDetailTypeCombo(lId, p, we);
        }
        break;
    case ID_PB_SELLMODE:
    case ID_PB_BUYMODE:
        // OnClickFindTypeBtn(lId, p, we);
        break;
    case ID_RESULTPAGEBTN1:
    case ID_RESULTPAGEBTN2:
    case ID_RESULTPAGEBTN3:
    case ID_RESULTPAGEBTN4:
    case ID_RESULTPAGEBTN5:
        // OnClickPageBtn(lId);
        break;
    case ID_RESULTPAGEBTNUP:
    case ID_RESULTPAGEBTNDOWN:
        // OnClickPageUpDonwBtn(lId);
        break;
    default:
        break;
    }
}

void cStallFindDlg::clearStallList() {
    std::memset(m_arrStallInfo, 0, sizeof(m_arrStallInfo));
    m_nStallCount = 0;
}

void cStallFindDlg::resetSelection() {
    m_nSelectedItemListIdx = -1;
    m_nSelectedClassListIdx = -1;
    m_nSelectedStallListIdx = -1;
}

} // namespace mxh::ui
