// helpdialog.hpp — modern port of 墨香 cHelpDialog (in-game
// help browser: shows a list of dialogues for the current page +
// clickable hyperlink rows that navigate to other pages).
//
// 1:1 port of legacy `cHelpDialog` from
//   `墨香【源码】\[Client]MH\HelpDialog.{h,cpp}` (≈180 B header
//   + ≈200 line .cpp; modern port preserves the data model +
//   state machine + linking flow and stubs the engine-side
//   HELPDICMGR (HelpDicManager) with a test-injectable content
//   pointer until Phase 13+ real impl lands).
//
// Modern port scope (this commit):
// - cHelpDialog : public cDialog.
// - 1 cListDialogEx* m_pListDlg (1:1 with legacy; the list
//   shows the page's dialogues + clickable hyperlink rows).
// - std::uint32_t m_dwCurPageId (1:1 with legacy).
// - HYPER m_sHyper[MAX_REGIST_HYPERLINK] (1:1 with legacy; the
//   index maps each clickable row to a HYPERLINK payload).
// - int m_nHyperCount (1:1 with legacy).
// - HYPER struct (1:1 with legacy `struct HYPER` from
//   `[CC]Header/CommonGameStruct.h`).
// - LINKTYPE enum (1:1 with legacy LINKTYPE / emLink_*).
// - MAX_REGIST_HYPERLINK constant (1:1 with legacy 70).
// - HI_LISTDLG constant (1:1 with legacy WindowIDEnum value;
//   the modern port uses a local 1-based id since the global
//   legacy id space isn't shared).
// - 1:1 surface: SetActive(BOOL) override / Linking / OpenDialog
//   / OpenLinkPage / EndDialog / GetHyperInfo / HyperLinkParser.
// - SetContent(mainPage, dialogueList, hyperTextList) replaces
//   the engine-side HELPDICMGR singleton lookup; the engine-
//   binder layer (Phase 14+) sets these once at dialog load.
//
// Modern-port simplifications (all documented in the .cpp file
// header):
// 1. **HELPDICMGR singleton is stubbed.** The legacy uses
//    `HELPDICMGR->GetMainPage / GetPage / GetDialogueList /
//    GetHyperTextList` (engine-side singleton). Modern port
//    uses 3 member pointers (m_pMainPage, m_pDialogueList,
//    m_pHyperTextList) set via SetContent() at dialog load.
//    The engine-binder layer (Phase 14+) will swap in the
//    real HELPDICMGR queries.
// 2. **cListItem::AddItem is replaced with cListDialogEx::
//    AddLinkItem + AddLinkItemChain.** The legacy cHelpDialog
//    uses `m_pListDlg->cListItem::AddItem(pItem)` which is a
//    multi-inheritance diamond (cListDialogEx : public
//    cListDialog, public cListItem). The modern port uses
//    the equivalent cListDialogEx API directly.
// 3. **LINKITEM is a local struct.** The legacy LINKITEM
//    (engine-side linked-list item) has string / rgb /
//    NextItem fields. Modern port uses a local struct with
//    the same fields for 1:1 behaviour, owned by cHelpDialog
//    (the OpenDialog flow heap-allocates and the dtor frees).
// 4. **LINKITEM allocation is via std::unique_ptr.** Legacy
//    uses raw `new LINKITEM` + manual `delete` in the dtor.
//    Modern uses std::vector<std::unique_ptr<LINKITEM>> for
//    automatic cleanup.
// 5. **LINKTYPE enum is local.** Legacy emLink_Null=0 /
//    emLink_Page=1 / emLink_End=2 / emLink_Open=3 / etc.
//    Modern port mirrors the first 4 entries (the 3 actually
//    used by cHelpDialog::HyperLinkParser + emLink_Null as
//    the no-link marker); the remaining 7 (Quest /
//    MapChange / Image / etc.) are reserved as enum values
//    for forward compatibility with cNpcScriptDialog.
// 6. **HYPER / HYPERLINK / DIALOGUE / cPage / cDialogueList /
//    cHyperTextList are reused** from cpage.hpp / cdialoguelist.hpp
//    / chypertextlist.hpp (already 1:1 ported).
// 7. **Render is a no-op.** The legacy cHelpDialog doesn't
//    override Render; cDialog::Render (also a no-op) is the
//    default.

#pragma once

#include "cDialog.hpp"
#include "cPage.hpp"          // HYPERLINK + cPage + cPageBase
#include "cDialogueList.hpp"  // DIALOGUE + cDialogueList
#include "cHyperTextList.hpp" // cHyperTextList

#include <cstdint>
#include <memory>
#include <vector>

namespace mxh::ui {

class cListDialogEx;

class cHelpDialog : public cDialog {
public:
    // -------------------------------------------------------------------------
    // Constants (1:1 with legacy CommonGameStruct.h).
    // -------------------------------------------------------------------------
    static constexpr std::uint32_t MAX_REGIST_HYPERLINK = 70;

    // Local id (1:1 with legacy HI_LISTDLG from WindowIDEnum.h;
    // the modern port uses a local 0-based offset since the
    // global legacy id space isn't shared).
    static constexpr std::int32_t ID_DLG    = 0;
    static constexpr std::int32_t ID_LISTDLG = 1;

    // Link type enum (1:1 with legacy LINKTYPE / emLink_*).
    // The first 4 entries are the ones cHelpDialog::HyperLinkParser
    // actually consumes (Null / Page / End / Open). The remaining
    // 7 are reserved as enum values for forward compatibility
    // with cNpcScriptDialog (Quest / MapChange / Image / etc.).
    enum LinkType : std::uint16_t {
        emLink_Null          = 0,
        emLink_Page          = 1,
        emLink_End           = 2,
        emLink_Open          = 3,
        emLink_Quest         = 4,
        emLink_QuestLink     = 5,
        emLink_QuestStart    = 6,
        emLink_QuestContinue = 7,
        emLink_QuestEnd      = 8,
        emLink_MapChange     = 9,
        emLink_EventQuestStart = 10,
        emLink_Image         = 11,
    };

    // 1:1 with legacy `struct HYPER` from CommonGameStruct.h.
    // Renamed to `HelpHyper` to avoid collision with the
    // Windows SDK's `HYPER` typedef (defined in <winnt.h> as
    // a 64-bit integer struct).
    struct HelpHyper {
        bool      bUse          = false;
        std::uint32_t dwListItemIdx = 0;
        HYPERLINK sHyper{};

        void Init() noexcept {
            bUse = false;
            dwListItemIdx = 0;
            sHyper = HYPERLINK{};
        }
    };

    // 1:1 with legacy `struct LINKITEM` (engine-side linked-list
    // item). The legacy cHelpDialog::OpenDialog walks the page
    // dialogues + page hyperlinks and creates one LINKITEM per
    // row (heap-allocated). The modern port uses the same
    // struct + std::unique_ptr for the owned storage.
    struct LINKITEM {
        char          string[1024] = {};
        std::uint32_t rgb          = 0;
        std::uint32_t dwType       = 0;
        LINKITEM*     NextItem     = nullptr;
    };

    cHelpDialog();
    ~cHelpDialog() override;

    cHelpDialog(const cHelpDialog&) = delete;
    cHelpDialog& operator=(const cHelpDialog&) = delete;

    // -------------------------------------------------------------------------
    // Engine-binder layer (Phase 14+) calls SetContent once at
    // dialog load to wire the help content. The modern port
    // replaces the engine-side HELPDICMGR singleton with these
    // 3 member pointers; the OpenDialog / OpenLinkPage flow
    // uses them directly.
    //
    // The test layer also uses SetContent to inject a fixed
    // main page + dialogue list + hyper text list, so the
    // OpenDialog flow can be exercised without the engine.
    // -------------------------------------------------------------------------
    void SetContent(cPage* mainPage,
                    cDialogueList* dialogueList,
                    cHyperTextList* hyperTextList) noexcept;

    // -------------------------------------------------------------------------
    // 1:1 with legacy. After Linking, m_pListDlg is resolved by
    // id (legacy GetWindowForID equivalent via modern
    // findWindowById).
    // -------------------------------------------------------------------------
    void Linking();

    // -------------------------------------------------------------------------
    // SetActive(BOOL) override: 1:1 with legacy. val==FALSE
    // triggers EndDialog; val==TRUE is a no-op (the legacy
    // SetActive calls cDialog::SetActiveRecursive). Modern
    // port uses the standard cDialog::SetActive + an
    // override-only pre-hook (EndDialog on close).
    // -------------------------------------------------------------------------
    void SetActive(bool val) noexcept override;

    // -------------------------------------------------------------------------
    // 1:1 with legacy. OpenDialog initializes the HYPER array
    // + clears the list + populates the list with the main
    // page's dialogues + hyperlink rows. Returns false if the
    // content isn't set or the main page is missing.
    // -------------------------------------------------------------------------
    bool OpenDialog();

    // -------------------------------------------------------------------------
    // 1:1 with legacy. OpenLinkPage swaps the current page to
    // the given page id (used when the user clicks a hyperlink
    // row). Returns false if the page id is not found.
    // -------------------------------------------------------------------------
    bool OpenLinkPage(std::uint32_t dwPageId);

    // -------------------------------------------------------------------------
    // 1:1 with legacy. EndDialog clears the HYPER array +
    // clears the list + resets m_nHyperCount.
    // -------------------------------------------------------------------------
    void EndDialog();

    // -------------------------------------------------------------------------
    // 1:1 with legacy. GetHyperInfo returns the HelpHyper* for the
    // given list item idx, or nullptr if not found.
    // -------------------------------------------------------------------------
    HelpHyper* GetHyperInfo(std::uint32_t dwIdx);

    // -------------------------------------------------------------------------
    // 1:1 with legacy. HyperLinkParser handles the link type
    // dispatch: emLink_Page → OpenLinkPage(dwData);
    //            emLink_Open → (no-op stub; engine-side chat
    //                          dialog open);
    //            emLink_End  → EndDialog().
    // The other emLink_* types (Quest / MapChange / Image) are
    // reserved for cNpcScriptDialog and ignored here.
    // -------------------------------------------------------------------------
    void HyperLinkParser(std::uint32_t dwIdx);

    // -------------------------------------------------------------------------
    // Accessors (1:1 with legacy).
    // -------------------------------------------------------------------------
    cListDialogEx* GetListDlg() const noexcept           { return m_pListDlg; }
    std::uint32_t  GetCurPageId() const noexcept         { return m_dwCurPageId; }
    int            GetHyperCount() const noexcept        { return m_nHyperCount; }
    HelpHyper*         GetHyperAt(int idx);
    cPage*         GetMainPage() const noexcept          { return m_pMainPage; }
    cDialogueList* GetDialogueList() const noexcept      { return m_pDialogueList; }
    cHyperTextList* GetHyperTextList() const noexcept    { return m_pHyperTextList; }

    // Test-only: get the LINKITEMs owned by cHelpDialog (the
    // modern port uses std::vector<unique_ptr<LINKITEM>> for
    // automatic cleanup; the legacy uses raw new + manual
    // delete in the dtor).
    const std::vector<std::unique_ptr<LINKITEM>>&
    GetLinkItemsForTesting() const noexcept { return m_linkItems; }

private:
    // 1:1 with legacy member fields.
    cListDialogEx* m_pListDlg           = nullptr;
    std::uint32_t  m_dwCurPageId        = 0;
    HelpHyper      m_sHyper[MAX_REGIST_HYPERLINK] = {};
    int            m_nHyperCount        = 0;

    // Modern-port additions: the 3 content pointers (replace
    // the engine-side HELPDICMGR singleton).
    cPage*         m_pMainPage          = nullptr;
    cDialogueList* m_pDialogueList      = nullptr;
    cHyperTextList* m_pHyperTextList    = nullptr;

    // Modern-port storage for the heap-allocated LINKITEMs
    // created by OpenDialog / OpenLinkPage. The legacy uses
    // raw new + manual delete; modern uses unique_ptr.
    std::vector<std::unique_ptr<LINKITEM>> m_linkItems;

    // 1:1 with legacy private helper.
    void InitHyperArray() noexcept;
    void ClearLinkItems() noexcept;
    void PopulateFromPage(cPage* pPage) noexcept;
};

} // namespace mxh::ui
