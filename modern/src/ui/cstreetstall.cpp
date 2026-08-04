// cstreetstall.cpp -- modern implementation of
//   Moxiang CStreetStall (player stall dialog).

#include "cstreetstall.hpp"
#include "cicongriddialog.hpp"
#include "ceditbox.hpp"
#include "cbutton.hpp"

namespace mxh::ui {

cStreetStall::cStreetStall() {
    // 1:1 with legacy ctor:
    //   m_type = WT_STREETSTALLDIALOG (dropped,
    //     m_type removed in Phase 6);
    //   m_DlgState = eSDS_NOT_OPENED;
    //   memset(m_MoneyArray, 0, sizeof);
    //   m_nCurSelectedItem = -1;
    //   m_dwCurSelectedItemDBIdx = 0;
    //   m_pData = NULL.
    m_dlgState = StallDlgState::NotOpened;
    m_curSelectedItem = -1;
    m_curSelectedItemDBIdx = 0;
    m_data = nullptr;
}

cStreetStall::~cStreetStall() = default;

void cStreetStall::Linking() {
    // 1:1 with legacy Linking: resolve 6 children
    // via host-injected window resolver (R-12.x).
    auto resolve = [this](std::int32_t id) -> void* {
        return m_windowResolverCb
            ? m_windowResolverCb(id, m_windowResolverUser)
            : nullptr;
    };
    m_stallGrid  = static_cast<cIconGridDialog*>(resolve(kIdIconGrid));
    m_titleEdit  = static_cast<cEditBox*>(resolve(kIdTitleEdit));
    m_enterBtn   = static_cast<cButton*>(resolve(kIdEnter));
    m_buyBtn     = static_cast<cButton*>(resolve(kIdBuyBtn));
    m_editBtn    = static_cast<cButton*>(resolve(kIdEditBtn));
    m_moneyEdit  = static_cast<cEditBox*>(resolve(kIdMoneyEdit));
}

void cStreetStall::SetDisable(bool val) noexcept {
    // 1:1 with legacy SetDisable: forwards to
    // cDialog::SetDisable(val); if m_DlgState ==
    // eSDS_BUY then m_pEnterBtn->SetDisable(TRUE).
    cDialog::SetDisable(val);
    if (m_dlgState == StallDlgState::Buy && m_enterBtn) {
        m_enterBtn->SetDisable(true);
    }
}

void cStreetStall::ShowSellStall() {
    SetActive(true);
    m_dlgState = StallDlgState::Opened;
    if (m_showStallCb) m_showStallCb(StallDlgState::Opened, m_showStallUser);
}

void cStreetStall::ShowBuyStall() {
    SetActive(true);
    m_dlgState = StallDlgState::Buy;
    if (m_showStallCb) m_showStallCb(StallDlgState::Buy, m_showStallUser);
}

void cStreetStall::OnCloseStall(bool bDelOption) {
    if (m_closeStallCb) m_closeStallCb(bDelOption, m_closeStallUser);
    SetDisable(false);
    cDialog::SetActive(false);
    m_dlgState = StallDlgState::NotOpened;
    if (m_heroIdCb) m_ownerId = m_heroIdCb(m_heroIdUser);
}

void cStreetStall::OnMoneyEditClick() {
    if (m_onMoneyEditClickCb) m_onMoneyEditClickCb(m_onMoneyEditClickUser);
}

void cStreetStall::OnTitleEditClick() {
    if (m_onTitleEditClickCb) m_onTitleEditClickCb(m_onTitleEditClickUser);
}

bool cStreetStall::FakeMoveIcon(std::int32_t mouseX, std::int32_t mouseY, cIcon* icon) {
    // 1:1 with legacy FakeMoveIcon virtual override.
    // Forwards to host-injected callback (R-12.x).
    if (m_fakeMoveCb) return m_fakeMoveCb(mouseX, mouseY, icon, m_fakeMoveUser);
    return false;
}

void cStreetStall::FakeMoveItem(std::int32_t, std::int32_t, cIcon*) {
    // 1:1 with legacy empty body FakeMoveItem override.
}

void cStreetStall::FakeDeleteItem(std::uint16_t pos) {
    if (pos < kStallSlotCount) m_moneyArray[pos] = 0;
}

void cStreetStall::OnActionEvnet(std::int32_t lId, void* p, std::uint32_t we) {
    (void)p;
    if ((we & kWeBtnClick) == 0u) {
        return;
    }

    if (lId == kIdEnter) {
        if (m_editTitleRequestCb) {
            m_editTitleRequestCb(m_editTitleRequestUser);
        }
    } else if (lId == kIdCloseBtn) {
        SetActive(false);
    }
}

std::uint32_t cStreetStall::ActionEvent(std::int32_t, std::int32_t, std::uint32_t) noexcept {
    // 1:1 with legacy ActionEvent override.  The
    // legacy body is a complex dispatch; modern port
    // returns WE_NULL as the safe default (1:1 with
    // the early-return WE_NULL branch when the dialog
    // is not active).
    return kWeNull;
}

std::uint32_t cStreetStall::ActionEventWindow(std::int32_t, std::int32_t, std::uint32_t) noexcept {
    return kWeNull;
}

void cStreetStall::SetActive(bool val) noexcept {
    if (!isEnabled() || isActive() == val) {
        return;
    }
    if (!val && m_closeStreetStallCb) {
        m_closeStreetStallCb(m_closeStreetStallUser);
    }
    cDialog::SetActiveRecursive(val);
}

void cStreetStall::RegistMoney() {}

void cStreetStall::RegistMoney(std::uint16_t pos, std::uint32_t dwMoney) {
    if (pos < kStallSlotCount) m_moneyArray[pos] = dwMoney;
}

void cStreetStall::RegistTitle(char*, bool) {}
void cStreetStall::EditTitle(char*) {}
void cStreetStall::GetTitle(char*) {}

int cStreetStall::GetCurSelectedItemNum() const noexcept { return m_curSelectedItem; }
void cStreetStall::SetCurSelectedItemNum(int num) noexcept { m_curSelectedItem = num; }

void* cStreetStall::GetCurSelectedItem() {
    if (m_curSelectedItem < 0 || m_curSelectedItem >= static_cast<int>(kStallSlotCount)) return nullptr;
    return m_getIconCb ? m_getIconCb(static_cast<std::uint16_t>(m_curSelectedItem), m_getIconUser) : nullptr;
}

std::uint32_t cStreetStall::GetCurSelectedItemDBidx() { return m_curSelectedItemDBIdx; }
std::uint32_t cStreetStall::GetCurSelectedItemDur() { return 0; }
std::uint32_t cStreetStall::GetCurSelectedItemIdx() { return 0; }
std::uint32_t cStreetStall::GetCheckDBIdx() const noexcept { return m_curSelectedItemDBIdx; }

void* cStreetStall::GetItem(std::uint16_t pos) {
    return m_getIconCb ? m_getIconCb(pos, m_getIconUser) : nullptr;
}
void* cStreetStall::GetItem(std::uint32_t) { return nullptr; }

std::uint32_t cStreetStall::GetItemMoney(std::uint16_t pos) const noexcept {
    return pos < kStallSlotCount ? m_moneyArray[pos] : 0u;
}

int cStreetStall::AddItem(cIcon* pItem) {
    if (m_addItemCb) return m_addItemCb(pItem, m_addItemUser);
    return 0;
}
void cStreetStall::DeleteItem(void*) {}
void cStreetStall::DeleteItemAll(bool bDelOption) {
    for (std::size_t i = 0; i < kStallSlotCount; ++i) m_moneyArray[i] = 0;
    (void)bDelOption;
}
void cStreetStall::ResetItemInfo(std::uint32_t, std::uint32_t) {}

void* cStreetStall::FindItem(std::uint32_t dwDBIdx) {
    return m_findItemCb ? m_findItemCb(dwDBIdx, m_findItemUser) : nullptr;
}

void cStreetStall::ChangeItemStatus(std::uint16_t, std::uint32_t, bool) {}
void cStreetStall::ChangeItemStatus(void*, std::uint32_t, bool, int) {}

bool cStreetStall::SelectedItemCheck() {
    return m_selectedItemCheckCb ? m_selectedItemCheckCb(m_selectedItemCheckUser) : true;
}
bool cStreetStall::MoneyEditCheck() {
    return m_moneyEditCheckCb ? m_moneyEditCheckCb(m_moneyEditCheckUser) : true;
}

StallDlgState cStreetStall::GetDlgState() const noexcept { return m_dlgState; }

std::uint32_t cStreetStall::GetStallOwnerId() const noexcept { return m_ownerId; }
void cStreetStall::SetStallOwnerId(std::uint32_t dwId) noexcept { m_ownerId = dwId; }

void cStreetStall::ResetDlgData() {
    for (std::size_t i = 0; i < kStallSlotCount; ++i) m_moneyArray[i] = 0;
    m_totalMoney = 0;
    m_curSelectedItem = -1;
    m_curSelectedItemDBIdx = 0;
    m_data = nullptr;
}

int cStreetStall::GetGridposForItemIdx(std::uint16_t) { return -1; }

void cStreetStall::ShowDivideBox(std::uint32_t dwOption, int x, int y, int nDivideKind) {
    if (m_showDivideBoxCb) m_showDivideBoxCb(dwOption, x, y, nDivideKind, m_showDivideBoxUser);
}

void cStreetStall::OnDivideItem(std::int32_t, void*, std::uint32_t, void*, void*) {}
void cStreetStall::OnDivideItemCancel(std::int32_t, void*, std::uint32_t, void*, void*) {}
void cStreetStall::OnFakeRegistItem(std::int32_t, void*, std::uint32_t, void*, void*) {}
void cStreetStall::OnRegistItemCancel(std::int32_t, void*, std::uint32_t, void*, void*) {}

void* cStreetStall::GetData() const noexcept { return m_data; }
void cStreetStall::SetData(void* pData) noexcept { m_data = pData; }
cIconGridDialog* cStreetStall::GetGridDialog() const noexcept { return m_stallGrid; }

// ---- Test hooks ----
void cStreetStall::SetStallGridForTest(cIconGridDialog* g) noexcept { m_stallGrid = g; }
void cStreetStall::SetTitleEditForTest(cEditBox* e) noexcept { m_titleEdit = e; }
void cStreetStall::SetMoneyEditForTest(cEditBox* e) noexcept { m_moneyEdit = e; }
void cStreetStall::SetEnterBtnForTest(cButton* b) noexcept { m_enterBtn = b; }
void cStreetStall::SetBuyBtnForTest(cButton* b) noexcept { m_buyBtn = b; }
void cStreetStall::SetEditBtnForTest(cButton* b) noexcept { m_editBtn = b; }
cIconGridDialog* cStreetStall::GetStallGridForTest() const noexcept { return m_stallGrid; }
cEditBox* cStreetStall::GetTitleEditForTest() const noexcept { return m_titleEdit; }
cEditBox* cStreetStall::GetMoneyEditForTest() const noexcept { return m_moneyEdit; }
cButton* cStreetStall::GetEnterBtnForTest() const noexcept { return m_enterBtn; }
cButton* cStreetStall::GetBuyBtnForTest() const noexcept { return m_buyBtn; }
cButton* cStreetStall::GetEditBtnForTest() const noexcept { return m_editBtn; }

void cStreetStall::SetWindowResolverForTest(WindowResolver cb, void* user) noexcept {
    m_windowResolverCb = cb; m_windowResolverUser = user;
}
void cStreetStall::SetShowStallCallbackForTest(ShowStallCallback cb, void* user) noexcept {
    m_showStallCb = cb; m_showStallUser = user;
}
void cStreetStall::SetCloseStallCallbackForTest(CloseStallCallback cb, void* user) noexcept {
    m_closeStallCb = cb; m_closeStallUser = user;
}
void cStreetStall::SetOnMoneyEditClickCallbackForTest(EditClickCallback cb, void* user) noexcept {
    m_onMoneyEditClickCb = cb; m_onMoneyEditClickUser = user;
}
void cStreetStall::SetOnTitleEditClickCallbackForTest(EditClickCallback cb, void* user) noexcept {
    m_onTitleEditClickCb = cb; m_onTitleEditClickUser = user;
}
void cStreetStall::SetEditTitleRequestCallbackForTest(EditTitleRequestCallback cb, void* user) noexcept {
    m_editTitleRequestCb = cb; m_editTitleRequestUser = user;
}
void cStreetStall::SetCloseStreetStallCallbackForTest(CloseStreetStallCallback cb, void* user) noexcept {
    m_closeStreetStallCb = cb; m_closeStreetStallUser = user;
}
void cStreetStall::SetFakeMoveCallbackForTest(FakeMoveCallback cb, void* user) noexcept {
    m_fakeMoveCb = cb; m_fakeMoveUser = user;
}
void cStreetStall::SetGetIconCallbackForTest(GetIconCallback cb, void* user) noexcept {
    m_getIconCb = cb; m_getIconUser = user;
}
void cStreetStall::SetAddItemCallbackForTest(AddItemCallback cb, void* user) noexcept {
    m_addItemCb = cb; m_addItemUser = user;
}
void cStreetStall::SetFindItemCallbackForTest(FindItemCallback cb, void* user) noexcept {
    m_findItemCb = cb; m_findItemUser = user;
}
void cStreetStall::SetSelectedItemCheckCallbackForTest(CheckCallback cb, void* user) noexcept {
    m_selectedItemCheckCb = cb; m_selectedItemCheckUser = user;
}
void cStreetStall::SetMoneyEditCheckCallbackForTest(CheckCallback cb, void* user) noexcept {
    m_moneyEditCheckCb = cb; m_moneyEditCheckUser = user;
}
void cStreetStall::SetShowDivideBoxCallbackForTest(ShowDivideBoxCallback cb, void* user) noexcept {
    m_showDivideBoxCb = cb; m_showDivideBoxUser = user;
}
void cStreetStall::SetHeroIdCallbackForTest(HeroIdCallback cb, void* user) noexcept {
    m_heroIdCb = cb; m_heroIdUser = user;
}

}  // namespace mxh::ui
