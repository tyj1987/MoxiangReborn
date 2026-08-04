#include "minimapdlg.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using mxh::ui::BigMapIconKind;
using mxh::ui::MiniMapMode;
using mxh::ui::cMiniMapDlg;

TEST(MiniMapDlg, ConstantsMatchLegacy) {
    EXPECT_EQ(mxh::ui::kMiniMapFullWidth, 140);
    EXPECT_EQ(mxh::ui::kMiniMapFullHeight, 155);
    EXPECT_EQ(static_cast<int>(MiniMapMode::Full), 0);
    EXPECT_EQ(static_cast<int>(MiniMapMode::Small), 1);
    EXPECT_EQ(static_cast<int>(MiniMapMode::Max), 2);
}

TEST(MiniMapDlg, DefaultConstructionMatchesLegacy) {
    cMiniMapDlg dialog;
    EXPECT_FALSE(dialog.isActive());
    EXPECT_EQ(dialog.map_num(), 0);
    EXPECT_EQ(dialog.current_mode(), MiniMapMode::Full);
    EXPECT_EQ(dialog.icon_count(), 0u);
    EXPECT_FALSE(dialog.zoom_pushed());
}

TEST(MiniMapDlg, LinkingInitializesIconImages) {
    cMiniMapDlg dialog;
    dialog.Linking();
    EXPECT_TRUE(dialog.icons_initialized());
}

TEST(MiniMapDlg, InitStoresMapResetsFullModeAndRequestsImages) {
    cMiniMapDlg dialog;
    dialog.SetActive(true);
    dialog.InitMiniMap(101);
    EXPECT_EQ(dialog.map_num(), 101);
    EXPECT_EQ(dialog.current_mode(), MiniMapMode::Full);
    EXPECT_TRUE(dialog.map_images_requested());
    EXPECT_TRUE(dialog.isActive());
}

TEST(MiniMapDlg, InitDisablesNonViewMap) {
    cMiniMapDlg dialog;
    dialog.SetMapRules([](std::uint16_t) { return false; }, {});
    dialog.SetActive(true);
    dialog.InitMiniMap(101);
    EXPECT_FALSE(dialog.isActive());
}

TEST(MiniMapDlg, EventEntryOnlyAllowsLegacyExceptionMaps) {
    cMiniMapDlg dialog;
    dialog.SetActive(true);
    dialog.SetGameInInitKind(mxh::ui::cBigMapDlg::GameInInitKind::SuryunEnter);
    dialog.InitMiniMap(22);
    EXPECT_FALSE(dialog.isActive());
    dialog.SetActive(true);
    dialog.InitMiniMap(24);
    EXPECT_TRUE(dialog.isActive());
}

TEST(MiniMapDlg, SetActiveUpdatesMainBarState) {
    cMiniMapDlg dialog;
    bool pushed = false;
    dialog.SetMainBarStateCallback([&](bool active) { pushed = active; });
    dialog.SetActive(true);
    EXPECT_TRUE(pushed);
    dialog.SetActive(false);
    EXPECT_FALSE(pushed);
}

TEST(MiniMapDlg, ToggleBigMapUsesInverseActiveStateThenRefreshes) {
    cMiniMapDlg dialog;
    bool big_active = false;
    dialog.SetBigMapCallbacks([&] { return big_active; }, [&](bool active) { big_active = active; });
    dialog.ToggleMinimapMode();
    EXPECT_TRUE(big_active);
    EXPECT_TRUE(dialog.zoom_pushed());
    dialog.ToggleMinimapMode();
    EXPECT_FALSE(big_active);
    EXPECT_FALSE(dialog.zoom_pushed());
}

TEST(MiniMapDlg, AddIconReplacesDuplicate) {
    cMiniMapDlg dialog;
    dialog.AddIcon(BigMapIconKind::Weapon, 5, 1, 2);
    dialog.AddIcon(BigMapIconKind::Book, 5, 3, 4);
    ASSERT_EQ(dialog.icon_count(), 1u);
    ASSERT_NE(dialog.find_icon(5), nullptr);
    EXPECT_EQ(dialog.find_icon(5)->kind, BigMapIconKind::Book);
}

TEST(MiniMapDlg, PartyPositionUpdatesForwardToBigMap) {
    cMiniMapDlg dialog;
    dialog.AddPartyMemberIcon(9, 1, 2);
    std::uint32_t forwarded_id = 0;
    std::int32_t forwarded_x = 0;
    dialog.SetBigMapForwarders(
        [&](std::uint32_t id, std::int32_t x, std::int32_t) { forwarded_id = id; forwarded_x = x; },
        {}, {});
    dialog.SetPartyIconObjectPos(9, 7, 8);
    EXPECT_EQ(dialog.find_icon(9)->object_x, 7);
    EXPECT_EQ(dialog.find_icon(9)->target_x, 7);
    EXPECT_EQ(forwarded_id, 9u);
    EXPECT_EQ(forwarded_x, 7);
}

TEST(MiniMapDlg, MissingPartyStillForwardsPosition) {
    cMiniMapDlg dialog;
    int calls = 0;
    dialog.SetBigMapForwarders({}, [&](std::uint32_t, std::int32_t, std::int32_t) { ++calls; }, {});
    dialog.SetPartyIconTargetPos(77, 3, 4);
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(dialog.icon_count(), 0u);
}

TEST(MiniMapDlg, QuestMarkUpdatesLocalAndForwards) {
    cMiniMapDlg dialog;
    dialog.AddIcon(BigMapIconKind::Etc, 42);
    int forwarded_kind = 0;
    dialog.SetBigMapForwarders({}, {}, [&](std::uint32_t, std::int32_t kind) { forwarded_kind = kind; });
    dialog.ShowQuestMarkIcon(42, 3);
    EXPECT_EQ(dialog.find_icon(42)->quest_mark_kind, 3);
    EXPECT_EQ(forwarded_kind, 3);
}

TEST(MiniMapDlg, FullModeProcessResetsOrigin) {
    cMiniMapDlg dialog;
    dialog.AddHeroIcon(1, 400, 300);
    dialog.Process();
    EXPECT_EQ(dialog.origin_x(), 0);
    EXPECT_EQ(dialog.origin_y(), 0);
    EXPECT_EQ(dialog.process_count(), 1u);
}

TEST(MiniMapDlg, HeroIconIsSeparateFromTable) {
    cMiniMapDlg dialog;
    dialog.AddHeroIcon(10, 20, 30);
    ASSERT_TRUE(dialog.hero_icon().has_value());
    EXPECT_EQ(dialog.hero_icon()->kind, BigMapIconKind::Self);
    EXPECT_EQ(dialog.icon_count(), 0u);
}

TEST(MiniMapDlg, RemoveMissingIconIsNoOp) {
    cMiniMapDlg dialog;
    dialog.RemoveIcon(999);
    EXPECT_EQ(dialog.icon_count(), 0u);
}

TEST(MiniMapDlg, NonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<cMiniMapDlg>);
    EXPECT_FALSE(std::is_copy_assignable_v<cMiniMapDlg>);
}