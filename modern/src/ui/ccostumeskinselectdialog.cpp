// ccostumeskinselectdialog.cpp — modern port of 墨香 CCostumeSkinSelectDialog.

#include "mxh/ui/ccostumeskinselectdialog.hpp"
#include "legacy_window_event.hpp"
#include "mxh/ui/clistdialog.hpp"
#include "mxh/ui/cPushupButton.hpp"

#include <cstring>
#include <utility>

namespace mxh::ui {

cCostumeSkinSelectDialog::cCostumeSkinSelectDialog() {
    m_pCostumeSkinListDlg = nullptr;
    for (int i = 0; i < static_cast<int>(CostumeSkinTab::Max); ++i) {
        m_pCostumTabBtn[i] = nullptr;
    }
    m_dwSelectIdx = 0;
    m_dwSkinDelayTime = 0;
    m_bSkinDelayResult = false;
    m_currentTab = CostumeSkinTab::Hat;
}

cCostumeSkinSelectDialog::~cCostumeSkinSelectDialog() {
    ClearLists();
}

void cCostumeSkinSelectDialog::ClearLists() {
    m_skinHat.clear();
    m_skinDress.clear();
    m_skinAccessory.clear();
}

const std::vector<SkinSelectItemInfo>&
cCostumeSkinSelectDialog::skinListForTest(CostumeSkinTab kind) const {
    switch (kind) {
        case CostumeSkinTab::Hat:       return m_skinHat;
        case CostumeSkinTab::Dress:     return m_skinDress;
        case CostumeSkinTab::Accessory: return m_skinAccessory;
        default:                        return m_skinHat;
    }
}

void cCostumeSkinSelectDialog::SetCostumeSkinDataForTest(
    CostumeSkinTab kind, const std::vector<SkinSelectItemInfo>& data) {
    auto& target = (kind == CostumeSkinTab::Hat)       ? m_skinHat
                 : (kind == CostumeSkinTab::Dress)     ? m_skinDress
                 :                                       m_skinAccessory;
    target = data;
}

void cCostumeSkinSelectDialog::Linking() {
    // 1:1 with legacy Linking.  The legacy walks the
    // WINDOW_ID tree to find cListDialog / cIconDialog /
    // cPushupButton children.  Modern port defers the
    // WINDOW_ID walk; tests inject via SetChildWindowsForTest.
    if (m_childWindows.listDlg) {
        m_pCostumeSkinListDlg = m_childWindows.listDlg;
    }
    for (int i = 0; i < static_cast<int>(CostumeSkinTab::Max); ++i) {
        if (m_childWindows.tabBtns[i]) {
            m_pCostumTabBtn[i] = m_childWindows.tabBtns[i];
        }
    }
}

void cCostumeSkinSelectDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive.  When activated, populates
    // the list with the live tab's skin info.
    cDialog::SetActive(val);
    if (val) {
        CostumeSkinListInfo(m_currentTab);
    }
}

std::uint32_t cCostumeSkinSelectDialog::ActionEvent(void* /*mouseInfo*/) {
    // 1:1 with legacy ActionEvent.  The legacy forwards to
    // cDialog::ActionEvent + handles list-row clicks.  The
    // modern port is a no-op (cMouse not ported).
    return 0;
}

bool cCostumeSkinSelectDialog::OnActionEvent(std::int32_t lId, void* /*p*/, std::uint32_t we) {
    // 1:1 with legacy OnActionEvent.  Routes:
    //   * tab btn click -> switch tab + SetCostumTabBtnFocus
    //   * list row click -> update m_dwSelectIdx
    constexpr std::uint32_t kBtnClick = legacy_window_event::kButtonClick;
    if (we & kBtnClick) {
        if (lId == kTabBtnHatId) {
            m_currentTab = CostumeSkinTab::Hat;
            SetCostumTabBtnFocus(CostumeSkinTab::Hat);
            CostumeSkinListInfo(CostumeSkinTab::Hat);
            return true;
        } else if (lId == kTabBtnDressId) {
            m_currentTab = CostumeSkinTab::Dress;
            SetCostumTabBtnFocus(CostumeSkinTab::Dress);
            CostumeSkinListInfo(CostumeSkinTab::Dress);
            return true;
        } else if (lId == kTabBtnAccessoryId) {
            m_currentTab = CostumeSkinTab::Accessory;
            SetCostumTabBtnFocus(CostumeSkinTab::Accessory);
            CostumeSkinListInfo(CostumeSkinTab::Accessory);
            return true;
        }
    }
    return false;
}

void cCostumeSkinSelectDialog::CostumeSkinKindData() {
    // 1:1 with legacy CostumeSkinKindData.  The legacy reads
    // GAMERESRCMNGR->m_Hat / m_Dress / m_Accessory hash
    // tables.  The modern port is a no-op; the host
    // populates m_skinHat / m_skinDress / m_skinAccessory
    // via SetCostumeSkinDataForTest.
}

void cCostumeSkinSelectDialog::CostumeSkinListInfo(CostumeSkinTab kind) {
    // 1:1 with legacy CostumeSkinListInfo.  Clears the
    // cListDialog + adds an entry per item in the matching
    // vector.
    if (m_listRemoveAllCb) m_listRemoveAllCb(m_listRemoveAllUser);
    if (m_listAddItemCb) {
        const auto& list = skinListForTest(kind);
        for (const auto& info : list) {
            m_listAddItemCb(&info, m_listAddItemUser);
        }
    }
    m_currentTab = kind;
}

const SkinSelectItemInfo*
cCostumeSkinSelectDialog::GetCurrentSkinInfo(std::uint32_t dwSelectIdx) const {
    // 1:1 with legacy GetCurrentSkinInfo.  Returns the
    // dwSelectIdx-th entry in the current tab's list, or
    // nullptr if out of range.
    const auto& list = skinListForTest(m_currentTab);
    if (dwSelectIdx >= list.size()) return nullptr;
    return &list[dwSelectIdx];
}

void cCostumeSkinSelectDialog::SetCostumTabBtnFocus(CostumeSkinTab kind) {
    // 1:1 with legacy SetCostumTabBtnFocus.  Pushes the
    // matching tab btn + unpushes the others.
    if (m_tabBtnFocusCb) {
        m_tabBtnFocusCb(kind, m_tabBtnFocusUser);
    }
    m_currentTab = kind;
}

}  // namespace mxh::ui
