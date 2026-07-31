// cstreetstall.hpp -- modern port of Moxiang
//   CStreetStall (player stall: title/money/edit,
//   25 icon slots, sell vs buy states).
//
// 1:1 port of legacy CStreetStall from
//   [Client]MH/StreetStall.{h,cpp}.
//
// Surface (legacy):
//   - 6 children resolved in Linking via
//     GetWindowForID with ids SSI_ICONGRID,
//     SSI_TITLEEDIT, SSI_ENTER, SSI_BUYBTN,
//     SSI_EDITBTN, SSI_MONEYEDIT.
//   - 2 enums:
//     STALL_DLG_STATE { eSDS_NOT_OPENED=0,
//                      eSDS_OPENED=1,
//                      eSDS_BUY=2 }
//     STALL_OPTION { eSO_DIVE=0,
//                   eSO_INPUTMONEY=1,
//                   eSO_INPUTMONEY_DUP=2 }
//   - 25-slot MoneyArray (m_MoneyArray[SLOT_STREETSTALL_NUM]).
//   - 1:1 quirk: legacy uses public WT_STREETSTALLDIALOG
//     m_type tag (modern cWindow has no m_type; removed
//     in Phase 6 -- 1:1 quirk noted, modern ctor drops it).
//   - 30+ public methods (Linking / SetDisable /
//     ShowSellStall / ShowBuyStall / OnCloseStall /
//     OnMoneyEditClick / OnTitleEditClick / FakeMoveIcon /
//     FakeMoveItem / FakeDeleteItem / OnActionEvnet /
//     ActionEvent / ActionEventWindow / SetActive /
//     RegistMoney x2 / RegistTitle / EditTitle / GetTitle /
//     GetCurSelectedItemNum / SetCurSelectedItemNum /
//     GetCurSelectedItem / GetCurSelectedItemDBidx /
//     GetCurSelectedItemDur / GetCurSelectedItemIdx /
//     GetCheckDBIdx / GetItem x2 / GetItemMoney / AddItem /
//     DeleteItem / DeleteItemAll / ResetItemInfo / FindItem /
//     ChangeItemStatus x2 / SelectedItemCheck /
//     MoneyEditCheck / GetDlgState / GetStallOwnerId /
//     SetStallOwnerId / ResetDlgData / GetGridposForItemIdx /
//     ShowDivideBox / 4 static callbacks / GetData / SetData /
//     GetGridDialog).
//
// Modern port:
//   - Inherits cDialog (1:1 with legacy CStreetStall :
//     public cDialog).
//   - enum StallDlgState { NotOpened=0, Opened=1, Buy=2 }
//     (1:1 with legacy STALL_DLG_STATE).
//   - enum StallOption { Dive=0, InputMoney=1,
//     InputMoneyDup=2 } (1:1 with legacy STALL_OPTION).
//   - m_MoneyArray[kStallSlotCount] = { 0 }  (1:1 with
//     legacy m_MoneyArray[SLOT_STREETSTALL_NUM]).
//   - m_OldTitle[kTitleMaxLen + 1] = { 0 }  (1:1 with
//     legacy m_OldTitle[MAX_STREETSTALL_TITLELEN + 1]).
//   - All 30+ public methods preserved with 1:1 signatures.
//     R-12.x deferred operations (ITEMMGR, OBJECTSTATEMGR,
//     CHATMGR, STREETSTALLMGR, WINDOWMGR, HERO,
//     GameResourceManager) are routed through host-injected
//     callbacks.
//   - 4 static callbacks (OnDivideItem, OnDivideItemCancel,
//     OnFakeRegistItem, OnRegistItemCancel) are kept as
//     static methods; modern port keeps the static dispatch
//     contract (1:1 with legacy).
//   - No m_type assignment (modern cWindow has no m_type;
//     removed in Phase 6 -- 1:1 quirk noted).
//
// 1:1 quirks:
//   - 1:1 with legacy m_type = WT_STREETSTALLDIALOG:
//     modern port drops the assignment (m_type field
//     removed in Phase 6).
//   - 1:1 with legacy 25-slot MoneyArray storage shape.
//   - 1:1 with legacy 67-char title buffer (66 chars + 1
//     null terminator).
//   - 1:1 with legacy initial m_DlgState = eSDS_NOT_OPENED
//     (modern: m_dlgState = StallDlgState::NotOpened).
//   - 1:1 with legacy initial m_nCurSelectedItem = -1
//     (modern: m_curSelectedItem = -1).
//   - 1:1 with legacy initial m_pData = NULL
//     (modern: m_data = nullptr).
//   - 1:1 with legacy initial m_dwOwnnerId = HERO->GetID()
//     (set in OnCloseStall; modern sets via host callback).
//   - 1:1 with legacy FakeMoveItem empty body.
//   - 1:1 with legacy static callback signatures
//     (LONG iId, void* p, DWORD param1, void* vData1,
//     void* vData2).
//   - 1:1 with legacy cDialog::SetActive forwarding in
//     SetActive override.
//   - 1:1 with legacy default title DEFAULT_TITLE_TEXT
//     (CHATMGR->GetChatMsg(366), R-12.x deferred via
//     host callback).
//   - 1:1 with legacy default money DEFAULT_MONEY_TEXT
//     (Q + Q + 0 + Q + Q).
//

#pragma once

#include "cdialog.hpp"
#include "cwindow.hpp"

#include <cstdint>

namespace mxh::ui {

class cIcon;
class cIconGridDialog;
class cEditBox;
class cTextArea;
class cButton;
class cDivideBox;

// 1:1 with legacy STALL_DLG_STATE enum.
enum class StallDlgState : std::uint8_t {
    NotOpened = 0,
    Opened    = 1,
    Buy       = 2,
};

// 1:1 with legacy STALL_OPTION enum.
enum class StallOption : std::uint8_t {
    Dive           = 0,
    InputMoney     = 1,
    InputMoneyDup  = 2,
};

class cStreetStall : public cDialog {
public:
    // 1:1 with legacy SLOT_STREETSTALL_NUM = 25.
    static constexpr std::size_t kStallSlotCount = 25;
    // 1:1 with legacy MAX_STREETSTALL_TITLELEN = 66.
    static constexpr std::size_t kTitleMaxLen = 66;

    // 1:1 with legacy DEFAULT_MONEY_TEXT.
    static constexpr const char* kDefaultMoneyText = "0";

    cStreetStall();
    ~cStreetStall() override;

    cStreetStall(const cStreetStall&) = delete;
    cStreetStall& operator=(const cStreetStall&) = delete;

    // 1:1 with legacy Linking.  Resolves 6 children
    // via host-injected window resolver (R-12.x).
    void Linking();

    // 1:1 with legacy SetDisable override.
    void SetDisable(bool val) noexcept override;

    // 1:1 with legacy ShowSellStall.
    void ShowSellStall();

    // 1:1 with legacy ShowBuyStall.
    void ShowBuyStall();

    // 1:1 with legacy OnCloseStall(BOOL bDelOption).
    void OnCloseStall(bool bDelOption = false);

    // 1:1 with legacy OnMoneyEditClick.
    void OnMoneyEditClick();

    // 1:1 with legacy OnTitleEditClick.
    void OnTitleEditClick();

    // 1:1 with legacy FakeMoveIcon (virtual override).
    bool FakeMoveIcon(std::int32_t mouseX, std::int32_t mouseY, cIcon* icon);

    // 1:1 with legacy FakeMoveItem (virtual override,
    // empty body in legacy).
    void FakeMoveItem(std::int32_t mouseX, std::int32_t mouseY, cIcon* icon);

    // 1:1 with legacy FakeDeleteItem.
    void FakeDeleteItem(std::uint16_t pos);

    // 1:1 with legacy OnActionEvnet (sic -- legacy typo).
    void OnActionEvnet(std::int32_t lId, void* p, std::uint32_t we);

    // 1:1 with legacy ActionEvent override (returns
    // legacy WindowEvent code).
    std::uint32_t ActionEvent(std::int32_t mouseX,
                              std::int32_t mouseY,
                              std::uint32_t mouseFlags) noexcept override;

    // 1:1 with legacy ActionEventWindow override.
    std::uint32_t ActionEventWindow(std::int32_t mouseX,
                                   std::int32_t mouseY,
                                   std::uint32_t mouseFlags) noexcept;

    // 1:1 with legacy SetActive override.  Forwards
    // to cDialog::SetActive(val).
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy RegistMoney() / RegistMoney(pos, dwMoney).
    void RegistMoney();
    void RegistMoney(std::uint16_t pos, std::uint32_t dwMoney);

    // 1:1 with legacy RegistTitle / EditTitle.
    void RegistTitle(char* title, bool bSucess);
    void EditTitle(char* title);

    // 1:1 with legacy GetTitle.
    void GetTitle(char* pStrTitle);

    // 1:1 with legacy GetCurSelectedItemNum /
    // SetCurSelectedItemNum.
    int GetCurSelectedItemNum() const noexcept;
    void SetCurSelectedItemNum(int num) noexcept;

    // 1:1 with legacy GetCurSelectedItem (returns cIcon*).
    void* GetCurSelectedItem();

    // 1:1 with legacy GetCurSelectedItemDBidx / Dur / Idx.
    std::uint32_t GetCurSelectedItemDBidx();
    std::uint32_t GetCurSelectedItemDur();
    std::uint32_t GetCurSelectedItemIdx();

    // 1:1 with legacy GetCheckDBIdx.
    std::uint32_t GetCheckDBIdx() const noexcept;

    // 1:1 with legacy GetItem(pos) / GetItem(dbIdx).
    void* GetItem(std::uint16_t pos);
    void* GetItem(std::uint32_t dwDBIdx);

    // 1:1 with legacy GetItemMoney(pos).
    std::uint32_t GetItemMoney(std::uint16_t pos) const noexcept;

    // 1:1 with legacy AddItem.
    int AddItem(cIcon* pItem);

    // 1:1 with legacy DeleteItem / DeleteItemAll.
    void DeleteItem(void* pbase);
    void DeleteItemAll(bool bDelOption = false);

    // 1:1 with legacy ResetItemInfo.
    void ResetItemInfo(std::uint32_t dwDBIdx, std::uint32_t count);

    // 1:1 with legacy FindItem.
    void* FindItem(std::uint32_t dwDBIdx);

    // 1:1 with legacy ChangeItemStatus / ChangeItemStatus
    // (overload with nDivideKind).
    void ChangeItemStatus(std::uint16_t pos, std::uint32_t money, bool bLock);
    void ChangeItemStatus(void* pbase, std::uint32_t money, bool bLock, int nDivideKind = 0);

    // 1:1 with legacy SelectedItemCheck / MoneyEditCheck.
    bool SelectedItemCheck();
    bool MoneyEditCheck();

    // 1:1 with legacy GetDlgState.
    StallDlgState GetDlgState() const noexcept;

    // 1:1 with legacy GetStallOwnerId / SetStallOwnerId.
    std::uint32_t GetStallOwnerId() const noexcept;
    void SetStallOwnerId(std::uint32_t dwId) noexcept;

    // 1:1 with legacy ResetDlgData.
    void ResetDlgData();

    // 1:1 with legacy GetGridposForItemIdx.
    int GetGridposForItemIdx(std::uint16_t wIdx);

    // 1:1 with legacy ShowDivideBox.
    void ShowDivideBox(std::uint32_t dwOption = static_cast<std::uint32_t>(StallOption::Dive),
                     int x = 0, int y = 0, int nDivideKind = 0);

    // 1:1 with legacy 4 static callbacks.
    static void OnDivideItem(std::int32_t iId, void* p, std::uint32_t param1, void* vData1, void* vData2);
    static void OnDivideItemCancel(std::int32_t iId, void* p, std::uint32_t param1, void* vData1, void* vData2);
    static void OnFakeRegistItem(std::int32_t iId, void* p, std::uint32_t param1, void* vData1, void* vData2);
    static void OnRegistItemCancel(std::int32_t iId, void* p, std::uint32_t param1, void* vData1, void* vData2);

    // 1:1 with legacy GetData / SetData (opaque).
    void* GetData() const noexcept;
    void SetData(void* pData) noexcept;

    // 1:1 with legacy GetGridDialog (returns cIconGridDialog*).
    cIconGridDialog* GetGridDialog() const noexcept;

    // ---- 1:1 id constants (legacy WindowIDs.h) ----
    // 1:1 with legacy SSI_STALLDLG.
    static constexpr std::int32_t kIdDialog    = 379;
    // 1:1 with legacy SSI_ICONGRID.
    static constexpr std::int32_t kIdIconGrid  = 380;
    // 1:1 with legacy SSI_TITLEEDIT.
    static constexpr std::int32_t kIdTitleEdit = 381;
    // 1:1 with legacy SSI_ENTER.
    static constexpr std::int32_t kIdEnter     = 382;
    // 1:1 with legacy SSI_BUYBTN.
    static constexpr std::int32_t kIdBuyBtn    = 383;
    // 1:1 with legacy SSI_EDITBTN.
    static constexpr std::int32_t kIdEditBtn   = 385;
    // 1:1 with legacy SSI_MONEYEDIT.
    static constexpr std::int32_t kIdMoneyEdit = 386;

    // ---- Test hooks ----
    void SetStallGridForTest(cIconGridDialog* g) noexcept;
    void SetTitleEditForTest(cEditBox* e) noexcept;
    void SetMoneyEditForTest(cEditBox* e) noexcept;
    void SetEnterBtnForTest(cButton* b) noexcept;
    void SetBuyBtnForTest(cButton* b) noexcept;
    void SetEditBtnForTest(cButton* b) noexcept;
    cIconGridDialog* GetStallGridForTest() const noexcept;
    cEditBox* GetTitleEditForTest() const noexcept;
    cEditBox* GetMoneyEditForTest() const noexcept;
    cButton* GetEnterBtnForTest() const noexcept;
    cButton* GetBuyBtnForTest() const noexcept;
    cButton* GetEditBtnForTest() const noexcept;

    // ---- Host-injected callbacks (R-12.x deferred surfaces) ----
    // 1:1 with legacy Linking window-resolver.
    using WindowResolver = void*(*)(std::int32_t id, void* user);
    void SetWindowResolverForTest(WindowResolver cb, void* user) noexcept;

    // 1:1 with legacy ShowSellStall / ShowBuyStall hooks.
    using ShowStallCallback = void(*)(StallDlgState state, void* user);
    void SetShowStallCallbackForTest(ShowStallCallback cb, void* user) noexcept;

    // 1:1 with legacy OnCloseStall hook (R-12.x).
    using CloseStallCallback = void(*)(bool bDelOption, void* user);
    void SetCloseStallCallbackForTest(CloseStallCallback cb, void* user) noexcept;

    // 1:1 with legacy OnMoneyEditClick / OnTitleEditClick hooks.
    using EditClickCallback = void(*)(void* user);
    void SetOnMoneyEditClickCallbackForTest(EditClickCallback cb, void* user) noexcept;
    void SetOnTitleEditClickCallbackForTest(EditClickCallback cb, void* user) noexcept;

    // 1:1 with legacy FakeMoveIcon hook.
    using FakeMoveCallback = bool(*)(std::int32_t mouseX, std::int32_t mouseY, void* icon, void* user);
    void SetFakeMoveCallbackForTest(FakeMoveCallback cb, void* user) noexcept;

    // 1:1 with legacy GetCurSelectedItem hook.
    using GetIconCallback = void*(*)(std::uint16_t pos, void* user);
    void SetGetIconCallbackForTest(GetIconCallback cb, void* user) noexcept;

    // 1:1 with legacy AddItem / FindItem hook (R-12.x).
    using AddItemCallback = int(*)(void* pItem, void* user);
    void SetAddItemCallbackForTest(AddItemCallback cb, void* user) noexcept;
    using FindItemCallback = void*(*)(std::uint32_t dwDBIdx, void* user);
    void SetFindItemCallbackForTest(FindItemCallback cb, void* user) noexcept;

    // 1:1 with legacy SelectedItemCheck / MoneyEditCheck hooks.
    using CheckCallback = bool(*)(void* user);
    void SetSelectedItemCheckCallbackForTest(CheckCallback cb, void* user) noexcept;
    void SetMoneyEditCheckCallbackForTest(CheckCallback cb, void* user) noexcept;

    // 1:1 with legacy ShowDivideBox hook.
    using ShowDivideBoxCallback = void(*)(std::uint32_t option, int x, int y, int kind, void* user);
    void SetShowDivideBoxCallbackForTest(ShowDivideBoxCallback cb, void* user) noexcept;

    // 1:1 with legacy SetActive check hook (m_bActive).
    using HeroIdCallback = std::uint32_t(*)(void* user);
    void SetHeroIdCallbackForTest(HeroIdCallback cb, void* user) noexcept;

    // 1:1 with legacy WE_NULL return from ActionEvent.
    static constexpr std::uint32_t kWeNull = 0;

private:
    // 1:1 with legacy m_DlgState.
    StallDlgState m_dlgState = StallDlgState::NotOpened;

    // 1:1 with legacy child window members.
    cEditBox* m_titleEdit = nullptr;
    cEditBox* m_moneyEdit = nullptr;
    cButton* m_enterBtn = nullptr;
    cButton* m_buyBtn = nullptr;
    cButton* m_editBtn = nullptr;
    cIconGridDialog* m_stallGrid = nullptr;

    // 1:1 with legacy m_MoneyArray[SLOT_STREETSTALL_NUM].
    std::uint32_t m_moneyArray[kStallSlotCount] = {0};

    // 1:1 with legacy m_TotalMoney.
    std::uint32_t m_totalMoney = 0;

    // 1:1 with legacy m_OldTitle[MAX_STREETSTALL_TITLELEN + 1].
    char m_oldTitle[kTitleMaxLen + 1] = {0};

    // 1:1 with legacy m_nCurSelectedItem /
    // m_dwCurSelectedItemDBIdx.
    int m_curSelectedItem = -1;
    std::uint32_t m_curSelectedItemDBIdx = 0;

    // 1:1 with legacy m_dwOwnnerId (sic -- legacy typo).
    std::uint32_t m_ownerId = 0;

    // 1:1 with legacy m_pData (opaque).
    void* m_data = nullptr;

    // Host-injected callback storage.
    WindowResolver m_windowResolverCb = nullptr;
    void* m_windowResolverUser = nullptr;
    ShowStallCallback m_showStallCb = nullptr;
    void* m_showStallUser = nullptr;
    CloseStallCallback m_closeStallCb = nullptr;
    void* m_closeStallUser = nullptr;
    EditClickCallback m_onMoneyEditClickCb = nullptr;
    void* m_onMoneyEditClickUser = nullptr;
    EditClickCallback m_onTitleEditClickCb = nullptr;
    void* m_onTitleEditClickUser = nullptr;
    FakeMoveCallback m_fakeMoveCb = nullptr;
    void* m_fakeMoveUser = nullptr;
    GetIconCallback m_getIconCb = nullptr;
    void* m_getIconUser = nullptr;
    AddItemCallback m_addItemCb = nullptr;
    void* m_addItemUser = nullptr;
    FindItemCallback m_findItemCb = nullptr;
    void* m_findItemUser = nullptr;
    CheckCallback m_selectedItemCheckCb = nullptr;
    void* m_selectedItemCheckUser = nullptr;
    CheckCallback m_moneyEditCheckCb = nullptr;
    void* m_moneyEditCheckUser = nullptr;
    ShowDivideBoxCallback m_showDivideBoxCb = nullptr;
    void* m_showDivideBoxUser = nullptr;
    HeroIdCallback m_heroIdCb = nullptr;
    void* m_heroIdUser = nullptr;
};

}  // namespace mxh::ui
