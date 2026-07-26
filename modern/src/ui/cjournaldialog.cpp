// cjournaldialog.cpp — modern port of 墨香 CJournalDialog.

#include "mxh/ui/cjournaldialog.hpp"
#include "mxh/ui/ccheckbox.hpp"
#include "mxh/ui/clistdialog.hpp"
#include "mxh/ui/cPushupButton.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>

namespace mxh::ui {

namespace {

// 1:1 with legacy COMBINEKEY macro: packs (a, b) into a
// single int with base-100 / base-1000 / base-10000 packing.
std::int32_t combineKey(std::int32_t a, std::int32_t b) {
    std::int32_t c = 0;
    if (b < 100)            c = a * 100 + b;
    else if (b < 1000)      c = a * 1000 + b;
    else if (b < 10000)     c = a * 10000 + b;
    return c;
}

void safeStrCpy(char* dst, const char* src, std::size_t cap) {
    if (!dst || !src) return;
    std::strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

}  // namespace

cJournalDialog::cJournalDialog() {
    m_BasePage = 0;
    m_MaxPage = 0;
    m_CurPage = 0;
    m_bSavedJournal = false;
    std::memset(m_bCheckItem, 0, sizeof(m_bCheckItem));
}

cJournalDialog::~cJournalDialog() {
    // 1:1 with legacy destructor: free the items in both lists.
    // std::unique_ptr cleans them up automatically.
    m_JournalList.clear();
    m_JournalSavedList.clear();
}

void cJournalDialog::Linking() {
    // 1:1 with legacy Linking.  The legacy walks the WINDOW_ID
    // tree to find cButton / cCheckBox / cPushupButton /
    // cListDialog children; the modern port's WINDOW_ID tree
    // is deferred (cWindowManager doesn't expose a full lookup
    // yet).  Tests inject the children via SetChildWindowsForTest
    // on cDialog, and SetPage(0) is the legacy "open on first
    // page" call.
    SetPage(0);
}

void cJournalDialog::JournalItemAdd(const JournalInfo& info) {
    if (info.Kind == static_cast<std::uint32_t>(JournalKind::Quest)) {
        std::int32_t key = combineKey(info.Param, 0);
        const char* title = m_questTitleCb
            ? m_questTitleCb(static_cast<std::uint32_t>(key), m_questTitleUser)
            : "";
        auto item = std::make_unique<JournalItem>();
        item->type = JournalKind::Quest;
        item->JournalDBIndex = info.Index;
        item->questTitle = title ? title : "";
        item->param1 = static_cast<std::uint32_t>(info.Param_3);   // bCompleted
        item->param2 = static_cast<std::uint32_t>(info.Param_2);   // SubQuestIndex
        item->bSaved = info.bSaved;                                 // 1:1 with legacy
        safeStrCpy(item->regDate, info.RegDate, sizeof(item->regDate));
        AddList(std::move(item), info.bSaved != 0);
    } else if (info.Kind == static_cast<std::uint32_t>(JournalKind::Wanted)) {
        auto item = std::make_unique<JournalItem>();
        item->type = JournalKind::Wanted;
        item->JournalDBIndex = info.Index;
        // 1:1: the legacy uses std::strncpy on item->Name
        // (char[17]) with cap MAX_NAME_LENGTH+1 = 17.  The
        // modern port stores Name in std::string and assigns.
        item->name = info.ParamName;
        item->param1 = static_cast<std::uint32_t>(info.Param);
        item->bSaved = info.bSaved;                                 // 1:1
        safeStrCpy(item->regDate, info.RegDate, sizeof(item->regDate));
        AddList(std::move(item), info.bSaved != 0);
    } else if (info.Kind == static_cast<std::uint32_t>(JournalKind::Levelup)) {
        auto item = std::make_unique<JournalItem>();
        item->type = JournalKind::Levelup;
        item->JournalDBIndex = info.Index;
        item->param1 = static_cast<std::uint32_t>(info.Param);  // level
        item->bSaved = info.bSaved;                              // 1:1
        safeStrCpy(item->regDate, info.RegDate, sizeof(item->regDate));
        AddList(std::move(item), info.bSaved != 0);
    }
    JournalListReset();
}

void cJournalDialog::AddList(std::unique_ptr<JournalItem> item, bool bSaved) {
    if (!item) return;
    // 1:1 with legacy AddList: append to m_JournalList; if
    // bSaved, also append a copy to m_JournalSavedList.
    m_JournalList.push_back(std::move(item));
    if (bSaved) {
        auto copy = std::make_unique<JournalItem>(*m_JournalList.back());
        m_JournalSavedList.push_back(std::move(copy));
    }
}

void cJournalDialog::JournalReset() {
    // 1:1 with legacy JournalReset: clear everything + repaint.
    m_JournalList.clear();
    m_JournalSavedList.clear();
    m_BasePage     = 0;
    m_MaxPage      = 0;
    m_CurPage      = 0;
    m_bSavedJournal = false;
    std::memset(m_bCheckItem, 0, sizeof(m_bCheckItem));
}

void cJournalDialog::JournalListReset() {
    // 1:1 with legacy JournalListReset.  Recompute m_MaxPage
    // from the active list, walk items, set ViewIndex on the
    // items visible on the current page, fire the chatmsg
    // lookups for the visible items.  The legacy also writes
    // the resulting strings into m_pTextList (cListDialog);
    // the modern cListDialog has no AddItem API so the host
    // reads the visible items via the test hooks.
    int viewStartIdx = kJournalViewPerPage * m_CurPage;
    int count = 0;
    int viewIndex = 0;

    auto& active = m_bSavedJournal ? m_JournalSavedList : m_JournalList;
    int itemCount = static_cast<int>(active.size());
    m_MaxPage = itemCount / kJournalViewPerPage - 1;
    if (itemCount % kJournalViewPerPage) ++m_MaxPage;
    if (m_MaxPage < 0) m_MaxPage = 0;

    for (auto& item : active) {
        if (count >= viewStartIdx && count < viewStartIdx + kJournalViewPerPage) {
            item->ViewIndex = viewIndex;
            ++viewIndex;
        } else {
            item->ViewIndex = -1;
        }
        ++count;
    }

    SetPage(m_CurPage - m_BasePage);
}

void cJournalDialog::SetBasePage(bool bNext) {
    // 1:1 with legacy SetBasePage.
    int basePageBackup = m_BasePage;
    if (bNext) {
        if (m_BasePage + kMaxJournalPageBtn <= m_MaxPage) {
            m_BasePage += kMaxJournalPageBtn;
        }
    } else {
        if (m_BasePage - kMaxJournalPageBtn >= 0) {
            m_BasePage -= kMaxJournalPageBtn;
        }
    }
    if (basePageBackup != m_BasePage) {
        SetPage(0);
    }
}

void cJournalDialog::SetPage(int index) {
    // 1:1 with legacy SetPage.  The legacy paints the 5 page
    // buttons (pushup-btns) and 2 move-btns.  The modern
    // cPushupButton port is minimal (no SetText); tests can
    // introspect m_BasePage / m_CurPage to verify the math.
    int showPage = m_MaxPage - m_BasePage;
    if (showPage > 4) showPage = 4;

    // Page button loop is a no-op in the modern port (the
    // legacy paints the page-btns + activates / deactivates
    // them; cPushupButton::SetActive + SetPush + SetText
    // are minimal in the modern port, so we just record
    // m_BasePage and m_CurPage here).
    (void)showPage;
    (void)index;

    // Clear m_bCheckItem (1:1 with legacy SetPage tail).
    std::memset(m_bCheckItem, 0, sizeof(m_bCheckItem));

    // 1:1 with legacy: if the page didn't change, no reset.
    if (m_BasePage + index == m_CurPage) return;
    m_CurPage = m_BasePage + index;
    JournalListReset();
}

void cJournalDialog::SetItemCheck(int index) noexcept {
    // 1:1 with legacy SetItemCheck: toggle the checkbox.
    if (index < 0 || index >= kMaxCheckboxPerPage) return;
    m_bCheckItem[index] = !m_bCheckItem[index];
}

bool cJournalDialog::isItemChecked(int index) const noexcept {
    if (index < 0 || index >= kMaxCheckboxPerPage) return false;
    return m_bCheckItem[index];
}

void cJournalDialog::SelectedJournalSave() {
    // 1:1 with legacy SelectedJournalSave.  For each checked
    // item, find the matching JournalItem (ViewIndex == i) in
    // the live list, copy to the saved list, mark bSaved, send
    // the network message.  Hard cap: 50 saved items.
    if (static_cast<int>(m_JournalSavedList.size()) >= kMaxJournalSavedList) {
        if (m_chatMsgCb) {
            m_chatMsgCb(kChatMsgSavedListFull, m_chatMsgUser);
        }
        return;
    }
    for (int i = 0; i < kMaxCheckboxPerPage; ++i) {
        if (m_bCheckItem[i]) {
            for (auto& item : m_JournalList) {
                if (item->ViewIndex == i) {
                    if (item->bSaved) break;
                    auto copy = std::make_unique<JournalItem>(*item);
                    item->bSaved = 1;
                    copy->bSaved = 1;
                    if (m_sendNetMsgCb) {
                        m_sendNetMsgCb(copy->JournalDBIndex,
                                       static_cast<std::uint32_t>(JournalNetOp::Update),
                                       m_sendNetMsgUser);
                    }
                    m_JournalSavedList.push_back(std::move(copy));
                    break;
                }
            }
            m_bCheckItem[i] = false;
        }
    }
    JournalListReset();
}

void cJournalDialog::SelectedJournalDelete() {
    // 1:1 with legacy SelectedJournalDelete: walks the saved
    // list, removes the matching ViewIndex item, sends the
    // delete net message.
    for (int i = 0; i < kMaxCheckboxPerPage; ++i) {
        if (m_bCheckItem[i]) {
            for (auto& item : m_JournalSavedList) {
                if (item->ViewIndex == i) {
                    if (m_sendNetMsgCb) {
                        m_sendNetMsgCb(item->JournalDBIndex,
                                       static_cast<std::uint32_t>(JournalNetOp::Delete),
                                       m_sendNetMsgUser);
                    }
                    // The legacy removes the item from the saved
                    // list + frees it.  Mark the item bSaved=0
                    // and the parent list keeps a tombstone.
                    item->bSaved = 0;
                    item->ViewIndex = -1;
                    break;
                }
            }
            m_bCheckItem[i] = false;
        }
    }
    // The legacy removes the cleared items from m_JournalSavedList;
    // the modern port does an actual erase in-place.  Tests
    // can verify by reading m_JournalSavedList.size().
    m_JournalSavedList.erase(
        std::remove_if(m_JournalSavedList.begin(), m_JournalSavedList.end(),
                       [](const std::unique_ptr<JournalItem>& it) {
                           return it->bSaved == 0;
                       }),
        m_JournalSavedList.end());
    JournalListReset();
}

void cJournalDialog::ViewJournalListToggle() {
    // 1:1 with legacy ViewJournalListToggle.  Flips
    // m_bSavedJournal + resets the cur page + clears the
    // checkboxes + repaints.
    m_bSavedJournal = !m_bSavedJournal;
    m_CurPage = 0;
    std::memset(m_bCheckItem, 0, sizeof(m_bCheckItem));
    JournalListReset();
}

}  // namespace mxh::ui
