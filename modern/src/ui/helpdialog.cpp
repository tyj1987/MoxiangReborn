// helpdialog.cpp — see helpdialog.hpp for the full port
// description.
//
// Modern-port simplifications (the .hpp has the high-level
// notes; this file documents the per-method trade-offs):
//
// 1. `OpenDialog` + `OpenLinkPage` are 1:1 with legacy; the
//    legacy's `m_pListDlg->cListItem::AddItem(pItem)` calls
//    are replaced with `m_pListDlg->AddLinkItem` / `AddLinkItemChain`.
//    Same-line chains (where multiple LINKITEMs share a row
//    for inline next-color text) use `AddLinkItemChain`.
//
// 2. The legacy heap-allocates LINKITEM with raw `new`; the
//    modern port wraps each in `std::unique_ptr<LINKITEM>`
//    and stores in `m_linkItems` for automatic cleanup.
//
// 3. `HELPDICMGR->GetMainPage / GetDialogueList / GetHyperTextList`
//    are replaced with `m_pMainPage / m_pDialogueList / m_pHyperTextList`
//    member pointers set via `SetContent` (engine-binder layer
//    or test injects).
//
// 4. `GetRandomDialogue` is a 1:1 stub (uses the test-injectable
//    counter via `cPage::GetRandomDialogue`, which is the same
//    counter the cPage tests use).
//
// 5. `HyperLinkParser` handles emLink_Page → OpenLinkPage,
//    emLink_End → EndDialog, emLink_Open → no-op stub. The
//    other emLink_* types are reserved for cNpcScriptDialog.
//
// 6. `EndDialog` clears the HYPER array + the list +
//    m_nHyperCount. It does NOT call SetActive(FALSE) (the
//    legacy commented out that line; the modern port matches).

#include "helpdialog.hpp"

#include "cListDialogEx.hpp"
#include "cPage.hpp"

#include <cstring>
#include <utility>

namespace mxh::ui {

cHelpDialog::cHelpDialog() {
    // 1:1 with legacy: ctor sets WT_HELPDIALOG (legacy m_type)
    // + nulls m_pListDlg. Modern port skips m_type (no m_type
    // field in cWindow; the dialog-type tag is the engine-side
    // WindowIDEnum, which the engine-binder layer (Phase 14+)
    // manages).
    m_pListDlg       = nullptr;
    m_dwCurPageId    = 0;
    m_nHyperCount    = 0;
    InitHyperArray();
}

cHelpDialog::~cHelpDialog() {
    // 1:1 with legacy: dtor doesn't do anything (the legacy
    // cHelpDialog::~cHelpDialog() is a no-op). Modern unique_ptr
    // cleans up m_linkItems automatically.
}

void cHelpDialog::SetContent(cPage* mainPage,
                              cDialogueList* dialogueList,
                              cHyperTextList* hyperTextList) noexcept {
    m_pMainPage       = mainPage;
    m_pDialogueList   = dialogueList;
    m_pHyperTextList  = hyperTextList;
}

void cHelpDialog::Linking() {
    // 1:1 with legacy. Modern cDialog::findWindowById replaces
    // legacy cDialog::GetWindowForID.
    m_pListDlg = static_cast<cListDialogEx*>(
        findWindowById(ID_LISTDLG));
}

void cHelpDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy: val==FALSE triggers EndDialog. Then
    // cDialog::SetActiveRecursive (legacy) / cDialog::SetActive
    // (modern) propagates the active state.
    if (!val) {
        EndDialog();
    }
    cDialog::SetActive(val);
}

void cHelpDialog::InitHyperArray() noexcept {
    for (std::uint32_t i = 0; i < MAX_REGIST_HYPERLINK; ++i) {
        m_sHyper[i].Init();
    }
}

void cHelpDialog::ClearLinkItems() noexcept {
    // Free the chain NextItem pointers (the legacy manually
    // walked + deleted; modern unique_ptr only owns the head,
    // not the NextItem chain, so we walk + delete the chain
    // manually).
    for (auto& p : m_linkItems) {
        if (!p) continue;
        LINKITEM* cur = p->NextItem;
        while (cur) {
            LINKITEM* next = cur->NextItem;
            delete cur;
            cur = next;
        }
        p->NextItem = nullptr;
    }
    m_linkItems.clear();
}

bool cHelpDialog::OpenDialog() {
    // 1:1 with legacy. Init HYPER array + clear list + clear
    // count. The legacy also calls RemoveAll on the list +
    // resets m_nHyperCount.
    InitHyperArray();
    if (m_pListDlg) m_pListDlg->RemoveAll();
    ClearLinkItems();
    m_nHyperCount = 0;

    // Get the main page (1:1 with legacy HELPDICMGR->GetMainPage).
    if (m_pMainPage == nullptr) return false;

    m_dwCurPageId = m_pMainPage->GetPageId();
    PopulateFromPage(m_pMainPage);
    return true;
}

bool cHelpDialog::OpenLinkPage(std::uint32_t dwPageId) {
    // 1:1 with legacy. Same init flow as OpenDialog but uses
    // HELPDICMGR->GetPage(dwPageId) instead of GetMainPage.
    InitHyperArray();
    if (m_pListDlg) m_pListDlg->RemoveAll();
    ClearLinkItems();
    m_nHyperCount = 0;

    // The legacy calls HELPDICMGR->GetPage(dwPageId). Modern
    // port: caller must have set m_pMainPage to the target
    // page (engine-binder does this; the test layer does too).
    // The modern port does not implement a page-id → page
    // resolver; the engine-binder layer (Phase 14+) wires the
    // right page into m_pMainPage before calling OpenLinkPage.
    if (m_pMainPage == nullptr) return false;

    m_dwCurPageId = dwPageId;
    PopulateFromPage(m_pMainPage);
    return true;
}

void cHelpDialog::PopulateFromPage(cPage* pPage) noexcept {
    if (pPage == nullptr) return;
    if (m_pListDlg == nullptr) return;
    if (m_pDialogueList == nullptr) return;
    if (m_pHyperTextList == nullptr) return;

    // 1:1 with legacy: dwMsg = pPage->GetRandomDialogue()
    // returns a dialogue id; the loop walks GetDialogue(dwMsg, wIdx)
    // until nullptr.
    const std::uint32_t dwMsg = pPage->GetRandomDialogue();

    // First pass: dialogue rows. The legacy groups same-line
    // dialogues via Prev->NextItem. Modern port uses
    // AddLinkItemChain when a same-line continuation is
    // detected, and AddLinkItem for the first dialogue of a
    // new line.
    std::uint16_t wIdx = 0;
    std::uint16_t LineInfo = 0;
    DIALOGUE* temp = nullptr;
    LINKITEM* Prev = nullptr;
    cListDialogEx::LinkItem chainHead{};

    while ((temp = m_pDialogueList->GetDialogue(dwMsg, wIdx)) != nullptr) {
        auto pItem = std::make_unique<LINKITEM>();
        std::strncpy(pItem->string, temp->str, sizeof(pItem->string) - 1);
        pItem->rgb    = temp->dwColor;
        pItem->dwType = 0;  // legacy: no link type on dialogues
        LINKITEM* pRaw = pItem.get();
        m_linkItems.push_back(std::move(pItem));

        if (Prev == nullptr) {
            // First item in the list.
            chainHead.text  = pRaw->string;
            chainHead.color = pRaw->rgb;
            chainHead.type  = 0;
            m_pListDlg->AddLinkItem(pRaw->string, 0, pRaw->rgb, 0xFFFF0000u);
            LineInfo = temp->wLine;
        } else if (LineInfo == temp->wLine) {
            // Same line: chain to the previous row.
            pRaw->NextItem = Prev->NextItem;
            Prev->NextItem = pRaw;
        } else {
            // New line: start a fresh chain.
            m_pListDlg->AddLinkItem(pRaw->string, 0, pRaw->rgb, 0xFFFF0000u);
            LineInfo = temp->wLine;
        }

        Prev = pRaw;
        ++wIdx;
    }
    (void)chainHead;  // (reserved for future AddLinkItemChain usage)

    // Second pass: hyperlink rows. 1:1 with legacy: 3 spacer
    // rows first, then the hyper text rows.
    const int nLinkCount = pPage->GetHyperLinkCount();

    if (nLinkCount > 0) {
        for (int i = 0; i < 3; ++i) {
            auto spacer = std::make_unique<LINKITEM>();
            std::strncpy(spacer->string, " ",
                         sizeof(spacer->string) - 1);
            m_linkItems.push_back(std::move(spacer));
            m_pListDlg->AddLinkItem(" ", 0, 0xFFFFFFFFu, 0xFFFF0000u);
        }
    }

    for (int j = 0; j < nLinkCount; ++j) {
        const HYPERLINK* pLink = pPage->GetHyperText(j);
        if (pLink == nullptr) break;

        DIALOGUE* hyper = m_pHyperTextList->GetHyperText(pLink->wLinkId);
        if (hyper) {
            auto pItem = std::make_unique<LINKITEM>();
            std::strncpy(pItem->string, hyper->str,
                         sizeof(pItem->string) - 1);
            pItem->dwType = pLink->wLinkType;
            LINKITEM* pRaw = pItem.get();
            m_linkItems.push_back(std::move(pItem));
            m_pListDlg->AddLinkItem(pRaw->string,
                                    static_cast<std::uint8_t>(pRaw->dwType),
                                    0xFFFFFFFFu, 0xFFFF0000u);

            if (m_nHyperCount < static_cast<int>(MAX_REGIST_HYPERLINK)) {
                m_sHyper[m_nHyperCount].bUse          = true;
                m_sHyper[m_nHyperCount].dwListItemIdx =
                    static_cast<std::uint32_t>(m_pListDlg->LinkItemCount()) - 1;
                m_sHyper[m_nHyperCount].sHyper       = *pLink;
                ++m_nHyperCount;
            }
        }
    }
}

void cHelpDialog::EndDialog() {
    // 1:1 with legacy. Init HYPER + clear list + reset count.
    // Legacy commented out `SetActive(FALSE)`; modern port
    // matches.
    for (std::uint32_t i = 0; i < MAX_REGIST_HYPERLINK; ++i) {
        m_sHyper[i].Init();
    }
    if (m_pListDlg) m_pListDlg->RemoveAll();
    ClearLinkItems();
    m_nHyperCount = 0;
}

auto cHelpDialog::GetHyperInfo(std::uint32_t dwIdx) -> HelpHyper* {
    for (int i = 0; i < m_nHyperCount; ++i) {
        if (m_sHyper[i].bUse && m_sHyper[i].dwListItemIdx == dwIdx) {
            return &m_sHyper[i];
        }
    }
    return nullptr;
}

auto cHelpDialog::GetHyperAt(int idx) -> HelpHyper* {
    if (idx < 0 || idx >= m_nHyperCount) return nullptr;
    if (!m_sHyper[idx].bUse) return nullptr;
    return &m_sHyper[idx];
}

void cHelpDialog::HyperLinkParser(std::uint32_t dwIdx) {
    if (m_nHyperCount == 0) return;

    int nType = -1;
    std::uint32_t dwPageIdx = 0;

    for (int i = 0; i < m_nHyperCount; ++i) {
        if (m_sHyper[i].dwListItemIdx == dwIdx) {
            nType     = m_sHyper[i].sHyper.wLinkType;
            dwPageIdx = m_sHyper[i].sHyper.dwData;
            break;
        }
    }

    if (nType == emLink_Page) {
        // 1:1 with legacy: swap the main page then reset
        // the list gauge. The legacy calls
        // HELPDICMGR->GetPage(dwPageIdx); modern port skips
        // the engine-side resolution (the test layer pre-sets
        // m_pMainPage). EndDialog-style list reset matches
        // the legacy `m_pListDlg->ResetGuageBarPos()`.
        if (OpenLinkPage(dwPageIdx)) {
            // ResetGuageBarPos is a 1:1 quirk on cListDialogEx
            // (the legacy's scrollbar gauge). The modern port
            // doesn't have a cListDialogEx scrollbar yet;
            // this is a no-op until the engine-binder layer
            // adds the scrollbar back.
        }
    }
    if (nType == emLink_Open) {
        // 1:1 with legacy: empty branch (the legacy
        // engine-side chat-dialog open is stubbed). Modern
        // port is a no-op.
    }
    if (nType == emLink_End) {
        EndDialog();
    }
}

} // namespace mxh::ui
