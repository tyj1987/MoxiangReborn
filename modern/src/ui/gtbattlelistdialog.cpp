#include "gtbattlelistdialog.hpp"

#include "mxh/ui/cListCtrl.hpp"

#include <cstdio>
#include <cstring>

namespace mxh::ui {

namespace {
constexpr std::size_t kBufferSize = 256;
}

cGTBattleListDialog::cGTBattleListDialog() = default;
cGTBattleListDialog::~cGTBattleListDialog() = default;

char cGTBattleListDialog::GroupInitial(GTBattleGroup group) noexcept {
    switch (group) {
    case GTBattleGroup::A: return 'A';
    case GTBattleGroup::B: return 'B';
    case GTBattleGroup::C: return 'C';
    case GTBattleGroup::D: return 'D';
    default: return '?';
    }
}

void cGTBattleListDialog::SetCallbacks(ChatTextFn chatText,
                                       InsertRowFn onInsertRow,
                                       ObserverJoinFn onObserverJoin,
                                       void* userData) noexcept {
    m_chatTextFn = chatText;
    m_onInsertRowFn = onInsertRow;
    m_observerJoinFn = onObserverJoin;
    m_callbackUserData = userData;
}

void cGTBattleListDialog::SetBattlesForTest(const GTBattleInfo* battles,
                                           std::size_t count) noexcept {
    m_testBattles = battles;
    m_testCount = count;
}

void cGTBattleListDialog::SetControlsForTest(cListCtrl* ctrl) noexcept {
    m_pBattleListCtrl = ctrl;
}

void cGTBattleListDialog::Linking() {
    m_pBattleListCtrl = dynamic_cast<cListCtrl*>(findWindowById(kBattleListId));
    m_BattleList.clear();
    m_BattleCount = 0;
    m_bPlayOff = false;
    m_nPreSelectedIndex = kNoSelection;
}

void cGTBattleListDialog::SetActive(bool val) noexcept {
    cDialog::SetActive(val);
    if (!val) {
        DeleteAllBattleInfo();
    }
}

const GTBattleInfo* cGTBattleListDialog::FindBattle(std::uint32_t battleId) const noexcept {
    for (const auto& b : m_BattleList) {
        if (b.battleId == battleId) {
            return &b;
        }
    }
    return nullptr;
}

bool cGTBattleListDialog::hasBattle(std::uint32_t battleId) const noexcept {
    return FindBattle(battleId) != nullptr;
}

void cGTBattleListDialog::ApplySelectionColor(std::size_t rowIdx) {
    if (!m_pBattleListCtrl) {
        return;
    }
    // Color application is a no-op for the test-only cListCtrl API since
    // we don't expose row mutation by index. Track selection separately.
    (void)rowIdx;
}

void cGTBattleListDialog::RestoreColor(std::size_t rowIdx) {
    if (!m_pBattleListCtrl) {
        return;
    }
    (void)rowIdx;
}

void cGTBattleListDialog::HandleMouseAction(std::int32_t mouseX,
                                            std::int32_t mouseY,
                                            std::uint32_t weFromDialog) {
    if (!isActive()) {
        return;
    }
    if (!m_pBattleListCtrl) {
        return;
    }
    if ((weFromDialog & kActionRowClick) == 0) {
        return;
    }
    const std::int32_t idx = m_pBattleListCtrl->selectedRowIdx();
    if (idx < 0 || static_cast<std::uint32_t>(idx) >= m_BattleCount) {
        return;
    }
    if (m_nPreSelectedIndex != idx) {
        ApplySelectionColor(static_cast<std::size_t>(idx));
        if (m_nPreSelectedIndex > kNoSelection) {
            RestoreColor(static_cast<std::size_t>(m_nPreSelectedIndex));
        }
    }
    m_nPreSelectedIndex = idx;
}

bool cGTBattleListDialog::FormatBattleText(char* buf, std::size_t bufSize,
                                            const GTBattleInfo& info) const {
    if (!buf || bufSize == 0) {
        return false;
    }
    const char* groupFmt = m_chatTextFn
        ? m_chatTextFn(m_bPlayOff ? kPlayOffMessageId : kGroupMessageId,
                       m_callbackUserData)
        : nullptr;
    char groupBuf[kBufferSize]{};
    if (groupFmt) {
        std::snprintf(groupBuf, sizeof(groupBuf), groupFmt,
                      GroupInitial(info.group));
    }
    const char* pairFmt = m_chatTextFn
        ? m_chatTextFn(kGuildPairMessageId, m_callbackUserData)
        : nullptr;
    char pairBuf[kBufferSize]{};
    if (pairFmt) {
        std::snprintf(pairBuf, sizeof(pairBuf), pairFmt,
                      info.guildName1, info.guildName2);
    }
    std::snprintf(buf, bufSize, "%s|%s", groupBuf, pairBuf);
    return true;
}

void cGTBattleListDialog::InsertRowCtrl(const GTBattleInfo& info) {
    if (!m_pBattleListCtrl) {
        return;
    }
    if (m_onInsertRowFn) {
        m_onInsertRowFn(info.battleId, m_callbackUserData);
    }
    char col0[kBufferSize]{};
    char col1[kBufferSize]{};
    if (m_chatTextFn) {
        const char* groupFmt = m_chatTextFn(
            m_bPlayOff ? kPlayOffMessageId : kGroupMessageId, m_callbackUserData);
        if (groupFmt) {
            std::snprintf(col0, sizeof(col0), groupFmt,
                          GroupInitial(info.group));
        }
        const char* pairFmt = m_chatTextFn(kGuildPairMessageId,
                                           m_callbackUserData);
        if (pairFmt) {
            std::snprintf(col1, sizeof(col1), pairFmt,
                          info.guildName1, info.guildName2);
        }
    }
    cListCtrl::Row row{};
    row.texts = {col0, col1};
    row.colors = {kDefaultRgb, kDefaultRgb};
    m_pBattleListCtrl->AddRow(std::move(row));
}

void cGTBattleListDialog::RefreshBattleList() {
    if (!m_pBattleListCtrl) {
        return;
    }
    m_pBattleListCtrl->RemoveAll();
    std::size_t count = m_useTestBattles ? m_testCount : m_BattleList.size();
    const GTBattleInfo* source = m_useTestBattles
        ? m_testBattles
        : (m_BattleList.empty() ? nullptr : m_BattleList.data());
    for (std::size_t i = 0; i < count; ++i) {
        InsertRowCtrl(source[i]);
    }
}

void cGTBattleListDialog::AddBattleInfo(const GTBattleInfo& info) {
    if (std::strlen(info.guildName1) == 0 || std::strlen(info.guildName2) == 0) {
        return;
    }
    m_BattleList.push_back(info);
    ++m_BattleCount;
}

bool cGTBattleListDialog::DeleteBattleInfo(std::uint32_t battleId) {
    auto it = m_BattleList.begin();
    for (; it != m_BattleList.end(); ++it) {
        if (it->battleId == battleId) {
            m_BattleList.erase(it);
            if (m_BattleCount > 0) {
                --m_BattleCount;
            }
            RefreshBattleList();
            return true;
        }
    }
    return false;
}

void cGTBattleListDialog::DeleteAllBattleInfo() {
    if (m_pBattleListCtrl) {
        m_pBattleListCtrl->RemoveAll();
    }
    m_bPlayOff = false;
    m_BattleCount = 0;
    m_nPreSelectedIndex = kNoSelection;
    m_BattleList.clear();
}

void cGTBattleListDialog::DeleteAddBattleInfo() {
    DeleteAllBattleInfo();
}

bool cGTBattleListDialog::EnterBattleOnObserver() {
    if (!m_pBattleListCtrl || m_nPreSelectedIndex < 0
        || static_cast<std::size_t>(m_nPreSelectedIndex) >= m_BattleList.size()) {
        return false;
    }
    const GTBattleInfo& info = m_BattleList[static_cast<std::size_t>(m_nPreSelectedIndex)];
    if (m_observerJoinFn) {
        return m_observerJoinFn(info.battleId, m_callbackUserData);
    }
    return true;
}

} // namespace mxh::ui
