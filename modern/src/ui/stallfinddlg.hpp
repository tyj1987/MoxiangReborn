// stallfinddlg.hpp — modern port of 墨香 CStallFindDlg (street-
// stall item-search dialog: search a player's open market for
// items by type / class / price; pagination over results).
//
// 1:1 port of legacy `CStallFindDlg` from
//   `墨香【源码】\[Client]MH\StallFindDlg.{h,cpp}` (~700 lines
//   legacy code; the modern port keeps the data model + state
//   machine + 1:1 quirks and stubs the engine-side singletons
//   with no-ops until Phase 13+ real impl lands).
//
// Modern port scope (this commit):
//   - 10 cComboBox children (1 main + 9 detail combos indexed by
//     ITEM_TYPE enum). 1:1 with legacy cComboBox port (the
//     c0c13d0 commit).
//   - 3 cListDialog children (item list / class list / result
//     list). 1:1 with legacy cListDialog port.
//   - 7 cPushupButton children (2 sell/buy mode + 5 page + 1 type
//     combo + 1 detail combo trigger). 1:1 with legacy
//     cPushupButton port.
//   - 2 cButton children (page up / down). 1:1 with legacy
//     cButton port.
//   - 2 cStatic children (name static + price static). 1:1
//     with legacy cStatic port.
//   - State: 5 enum ITEM_TYPE + 9 array index + 8 misc state
//     fields (m_bSearchedAll, m_dwSearchType, m_nStallCount,
//     m_arrStallInfo[40], m_nBasePage, m_nMaxPage,
//     m_nCurrentPage, m_nItemType, m_nItemDetailType,
//     m_nSelectedItemListIdx, m_nSelectedClassListIdx,
//     m_nSelectedStallListIdx, m_dwSelectedObjectIndex,
//     m_dwPrevSelectedType, m_ptrItemInfo).
//   - Linking: 1:1 with legacy. Resolve 24+ children by id
//     (legacy GetWindowForID equivalents via modern
//     findWindowById).
//   - SetActive(BOOL) override: 1:1 with legacy (val==FALSE
//     triggers OnClose, val==TRUE triggers UpdateItemList).
//   - ActionEvent(CMouse*) override: 1:1 with legacy
//     (cDialog::ActionEvent + WE_LBTNDBLCLICK on result row
//     triggers SendItemViewMsg).
//   - OnActionEvent(lId, p, we) handler for the 24+ button
//     ids. Engine-side network-send + ObjectManager + msgbox
//     are stubbed; data-side state is preserved.
//   - LoadItemList: load item catalog from a .bin/.txt file.
//     Engine-side stubbed (no file in modern); modern port's
//     load is a no-op.
//   - UpdateItemList / UpdateStallList / SortStallList /
//     SetPage / SetBasePage: data-side helpers used by the
//     engine's search flow. All 1:1 with legacy; the modern
//     port preserves the data shape + state transitions.
//   - SetStallPriceInfo / SendItemViewMsg: data-side helpers
//     for the engine's search-result flow. 1:1 with legacy;
//     the modern port preserves the data shape.
//
// Modern-port simplifications (all documented in the .cpp file
// header):
// 1. **Engine singleton dependencies stubbed.** GameResourceManager
//    / ITEMMGR / CHATMGR / OBJECTMGR / NETWORK / WINDOWMGR /
//    RESRCMGR / MHFile / HERO / GAMEIN / PKMGR are all no-op
//    stubs. The data-side state (m_nStallCount / m_arrStallInfo
//    / m_nBasePage / m_nMaxPage / m_nCurrentPage / m_nItemType /
//    m_nItemDetailType / m_nSelectedItemListIdx / etc.) is
//    preserved 1:1.
// 2. **m_arrStallInfo is opaque.** The legacy uses
//    STREETSTALL_PRICE_INFO (engine-side struct). Modern port
//    uses a placeholder struct with the same fields (strName +
//    dwPrice + dwOwnerIdx) so the data shape is preserved.
// 3. **m_ptrItemInfo is stubbed.** The legacy uses cPtrList
//    (engine-side linked list) of TItemInfo. Modern port uses
//    std::vector<TItemInfo> with size 0 (no bin file loaded).
// 4. **Render is a no-op.** The legacy Render is the cDialog's
//    default (the dialog has no Render override; children do
//    their own rendering).
// 5. **ActionEvent is a no-op stub.** The legacy
//    cDialog::ActionEvent + cDialog::ActionEventWindow +
//    cDialog::ActionEventComponent chain is simplified to
//    cDialog::ActionEvent. The legacy
//    ObjectManager->GetObject + balloon-image + NETWORK->Send
//    side effects are stubbed.
// 6. **SortStallList is a real impl.** The legacy uses a shell
//    sort (descending or ascending by dwPrice). Modern port
//    preserves the algorithm; it operates on the
//    m_arrStallInfo[40] array.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this is a
// Tier 2 dialog port (0.13.48). It unblocks any Tier 2 dialog
// that needs street-stall item-search (no others in the current
// P2-12 backlog — this is the only one).

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::ui {

class cStatic;
class cListDialog;
class cComboBox;
class cPushupButton;
class cButton;

class cStallFindDlg : public cDialog {
public:
    // -------------------------------------------------------------------------
    // Constants (1:1 with legacy).
    // -------------------------------------------------------------------------
    static constexpr std::uint16_t SEARCH_DELAY      = 3000;
    static constexpr std::uint16_t ITEMVIEW_DELAY    = 1000;
    static constexpr std::uint16_t MAX_RESULT_PAGE   = 5;
    static constexpr std::uint16_t MAX_LINE_PER_PAGE = 6;
    static constexpr std::uint16_t MAX_STALLITEM_NUM = 40;
    static constexpr std::uint16_t ITEM_TYPE_COUNT   = 9;

    // 1:1 with legacy ITEM_TYPE enum (the 9 item categories).
    enum ItemType : std::int32_t {
        WEAPON      = 0,
        CLOTHES     = 1,
        ACCESSORY   = 2,
        POTION      = 3,
        MATERIAL    = 4,
        ETC         = 5,
        ITEM_MALL   = 6,
        TITAN_ITEM  = 7,
        // ITEM_TYPE_COUNT = 9 (last enum value, used for array
        // bounds; the legacy doesn't have an explicit
        // ITEM_TYPE_COUNT entry in the enum).
    };

    // 1:1 with legacy search-type enum (eSK_SELL / eSK_BUY).
    enum SearchKind : std::uint32_t {
        SK_SELL = 0,
        SK_BUY  = 1,
    };

    // 1:1 with legacy TItemInfo struct.
    struct ItemInfo {
        std::int32_t  type        = 0;
        std::int32_t  detailType  = 0;
        std::uint32_t itemIdx     = 0;
    };

    // 1:1 with legacy STREETSTALL_PRICE_INFO. Opaque to modern
    // (engine-side struct) but the modern port stores the
    // essential fields for 1:1 behavior.
    struct StallPriceInfo {
        std::string  strName;
        std::uint32_t dwPrice   = 0;
        std::uint32_t dwOwnerIdx = 0;
    };

    // -------------------------------------------------------------------------
    // Local id range (1:1 with legacy PLI_*-style ids, but
    // using a small local offset since the modern port doesn't
    // share the global legacy id space). The id range 0-29
    // covers all 24+ children of this dialog. The id values
    // are aligned to the legacy PLI_* ordering so the modern
    // port can be cross-referenced against the legacy enum
    // (see legacy `墨香【源码】/[Client]MH/WindowIDs.h`
    // SFR_* entries).
    // -------------------------------------------------------------------------
    static constexpr std::int32_t ID_DLG                 = 0;
    static constexpr std::int32_t ID_TYPECOMBO           = 1;
    static constexpr std::int32_t ID_TYPECOMBOBTN        = 2;
    static constexpr std::int32_t ID_DETAILTYPECOMBOBTN  = 3;
    static constexpr std::int32_t ID_WEAPON_DETAILCOMBO  = 4;
    static constexpr std::int32_t ID_CLOTHES_DETAILCOMBO = 5;
    static constexpr std::int32_t ID_ACCESSORY_DETAILCOMBO = 6;
    static constexpr std::int32_t ID_POTION_DETAILCOMBO  = 7;
    static constexpr std::int32_t ID_MATERIAL_DETAILCOMBO = 8;
    static constexpr std::int32_t ID_ETC_DETAILCOMBO     = 9;
    static constexpr std::int32_t ID_ITEMMALL_DETAILCOMBO = 10;
    static constexpr std::int32_t ID_TITAN_DETAILCOMBO   = 11;
    static constexpr std::int32_t ID_ITEMLIST            = 12;
    static constexpr std::int32_t ID_CLASSLIST           = 13;
    static constexpr std::int32_t ID_RESULTLIST          = 14;
    static constexpr std::int32_t ID_NAMESTATIC          = 15;
    static constexpr std::int32_t ID_PRICESTATIC         = 16;
    static constexpr std::int32_t ID_PB_SELLMODE         = 17;
    static constexpr std::int32_t ID_PB_BUYMODE          = 18;
    static constexpr std::int32_t ID_RESULTPAGEBTN1      = 19;
    static constexpr std::int32_t ID_RESULTPAGEBTN2      = 20;
    static constexpr std::int32_t ID_RESULTPAGEBTN3      = 21;
    static constexpr std::int32_t ID_RESULTPAGEBTN4      = 22;
    static constexpr std::int32_t ID_RESULTPAGEBTN5      = 23;
    static constexpr std::int32_t ID_RESULTPAGEBTNUP     = 24;
    static constexpr std::int32_t ID_RESULTPAGEBTNDOWN   = 25;
    static constexpr std::int32_t ID_SEARCHBTN           = 26;
    static constexpr std::int32_t ID_SEARCHALLBTN        = 27;

    cStallFindDlg();
    ~cStallFindDlg() override;

    cStallFindDlg(const cStallFindDlg&) = delete;
    cStallFindDlg& operator=(const cStallFindDlg&) = delete;

    // -------------------------------------------------------------------------
    // Linking: 1:1 with legacy. Resolves 24+ children by id
    // (legacy GetWindowForID equivalents via modern
    // findWindowById). Also calls LoadItemList() (1:1 with
    // legacy).
    // -------------------------------------------------------------------------
    void Linking();

    // SetActive(BOOL) override: 1:1 with legacy. val==FALSE
    // triggers OnClose(); val==TRUE triggers UpdateItemList().
    void SetActive(bool val) noexcept override;

    // ActionEvent: 1:1 with legacy. Drives the WE_LBTNDBLCLICK
    // → SendItemViewMsg flow. The modern port is a no-op
    // for the engine-side effects (CMouse / real
    // ObjectManager / real NETWORK).
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // OnActionEvent: handles the 24+ button ids. Engine-side
    // network-send + ObjectManager + msgbox are stubbed; the
    // data-side state is preserved.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // -------------------------------------------------------------------------
    // Data-side helpers (used by the engine's search flow).
    // -------------------------------------------------------------------------

    // LoadItemList: load item catalog from a .bin/.txt file.
    // 1:1 with legacy; engine-side stubbed (no file in
    // modern).
    void LoadItemList();

    // UpdateItemList: populates m_pItemList (the item list
    // cListDialog) from m_ptrItemInfo filtered by
    // m_nItemType + m_nItemDetailType. 1:1 with legacy.
    void UpdateItemList();

    // UpdateStallList: populates m_pStallList (the result
    // cListDialog) from m_arrStallInfo for the current page.
    // 1:1 with legacy.
    void UpdateStallList();

    // SortStallList: shell sort by dwPrice (ascending or
    // descending). 1:1 with legacy.
    void SortStallList(bool ascending);

    // SetPage: sets the current page index, updates the page
    // button labels + active state, then calls
    // UpdateStallList. 1:1 with legacy.
    void SetPage(int index);

    // SetBasePage: shifts the base page (5 pages visible) by
    // ±MAX_RESULT_PAGE. 1:1 with legacy.
    void SetBasePage(bool next);

    // CheckDelay: time-based delay gate (returns true if the
    // cooldown is satisfied). 1:1 with legacy (per-ID static
    // state).
    bool CheckDelay(std::uint32_t dwDelayTime, int nID);

    // SetStallPriceInfo: receives the search-result data from
    // the engine. Populates m_arrStallInfo + m_nStallCount +
    // sorts + sets page 0. 1:1 with legacy.
    void SetStallPriceInfo(const std::vector<StallPriceInfo>& prices);

    // SendItemViewMsg: sends the MP_STREETSTALL_ITEMVIEW_SYN
    // packet for the currently-selected stall row. 1:1 with
    // legacy; engine-side NETWORK->Send stubbed.
    void SendItemViewMsg();

    // SetSearchType: sets the search-type flag (SK_SELL or
    // SK_BUY). 1:1 with legacy.
    void SetSearchType(SearchKind val) noexcept { m_dwSearchType = val; }

    // -------------------------------------------------------------------------
    // Test accessors.
    // -------------------------------------------------------------------------
    std::uint32_t GetSearchType()    const noexcept { return m_dwSearchType; }
    int           GetStallCount()     const noexcept { return m_nStallCount; }
    int           GetBasePage()       const noexcept { return m_nBasePage; }
    int           GetMaxPage()        const noexcept { return m_nMaxPage; }
    int           GetCurrentPage()    const noexcept { return m_nCurrentPage; }
    int           GetItemType()       const noexcept { return m_nItemType; }
    int           GetItemDetailType() const noexcept { return m_nItemDetailType; }
    int           GetSelectedItemListIdx()  const noexcept { return m_nSelectedItemListIdx; }
    int           GetSelectedClassListIdx() const noexcept { return m_nSelectedClassListIdx; }
    int           GetSelectedStallListIdx() const noexcept { return m_nSelectedStallListIdx; }
    std::uint32_t GetSelectedObjectIndex() const noexcept { return m_dwSelectedObjectIndex; }
    bool          IsSearchedAll()    const noexcept { return m_bSearchedAll; }
    void          SetSearchedAll(bool v) noexcept { m_bSearchedAll = v; }
    void          SetStallCount(int n) noexcept { m_nStallCount = n; }
    void          SetBasePageForTesting(int p) noexcept { m_nBasePage = p; }
    void          SetMaxPageForTesting(int p) noexcept { m_nMaxPage = p; }
    void          SetCurrentPageForTesting(int p) noexcept { m_nCurrentPage = p; }
    void          SetItemTypeForTesting(int t) noexcept { m_nItemType = t; }
    void          SetItemDetailTypeForTesting(int t) noexcept { m_nItemDetailType = t; }
    void          SetSelectedItemListIdxForTesting(int i) noexcept { m_nSelectedItemListIdx = i; }
    void          SetSelectedClassListIdxForTesting(int i) noexcept { m_nSelectedClassListIdx = i; }
    void          SetSelectedStallListIdxForTesting(int i) noexcept { m_nSelectedStallListIdx = i; }
    void          SetSelectedObjectIndexForTesting(std::uint32_t i) noexcept { m_dwSelectedObjectIndex = i; }
    void          SetStallInfoForTesting(int idx, const StallPriceInfo& s) noexcept {
        if (idx >= 0 && idx < static_cast<int>(MAX_STALLITEM_NUM)) {
            m_arrStallInfo[idx] = s;
        }
    }
    const StallPriceInfo& GetStallInfoForTesting(int idx) const noexcept {
        return m_arrStallInfo[idx];
    }

    // ----------------------------------------------------------------
    // Children accessors for tests (1:1 with legacy m_pItemTypeCombo /
    // m_arrItemDetailTypeCombo[ITEM_TYPE_COUNT] / m_pItemList /
    // m_pClassList / m_pStallList / m_pSellModeRadioBtn /
    // m_pBuyModeRadioBtn / m_pNameStatic / m_pPriceStatic /
    // m_parrPageBtn[5] / m_parrPageUpDownBtn[2]).
    // ----------------------------------------------------------------
    cComboBox*       GetItemTypeCombo()       const noexcept { return m_pItemTypeCombo; }
    cComboBox*       GetItemDetailTypeCombo(int idx) const noexcept;
    cListDialog*     GetItemList()             const noexcept { return m_pItemList; }
    cListDialog*     GetClassList()            const noexcept { return m_pClassList; }
    cListDialog*     GetStallList()            const noexcept { return m_pStallList; }
    cPushupButton*   GetSellModeRadioBtn()    const noexcept { return m_pSellModeRadioBtn; }
    cPushupButton*   GetBuyModeRadioBtn()     const noexcept { return m_pBuyModeRadioBtn; }
    cStatic*         GetNameStatic()           const noexcept { return m_pNameStatic; }
    cStatic*         GetPriceStatic()          const noexcept { return m_pPriceStatic; }
    cPushupButton*   GetPageBtn(int idx)      const noexcept;
    cButton*         GetPageUpDownBtn(int idx) const noexcept;

private:
    // Children (24+ total). Created in Linking; non-owning raw
    // pointers for legacy 1:1 accessors.
    cComboBox*       m_pItemTypeCombo = nullptr;
    cComboBox*       m_arrItemDetailTypeCombo[ITEM_TYPE_COUNT] = {};
    cListDialog*     m_pItemList = nullptr;
    cListDialog*     m_pClassList = nullptr;
    cListDialog*     m_pStallList = nullptr;
    cPushupButton*   m_pSellModeRadioBtn = nullptr;
    cPushupButton*   m_pBuyModeRadioBtn = nullptr;
    cStatic*         m_pNameStatic = nullptr;
    cStatic*         m_pPriceStatic = nullptr;
    cPushupButton*   m_parrPageBtn[MAX_RESULT_PAGE] = {};
    cButton*         m_parrPageUpDownBtn[2] = {};

    // State (1:1 with legacy).
    bool             m_bSearchedAll = false;
    std::uint32_t    m_dwSearchType = SK_SELL;

    int              m_nStallCount = 0;
    StallPriceInfo   m_arrStallInfo[MAX_STALLITEM_NUM] = {};

    int              m_nBasePage = 0;
    int              m_nMaxPage  = 0;
    int              m_nCurrentPage = -1;

    int              m_nItemType = 0;
    int              m_nItemDetailType = 0;

    int              m_nSelectedItemListIdx = -1;
    int              m_nSelectedClassListIdx = -1;
    int              m_nSelectedStallListIdx = -1;

    std::uint32_t    m_dwSelectedObjectIndex = 0;
    std::uint32_t    m_dwPrevSelectedType = SK_SELL;

    // Item catalog (1:1 with legacy cPtrList<TItemInfo>; modern
    // uses std::vector<ItemInfo>).
    std::vector<ItemInfo> m_ptrItemInfo;

    // Per-ID delay state (1:1 with legacy static DWORD dwPrevTime[5]).
    std::uint32_t    m_dwPrevTime[5] = {};

    // Internal helper.
    void clearStallList();
    void resetSelection();
};

} // namespace mxh::ui
