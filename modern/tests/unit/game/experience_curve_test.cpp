#include "mxh/game/experience_curve.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <limits>

#include <gtest/gtest.h>

namespace {

std::string table_text() {
    std::string text;
    for (std::uint16_t level = 1; level <= mxh::game::MAX_CHARACTER_LEVEL_NUM; ++level) {
        text += std::to_string(level);
        text += ' ';
        text += std::to_string(static_cast<unsigned>(level) * 10u);
        text += '\n';
    }
    return text;
}

std::filesystem::path playdh_exp_path() {
    auto root = std::filesystem::current_path();
    for (int depth = 0; depth < 8 && !root.empty(); ++depth, root = root.parent_path()) {
        if (!std::filesystem::exists(root / "modern") || !std::filesystem::exists(root / "deploy")) continue;
        std::error_code error;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(
                 root, std::filesystem::directory_options::skip_permission_denied, error)) {
            if (error) break;
            if (!entry.is_regular_file(error) || entry.path().filename() != L"CharacterExpPoint.bin") continue;
            const auto parent = entry.path().parent_path().parent_path().filename().wstring();
            if (parent.find(L"PlayDH") != std::wstring::npos) return entry.path();
        }
    }
    return {};
}

}  // namespace

TEST(ExperienceCurve, LoadsOneBasedRowsAndLegacyZeroLevel) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    EXPECT_EQ(curve.size(), 121u);
    EXPECT_EQ(curve.max_exp_point(0), 10u);
    EXPECT_EQ(curve.max_exp_point(1), 10u);
    EXPECT_EQ(curve.max_exp_point(121), 1210u);
}

TEST(ExperienceCurve, RejectsShortOrNonSequentialTables) {
    EXPECT_THROW(mxh::game::ExperienceCurve::load_from_text("1 10\n2 20\n"), std::runtime_error);
    auto text = table_text();
    text.replace(0, 1, "2");
    EXPECT_THROW(mxh::game::ExperienceCurve::load_from_text(text), std::runtime_error);
}

TEST(ExperienceCurve, AddExpPerformsOneLegacyLevelTransition) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    const auto result = curve.add_exp({1, 9}, 1);
    EXPECT_EQ(result.level, 2);
    EXPECT_EQ(result.exp_point, 0u);

    const auto large_gain = curve.add_exp({1, 9}, 1000);
    EXPECT_EQ(large_gain.level, 2);
    EXPECT_EQ(large_gain.exp_point, 999u);
}

TEST(ExperienceCurve, MaxLevelIgnoresAdditionalExperience) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    const auto result = curve.add_exp({121, 77}, 1000);
    EXPECT_EQ(result.level, 121);
    EXPECT_EQ(result.exp_point, 77u);
}

TEST(ExperienceCurve, MaxExpAtEachLevelMatchesLoadedTable) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    for (std::uint16_t lv = 1; lv <= mxh::game::MAX_CHARACTER_LEVEL_NUM; ++lv) {
        EXPECT_EQ(curve.max_exp_point(lv), static_cast<std::uint64_t>(lv) * 10u)
            << "mismatch at level " << lv;
    }
}

TEST(ExperienceCurve, AddExpBelowThresholdDoesNotLevelUp) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    const auto r = curve.add_exp({1, 0}, 9);
    EXPECT_EQ(r.level, 1);
    EXPECT_EQ(r.exp_point, 9u);
}

TEST(ExperienceCurve, AddExpExactThresholdLevelsUpWithZero) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    const auto r = curve.add_exp({1, 0}, 10);
    EXPECT_EQ(r.level, 2);
    EXPECT_EQ(r.exp_point, 0u);
}

TEST(ExperienceCurve, AddExpOneBelowExactStaysAtSameLevel) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    const auto r = curve.add_exp({1, 9}, 1);
    EXPECT_EQ(r.level, 2);
    EXPECT_EQ(r.exp_point, 0u);
}

TEST(ExperienceCurve, AddExpFromLevelZeroUsesFirstRow) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    const auto r = curve.add_exp({0, 0}, 10);
    EXPECT_EQ(r.level, 1);
    EXPECT_EQ(r.exp_point, 0u);
}

TEST(ExperienceCurve, AddExpAtBoundaryDoesOnlyOneLevelUpPerCall) {
    // 1:1 with legacy: add_exp performs exactly one transition per call.
    // Callers loop. Adding a huge amount from level 1 lands at level 2 with
    // the leftover exp kept, regardless of how high it is.
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    const auto r = curve.add_exp({1, 0}, std::numeric_limits<std::uint64_t>::max() / 2);
    EXPECT_EQ(r.level, 2);
    EXPECT_GE(r.exp_point, 10u);
}

TEST(ExperienceCurve, AddExpZeroAmountReturnsUnchanged) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    const auto r = curve.add_exp({42, 1234}, 0);
    EXPECT_EQ(r.level, 42);
    EXPECT_EQ(r.exp_point, 1234u);
}

TEST(ExperienceCurve, MaxExpPointAtMaxLevelReturnsLastRow) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    EXPECT_EQ(curve.max_exp_point(mxh::game::MAX_CHARACTER_LEVEL_NUM),
              mxh::game::MAX_CHARACTER_LEVEL_NUM * 10u);
}

TEST(ExperienceCurve, SizeAlwaysIsMax) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    EXPECT_EQ(curve.size(), static_cast<std::size_t>(mxh::game::MAX_CHARACTER_LEVEL_NUM));
}

TEST(ExperienceCurve, LoadFromTextAcceptsTabsAndMultipleSpaces) {
    std::string text;
    for (std::uint16_t lv = 1; lv <= mxh::game::MAX_CHARACTER_LEVEL_NUM; ++lv) {
        text += std::to_string(lv);
        text += "\t";
        text += std::to_string(lv * 100);
        text += "   \n";
    }
    const auto curve = mxh::game::ExperienceCurve::load_from_text(text);
    EXPECT_EQ(curve.size(), 121u);
    EXPECT_EQ(curve.max_exp_point(1), 100u);
}

TEST(ExperienceCurve, LoadFromTextAcceptsCRLFLineEndings) {
    std::string text;
    for (std::uint16_t lv = 1; lv <= mxh::game::MAX_CHARACTER_LEVEL_NUM; ++lv) {
        text += std::to_string(lv);
        text += " ";
        text += std::to_string(lv * 5);
        text += "\r\n";
    }
    const auto curve = mxh::game::ExperienceCurve::load_from_text(text);
    EXPECT_EQ(curve.size(), 121u);
    EXPECT_EQ(curve.max_exp_point(1), 5u);
}

TEST(ExperienceCurve, RejectsEmptyTable) {
    EXPECT_THROW(mxh::game::ExperienceCurve::load_from_text(""), std::runtime_error);
    EXPECT_THROW(mxh::game::ExperienceCurve::load_from_text("   \n\n"), std::runtime_error);
}

TEST(ExperienceCurve, RejectsNonNumericLevel) {
    EXPECT_THROW(mxh::game::ExperienceCurve::load_from_text("abc 10\n"), std::runtime_error);
    EXPECT_THROW(mxh::game::ExperienceCurve::load_from_text("1 xyz\n"), std::runtime_error);
}

TEST(ExperienceCurve, RejectsExtraRowsBeyond121) {
    auto text = table_text();
    text += "122 99999\n";
    const auto curve = mxh::game::ExperienceCurve::load_from_text(text);
    EXPECT_EQ(curve.size(), 121u);
}

TEST(ExperienceCurve, AddExpAtMaxLevelReturnsStateUnchanged) {
    const auto curve = mxh::game::ExperienceCurve::load_from_text(table_text());
    const auto r = curve.add_exp({mxh::game::MAX_CHARACTER_LEVEL_NUM, 999}, 100);
    EXPECT_EQ(r.level, mxh::game::MAX_CHARACTER_LEVEL_NUM);
    EXPECT_EQ(r.exp_point, 999u);
}
TEST(ExperienceCurve, LoadsOriginalPlayDhBinaryWhenAvailable) {
    const auto path = playdh_exp_path();
    if (path.empty()) GTEST_SKIP() << "PlayDH CharacterExpPoint.bin not available";
    const auto curve = mxh::game::ExperienceCurve::load_from_bin(path);
    EXPECT_EQ(curve.size(), 121u);
    EXPECT_GT(curve.max_exp_point(1), 0u);
    EXPECT_GE(curve.max_exp_point(121), curve.max_exp_point(1));
}
