#include "mxh/ui/serverlistdialog.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cListCtrl.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using mxh::ui::ServerListEntry;
using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cListCtrl;
using mxh::ui::cServerListDialog;

namespace {

ServerListEntry MakeServer(const char* name, bool enter) {
    ServerListEntry entry;
    std::memset(entry.ServerName, 0, sizeof(entry.ServerName));
    std::snprintf(entry.ServerName, sizeof(entry.ServerName), "%s", name);
    entry.bEnter = enter ? 1 : 0;
    return entry;
}

struct Harness {
    cServerListDialog dialog;
    cListCtrl* list = nullptr;
    cButton* connect = nullptr;
    cButton* exit = nullptr;
    std::vector<ServerListEntry> servers;

    Harness() {
        dialog.Init(0, 0, 300, 200, nullptr,
                    cServerListDialog::kServerListDialogId);

        auto listWindow = std::make_unique<cListCtrl>();
        listWindow->Init(0, 0, 200, 180, nullptr,
                         cServerListDialog::kListCtrlId);
        listWindow->InitListCtrl(2, 30);
        list = listWindow.get();
        dialog.Add(std::move(listWindow));

        auto connectWindow = std::make_unique<cButton>();
        connectWindow->Init(0, 0, 50, 20, nullptr, nullptr, nullptr,
                            {}, nullptr, cServerListDialog::kConnectButtonId);
        connect = connectWindow.get();
        dialog.Add(std::move(connectWindow));

        auto exitWindow = std::make_unique<cButton>();
        exitWindow->Init(0, 0, 50, 20, nullptr, nullptr, nullptr,
                         {}, nullptr, cServerListDialog::kExitButtonId);
        exit = exitWindow.get();
        dialog.Add(std::move(exitWindow));
    }

    void Link() {
        dialog.SetServerListSource(servers.data(),
            static_cast<std::int32_t>(servers.size()));
        dialog.Linking();
    }

    std::uint32_t Dispatch(std::uint32_t event, std::int32_t row) {
        dialog.SetActive(true);
        list->SetSelectedRowIdx(row);
        dialog.SetLastActionEventWeForTest(event);
        return dialog.ActionEvent(nullptr);
    }
};

struct ConnectCapture {
    std::int32_t calls = 0;
    std::int32_t index = -1;
};

void CaptureConnect(std::int32_t index, void* user) {
    auto* capture = static_cast<ConnectCapture*>(user);
    ++capture->calls;
    capture->index = index;
}

}  // namespace

TEST(ServerListDialogTest, ServerListEntryLayoutMatchesLegacy) {
    static_assert(sizeof(ServerListEntry) == 88);
    EXPECT_EQ(offsetof(ServerListEntry, DistributeIP), 0u);
    EXPECT_EQ(offsetof(ServerListEntry, DistributePort), 16u);
    EXPECT_EQ(offsetof(ServerListEntry, ServerName), 18u);
    EXPECT_EQ(offsetof(ServerListEntry, ServerNo), 82u);
    EXPECT_EQ(offsetof(ServerListEntry, bEnter), 84u);
}

TEST(ServerListDialogTest, ServerListEntryDefaultsMatchLegacy) {
    const ServerListEntry entry;
    EXPECT_STREQ(entry.DistributeIP, "211.233.35.36");
    EXPECT_EQ(entry.DistributePort, 400u);
    EXPECT_STREQ(entry.ServerName, "Test");
    EXPECT_EQ(entry.ServerNo, 1u);
    EXPECT_EQ(entry.bEnter, 1);
}

TEST(ServerListDialogTest, InheritsFromDialog) {
    static_assert(std::is_base_of_v<cDialog, cServerListDialog>);
    SUCCEED();
}

TEST(ServerListDialogTest, IsNonCopyable) {
    static_assert(!std::is_copy_constructible_v<cServerListDialog>);
    static_assert(!std::is_copy_assignable_v<cServerListDialog>);
    SUCCEED();
}

TEST(ServerListDialogTest, ConstantsMatchLegacy) {
    EXPECT_EQ(cServerListDialog::kServerListDialogId, 1128);
    EXPECT_EQ(cServerListDialog::kListCtrlId, 1129);
    EXPECT_EQ(cServerListDialog::kConnectButtonId, 1130);
    EXPECT_EQ(cServerListDialog::kExitButtonId, 1131);
    EXPECT_EQ(cServerListDialog::kRowClickEvent, 4096u);
    EXPECT_EQ(cServerListDialog::kRowDoubleClickEvent, 4194304u);
    EXPECT_EQ(cServerListDialog::kDefaultColor, 0xFFFFFFFFu);
    EXPECT_EQ(cServerListDialog::kSelectedColor, 0xFFFFEA00u);
    EXPECT_EQ(cServerListDialog::kUnavailableColor, 0xFFFF0000u);
}

TEST(ServerListDialogTest, ConstructorInitializesLegacyState) {
    cServerListDialog dialog;
    EXPECT_EQ(dialog.serverListCtrl(), nullptr);
    EXPECT_EQ(dialog.connectButton(), nullptr);
    EXPECT_EQ(dialog.exitButton(), nullptr);
    EXPECT_EQ(dialog.maxServerNum(), 0);
    EXPECT_EQ(dialog.GetSelectedIndex(), -1);
    EXPECT_EQ(dialog.rowCount(), 0u);
}

TEST(ServerListDialogTest, LinkingFindsControlsByExactIds) {
    Harness harness;
    harness.Link();
    EXPECT_EQ(harness.dialog.serverListCtrl(), harness.list);
    EXPECT_EQ(harness.dialog.connectButton(), harness.connect);
    EXPECT_EQ(harness.dialog.exitButton(), harness.exit);
}

TEST(ServerListDialogTest, LinkingLoadsServerRows) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true), MakeServer("Beta", false)};
    harness.Link();

    ASSERT_EQ(harness.dialog.rowCount(), 2u);
    ASSERT_EQ(harness.list->rowCount(), 2u);
    EXPECT_EQ(harness.dialog.maxServerNum(), 2);
    EXPECT_EQ(harness.dialog.rowAt(0).id, 1u);
    EXPECT_EQ(harness.dialog.rowAt(0).indexText, "1");
    EXPECT_EQ(harness.dialog.rowAt(0).serverName, "Alpha");
    EXPECT_EQ(harness.dialog.rowAt(1).id, 2u);
    EXPECT_EQ(harness.dialog.rowAt(1).indexText, "2");
    EXPECT_EQ(harness.dialog.rowAt(1).serverName, "Beta");
    EXPECT_EQ(harness.list->rowAt(1).texts[1], "Beta");
}

TEST(ServerListDialogTest, AvailableServerStartsWhite) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true)};
    harness.Link();
    EXPECT_EQ(harness.dialog.rowAt(0).indexColor, cServerListDialog::kDefaultColor);
    EXPECT_EQ(harness.dialog.rowAt(0).serverNameColor, cServerListDialog::kDefaultColor);
    EXPECT_EQ(harness.list->rowAt(0).colors[0], cServerListDialog::kDefaultColor);
    EXPECT_EQ(harness.list->rowAt(0).colors[1], cServerListDialog::kDefaultColor);
}

TEST(ServerListDialogTest, UnavailableServerStartsRed) {
    Harness harness;
    harness.servers = {MakeServer("Closed", false)};
    harness.Link();
    EXPECT_EQ(harness.dialog.rowAt(0).indexColor, cServerListDialog::kUnavailableColor);
    EXPECT_EQ(harness.dialog.rowAt(0).serverNameColor, cServerListDialog::kUnavailableColor);
    EXPECT_EQ(harness.list->rowAt(0).colors[0], cServerListDialog::kUnavailableColor);
    EXPECT_EQ(harness.list->rowAt(0).colors[1], cServerListDialog::kUnavailableColor);
}

TEST(ServerListDialogTest, ServerNameReadIsBoundedTo64Bytes) {
    Harness harness;
    ServerListEntry entry;
    std::memset(entry.ServerName, 'A', sizeof(entry.ServerName));
    entry.bEnter = 1;
    harness.servers = {entry};
    harness.Link();
    ASSERT_EQ(harness.dialog.rowCount(), 1u);
    EXPECT_EQ(harness.dialog.rowAt(0).serverName.size(), 64u);
    EXPECT_EQ(harness.dialog.rowAt(0).serverName, std::string(64, 'A'));
}

TEST(ServerListDialogTest, DestructorClearsAttachedListControl) {
    cListCtrl list;
    list.Init(0, 0, 100, 100, nullptr, cServerListDialog::kListCtrlId);
    list.InitListCtrl(2, 30);
    std::vector<ServerListEntry> servers = {MakeServer("Alpha", true)};

    {
        cServerListDialog dialog;
        dialog.SetControlsForTest(&list, nullptr, nullptr);
        dialog.SetServerListSource(servers.data(), 1);
        dialog.LoadServerList();
        ASSERT_EQ(list.rowCount(), 1u);
    }

    EXPECT_EQ(list.rowCount(), 0u);
}

TEST(ServerListDialogTest, InactiveActionReturnsNullAndDoesNotSelect) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true)};
    harness.Link();
    harness.list->SetSelectedRowIdx(0);
    harness.dialog.SetLastActionEventWeForTest(cServerListDialog::kRowClickEvent);

    EXPECT_EQ(harness.dialog.ActionEvent(nullptr), 0u);
    EXPECT_EQ(harness.dialog.GetSelectedIndex(), -1);
    EXPECT_EQ(harness.dialog.rowAt(0).indexColor, cServerListDialog::kDefaultColor);
}

TEST(ServerListDialogTest, RowClickSelectsAvailableServerInYellow) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true), MakeServer("Beta", true)};
    harness.Link();

    EXPECT_EQ(harness.Dispatch(cServerListDialog::kRowClickEvent, 1),
              cServerListDialog::kRowClickEvent);
    EXPECT_EQ(harness.dialog.GetSelectedIndex(), 1);
    EXPECT_EQ(harness.dialog.rowAt(1).indexColor, cServerListDialog::kSelectedColor);
    EXPECT_EQ(harness.dialog.rowAt(1).serverNameColor, cServerListDialog::kSelectedColor);
    EXPECT_EQ(harness.list->selectedRowIdx(), 1);
    EXPECT_EQ(harness.list->rowAt(1).colors[0], cServerListDialog::kSelectedColor);
}

TEST(ServerListDialogTest, RowClickKeepsUnavailableServerRed) {
    Harness harness;
    harness.servers = {MakeServer("Closed", false)};
    harness.Link();

    harness.Dispatch(cServerListDialog::kRowClickEvent, 0);
    EXPECT_EQ(harness.dialog.GetSelectedIndex(), 0);
    EXPECT_EQ(harness.dialog.rowAt(0).indexColor, cServerListDialog::kUnavailableColor);
    EXPECT_EQ(harness.list->rowAt(0).colors[0], cServerListDialog::kUnavailableColor);
}

TEST(ServerListDialogTest, RowClickRestoresPreviousAvailableServerToWhite) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true), MakeServer("Beta", true)};
    harness.Link();

    harness.Dispatch(cServerListDialog::kRowClickEvent, 0);
    harness.Dispatch(cServerListDialog::kRowClickEvent, 1);

    EXPECT_EQ(harness.dialog.rowAt(0).indexColor, cServerListDialog::kDefaultColor);
    EXPECT_EQ(harness.dialog.rowAt(1).indexColor, cServerListDialog::kSelectedColor);
    EXPECT_EQ(harness.list->rowAt(0).colors[0], cServerListDialog::kDefaultColor);
}

TEST(ServerListDialogTest, RowClickKeepsPreviousUnavailableServerRed) {
    Harness harness;
    harness.servers = {MakeServer("Closed", false), MakeServer("Beta", true)};
    harness.Link();

    harness.Dispatch(cServerListDialog::kRowClickEvent, 0);
    harness.Dispatch(cServerListDialog::kRowClickEvent, 1);

    EXPECT_EQ(harness.dialog.rowAt(0).indexColor, cServerListDialog::kUnavailableColor);
    EXPECT_EQ(harness.dialog.rowAt(1).indexColor, cServerListDialog::kSelectedColor);
}

TEST(ServerListDialogTest, ClickingSameRowPreservesItsColor) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true)};
    harness.Link();

    harness.Dispatch(cServerListDialog::kRowClickEvent, 0);
    harness.Dispatch(cServerListDialog::kRowClickEvent, 0);

    EXPECT_EQ(harness.dialog.GetSelectedIndex(), 0);
    EXPECT_EQ(harness.dialog.rowAt(0).indexColor, cServerListDialog::kSelectedColor);
}

TEST(ServerListDialogTest, RowClickAtMaxServerNumIsIgnored) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true), MakeServer("Beta", true)};
    harness.Link();

    EXPECT_EQ(harness.Dispatch(cServerListDialog::kRowClickEvent, 2),
              cServerListDialog::kRowClickEvent);
    EXPECT_EQ(harness.dialog.GetSelectedIndex(), -1);
}

TEST(ServerListDialogTest, NegativeSelectedRowIsIgnoredSafely) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true)};
    harness.Link();

    EXPECT_EQ(harness.Dispatch(cServerListDialog::kRowClickEvent, -1),
              cServerListDialog::kRowClickEvent);
    EXPECT_EQ(harness.dialog.GetSelectedIndex(), -1);
}

TEST(ServerListDialogTest, RowDoubleClickConnectsSelectedIndex) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true), MakeServer("Beta", true)};
    harness.Link();
    ConnectCapture capture;
    harness.dialog.SetConnectToServerCallback(&CaptureConnect, &capture);

    EXPECT_EQ(harness.Dispatch(cServerListDialog::kRowDoubleClickEvent, 1),
              cServerListDialog::kRowDoubleClickEvent);
    EXPECT_EQ(capture.calls, 1);
    EXPECT_EQ(capture.index, 1);
    EXPECT_EQ(harness.dialog.GetSelectedIndex(), -1);
}

TEST(ServerListDialogTest, RowDoubleClickConnectsUnavailableServer) {
    Harness harness;
    harness.servers = {MakeServer("Closed", false)};
    harness.Link();
    ConnectCapture capture;
    harness.dialog.SetConnectToServerCallback(&CaptureConnect, &capture);

    harness.Dispatch(cServerListDialog::kRowDoubleClickEvent, 0);
    EXPECT_EQ(capture.calls, 1);
    EXPECT_EQ(capture.index, 0);
}

TEST(ServerListDialogTest, RowClickWinsWhenBothEventBitsAreSet) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true), MakeServer("Beta", true)};
    harness.Link();
    ConnectCapture capture;
    harness.dialog.SetConnectToServerCallback(&CaptureConnect, &capture);

    const auto both = cServerListDialog::kRowClickEvent |
                      cServerListDialog::kRowDoubleClickEvent;
    EXPECT_EQ(harness.Dispatch(both, 1), both);
    EXPECT_EQ(harness.dialog.GetSelectedIndex(), 1);
    EXPECT_EQ(capture.calls, 0);
}

TEST(ServerListDialogTest, RowDoubleClickOutOfRangeDoesNotConnect) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true)};
    harness.Link();
    ConnectCapture capture;
    harness.dialog.SetConnectToServerCallback(&CaptureConnect, &capture);

    harness.Dispatch(cServerListDialog::kRowDoubleClickEvent, 1);
    EXPECT_EQ(capture.calls, 0);
}

TEST(ServerListDialogTest, ActionWithoutListControlReturnsEventSafely) {
    cServerListDialog dialog;
    dialog.SetActive(true);
    dialog.SetLastActionEventWeForTest(cServerListDialog::kRowClickEvent);
    EXPECT_EQ(dialog.ActionEvent(nullptr), cServerListDialog::kRowClickEvent);
    EXPECT_EQ(dialog.GetSelectedIndex(), -1);
}

TEST(ServerListDialogTest, InjectedEventIsConsumedAfterOneAction) {
    Harness harness;
    harness.servers = {MakeServer("Alpha", true)};
    harness.Link();
    harness.dialog.SetActive(true);
    harness.list->SetSelectedRowIdx(0);
    harness.dialog.SetLastActionEventWeForTest(cServerListDialog::kRowClickEvent);

    EXPECT_EQ(harness.dialog.ActionEvent(nullptr), cServerListDialog::kRowClickEvent);
    EXPECT_EQ(harness.dialog.ActionEvent(nullptr), 0u);
}
