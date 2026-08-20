#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

#include "ClientUiRuntime.hpp"
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
