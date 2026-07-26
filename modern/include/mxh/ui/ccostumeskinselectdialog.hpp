// ccostumeskinselectdialog.hpp — modern port of 墨香 CCostumeSkinSelectDialog.
//
// 1:1 port of legacy `CCostumeSkinSelectDialog` from
//   `墨香【源码】\[Client]MH\CostumeSkinSelectDialog.h` (no .cpp).
//
// The costume skin select dialog has 3 tabs (Hat / Dress /
// Accessory), each backed by a separate hash table of
// SKIN_SELECT_ITEM_INFO.  Selecting a row in the list shows
// the costume in m_CostumeSkinView + updates m_dwSelectIdx.
//
// The modern port keeps the 1:1 surface with the cross-cutting
// GAMERESRCMNGR / ItemShow / cIconDialog / cListDialog /
// cPushupButton dependencies stubbed via host-injected callbacks
// or test hooks.

#pragma once

#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cwindow.hpp"

#include <cstdint>
#include <vector>

namespace mxh::ui {

class cListDialog;
class cPushupButton;

// 1:1 with legacy CommonGameDefine.h constants used by the
// costume skin dialog.
inline constexpr std::int32_t kSkinItemListMax    = 3;
inline constexpr std::int32_t kMaxItemNameLength = 30;
inline constexpr std::int32_t kMaxItemNameBuf    = kMaxItemNameLength + 1;

// 1:1 with legacy TAB_BTN enum.
enum class CostumeSkinTab : std::int32_t {
    Hat       = 0,
    Dress     = 1,
    Accessory = 2,
    Max       = 3,
};

// 1:1 with legacy SKIN_SELECT_ITEM_INFO (legacy GameResourceStruct.h).
// The struct preserves the legacy field order + sizes so the
// binary layout is identical (the cost list resource file
// reads these straight off disk).
struct SkinSelectItemInfo {
    std::uint32_t dwIndex = 0;                                  // 1:1
    char          szSkinName[kMaxItemNameBuf] = {};             // 1:1
    std::uint32_t dwLimitLevel = 0;                             // 1:1
    std::uint16_t wEquipItem[kSkinItemListMax] = {};            // 1:1
};

class cCostumeSkinSelectDialog : public cDialog {
public:
    cCostumeSkinSelectDialog();
    ~cCostumeSkinSelectDialog() override;

    cCostumeSkinSelectDialog(const cCostumeSkinSelectDialog&) = delete;
    cCostumeSkinSelectDialog& operator=(const cCostumeSkinSelectDialog&) = delete;

    // 1:1 with legacy Linking.  Wires cListDialog / cIconDialog
    // / cPushupButton children by id.
    void Linking();

    // 1:1 with legacy SetActive(BOOL).
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy ActionEvent.
    std::uint32_t ActionEvent(/*CMouse**/ void* mouseInfo);

    // 1:1 with legacy OnActionEvent.
    bool OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // 1:1 with legacy CostumeSkinKindData.  Reads the costume
    // skin data from GAMERESRCMNGR.  Modern port: the host
    // injects the data via SetCostumeSkinDataForTest.
    void CostumeSkinKindData();

    // 1:1 with legacy CostumeSkinListInfo.  Populates the
    // cListDialog with rows for the given tab.
    void CostumeSkinListInfo(CostumeSkinTab kind);

    // 1:1 with legacy GetCurrentSkinInfo(DWORD).
    const SkinSelectItemInfo* GetCurrentSkinInfo(std::uint32_t dwSelectIdx) const;

    // 1:1 with legacy SetCostumTabBtnFocus(TAB_BTN).
    void SetCostumTabBtnFocus(CostumeSkinTab kind);

    // Test introspection.
    std::uint32_t selectIdx() const noexcept { return m_dwSelectIdx; }
    int hatCount()       const noexcept { return static_cast<int>(m_skinHat.size()); }
    int dressCount()     const noexcept { return static_cast<int>(m_skinDress.size()); }
    int accessoryCount() const noexcept { return static_cast<int>(m_skinAccessory.size()); }
    CostumeSkinTab currentTab() const noexcept { return m_currentTab; }
    const std::vector<SkinSelectItemInfo>& skinListForTest(CostumeSkinTab kind) const;

    // Test hook -- inject a cListDialog* / cIconDialog* / cPushupButton* for a tab.
    struct ChildWindows {
        cListDialog*   listDlg   = nullptr;
        cPushupButton* tabBtns[static_cast<std::int32_t>(CostumeSkinTab::Max)] = {};
    };
    void SetChildWindowsForTest(const ChildWindows& w) { m_childWindows = w; }

    // Test hook -- inject the costume-skin data (replaces the
    // legacy GAMERESRCMNGR->GetCostumeSkinHat / Dress / Accessory
    // lookup).
    void SetCostumeSkinDataForTest(CostumeSkinTab kind,
                                    const std::vector<SkinSelectItemInfo>& data);

    // Test hook -- inject the "list row click" callback (legacy
    // m_pCostumeSkinListDlg->ActionEvent -> SetActive + select).
    using ListRowClickCallback = void(*)(std::int32_t rowIdx, void* user);
    void SetListRowClickCallbackForTest(ListRowClickCallback cb, void* user) {
        m_listRowClickCb = cb; m_listRowClickUser = user;
    }

    // Test hook -- inject a "set the list" callback (legacy
    // m_pCostumeSkinListDlg->AddItem / RemoveAll).
    using ListAddItemCallback = void(*)(const SkinSelectItemInfo* info, void* user);
    using ListRemoveAllCallback = void(*)(void* user);
    void SetListAddItemCallbackForTest(ListAddItemCallback cb, void* user) {
        m_listAddItemCb = cb; m_listAddItemUser = user;
    }
    void SetListRemoveAllCallbackForTest(ListRemoveAllCallback cb, void* user) {
        m_listRemoveAllCb = cb; m_listRemoveAllUser = user;
    }

    // Test hook -- inject a "set tab btn focus" callback.
    using TabBtnFocusCallback = void(*)(CostumeSkinTab kind, void* user);
    void SetTabBtnFocusCallbackForTest(TabBtnFocusCallback cb, void* user) {
        m_tabBtnFocusCb = cb; m_tabBtnFocusUser = user;
    }

    // 1:1 with legacy tab btn ids.
    static constexpr std::int32_t kTabBtnHatId       = 1001;
    static constexpr std::int32_t kTabBtnDressId     = 1002;
    static constexpr std::int32_t kTabBtnAccessoryId = 1003;
    static constexpr std::int32_t kSkinListDlgId     = 1004;
    static constexpr std::int32_t kSkinIconDlgId     = 1005;

    // 1:1 with legacy cost delay.
    void  SetSkinDelayTime(std::uint32_t t) noexcept { m_dwSkinDelayTime = t; }
    std::uint32_t skinDelayTime() const noexcept { return m_dwSkinDelayTime; }
    void  SetSkinDelayResult(bool r) noexcept { m_bSkinDelayResult = r; }
    bool  skinDelayResult() const noexcept { return m_bSkinDelayResult; }

private:
    void ClearLists();

    cListDialog*   m_pCostumeSkinListDlg = nullptr;
    cPushupButton* m_pCostumTabBtn[static_cast<std::int32_t>(CostumeSkinTab::Max)] = {};
    ChildWindows   m_childWindows;

    std::uint32_t m_dwSelectIdx      = 0;
    std::uint32_t m_dwSkinDelayTime  = 0;
    bool          m_bSkinDelayResult = false;

    std::vector<SkinSelectItemInfo> m_skinHat;
    std::vector<SkinSelectItemInfo> m_skinDress;
    std::vector<SkinSelectItemInfo> m_skinAccessory;
    CostumeSkinTab m_currentTab = CostumeSkinTab::Hat;

    ListRowClickCallback   m_listRowClickCb   = nullptr;
    void*                  m_listRowClickUser = nullptr;
    ListAddItemCallback    m_listAddItemCb    = nullptr;
    void*                  m_listAddItemUser  = nullptr;
    ListRemoveAllCallback  m_listRemoveAllCb  = nullptr;
    void*                  m_listRemoveAllUser = nullptr;
    TabBtnFocusCallback    m_tabBtnFocusCb    = nullptr;
    void*                  m_tabBtnFocusUser  = nullptr;
};

} // namespace mxh::ui
