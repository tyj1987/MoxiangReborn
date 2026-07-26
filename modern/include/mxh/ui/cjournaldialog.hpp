// cjournaldialog.hpp — modern port of 墨香 CJournalDialog (quest / wanted /
// level-up journal).
//
// 1:1 port of legacy `CJournalDialog` from
//   `墨香【源码】\[Client]MH\JournalDialog.{h,cpp}`.
//
// The journal dialog has two views:
//   * m_bSavedJournal=false  -> the live m_JournalList (quest +
//     wanted + levelup items accumulated during the current
//     play session).
//   * m_bSavedJournal=true   -> the saved m_JournalSavedList
//     (capped at 50 entries, persistent across sessions).
//
// Each view is paged: JOURNALVIEW_PER_PAGE = 5 items per page.
// The legacy uses cPtrList to hold the items; the modern port
// uses std::vector<std::unique_ptr<JournalItem>>.  The
// legacy's QUESTMGR / CHATMGR / CQuestString / NETWORK
// dependencies are stubbed via host-injected callbacks.
//
// The 1:1 surface kept:
//   * JournalItem struct (1:1 with legacy)
//   * JournalItemAdd(quest / wanted / levelup) variants
//   * JournalListReset (rebuilds the visible page)
//   * SetBasePage(BOOL bNext)
//   * SetPage(int Index)
//   * SelectedJournalSave / SelectedJournalDelete
//   * ViewJournalListToggle
//   * m_bCheckItem[5] / SetItemCheck
//   * m_bSavedJournal getter

#pragma once

#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cwindow.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mxh::ui {

class cButton;
class cCheckBox;
class cPushupButton;
class cListDialog;

// 1:1 with legacy CommonGameDefine.h constants used by the
// journal dialog.
inline constexpr std::int32_t kMaxJournalPageBtn     = 5;
inline constexpr std::int32_t kMaxCheckboxPerPage    = 5;
inline constexpr std::int32_t kJournalViewPerPage    = 5;
inline constexpr std::int32_t kMaxJournalSavedList   = 50;

// 1:1 with legacy eJournal_Kind enum.
enum class JournalKind : std::uint32_t {
    Quest   = 0,
    Wanted  = 1,
    Levelup = 2,
};

// 1:1 with legacy eJournal_Update / eJournal_Delete.
enum class JournalNetOp : std::uint32_t {
    Update = 0,
    Delete = 1,
};

// 1:1 with legacy eJournal_Wanted_* outcome enum.
enum class JournalWantedResult : std::uint32_t {
    Doing              = 0,
    Succeed            = 1,
    FailbyOther        = 2,
    FailbyDelChr       = 3,
    MurderedbyChr      = 4,
    FailbyBeWantedChr  = 5,
    FailbyTime         = 6,
    FailbyDie          = 7,
};

// 1:1 with legacy JournalItem struct (legacy JournalDialog.h).
// The legacy uses CQuestString pointers; the modern port uses
// std::string (the quest title is the only field the legacy
// reads).  Param_1 is overloaded (quest: bCompleted, wanted:
// result, levelup: level).
struct JournalItem {
    JournalKind    type = JournalKind::Quest;
    std::uint32_t  JournalDBIndex = 0;
    std::int32_t   ViewIndex      = -1;
    std::string    questTitle;          // 1:1 with pQuestString->GetTitle()
    std::string    subQuestTitle;       // 1:1 with pSubQuestString (legacy: unused)
    std::string    name;                // 1:1 with Name (wanted: target name)
    char           regDate[11] = {};    // 1:1 with RegDate
    std::uint32_t  param1 = 0;
    std::uint32_t  param2 = 0;
    std::uint32_t  bSaved = 0;
};

// 1:1 with legacy JOURNALINFO input.  The struct preserves
// the legacy field order (used by JournalItemAdd).
struct JournalInfo {
    std::uint32_t  Index = 0;
    std::uint32_t  Kind  = 0;       // 0=Quest, 1=Wanted, 2=Levelup
    std::int32_t   Param = 0;
    std::int32_t   Param_2 = 0;
    std::int32_t   Param_3 = 0;      // bCompleted (quest)
    char           ParamName[32] = {};
    char           RegDate[11] = {};
    std::uint32_t  bSaved = 0;
    std::uint32_t  wMoveMapNum = 0;
    std::uint32_t  dwChangeMapState = 0;
};

class cJournalDialog : public cDialog {
public:
    cJournalDialog();
    ~cJournalDialog() override;

    cJournalDialog(const cJournalDialog&) = delete;
    cJournalDialog& operator=(const cJournalDialog&) = delete;

    // 1:1 with legacy Linking.  Wires the child widgets.
    void Linking();

    // 1:1 with legacy JournalReset.  Clears all items +
    // resets page counters + repaints.
    void JournalReset();

    // 1:1 with legacy JournalItemAdd(JOURNALINFO*).
    void JournalItemAdd(const JournalInfo& info);

    // 1:1 with legacy JournalListReset.  Repaints the
    // current page from m_JournalList or m_JournalSavedList
    // depending on m_bSavedJournal.
    void JournalListReset();

    // 1:1 with legacy SetBasePage(BOOL bNext).  bNext=true
    // advances the page-button group by MAX_JOURNAL_PAGEBTN;
    // bNext=false retreats.
    void SetBasePage(bool bNext);

    // 1:1 with legacy SetPage(int Index).
    void SetPage(int index);

    // 1:1 with legacy IsSavedJournal / SetItemCheck.
    bool isSavedJournal() const noexcept { return m_bSavedJournal; }
    void SetItemCheck(int index) noexcept;

    // 1:1 with legacy SelectedJournalSave / SelectedJournalDelete.
    void SelectedJournalSave();
    void SelectedJournalDelete();

    // 1:1 with legacy ViewJournalListToggle.  Switches
    // between the live list and the saved list.
    void ViewJournalListToggle();

    // Test introspection.
    int                basePage() const noexcept { return m_BasePage; }
    int                maxPage()  const noexcept { return m_MaxPage; }
    int                curPage()  const noexcept { return m_CurPage; }
    int                liveListCount()  const noexcept { return static_cast<int>(m_JournalList.size()); }
    int                savedListCount() const noexcept { return static_cast<int>(m_JournalSavedList.size()); }
    bool               isItemChecked(int index) const noexcept;

    // Test hook -- inject a "send net message" callback.  The
    // legacy sends SEND_JOURNAL_DWORD to MP_JOURNAL; the
    // modern port routes the (Index, type) pair through this
    // callback.  type is JournalNetOp (0=Update, 1=Delete).
    using SendNetMsgCallback = void(*)(std::uint32_t index,
                                       std::uint32_t type,
                                       void* user);
    void SetSendNetMsgCallbackForTest(SendNetMsgCallback cb, void* user) {
        m_sendNetMsgCb = cb; m_sendNetMsgUser = user;
    }

    // Test hook -- inject a "look up quest title" callback
    // (legacy QUESTMGR->GetQuestString(Key)->GetTitle()).
    using QuestTitleCallback = const char*(*)(std::uint32_t questKey, void* user);
    void SetQuestTitleCallbackForTest(QuestTitleCallback cb, void* user) {
        m_questTitleCb = cb; m_questTitleUser = user;
    }

    // Test hook -- inject a "chatmsg lookup" callback (legacy
    // CHATMGR->GetChatMsg).  The dialog uses chatmsg 599
    // (incomplete), 600 (complete), 614 ("Wanted"),
    // 615..616/606/636/684 (wanted result messages),
    // 631..634 (save-button text), 635 (levelup message),
    // 671 (saved-list-full).
    using ChatMsgCallback = const char*(*)(int chatMsgId, void* user);
    void SetChatMsgCallbackForTest(ChatMsgCallback cb, void* user) {
        m_chatMsgCb = cb; m_chatMsgUser = user;
    }

    // 1:1 chatmsg ids used by the journal dialog.
    static constexpr int kChatMsgQuestIncomplete     = 599;
    static constexpr int kChatMsgQuestComplete       = 600;
    static constexpr int kChatMsgWantedHeader        = 614;
    static constexpr int kChatMsgWantedSucceed       = 615;
    static constexpr int kChatMsgWantedFailbyOther   = 616;
    static constexpr int kChatMsgWantedFailbyDelChr  = 606;
    static constexpr int kChatMsgMurderedbyChr       = 636;
    static constexpr int kChatMsgWantedFailbyTime    = 684;
    static constexpr int kChatMsgSaveBtnLive         = 631;
    static constexpr int kChatMsgSaveBtnSaved        = 632;
    static constexpr int kChatMsgSavedListBtnLive    = 633;
    static constexpr int kChatMsgSavedListBtnSaved   = 634;
    static constexpr int kChatMsgLevelupFormat       = 635;
    static constexpr int kChatMsgSavedListFull       = 671;

    // 1:1 RGB colors used by m_pTextList->AddItem.
    static constexpr std::uint32_t kSubquestTitleColorSel = 0xFFFFC800u;
    static constexpr std::uint32_t kQuestDescColor        = 0xFFFFFFFFu;
    static constexpr std::uint32_t kQuestDescHighlight    = 0xFF32C8FAu;

    // 1:1 with legacy AddList.
    void AddList(std::unique_ptr<JournalItem> item, bool bSaved);

private:
    std::vector<std::unique_ptr<JournalItem>> m_JournalList;
    std::vector<std::unique_ptr<JournalItem>> m_JournalSavedList;
    int  m_BasePage     = 0;
    int  m_MaxPage      = 0;
    int  m_CurPage      = 0;
    bool m_bSavedJournal = false;
    bool m_bCheckItem[kMaxCheckboxPerPage] = {};

    SendNetMsgCallback m_sendNetMsgCb = nullptr;
    void*              m_sendNetMsgUser = nullptr;
    QuestTitleCallback m_questTitleCb = nullptr;
    void*              m_questTitleUser = nullptr;
    ChatMsgCallback    m_chatMsgCb = nullptr;
    void*              m_chatMsgUser = nullptr;
};

} // namespace mxh::ui
