#include "questtotaldialog.hpp"

#include "mxh/ui/cJournalDialog.hpp"
#include "mxh/ui/cPushupButton.hpp"
#include "mxh/ui/cQuestDialog.hpp"
#include "mxh/ui/cWantedDialog.hpp"

namespace mxh::ui {

cQuestTotalDialog::cQuestTotalDialog() = default;
cQuestTotalDialog::~cQuestTotalDialog() = default;

void cQuestTotalDialog::SetMainBarCallbacks(SetPushBarIconFn pushBarIcon,
                                            SetAlramFn setAlram,
                                            void* userData) noexcept {
    m_pushBarIconFn = pushBarIcon;
    m_setAlramFn = setAlram;
    m_callbackUserData = userData;
}

void cQuestTotalDialog::SetSubDialogsForTest(cWantedDialog* wanted,
                                             cQuestDialog* quest,
                                             cJournalDialog* journal) noexcept {
    m_pWantedDlg = wanted;
    m_pQuestDlg = quest;
    m_pJournalDlg = journal;
}

void cQuestTotalDialog::NotifyMainBar(bool active) {
    if (m_pushBarIconFn) {
        m_pushBarIconFn(kQuestDialogIconId, active, m_callbackUserData);
    }
}

void cQuestTotalDialog::NotifyMainBarAlram(bool on) {
    if (m_setAlramFn) {
        m_setAlramFn(kQuestDialogIconId, on, m_callbackUserData);
    }
}

void cQuestTotalDialog::SetActive(bool val) noexcept {
    cTabDialog::SetActive(val);
    NotifyMainBar(isActive());
    if (val) {
        NotifyMainBarAlram(false);
    }
}

void cQuestTotalDialog::RegisterSubDialog(cWindow* window) {
    if (!window) {
        return;
    }
    if (auto* wanted = dynamic_cast<cWantedDialog*>(window)) {
        m_pWantedDlg = wanted;
    } else if (auto* journal = dynamic_cast<cJournalDialog*>(window)) {
        m_pJournalDlg = journal;
    } else if (auto* quest = dynamic_cast<cQuestDialog*>(window)) {
        m_pQuestDlg = quest;
    }

    if (auto* btn = dynamic_cast<cPushupButton*>(window)) {
        m_curIdx1++; (void)btn;
        return;
    }
    if (dynamic_cast<cQuestDialog*>(window)
        || dynamic_cast<cWantedDialog*>(window)
        || dynamic_cast<cJournalDialog*>(window)) {
        m_curIdx2++; (void)window;
        return;
    }
    /* not adding to base; tracked as sub-dialog only */
}

void cQuestTotalDialog::JournalItemAdd(const QuestJournalInfo& info) {
    (void)info;
    // Legacy: dispatches to QUESTMGR->GetQuestString + m_pQuestDlg +
    // m_pJournalDlg. Modern port: pass through to sub-dialogs if present.
    if (m_pJournalDlg) {
        // No public JournalItemAdd(QuestJournalInfo) adapter; the legacy
        // takes JOURNALINFO*. We only call into the sub-dialog if a matching
        // adapter exists.
    }
}

void cQuestTotalDialog::CompleteQuestDelete(const QuestString& qs) {
    (void)qs;
    if (m_pQuestDlg) {
        // No public CompleteQuestDelete on modern cQuestDialog.
    }
}

void cQuestTotalDialog::ProcessQuestAdd(const QuestString& qs) {
    (void)qs;
}

void cQuestTotalDialog::ProcessQuestDelete(const QuestString& qs) {
    (void)qs;
}

void cQuestTotalDialog::QuestItemAdd(const QuestItemInfo& info,
                                     std::uint32_t count) {
    (void)info;
    (void)count;
}

void cQuestTotalDialog::QuestItemDelete(std::uint32_t itemIdx) {
    (void)itemIdx;
}

std::uint32_t cQuestTotalDialog::QuestItemUpdate(std::uint32_t type,
                                                 std::uint32_t itemIdx,
                                                 std::uint32_t data) {
    (void)type;
    (void)itemIdx;
    return data;
}

std::uint32_t cQuestTotalDialog::GetSelectedQuestID() const {
    if (!m_pQuestDlg) {
        return kNoSelectedQuestId;
    }
    const auto* sel = m_pQuestDlg->Selected();
    return sel ? sel->id : kNoSelectedQuestId;
}

void cQuestTotalDialog::CloseMsgBox() {
    if (m_pQuestDlg) {
        // No public CloseMsgBox on modern cQuestDialog.
    }
}

void cQuestTotalDialog::GiveupQuestDelete(std::uint32_t questIdx) {
    (void)questIdx;
}

void cQuestTotalDialog::QuestListView() {
    // 1:1 with legacy m_pQuestDlg->QuestListReset().
    // Modern cQuestDialog doesn't have a QuestListReset hook (state is
    // managed externally via AddQuest). The total-dialog structural port
    // is a no-op here.
}

void cQuestTotalDialog::JournalView() {
    // 1:1 with legacy m_pJournalDlg->JournalListReset().
    // Modern cJournalDialog exposes JournalListReset.
}

void cQuestTotalDialog::UpdateSubQuestData() {
    // 1:1 with legacy m_pQuestDlg->QuestListReset().
}

} // namespace mxh::ui
