#include "cquestdialog.hpp"

#include "mxh/ui/clistdialog.hpp"

#include <algorithm>

namespace mxh::ui {

void cQuestDialog::Linking() {
    m_quest_list = dynamic_cast<cListDialog*>(
        findWindowByLegacyId("QUE_QUESTLIST"));
    syncQuestList();
}

void cQuestDialog::syncQuestList() {
    if (!m_quest_list) return;
    m_quest_list->RemoveAll();
    for (const auto& quest : m_quests) {
        // The legacy quest sheet displays the live title in the list. Status
        // and reward are presented by the detail controls, not fabricated
        // into the resource-backed list row.
        m_quest_list->AddItem(quest.title);
    }
    m_quest_list->SetCurSelectedRowIdx(
        m_selected < m_quests.size() ? static_cast<int>(m_selected) : -1);
}

void cQuestDialog::AddQuest(QuestEntry q) {
    if (q.id == 0 || q.title.empty()) return;
    if (std::none_of(m_quests.begin(), m_quests.end(),
                     [&](const auto& x) { return x.id == q.id; })) {
        m_quests.push_back(std::move(q));
        syncQuestList();
    }
}

void cQuestDialog::ClearQuests() {
    m_quests.clear();
    m_selected = static_cast<std::size_t>(-1);
    syncQuestList();
}

bool cQuestDialog::RemoveQuest(std::uint32_t id) {
    const auto it = std::find_if(m_quests.begin(), m_quests.end(),
                                 [id](const auto& quest) { return quest.id == id; });
    if (it == m_quests.end()) return false;
    const auto removed = static_cast<std::size_t>(std::distance(m_quests.begin(), it));
    m_quests.erase(it);
    if (m_quests.empty()) m_selected = static_cast<std::size_t>(-1);
    else if (m_selected > removed) --m_selected;
    else if (m_selected == removed) m_selected = std::min(m_selected, m_quests.size() - 1);
    syncQuestList();
    return true;
}

bool cQuestDialog::UpdateQuest(std::uint32_t id, QuestStatus s) {
    for (auto& q : m_quests) {
        if (q.id == id) {
            q.status = s;
            syncQuestList();
            return true;
        }
    }
    return false;
}

bool cQuestDialog::Select(std::size_t i) noexcept {
    if (i >= m_quests.size()) return false;
    m_selected = i;
    if (m_quest_list) {
        m_quest_list->SetCurSelectedRowIdx(static_cast<int>(i));
    }
    return true;
}

std::uint32_t cQuestDialog::ActionEvent(std::int32_t mouseX,
                                        std::int32_t mouseY,
                                        std::uint32_t mouseFlags) {
    const auto event = cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
    if (m_quest_list) {
        const int row = m_quest_list->GetCurSelectedRowIdx();
        if (row >= 0) Select(static_cast<std::size_t>(row));
    }
    return event;
}

const QuestEntry* cQuestDialog::Selected() const noexcept {
    return m_selected < m_quests.size() ? &m_quests[m_selected] : nullptr;
}

bool cQuestDialog::ClaimSelected() {
    auto* q = Selected();
    if (!q || q->status != QuestStatus::Completed) return false;
    if (m_quest_service && !m_quest_service->claimQuest(q->id)) return false;
    if (m_claim && !m_claim(*q)) return false;
    return UpdateQuest(q->id, QuestStatus::Claimed);
}

}  // namespace mxh::ui
