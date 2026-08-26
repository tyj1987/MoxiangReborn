#include <gtest/gtest.h>

#include "ClientSettings.hpp"

#include <filesystem>
#include <cstdio>
#include <fstream>

#include <cstdlib>
#include <array>
#include <filesystem>

#include "ClientUiRuntime.hpp"
#include "CCharMake.hpp"
#include "CCharSelectState.hpp"
#include "CEngine.hpp"
#include "CInGameState.hpp"
#include "CMainTitle.hpp"
#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/ui/cEditBox.hpp"
#include "mxh/ui/cGuagen.hpp"
#include "mxh/ui/cImage.hpp"
#include "mxh/ui/cResourceManager.hpp"
#include "mxh/ui/cSpriteAtlas.hpp"

namespace {

TEST(ClientUiRuntime, SetProgressValueUpdatesNestedGauges) {
    mxh::client::ClientUiRuntime runtime;
    auto dialog = std::make_unique<mxh::ui::cDialog>();
    dialog->Init(0, 0, 200, 100, nullptr, 1);
    auto gauge = std::make_unique<mxh::ui::cGuagen>();
    gauge->Init(0, 0, 100, 10, nullptr, 2);
    auto* gaugePtr = gauge.get();
    dialog->Add(std::move(gauge));
    runtime.dialogsMutable().push_back(std::move(dialog));
    EXPECT_EQ(runtime.setProgressValue(0.75f), 1u);
    EXPECT_FLOAT_EQ(gaugePtr->GetValue(), 0.75f);
    EXPECT_EQ(runtime.setProgressValue(2.0f), 1u);
    EXPECT_FLOAT_EQ(gaugePtr->GetValue(), 1.0f);
}

TEST(ClientSettings, AtomicRoundTripAndValidation) {
    const auto path = std::filesystem::temp_directory_path() / "mxh-settings-roundtrip.json";
    mxh::client::ClientSettingsV1 expected;
    expected.resource_profile_id = "playdh-current";
    expected.post_login_width = 1920;
    expected.post_login_height = 1080;
    expected.borderless = true;
    expected.vsync = false;
    expected.bgm_volume = 0.25f;
    expected.sfx_volume = 0.75f;
    expected.last_account = "测试账号";
    std::string error;
    ASSERT_TRUE(mxh::client::ClientSettingsStore::save_atomic(path, expected, &error)) << error;
    const auto actual = mxh::client::ClientSettingsStore::load(path, &error);
    EXPECT_EQ(actual.post_login_width, 1920u);
    EXPECT_EQ(actual.post_login_height, 1080u);
    EXPECT_TRUE(actual.borderless);
    EXPECT_FALSE(actual.vsync);
    EXPECT_FLOAT_EQ(actual.bgm_volume, 0.25f);
    EXPECT_FLOAT_EQ(actual.sfx_volume, 0.75f);
    EXPECT_EQ(actual.last_account, "测试账号");
    std::filesystem::remove(path);
}

TEST(ClientSettings, EscapedAccountNameRoundTripsAndMalformedEscapeFallsBack) {
    const auto path = std::filesystem::temp_directory_path() / "mxh-settings-escaped.json";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << R"({"schemaVersion":1,"resourceProfileId":"playdh-current",)"
                  R"("postLoginWidth":1024,"postLoginHeight":768,"lastAccount":"a\"b\n"})";
    }
    std::string warning;
    const auto actual = mxh::client::ClientSettingsStore::load(path, &warning);
    EXPECT_EQ(actual.last_account, "a\"b\n");
    std::filesystem::remove(path);

    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << R"({"schemaVersion":1,"resourceProfileId":"playdh-current",)"
                  R"("postLoginWidth":1024,"postLoginHeight":768,"lastAccount":"bad\q"})";
    }
    warning.clear();
    const auto malformed = mxh::client::ClientSettingsStore::load(path, &warning);
    EXPECT_TRUE(malformed.last_account.empty());
    std::filesystem::remove(path);
}

TEST(ClientSettings, UnknownSchemaIsBackedUpAndDefaultsAreUsed) {
    const auto path = std::filesystem::temp_directory_path() / "mxh-settings-schema.json";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << R"({"schemaVersion":99,"resourceProfileId":"playdh-current",)"
                  R"("postLoginWidth":1920,"postLoginHeight":1080})";
    }
    std::string warning;
    const auto actual = mxh::client::ClientSettingsStore::load(path, &warning);
    EXPECT_EQ(actual.schema_version, 1u);
    EXPECT_EQ(actual.post_login_width, 1024u);
    EXPECT_NE(warning.find("unsupported settings schema"), std::string::npos);
    EXPECT_FALSE(std::filesystem::exists(path));
    for (const auto& entry : std::filesystem::directory_iterator(path.parent_path())) {
        if (entry.path().filename().string().rfind("mxh-settings-schema.json.corrupt", 0) == 0) {
            std::error_code ignored;
            std::filesystem::remove(entry.path(), ignored);
        }
    }
}

TEST(ClientSettings, InvalidFileIsBackedUpBeforeDefaults) {
    const auto path = std::filesystem::temp_directory_path() /
                      "mxh-settings-invalid.json";
    for (const auto& entry : std::filesystem::directory_iterator(path.parent_path())) {
        const auto name = entry.path().filename().string();
        if (name.rfind("mxh-settings-invalid.json.corrupt", 0) == 0) {
            std::error_code ignored;
            std::filesystem::remove(entry.path(), ignored);
        }
    }
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "{ not valid settings";
    }
    std::string warning;
    const auto actual = mxh::client::ClientSettingsStore::load(path, &warning);
    EXPECT_EQ(actual.post_login_width, 1024u);
    EXPECT_EQ(actual.post_login_height, 768u);
    EXPECT_NE(warning.find("moved to"), std::string::npos);
    EXPECT_FALSE(std::filesystem::exists(path));
    bool found_backup = false;
    for (const auto& entry : std::filesystem::directory_iterator(path.parent_path())) {
        if (entry.path().filename().string().rfind(
                "mxh-settings-invalid.json.corrupt", 0) == 0) {
            found_backup = true;
            std::filesystem::remove(entry.path());
        }
    }
    EXPECT_TRUE(found_backup);
}

std::filesystem::path find_playdh_root() {
    if (const char* configured = std::getenv("MXH_PLAYDH_ROOT")) {
        return std::filesystem::path(configured);
    }
    std::filesystem::path cursor = std::filesystem::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        const auto candidate = cursor / "modern" / "data" / "PlayDH";
        if (std::filesystem::exists(
                candidate / "Image" / "InterfaceScript" /
                "CharSelectDlg.bin")) {
            return std::filesystem::absolute(candidate);
        }
        if (std::filesystem::exists(
                cursor / "data" / "PlayDH" / "Image" /
                "InterfaceScript" / "CharSelectDlg.bin")) {
            return std::filesystem::absolute(cursor / "data" / "PlayDH");
        }
        if (!cursor.has_parent_path() || cursor == cursor.parent_path()) break;
        cursor = cursor.parent_path();
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
    EXPECT_EQ(runtime.focusedWindow(), first);
    const auto released = runtime.onMouseButton(true, false, x, y);
    ASSERT_TRUE(released.activation.has_value());
    EXPECT_EQ(released.activation->legacy_id, "MT_FIRSTCHOSEBTN");
}

TEST(ClientUiRuntime, TabAndShiftTabCycleRealLoginControls) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());
    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "IDDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error)) << error;

    ASSERT_TRUE(runtime.onKey(true, 9, false));
    ASSERT_NE(runtime.focusedWindow(), nullptr);
    const auto first = runtime.focusedWindow();
    ASSERT_TRUE(runtime.onKey(true, 9, true));
    ASSERT_NE(runtime.focusedWindow(), nullptr);
    EXPECT_NE(runtime.focusedWindow(), first);
    ASSERT_TRUE(runtime.onKey(true, 9, false));
    EXPECT_EQ(runtime.focusedWindow(), first);
}

TEST(ClientUiRuntime, KeyboardConfirmProducesButtonActivation) {
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
    ASSERT_TRUE(runtime.onKey(true, 13, false));
    const auto activation = runtime.consumeKeyActivation();
    ASSERT_TRUE(activation.has_value());
    EXPECT_EQ(activation->legacy_id, "MT_FIRSTCHOSEBTN");

    ASSERT_TRUE(runtime.onKey(true, 32, false));
    const auto space_activation = runtime.consumeKeyActivation();
    ASSERT_TRUE(space_activation.has_value());
    EXPECT_EQ(space_activation->legacy_id, "MT_FIRSTCHOSEBTN");
}

TEST(ClientUiRuntime, HidingActiveSetClearsStaleKeyboardFocus) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());
    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "IDDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error)) << error;
    ASSERT_TRUE(runtime.onKey(true, 9, false));
    ASSERT_NE(runtime.focusedWindow(), nullptr);
    runtime.applyActiveSet(std::array<std::string_view, 0>{});
    EXPECT_EQ(runtime.focusedWindow(), nullptr);
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

TEST(ClientUiRuntime, ActiveChildAlsoActivatesOwningDialog) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());
    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error)) << error;
    ASSERT_NE(runtime.findWindowByLegacyId("MT_FIRSTCHOSEBTN"), nullptr);
    runtime.applyActiveSet(std::array<std::string_view, 1>{"MT_FIRSTCHOSEBTN"});
    EXPECT_TRUE(runtime.isDialogActive("CS_CHARSELECTDLG"));
    EXPECT_TRUE(runtime.isDialogActive("MT_FIRSTCHOSEBTN"));
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

TEST(InGameUiRuntime, InventoryTabButtonsSwitchRealGridDialog) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.toggle_inventory();
    ASSERT_TRUE(state.inventory_open());
    EXPECT_EQ(state.inventory_tab(), 0u);
    ASSERT_NE(state.ui_runtime().findWindowByLegacyId("IN_TABDLG1"), nullptr);

    mxh::client::ClientUiActivation tab;
    tab.legacy_id = "IN_TABBTN3";
    EXPECT_TRUE(state.handle_ui_activation(tab));
    EXPECT_EQ(state.inventory_tab(), 2u);
    EXPECT_NE(state.ui_runtime().findWindowByLegacyId("IN_TABDLG3"), nullptr);
}

TEST(InGameUiRuntime, TotalInfoLocalRefreshesEquipmentAppearance) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::net::Message total;
    total.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    total.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    mxh::game::ItemTotalInfo items{};
    items.WearedItem[0] = mxh::game::make_item(9901u, 777u,
                                                mxh::game::TP_WEAREDITEM_START);
    total.payload.resize(sizeof(items));
    std::memcpy(total.payload.data(), &items, sizeof(items));
    state.on_message(mxh::net::make_connection_id(1), total);
    EXPECT_EQ(state.game_info().items.WearedItem[0].wIconIdx, 777u);
    EXPECT_EQ(state.game_info().weared_item_idx[0], 777u);
}

TEST(InGameUiRuntime, CharacterInfoHudButtonTogglesLiveDialog) {
    mxh::client::CInGameState state;
    state.Init(nullptr);

    mxh::client::ClientUiActivation click;
    click.legacy_id = "CI_BESTTIP";
    EXPECT_TRUE(state.handle_ui_activation(click));
    EXPECT_TRUE(state.character_open());
    EXPECT_TRUE(state.ui_runtime().isDialogActive("CI_CHARDLG"));

    click.legacy_id = "CMI_CLOSEBTN";
    click.dialog_legacy_id = "CI_CHARDLG";
    EXPECT_TRUE(state.handle_ui_activation(click));
    EXPECT_FALSE(state.character_open());
    EXPECT_FALSE(state.ui_runtime().isDialogActive("CI_CHARDLG"));
}

TEST(InGameUiRuntime, EnterAndEscapeOwnRealChatDialog) {
    mxh::client::CInGameState state;
    state.Init(nullptr);

    state.OnKeyEvent(true, mxh::client::kVkReturn);
    EXPECT_TRUE(state.chat_open());
    EXPECT_TRUE(state.ui_runtime().isDialogActive("CTI_DLG"));

    state.OnKeyEvent(true, mxh::client::kVkEscape);
    EXPECT_FALSE(state.chat_open());
    EXPECT_FALSE(state.ui_runtime().isDialogActive("CTI_DLG"));
}

TEST(InGameUiRuntime, FocusedChatEditboxSynchronizesAndEnterSubmits) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.OnKeyEvent(true, mxh::client::kVkReturn);
    ASSERT_TRUE(state.chat_open());

    auto* edit = state.ui_runtime().findWindowByLegacyId("MI_CHATEDITBOX");
    ASSERT_NE(edit, nullptr);
    auto* edit_box = dynamic_cast<mxh::ui::cEditBox*>(edit);
    ASSERT_NE(edit_box, nullptr);
    const auto x = edit->absX() + edit->width() / 2;
    const auto y = edit->absY() + edit->height() / 2;
    state.OnMouseButton(true, true, x, y);
    EXPECT_TRUE(edit_box->hasFocus());
    EXPECT_TRUE(state.chat_open());
    state.OnChar('H');
    state.OnChar('i');
    EXPECT_EQ(edit_box->editText(), "Hi");
    EXPECT_EQ(state.chat_buffer(), "Hi");

    state.OnKeyEvent(true, mxh::client::kVkReturn);
    EXPECT_FALSE(state.chat_open());
    EXPECT_TRUE(state.chat_buffer().empty());
}

TEST(InGameUiRuntime, QuestPageButtonsSelectLiveQuestEntry) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    auto catalog = mxh::compat::parse_quest_string_text(
        "$SUBQUESTSTR 10 0\n{\n#TITLE One\n}\n"
        "$SUBQUESTSTR 20 0\n{\n#TITLE Two\n}\n"
        "$SUBQUESTSTR 30 0\n{\n#TITLE Three\n}\n");
    state.set_quest_catalog(std::move(catalog));

    mxh::client::ClientUiActivation page;
    page.legacy_id = "QUE_PAGE2BTN";
    EXPECT_TRUE(state.handle_ui_activation(page));
    ASSERT_NE(state.selected_quest(), nullptr);
    EXPECT_EQ(state.selected_quest()->quest_id, 20u);
    EXPECT_EQ(state.quest_id(), 20u);

    page.legacy_id = "QUE_PAGE5BTN";
    EXPECT_FALSE(state.handle_ui_activation(page));
    EXPECT_EQ(state.quest_id(), 20u);
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

TEST(ClientUiRuntime, MessageBoxCentersInCommittedResolutionMode) {
    const auto root = find_playdh_root();
    ASSERT_FALSE(root.empty());
    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(root, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Mid1024x768, &error)) << error;
    ASSERT_TRUE(runtime.showMessage(9003, "Resolution-aware message"));
    ASSERT_FALSE(runtime.dialogs().empty());
    const auto* modal = runtime.dialogs().back().get();
    ASSERT_NE(modal, nullptr);
    EXPECT_EQ(modal->absX(), (1024 - 197) / 2);
    EXPECT_EQ(modal->absY(), (768 - 150) / 2);
}

TEST(ClientUiRuntime, CharSelectActivateAllMakesRootActiveAndCreateClickable) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    runtime.activateAllLoadedDialogs();
    ASSERT_TRUE(runtime.isDialogActive("CS_CHARSELECTDLG"));
    auto* root = runtime.findWindowByLegacyId("CS_CHARSELECTDLG");
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->isVisible());

    auto* create = runtime.findWindowByLegacyFunc("CS_BtnFuncCreateChar");
    ASSERT_NE(create, nullptr);
    const auto x = create->absX() + create->width() / 2;
    const auto y = create->absY() + create->height() / 2;
    EXPECT_TRUE(runtime.onMouseButton(true, true, x, y).consumed);
    const auto released = runtime.onMouseButton(true, false, x, y);
    ASSERT_TRUE(released.activation.has_value());
    EXPECT_EQ(released.activation->legacy_func, "CS_BtnFuncCreateChar");
}

TEST(ClientUiRuntime, CharMakeActivateAllMakesEveryRootActive) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "CharMakeNewDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    runtime.activateAllLoadedDialogs();
    ASSERT_FALSE(runtime.dialogs().empty());
    for (const auto& dialog : runtime.dialogs()) {
        ASSERT_NE(dialog, nullptr);
        EXPECT_TRUE(dialog->isActive()) << dialog->legacyId();
        EXPECT_TRUE(dialog->isVisible()) << dialog->legacyId();
    }
}

TEST(ClientUiRuntime, CharSelectRootBasicImageBoundWhenSpriteHookRegistered) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    static char mock_sprite = 'S';
    auto hook = [](void*, const std::string& path) -> void* {
        EXPECT_FALSE(path.empty());
        return &mock_sprite;
    };
    mxh::ui::cDialogLoader::SetSpriteLoader(
        static_cast<mxh::ui::LoadSpriteFn>(hook), nullptr);
    const auto image_dir = playdh / "Image";
    if (!mxh::ui::cResourceManager::getInstance().allLoaded()) {
        mxh::ui::cResourceManager::getInstance().InitScriptManager(image_dir);
    }
    if (!mxh::ui::cSpriteAtlas::getInstance().loaded()) {
        mxh::ui::cSpriteAtlas::getInstance().Init(playdh);
    }

    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    runtime.activateAllLoadedDialogs();
    auto* root = runtime.findWindowByLegacyId("CS_CHARSELECTDLG");
    ASSERT_NE(root, nullptr);
    auto* image = static_cast<mxh::ui::cImage*>(root->basicImage());
    ASSERT_NE(image, nullptr);
    EXPECT_FALSE(image->IsNull());
}

namespace {

mxh::client::ClientUiInputResult click_window_center(
    mxh::client::ClientUiRuntime& runtime, mxh::ui::cWindow* window) {
    const auto x = window->absX() + static_cast<std::int32_t>(window->width() / 2);
    const auto y = window->absY() + static_cast<std::int32_t>(window->height() / 2);
    runtime.onMouseButton(true, true, x, y);
    return runtime.onMouseButton(true, false, x, y);
}

}  // namespace

TEST(ClientUiRuntime, CharSelectSlotThenEnterHitboxesDispatchShippedCommands) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::CCharSelectState select_state;
    EXPECT_FALSE(select_state.auto_select_for_test());

    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    runtime.activateAllLoadedDialogs();

    auto* slot = runtime.findWindowByLegacyId("MT_FIRSTCHOSEBTN");
    ASSERT_NE(slot, nullptr);
    const auto slot_click = click_window_center(runtime, slot);
    ASSERT_TRUE(slot_click.activation.has_value());
    const auto slot_cmd =
        mxh::client::resolve_char_select_ui_command(*slot_click.activation);
    EXPECT_EQ(slot_cmd.kind, mxh::client::CharSelectUiCommandKind::SelectSlot);
    EXPECT_EQ(slot_cmd.slot_index, 0u);

    auto* enter = runtime.findWindowByLegacyId("MT_ENTERBTN");
    if (!enter) enter = runtime.findWindowByLegacyFunc("CS_BtnFuncEnter");
    ASSERT_NE(enter, nullptr);
    const auto enter_click = click_window_center(runtime, enter);
    ASSERT_TRUE(enter_click.activation.has_value());
    const auto enter_cmd =
        mxh::client::resolve_char_select_ui_command(*enter_click.activation);
    EXPECT_EQ(enter_cmd.kind, mxh::client::CharSelectUiCommandKind::Enter);

    auto* create = runtime.findWindowByLegacyFunc("CS_BtnFuncCreateChar");
    ASSERT_NE(create, nullptr);
    const auto create_click = click_window_center(runtime, create);
    ASSERT_TRUE(create_click.activation.has_value());
    EXPECT_EQ(mxh::client::resolve_char_select_ui_command(*create_click.activation).kind,
              mxh::client::CharSelectUiCommandKind::Create);
}

TEST(ClientUiRuntime, CharSelectCreateAndEnterHitboxesDispatchShippedCommands) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "CharSelectDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    runtime.activateAllLoadedDialogs();
    ASSERT_TRUE(runtime.isDialogActive("CS_CHARSELECTDLG"));

    auto* create = runtime.findWindowByLegacyFunc("CS_BtnFuncCreateChar");
    ASSERT_NE(create, nullptr);
    const auto create_click = click_window_center(runtime, create);
    ASSERT_TRUE(create_click.activation.has_value());
    const auto create_cmd =
        mxh::client::resolve_char_select_ui_command(*create_click.activation);
    EXPECT_EQ(create_cmd.kind, mxh::client::CharSelectUiCommandKind::Create);

    auto* enter = runtime.findWindowByLegacyId("MT_ENTERBTN");
    if (!enter) enter = runtime.findWindowByLegacyFunc("CS_BtnFuncEnter");
    ASSERT_NE(enter, nullptr);
    const auto enter_click = click_window_center(runtime, enter);
    ASSERT_TRUE(enter_click.activation.has_value());
    const auto enter_cmd =
        mxh::client::resolve_char_select_ui_command(*enter_click.activation);
    EXPECT_EQ(enter_cmd.kind, mxh::client::CharSelectUiCommandKind::Enter);
}

TEST(ClientUiRuntime, CharMakeSubmitCancelAndNameHitboxesDispatchShippedCommands) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "CharMakeNewDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    runtime.activateAllLoadedDialogs();

    auto* name = dynamic_cast<mxh::ui::cEditBox*>(
        runtime.findWindowByLegacyId("CMID_IDEDITBOX"));
    ASSERT_NE(name, nullptr);
    name->InitEditbox(static_cast<std::uint16_t>(name->width()), 17);
    const auto name_down = runtime.onMouseButton(
        true, true, name->absX() + 1, name->absY() + 1);
    EXPECT_TRUE(name_down.consumed);
    EXPECT_TRUE(runtime.onChar('Z'));
    EXPECT_EQ(name->editText(), "Z");

    auto* submit = runtime.findWindowByLegacyId("CMID_CharMake");
    if (!submit) submit = runtime.findWindowByLegacyFunc("CM_CharMakeBtnFunc");
    ASSERT_NE(submit, nullptr);
    const auto submit_click = click_window_center(runtime, submit);
    ASSERT_TRUE(submit_click.activation.has_value());
    const auto submit_cmd =
        mxh::client::resolve_char_make_ui_command(*submit_click.activation);
    EXPECT_EQ(submit_cmd.kind, mxh::client::CharMakeUiCommandKind::Submit);

    auto* cancel = runtime.findWindowByLegacyId("CMID_CharCancel");
    if (!cancel) cancel = runtime.findWindowByLegacyFunc("CM_CharCancelBtnFunc");
    ASSERT_NE(cancel, nullptr);
    const auto cancel_click = click_window_center(runtime, cancel);
    ASSERT_TRUE(cancel_click.activation.has_value());
    const auto cancel_cmd =
        mxh::client::resolve_char_make_ui_command(*cancel_click.activation);
    EXPECT_EQ(cancel_cmd.kind, mxh::client::CharMakeUiCommandKind::Cancel);
}

TEST(InGameUiRuntime, DefaultHudActiveAndMissClickIsNotConsumed) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::CInGameState state;
    state.Init(nullptr);
    auto& runtime = state.ui_runtime();
    ASSERT_FALSE(runtime.empty());

    EXPECT_TRUE(runtime.isDialogActive("MI_MAINDLG"));
    EXPECT_TRUE(runtime.isDialogActive("QI_QUICKDLG"));
    EXPECT_TRUE(runtime.isDialogActive("MNM_DIALOG"));
    EXPECT_TRUE(runtime.isDialogActive("CG_GUAGEDLG"));
    EXPECT_FALSE(runtime.isDialogActive("IN_INVENTORYDLG"));
    EXPECT_FALSE(runtime.isDialogActive("QUE_TOTALDLG"));
    EXPECT_FALSE(runtime.isDialogActive("ITMALL_BASEDLG"));

    int miss_x = -1;
    int miss_y = -1;
    for (int y = 8; y < 600 && miss_x < 0; y += 16) {
        for (int x = 8; x < 800; x += 16) {
            bool covered = false;
            for (const auto& dialog : runtime.dialogs()) {
                if (!dialog || !dialog->isActive()) continue;
                if (dialog->PtInWindow(x, y)) {
                    covered = true;
                    break;
                }
            }
            if (!covered) {
                miss_x = x;
                miss_y = y;
                break;
            }
        }
    }
    ASSERT_GE(miss_x, 0) << "default HUD covers the entire 800x600 viewport";
    const auto miss = runtime.onMouseButton(true, true, miss_x, miss_y);
    EXPECT_FALSE(miss.consumed);
    EXPECT_FALSE(miss.activation.has_value());
}

TEST(InGameUiRuntime, IKeyTogglesInventoryOnShippedGameInTree) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::CInGameState state;
    state.Init(nullptr);
    EXPECT_FALSE(state.inventory_open());
    EXPECT_FALSE(state.ui_runtime().isDialogActive("IN_INVENTORYDLG"));

    state.OnKeyEvent(true, 0x49);  // VK 'I' — shipped inventory toggle
    EXPECT_TRUE(state.inventory_open());
    EXPECT_TRUE(state.ui_runtime().isDialogActive("IN_INVENTORYDLG"));

    state.OnKeyEvent(true, 0x49);
    EXPECT_FALSE(state.inventory_open());
    EXPECT_FALSE(state.ui_runtime().isDialogActive("IN_INVENTORYDLG"));
}

TEST(InGameUiRuntime, DefaultHudRootsBindNonNullSprites) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    static char mock_sprite = 'H';
    auto hook = [](void*, const std::string& path) -> void* {
        EXPECT_FALSE(path.empty());
        return &mock_sprite;
    };
    mxh::ui::cDialogLoader::SetSpriteLoader(
        static_cast<mxh::ui::LoadSpriteFn>(hook), nullptr);
    const auto image_dir = playdh / "Image";
    if (!mxh::ui::cResourceManager::getInstance().allLoaded()) {
        mxh::ui::cResourceManager::getInstance().InitScriptManager(image_dir);
    }
    if (!mxh::ui::cSpriteAtlas::getInstance().loaded()) {
        mxh::ui::cSpriteAtlas::getInstance().Init(playdh);
    }

    mxh::client::CInGameState state;
    state.Init(nullptr);
    auto* root = state.ui_runtime().findWindowByLegacyId("MI_MAINDLG");
    ASSERT_NE(root, nullptr);
    auto* image = static_cast<mxh::ui::cImage*>(root->basicImage());
    ASSERT_NE(image, nullptr);
    EXPECT_FALSE(image->IsNull());
}

TEST(ClientUiRuntime, LoginDlgOkAndExitHitboxesDispatchShippedCommands) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "IDDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    runtime.activateAllLoadedDialogs();
    ASSERT_TRUE(runtime.isDialogActive("MT_LOGINDLG"));

    auto* dlg = runtime.findWindowByLegacyId("MT_LOGINDLG");
    ASSERT_NE(dlg, nullptr);
    EXPECT_TRUE(dlg->isVisible());

    auto* ok = runtime.findWindowByLegacyId("MT_OKBTN");
    if (!ok) ok = runtime.findWindowByLegacyFunc("MT_LogInOkBtnFunc");
    ASSERT_NE(ok, nullptr);
    EXPECT_GT(ok->absY() + static_cast<std::int32_t>(ok->height()),
              dlg->absY() + static_cast<std::int32_t>(dlg->height()))
        << "IDDlg #POINT is the caption; OK sits below it";
    const auto ok_click = click_window_center(runtime, ok);
    ASSERT_TRUE(ok_click.activation.has_value());
    EXPECT_EQ(mxh::client::resolve_login_ui_command(*ok_click.activation).kind,
              mxh::client::LoginUiCommandKind::Submit);

    auto* end = runtime.findWindowByLegacyId("MT_ENDBTN");
    if (!end) end = runtime.findWindowByLegacyFunc("MT_ExitBtnFunc");
    ASSERT_NE(end, nullptr);
    const auto end_click = click_window_center(runtime, end);
    ASSERT_TRUE(end_click.activation.has_value());
    EXPECT_EQ(mxh::client::resolve_login_ui_command(*end_click.activation).kind,
              mxh::client::LoginUiCommandKind::Exit);
}

TEST(ClientUiRuntime, LoginIdAndPasswordHitboxesAcceptTypedText) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::ClientUiRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(playdh, "IDDlg.bin",
                             mxh::ui::ResolutionMode::Low800x600, &error))
        << error;
    runtime.activateAllLoadedDialogs();

    auto* id = dynamic_cast<mxh::ui::cEditBox*>(
        runtime.findWindowByLegacyId("MT_IDEDITBOX"));
    ASSERT_NE(id, nullptr);
    id->InitEditbox(static_cast<std::uint16_t>(id->width()), 17);
    const auto id_down = runtime.onMouseButton(
        true, true, id->absX() + 1, id->absY() + 1);
    EXPECT_TRUE(id_down.consumed);
    EXPECT_TRUE(runtime.onChar('A'));
    EXPECT_EQ(id->editText(), "A");

    auto* pwd = dynamic_cast<mxh::ui::cEditBox*>(
        runtime.findWindowByLegacyId("MT_PWDEDITBOX"));
    ASSERT_NE(pwd, nullptr);
    pwd->InitEditbox(static_cast<std::uint16_t>(pwd->width()), 17);
    pwd->SetSecret(true);
    const auto pwd_down = runtime.onMouseButton(
        true, true, pwd->absX() + 1, pwd->absY() + 1);
    EXPECT_TRUE(pwd_down.consumed);
    EXPECT_TRUE(runtime.onChar('B'));
    EXPECT_EQ(pwd->editText(), "B");
    EXPECT_EQ(pwd->displayText(), "*");
}

TEST(CMainTitle, StartLoadsIdDlgAndOkSubmitsCredentials) {
    const auto playdh = find_playdh_root();
    ASSERT_FALSE(playdh.empty());

    mxh::client::CEngine engine;
    engine.SetPlaydhRoot(playdh);

    mxh::client::CMainTitle title;
    title.Init(nullptr);
    title.Start(&engine, "acct", "secret");
    ASSERT_FALSE(title.ui_runtime().empty());
    ASSERT_TRUE(title.ui_runtime().isDialogActive("MT_LOGINDLG"));
    EXPECT_EQ(title.username(), "acct");
    EXPECT_EQ(title.password(), "secret");

    auto* id = dynamic_cast<mxh::ui::cEditBox*>(
        title.ui_runtime().findWindowByLegacyId("MT_IDEDITBOX"));
    ASSERT_NE(id, nullptr);
    EXPECT_EQ(id->editText(), "acct");

    auto* pwd = dynamic_cast<mxh::ui::cEditBox*>(
        title.ui_runtime().findWindowByLegacyId("MT_PWDEDITBOX"));
    ASSERT_NE(pwd, nullptr);
    EXPECT_TRUE(pwd->IsSecret());
    EXPECT_EQ(pwd->editText(), "secret");

    auto* ok = title.ui_runtime().findWindowByLegacyId("MT_OKBTN");
    if (!ok) ok = title.ui_runtime().findWindowByLegacyFunc("MT_LogInOkBtnFunc");
    ASSERT_NE(ok, nullptr);
    title.OnMouseButton(true, true,
                        ok->absX() + static_cast<std::int32_t>(ok->width() / 2),
                        ok->absY() + static_cast<std::int32_t>(ok->height() / 2));
    title.OnMouseButton(true, false,
                        ok->absX() + static_cast<std::int32_t>(ok->width() / 2),
                        ok->absY() + static_cast<std::int32_t>(ok->height() / 2));
    EXPECT_TRUE(title.consumeSubmit());
    EXPECT_EQ(title.username(), "acct");
    EXPECT_EQ(title.password(), "secret");
    EXPECT_FALSE(title.consumeSubmit());

    auto* end = title.ui_runtime().findWindowByLegacyId("MT_ENDBTN");
    if (!end) end = title.ui_runtime().findWindowByLegacyFunc("MT_ExitBtnFunc");
    ASSERT_NE(end, nullptr);
    title.OnMouseButton(true, true,
                        end->absX() + static_cast<std::int32_t>(end->width() / 2),
                        end->absY() + static_cast<std::int32_t>(end->height() / 2));
    title.OnMouseButton(true, false,
                        end->absX() + static_cast<std::int32_t>(end->width() / 2),
                        end->absY() + static_cast<std::int32_t>(end->height() / 2));
    EXPECT_TRUE(title.username().empty());
    EXPECT_TRUE(title.password().empty());
    title.Release();
}
