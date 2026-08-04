#include "bigmapdlg.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using mxh::ui::BigMapIconKind;
using mxh::ui::cBigMapDlg;

TEST(BigMapDlg, ConstantsMatchLegacy) {
    EXPECT_EQ(mxh::ui::kBigMapWidth, 512);
    EXPECT_EQ(mxh::ui::kBigMapHeight, 512);
    EXPECT_EQ(static_cast<int>(BigMapIconKind::Self), 0);
    EXPECT_EQ(static_cast<int>(BigMapIconKind::PartyMember), 1);
    EXPECT_EQ(static_cast<int>(BigMapIconKind::Camera), 19);
    EXPECT_EQ(static_cast<int>(BigMapIconKind::Max), 20);
}

TEST(BigMapDlg, DefaultConstructionMatchesLegacyIdleState) {
    cBigMapDlg dialog;
    EXPECT_FALSE(dialog.isActive());
    EXPECT_EQ(dialog.map_num(), 0);
    EXPECT_EQ(dialog.icon_count(), 0u);
    EXPECT_FALSE(dialog.hero_icon().has_value());
    EXPECT_FALSE(dialog.icons_initialized());
}

TEST(BigMapDlg, LinkingInitializesIconImages) {
    cBigMapDlg dialog;
    dialog.Linking();
    EXPECT_TRUE(dialog.icons_initialized());
}

TEST(BigMapDlg, InitBigMapStoresMapAndRequestsImage) {
    cBigMapDlg dialog;
    dialog.SetActive(true);
    dialog.InitBigMap(101);
    EXPECT_EQ(dialog.map_num(), 101);
    EXPECT_TRUE(dialog.map_image_requested());
    EXPECT_TRUE(dialog.isActive());
}

TEST(BigMapDlg, InitBigMapDisablesNonViewMap) {
    cBigMapDlg dialog;
    dialog.SetMapRules([](std::uint16_t) { return false; }, {});
    dialog.SetActive(true);
    dialog.InitBigMap(101);
    EXPECT_FALSE(dialog.isActive());
}

TEST(BigMapDlg, EventEntryOnlyAllowsLegacyExceptionMaps) {
    cBigMapDlg dialog;
    dialog.SetActive(true);
    dialog.SetGameInInitKind(cBigMapDlg::GameInInitKind::EventMapEnter);
    dialog.InitBigMap(22);
    EXPECT_FALSE(dialog.isActive());
    dialog.SetActive(true);
    dialog.InitBigMap(23);
    EXPECT_TRUE(dialog.isActive());
}

TEST(BigMapDlg, SurvivalMapOnlyAllowsLegacyExceptionMaps) {
    cBigMapDlg dialog;
    dialog.SetMapRules([](std::uint16_t) { return true; }, [](std::uint16_t) { return true; });
    dialog.SetActive(true);
    dialog.InitBigMap(29);
    EXPECT_TRUE(dialog.isActive());
    dialog.InitBigMap(30);
    EXPECT_FALSE(dialog.isActive());
}

TEST(BigMapDlg, SetActiveHonorsCanActiveAndRefreshesMiniMap) {
    cBigMapDlg dialog;
    int refresh_count = 0;
    dialog.SetRefreshMiniMapCallback([&] { ++refresh_count; });
    dialog.SetMapRules([](std::uint16_t) { return false; }, {});
    dialog.InitBigMap(101);
    dialog.SetActive(true);
    EXPECT_FALSE(dialog.isActive());
    EXPECT_EQ(refresh_count, 1);
}

TEST(BigMapDlg, AddIconReplacesDuplicateObjectId) {
    cBigMapDlg dialog;
    dialog.AddIcon(BigMapIconKind::Weapon, 7, 10, 20);
    dialog.AddIcon(BigMapIconKind::Doctor, 7, 30, 40);
    ASSERT_EQ(dialog.icon_count(), 1u);
    const auto* icon = dialog.find_icon(7);
    ASSERT_NE(icon, nullptr);
    EXPECT_EQ(icon->kind, BigMapIconKind::Doctor);
    EXPECT_EQ(icon->object_x, 30);
}

TEST(BigMapDlg, PartyPositionUpdatesMatchLegacy) {
    cBigMapDlg dialog;
    dialog.AddPartyMemberIcon(9, 1, 2);
    dialog.SetPartyIconTargetPos(9, 7, 8);
    ASSERT_NE(dialog.find_icon(9), nullptr);
    EXPECT_EQ(dialog.find_icon(9)->object_x, 1);
    EXPECT_EQ(dialog.find_icon(9)->target_x, 7);
    dialog.SetPartyIconObjectPos(9, 11, 12);
    EXPECT_EQ(dialog.find_icon(9)->object_x, 11);
    EXPECT_EQ(dialog.find_icon(9)->object_z, 12);
    EXPECT_EQ(dialog.find_icon(9)->target_x, 11);
    EXPECT_EQ(dialog.find_icon(9)->target_z, 12);
}

TEST(BigMapDlg, MissingPartyPositionUpdateIsNoOp) {
    cBigMapDlg dialog;
    dialog.SetPartyIconObjectPos(999, 1, 2);
    dialog.SetPartyIconTargetPos(999, 3, 4);
    EXPECT_EQ(dialog.icon_count(), 0u);
}

TEST(BigMapDlg, RemoveIconAndQuestMarkMatchLegacy) {
    cBigMapDlg dialog;
    dialog.AddIcon(BigMapIconKind::Etc, 42);
    dialog.ShowQuestMarkIcon(42, 3);
    ASSERT_NE(dialog.find_icon(42), nullptr);
    EXPECT_EQ(dialog.find_icon(42)->quest_mark_kind, 3);
    dialog.RemoveIcon(42);
    EXPECT_EQ(dialog.find_icon(42), nullptr);
    dialog.ShowQuestMarkIcon(42, 1);
}

TEST(BigMapDlg, HeroIconIsSeparateFromIconTable) {
    cBigMapDlg dialog;
    dialog.AddHeroIcon(100, 5, 6);
    ASSERT_TRUE(dialog.hero_icon().has_value());
    EXPECT_EQ(dialog.hero_icon()->kind, BigMapIconKind::Self);
    EXPECT_EQ(dialog.hero_icon()->target_x, 5);
    EXPECT_EQ(dialog.icon_count(), 0u);
}

TEST(BigMapDlg, ProcessRunsEvenWithoutIcons) {
    cBigMapDlg dialog;
    dialog.Process();
    dialog.Process();
    EXPECT_EQ(dialog.process_count(), 2u);
}

TEST(BigMapDlg, NonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<cBigMapDlg>);
    EXPECT_FALSE(std::is_copy_assignable_v<cBigMapDlg>);
}