#include <gtest/gtest.h>
#include <cstdio>

#include <cstdlib>
#include <array>
#include <filesystem>

#include "ClientUiRuntime.hpp"
#include "CInGameState.hpp"
#include "mxh/ui/cEditBox.hpp"

namespace {

std::filesystem::path find_playdh_root() {
    if (const char* configured = std::getenv("MXH_PLAYDH_ROOT")) {
        return std::filesystem::path(configured);
    }
    for (const auto& candidate : {
             std::filesystem::path("modern/data/PlayDH"),
             std::filesystem::path("data/PlayDH"),
             std::filesystem::path("../data/PlayDH"),
             std::filesystem::path("../../data/PlayDH"),
             std::filesystem::path("../../../../data/PlayDH"),
             std::filesystem::path("C:/moxiang/modern/data/PlayDH")}) {
        if (std::filesystem::exists(
                candidate / "Image" / "InterfaceScript" /
                "CharSelectDlg.bin")) {
            return std::filesystem::absolute(candidate);
        }
    }
    return {};
}

} // namespace

TEST(ClientUiRuntime, DispatchesRealLegacyFunctionButton) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    auto* create = runtime.findWindowByLegacyFunc("CS_BtnFuncCreateChar");
    ASSERT_NE(create, nullptr);

    const auto x = create->absX() + create->width() / 2;
    const auto y = create->absY() + create->height() / 2;
    EXPECT_TRUE(runtime.onMouseButton(true, true, x, y).consumed);
    const auto released = runtime.onMouseButton(true, false, x, y);
    ASSERT_TRUE(released.consumed);
    ASSERT_TRUE(released.activation.has_value());
    EXPECT_EQ(released.activation->legacy_func, "CS_BtnFuncCreateChar");
    EXPECT_EQ(released.activation->dialog_legacy_id, "CS_CHARSELECTDLG");
}

TEST(ClientUiRuntime, DispatchesRealPushupCharacterSlot) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    ASSERT_TRUE(runtime.load(playdh, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600));
    auto* first = runtime.findWindowByLegacyId("MT_FIRSTCHOSEBTN");
    ASSERT_NE(first, nullptr);

    const auto x = first->absX() + first->width() / 2;
    const auto y = first->absY() + first->height() / 2;
    runtime.onMouseButton(true, true, x, y);
    const auto released = runtime.onMouseButton(true, false, x, y);
    ASSERT_TRUE(released.activation.has_value());
    EXPECT_EQ(released.activation->legacy_id, "MT_FIRSTCHOSEBTN");
}

TEST(ClientUiRuntime, RoutesCharactersOnlyToFocusedEditBox) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "CharMakeNewDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    auto* edit = dynamic_cast<mxh::ui::cEditBox*>(
        runtime.findWindowByLegacyId("CMID_IDEDITBOX"));
    ASSERT_NE(edit, nullptr);
    edit->InitEditbox(static_cast<std::uint16_t>(edit->width()), 17);

    EXPECT_FALSE(runtime.onChar('X'));
    const auto x = edit->absX() + 1;
    const auto y = edit->absY() + 1;
    EXPECT_TRUE(runtime.onMouseButton(true, true, x, y).consumed);
    EXPECT_TRUE(runtime.onChar('X'));
    EXPECT_EQ(edit->editText(), "X");

    runtime.setActive(false);
    EXPECT_FALSE(runtime.onChar('Y'));
    EXPECT_EQ(edit->editText(), "X");
}

TEST(ClientUiRuntime, ClearReleasesStateOwnedDialogTree) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    ASSERT_TRUE(runtime.load(playdh, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600));
    ASSERT_FALSE(runtime.empty());
    runtime.clear();
    EXPECT_TRUE(runtime.empty());
    EXPECT_FALSE(runtime.isActive());
    EXPECT_EQ(runtime.findWindowByLegacyId("CS_CHARSELECTDLG"), nullptr);
}

TEST(ClientUiRuntime, LoadsGameDialogsWithoutOverwritingLegacyActiveFlags) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    constexpr std::array<std::string_view, 2> scripts{"15.bin", "11.bin"};
    std::string error;
    ASSERT_TRUE(runtime.loadMany(playdh, scripts,
                                mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    EXPECT_EQ(runtime.dialogs().size(), 2u);
    EXPECT_TRUE(runtime.isDialogActive("MI_MAINDLG"));
    EXPECT_FALSE(runtime.isDialogActive("IN_INVENTORYDLG"));

    runtime.setActive(false);
    runtime.setActive(true);
    EXPECT_TRUE(runtime.isDialogActive("MI_MAINDLG"));
    EXPECT_FALSE(runtime.isDialogActive("IN_INVENTORYDLG"));

    EXPECT_TRUE(runtime.setDialogActive("IN_INVENTORYDLG", true));
    EXPECT_TRUE(runtime.isDialogActive("IN_INVENTORYDLG"));
}

TEST(ClientUiRuntime, FailedMultiLoadKeepsPreviousTreeIntact) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    ASSERT_TRUE(runtime.load(playdh, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600));
    constexpr std::array<std::string_view, 2> scripts{
        "15.bin", "MissingRequiredGameDialog.bin"};
    std::string error;
    EXPECT_FALSE(runtime.loadMany(playdh, scripts,
                                 mxh::ui::ResolutionMode::Low800x600, &error));
    EXPECT_NE(error.find("MissingRequiredGameDialog.bin"), std::string::npos);
    EXPECT_NE(runtime.findWindowByLegacyId("CS_CHARSELECTDLG"), nullptr);
    EXPECT_EQ(runtime.findWindowByLegacyId("MI_MAINDLG"), nullptr);
}

TEST(ClientUiRuntime, LoadsLegacyChinaGameInCoreDialogSet) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    constexpr std::array<std::string_view, 13> scripts{
        "15.bin", "51.bin", "24.bin", "10.bin", "11.bin", "23.bin",
        "19.bin", "22.bin", "31.bin", "14.bin", "17.bin",
        "QuestTotal.bin", "ItemShop.bin"};
    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.loadMany(playdh, scripts,
                                mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    EXPECT_GE(runtime.dialogs().size(), scripts.size());
    // Verify the dialog tree loaded every requested script. legacy_id
    // lookup intentionally skips inactive dialogs because CMI_CLOSEBTN
    // exists in many .bin files; instead, count dialogs by their own
    // legacyId directly on the dialog vector.
    std::size_t found_inventory = 0, found_quest = 0, found_itemshop = 0;
    for (const auto& d : runtime.dialogs()) {
        if (!d) continue;
        const auto& id = d->legacyId();
        if (id == "IN_INVENTORYDLG") ++found_inventory;
        else if (id == "QUE_TOTALDLG") ++found_quest;
        else if (id == "ITMALL_BASEDLG") ++found_itemshop;
    }
    EXPECT_EQ(found_inventory, 1u);
    EXPECT_EQ(found_quest, 1u);
    EXPECT_EQ(found_itemshop, 1u);
    EXPECT_NE(runtime.findWindowByLegacyId("MI_MAINDLG"), nullptr);
    EXPECT_NE(runtime.findWindowByLegacyId("CTI_DLG"), nullptr);
    EXPECT_FALSE(runtime.isDialogActive("IN_INVENTORYDLG"));
    EXPECT_FALSE(runtime.isDialogActive("QUE_TOTALDLG"));
    EXPECT_FALSE(runtime.isDialogActive("ITMALL_BASEDLG"));
}

TEST(InGameUiRuntime, InventoryCloseActivatesHandleUiActivation) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.set_quest_catalog({});

    // 1:1 legacy: `IN_INVENTORYDLG` ships with `#ACTIVE 0`. The player
    // presses 'I' (toggle_inventory) to flip the dialog active before any
    // button inside can receive clicks.
    state.toggle_inventory();
    ASSERT_TRUE(state.inventory_open());
    state.ui_runtime().setDialogActive("IN_INVENTORYDLG", true);
    ASSERT_TRUE(state.ui_runtime().isDialogActive("IN_INVENTORYDLG"));

    // Synthesize the close click activation. Pixel-level hit testing is
    // covered by the dispatch tests below; here we focus on the activation
    // funnel that flips state when the player closes the inventory.
    mxh::client::ClientUiActivation close_click;
    close_click.legacy_id = "CMI_CLOSEBTN";
    close_click.dialog_legacy_id = "IN_INVENTORYDLG";
    EXPECT_TRUE(state.handle_ui_activation(close_click));
    EXPECT_FALSE(state.inventory_open());
    EXPECT_FALSE(state.ui_runtime().isDialogActive("IN_INVENTORYDLG"));
}

TEST(InGameUiRuntime, MainBarButtonConsumesClickInActiveDialog) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.set_quest_catalog({});

    // MAINDLG is active by default; its MI_BTN_SIZE button is reachable
    // on screen at the bottom of the 800x600 logical viewport.
    mxh::ui::cWindow* size_btn = nullptr;
    for (const auto& d : state.ui_runtime().dialogs()) {
        if (d && d->legacyId() == "MI_MAINDLG") {
            size_btn = d->findWindowByLegacyId("MI_BTN_SIZE");
            break;
        }
    }
    ASSERT_NE(size_btn, nullptr);

    const auto cx = size_btn->absX() + size_btn->width() / 2;
    const auto cy = size_btn->absY() + size_btn->height() / 2;
    const auto down = state.ui_runtime().onMouseButton(true, true, cx, cy);
    EXPECT_TRUE(down.consumed);
    const auto up = state.ui_runtime().onMouseButton(true, false, cx, cy);
    // cButton with only legacy_id (no legacy_func) is consumed but does
    // not produce a routed activation — its handler is wired in the
    // legacy MI_DlgFunc callback, which the modern runtime drives from
    // `cMainBarDialog::Linking()` (deferred to a follow-up).
    EXPECT_TRUE(up.consumed);
}

TEST(ClientUiRuntime, ConfirmationOwnsModalInputAndCleansUpAfterEnter) {
    const auto root = find_playdh_root();
    ASSERT_FALSE(root.empty());
    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(root, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error)) << error;
    const auto before = runtime.dialogs().size();
    bool called = false;
    bool confirmed = false;
    ASSERT_TRUE(runtime.showConfirmation(9001, "Delete this character?",
        [&](bool value) {
            called = true;
            confirmed = value;
        }));
    EXPECT_TRUE(runtime.hasModal());
    EXPECT_EQ(runtime.dialogs().size(), before + 1);

    EXPECT_TRUE(runtime.onKey(true, 13));

    EXPECT_TRUE(called);
    EXPECT_TRUE(confirmed);
    EXPECT_FALSE(runtime.hasModal());
    EXPECT_EQ(runtime.dialogs().size(), before);
}

TEST(ClientUiRuntime, MessageBoxClosesOnEscapeAndRunsCallbackOnce) {
    const auto root = find_playdh_root();
    ASSERT_FALSE(root.empty());
    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(root, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error)) << error;
    const auto before = runtime.dialogs().size();
    int calls = 0;
    ASSERT_TRUE(runtime.showMessage(9002, "Character deletion failed.",
                                    [&] { ++calls; }));
    ASSERT_TRUE(runtime.hasModal());

    EXPECT_TRUE(runtime.onKey(true, 27));

    EXPECT_EQ(calls, 1);
    EXPECT_FALSE(runtime.hasModal());
    EXPECT_EQ(runtime.dialogs().size(), before);
}
