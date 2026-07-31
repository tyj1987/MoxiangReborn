#include "mxh/game/experience_curve.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

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

TEST(ExperienceCurve, LoadsOriginalPlayDhBinaryWhenAvailable) {
    const auto path = playdh_exp_path();
    if (path.empty()) GTEST_SKIP() << "PlayDH CharacterExpPoint.bin not available";
    const auto curve = mxh::game::ExperienceCurve::load_from_bin(path);
    EXPECT_EQ(curve.size(), 121u);
    EXPECT_GT(curve.max_exp_point(1), 0u);
    EXPECT_GE(curve.max_exp_point(121), curve.max_exp_point(1));
}
