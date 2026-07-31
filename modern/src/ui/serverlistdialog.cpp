#include "serverlistdialog.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cListCtrl.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mxh::ui {

cServerListDialog::cServerListDialog() = default;

cServerListDialog::~cServerListDialog() {
    if (m_pServerListCtrl != nullptr) {
        m_pServerListCtrl->RemoveAll();
    }
}

void cServerListDialog::SetServerListSource(const ServerListEntry* entries,
    std::int32_t count) noexcept {
    m_pServerList = entries;
    m_serverListCount = count;
}

void cServerListDialog::SetConnectToServerCallback(
    ConnectToServerCallback callback, void* user) noexcept {
    m_connectCallback = callback;
    m_connectUser = user;
}

void cServerListDialog::SetControlsForTest(cListCtrl* listCtrl,
    cButton* connectButton,
    cButton* exitButton) noexcept {
    m_pServerListCtrl = listCtrl;
    m_pConnectBtn = connectButton;
    m_pExitBtn = exitButton;
}

void cServerListDialog::Linking() {
    m_pServerListCtrl = dynamic_cast<cListCtrl*>(findWindowById(kListCtrlId));
    m_pConnectBtn = dynamic_cast<cButton*>(findWindowById(kConnectButtonId));
    m_pExitBtn = dynamic_cast<cButton*>(findWindowById(kExitButtonId));
    LoadServerList();
}

void cServerListDialog::LoadServerList() {
    m_nMaxServerNum = std::max(m_serverListCount, 0);

    if (m_pServerList != nullptr) {
        for (std::int32_t index = 0; index < m_nMaxServerNum; ++index) {
            const ServerListEntry& entry = m_pServerList[index];
            Row row;
            row.id = static_cast<std::uint32_t>(index + 1);
            row.indexText = std::to_string(index + 1);

            std::size_t nameLength = 0;
            while (nameLength < sizeof(entry.ServerName) &&
                   entry.ServerName[nameLength] != 0) {
                ++nameLength;
            }
            row.serverName.assign(entry.ServerName, nameLength);

            const std::uint32_t color = entry.bEnter != 0
                ? kDefaultColor
                : kUnavailableColor;
            row.indexColor = color;
            row.serverNameColor = color;

            m_rows.push_back(std::move(row));
        }
    }

    SyncListControl();
}

bool cServerListDialog::IsValidServerIndex(std::int32_t index) const noexcept {
    return index >= 0 &&
           index < m_nMaxServerNum &&
           index < m_serverListCount &&
           static_cast<std::size_t>(index) < m_rows.size() &&
           m_pServerList != nullptr;
}

void cServerListDialog::SetRowColor(std::int32_t index, std::uint32_t color) {
    if (index < 0 || static_cast<std::size_t>(index) >= m_rows.size()) {
        return;
    }
    Row& row = m_rows[static_cast<std::size_t>(index)];
    row.indexColor = color;
    row.serverNameColor = color;
}

void cServerListDialog::SyncListControl() {
    if (m_pServerListCtrl == nullptr) {
        return;
    }

    m_pServerListCtrl->RemoveAll();
    for (const Row& row : m_rows) {
        cListCtrl::Row listRow;
        listRow.texts = {row.indexText, row.serverName};
        listRow.colors = {row.indexColor, row.serverNameColor};
        m_pServerListCtrl->AddRow(std::move(listRow));
    }

    if (m_nIndex >= 0 &&
        static_cast<std::size_t>(m_nIndex) < m_rows.size()) {
        m_pServerListCtrl->SetSelectedRowIdx(m_nIndex);
    } else {
        m_pServerListCtrl->ClearSelection();
    }
}

std::uint32_t cServerListDialog::ActionEvent(void* /*mouseInfo*/) {
    const std::uint32_t we = m_lastWe;
    m_lastWe = 0;

    if (!isActive()) {
        return 0;
    }
    if (m_pServerListCtrl == nullptr) {
        return we;
    }

    const std::int32_t index = m_pServerListCtrl->selectedRowIdx();
    if ((we & kRowClickEvent) != 0) {
        if (IsValidServerIndex(index)) {
            if (m_nIndex != index) {
                SetRowColor(index, m_pServerList[index].bEnter != 0
                    ? kSelectedColor
                    : kUnavailableColor);

                if (m_nIndex > -1 && IsValidServerIndex(m_nIndex)) {
                    SetRowColor(m_nIndex, m_pServerList[m_nIndex].bEnter != 0
                        ? kDefaultColor
                        : kUnavailableColor);
                }
            }
            m_nIndex = index;
            SyncListControl();
        }
    } else if ((we & kRowDoubleClickEvent) != 0) {
        if (IsValidServerIndex(index) && m_connectCallback != nullptr) {
            m_connectCallback(index, m_connectUser);
        }
    }

    return we;
}

}  // namespace mxh::ui
