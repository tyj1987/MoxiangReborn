#pragma once

#include "mxh/ui/cTabDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cWantedDialog;
class cQuestDialog;
class cJournalDialog;
class cPushupButton;

// Forward declaration of legacy JOURNALINFO / QUEST_ITEM_INFO shapes used by
// the dialog's forwarding surface. We define minimal POD stubs here so that
// the port can be compiled and tested without dragging the full quest system
// runtime in.
struct QuestJournalInfo {
    std::int32_t kind = 0;
    std::int32_t param = 0;
    std::uint32_t questId = 0;
    char text[64]{};
};

struct QuestItemInfo {
    std::uint32_t itemIdx = 0;
    std::uint32_t data = 0;
    std::uint32_t type = 0;
};

struct QuestString {
    std::uint32_t questId = 0;
    char title[64]{};
};

class cQuestTotalDialog final : public cTabDialog {
public:
    // Main-bar icon API. Legacy pushes OPT_QUESTDLGICON + SetAlram(FALSE)
    // when activating. The modern port routes both calls through these
    // function pointers; default no-op.
    using SetPushBarIconFn = void (*)(std::int32_t iconId, bool active,
                                     void* userData);
    using SetAlramFn = void (*)(std::int32_t iconId, bool on,
                                void* userData);

    static constexpr std::int32_t kQuestDialogIconId = 78;  // OPT_QUESTDLGICON
    static constexpr std::int32_t kNoSelectedQuestId = 0;
    static constexpr std::size_t kMaxSubDialogs = 3;

    cQuestTotalDialog();
    ~cQuestTotalDialog() override;

    cQuestTotalDialog(const cQuestTotalDialog&) = delete;
    cQuestTotalDialog& operator=(const cQuestTotalDialog&) = delete;

    void RegisterSubDialog(cWindow* window);
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy forwarding methods. Bodies are no-ops when the
    // corresponding sub-dialog pointer is null.
    void JournalItemAdd(const QuestJournalInfo& info);
    void CompleteQuestDelete(const QuestString& questString);
    void ProcessQuestAdd(const QuestString& questString);
    void ProcessQuestDelete(const QuestString& questString);

    void QuestItemAdd(const QuestItemInfo& info, std::uint32_t count);
    void QuestItemDelete(std::uint32_t itemIdx);
    std::uint32_t QuestItemUpdate(std::uint32_t type, std::uint32_t itemIdx,
                                  std::uint32_t data);

    std::uint32_t GetSelectedQuestID() const;
    void CloseMsgBox();
    void GiveupQuestDelete(std::uint32_t questIdx);

    void QuestListView();
    void JournalView();

    void UpdateSubQuestData();

    // Test/host injection.
    void SetSubDialogsForTest(cWantedDialog* wanted,
                              cQuestDialog* quest,
                              cJournalDialog* journal) noexcept;
    void SetMainBarCallbacks(SetPushBarIconFn pushBarIcon,
                             SetAlramFn setAlram,
                             void* userData = nullptr) noexcept;

    // Read accessors.
    cWantedDialog* wantedDialog() const noexcept { return m_pWantedDlg; }
    cQuestDialog* questDialog() const noexcept { return m_pQuestDlg; }
    cJournalDialog* journalDialog() const noexcept { return m_pJournalDlg; }

private:
    void NotifyMainBar(bool active);
    void NotifyMainBarAlram(bool on);

    cWantedDialog* m_pWantedDlg = nullptr;
    cQuestDialog* m_pQuestDlg = nullptr;
    cJournalDialog* m_pJournalDlg = nullptr;

    std::uint8_t m_curIdx1 = 0;
    std::uint8_t m_curIdx2 = 0;

    SetPushBarIconFn m_pushBarIconFn = nullptr;
    SetAlramFn m_setAlramFn = nullptr;
    void* m_callbackUserData = nullptr;
};

} // namespace mxh::ui
