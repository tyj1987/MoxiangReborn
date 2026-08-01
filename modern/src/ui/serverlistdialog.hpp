// Modern 1:1 port of legacy CServerListDialog.
//
// GAMERESRCMNGR server data and TITLE->ConnectToServer are host-injected.
// Linking still resolves the exact legacy WindowIDs, and ActionEvent keeps
// the original WE_ROWCLICK / WE_ROWDBLCLICK priority and color transitions.
// Null and negative-index guards only replace legacy invalid-input crashes.
#pragma once

#include "mxh/ui/cDialog.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mxh::ui {

class cButton;
class cListCtrl;

// 1:1 layout of legacy SEVERLIST from GameResourceStruct.h.
struct ServerListEntry {
    char DistributeIP[16] = "211.233.35.36";
    std::uint16_t DistributePort = 400;
    char ServerName[64] = "Test";
    std::uint16_t ServerNo = 1;
    std::int32_t bEnter = 1;
};

static_assert(sizeof(ServerListEntry) == 88);

class cServerListDialog final : public cDialog {
public:
    static constexpr std::int32_t kServerListDialogId = 1066;
    static constexpr std::int32_t kListCtrlId = 1067;
    static constexpr std::int32_t kConnectButtonId = 1068;
    static constexpr std::int32_t kExitButtonId = 1069;

    static constexpr std::uint32_t kRowClickEvent = 4096;
    static constexpr std::uint32_t kRowDoubleClickEvent = 4194304;

    static constexpr std::uint32_t kDefaultColor = 0xFFFFFFFFu;
    static constexpr std::uint32_t kSelectedColor = 0xFFFFEA00u;
    static constexpr std::uint32_t kUnavailableColor = 0xFFFF0000u;

    struct Row {
        std::uint32_t id = 0;
        std::string indexText;
        std::string serverName;
        std::uint32_t indexColor = kDefaultColor;
        std::uint32_t serverNameColor = kDefaultColor;
    };

    using ConnectToServerCallback = void(*)(std::int32_t index, void* user);

    cServerListDialog();
    ~cServerListDialog() override;

    cServerListDialog(const cServerListDialog&) = delete;
    cServerListDialog& operator=(const cServerListDialog&) = delete;

    void Linking();
    void LoadServerList();
    std::uint32_t ActionEvent(void* mouseInfo);

    std::int32_t GetSelectedIndex() const noexcept { return m_nIndex; }
    std::int32_t maxServerNum() const noexcept { return m_nMaxServerNum; }

    void SetServerListSource(const ServerListEntry* entries, std::int32_t count) noexcept;
    void SetConnectToServerCallback(ConnectToServerCallback callback, void* user) noexcept;

    void SetControlsForTest(cListCtrl* listCtrl, cButton* connectButton,
                            cButton* exitButton) noexcept;
    void SetLastActionEventWeForTest(std::uint32_t we) noexcept { m_lastWe = we; }

    cListCtrl* serverListCtrl() const noexcept { return m_pServerListCtrl; }
    cButton* connectButton() const noexcept { return m_pConnectBtn; }
    cButton* exitButton() const noexcept { return m_pExitBtn; }
    std::size_t rowCount() const noexcept { return m_rows.size(); }
    const Row& rowAt(std::size_t index) const { return m_rows.at(index); }

private:
    bool IsValidServerIndex(std::int32_t index) const noexcept;
    void SetRowColor(std::int32_t index, std::uint32_t color);
    void SyncListControl();

    cListCtrl* m_pServerListCtrl = nullptr;
    std::int32_t m_nMaxServerNum = 0;
    std::int32_t m_nIndex = -1;
    cButton* m_pConnectBtn = nullptr;
    cButton* m_pExitBtn = nullptr;

    const ServerListEntry* m_pServerList = nullptr;
    std::int32_t m_serverListCount = 0;
    std::vector<Row> m_rows;

    ConnectToServerCallback m_connectCallback = nullptr;
    void* m_connectUser = nullptr;
    std::uint32_t m_lastWe = 0;
};

}  // namespace mxh::ui
