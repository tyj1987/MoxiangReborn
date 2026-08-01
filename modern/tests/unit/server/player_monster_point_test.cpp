#include "mxh/server/player_monster_point.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

namespace {

std::string table_text() {
    std::string text;
    for (std::uint16_t level = 1; level <= mxh::server::MAX_PLAYER_LEVEL_NUM; ++level) {
        for (std::int32_t column = 0; column < static_cast<std::int32_t>(
                 mxh::server::PLAYER_MONSTER_POINT_COLUMN_COUNT); ++column) {
            text += std::to_string(static_cast<unsigned>(level) * 100u +
                                   static_cast<unsigned>(column));
            text += ' ';
        }
    }
    return text;
}

std::filesystem::path deploy_table_path() {
    auto root = std::filesystem::current_path();
    for (int depth = 0; depth < 8 && !root.empty(); ++depth, root = root.parent_path()) {
        if (!std::filesystem::exists(root / "modern") || !std::filesystem::exists(root / "deploy")) continue;
        std::error_code error;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(
                 root, std::filesystem::directory_options::skip_permission_denied, error)) {
            if (error) break;
            if (!entry.is_regular_file(error) || entry.path().filename() != L"PlayerxMonsterPoint.bin") continue;
            const auto parent = entry.path().parent_path().parent_path().parent_path().filename().wstring();
            if (parent == L"Distribute") return entry.path();
        }
    }
    return {};
}

TEST(PlayerMonsterPoint, GetAtLevelBoundariesAndMidpoints) {
    const auto t = mxh::server::PlayerMonsterPointTable::load_from_text(table_text());
    // Each row at level L, column C is loaded with L*100+C.
    EXPECT_EQ(t.get(1, -6), 100u);
    EXPECT_EQ(t.get(1, 9),  115u);
    EXPECT_EQ(t.get(60, 0), 6006u);
    EXPECT_EQ(t.get(121, -6), 12100u);
}

TEST(PlayerMonsterPoint, GetThrowsAtZeroOrAboveMaxLevel) {
    const auto t = mxh::server::PlayerMonsterPointTable::load_from_text(table_text());
    EXPECT_THROW(t.get(0, 0), std::out_of_range);
    EXPECT_THROW(t.get(122, 0), std::out_of_range);  // MAX+1
    EXPECT_THROW(t.get(mxh::server::MAX_PLAYER_LEVEL_NUM + 1, 0), std::out_of_range);
}

TEST(PlayerMonsterPoint, GetThrowsAtOutOfRangeGap) {
    const auto t = mxh::server::PlayerMonsterPointTable::load_from_text(table_text());
    // gap < -6 -> throw (-7..-INF)
    EXPECT_THROW(t.get(50, -7), std::out_of_range);
    EXPECT_THROW(t.get(50, -100), std::out_of_range);
    // gap > 9 -> throw
    EXPECT_THROW(t.get(50, 10), std::out_of_range);
    EXPECT_THROW(t.get(50, 100), std::out_of_range);
}

TEST(PlayerMonsterPoint, GetPlayerPointClampsGapToBoundary) {
    const auto t = mxh::server::PlayerMonsterPointTable::load_from_text(table_text());
    // get_player_point silently clamps gap to [-6, 9];
    EXPECT_EQ(t.get_player_point(50, -1000), t.get_player_point(50, -6));
    EXPECT_EQ(t.get_player_point(50, 1000), t.get_player_point(50, 9));
    EXPECT_EQ(t.get_player_point(50, -6), t.get(50, -6));
    EXPECT_EQ(t.get_player_point(50, 9), t.get(50, 9));
}

TEST(PlayerMonsterPoint, GetPlayerPointAtMaxLevelIsZero) {
    const auto t = mxh::server::PlayerMonsterPointTable::load_from_text(table_text());
    // Legacy: at MAX level, no further point progression -> always 0.
    for (int gap = -6; gap <= 9; ++gap) {
        EXPECT_EQ(t.get_player_point(mxh::server::MAX_PLAYER_LEVEL_NUM, gap), 0u)
            << "gap=" << gap;
    }
}

TEST(PlayerMonsterPoint, RejectsNonNumericToken) {
    EXPECT_THROW(mxh::server::PlayerMonsterPointTable::load_from_text("abc 10"), std::runtime_error);
    EXPECT_THROW(mxh::server::PlayerMonsterPointTable::load_from_text("10 xyz"), std::runtime_error);
}

TEST(PlayerMonsterPoint, ZeroGapsAreDistinctFromOutOfRange) {
    const auto t = mxh::server::PlayerMonsterPointTable::load_from_text(table_text());
    // gap=0 is valid, returns row[column=6] = L*100+6.
    for (std::uint16_t L = 1; L <= 3; ++L) {
        EXPECT_EQ(t.get(L, 0), static_cast<std::uint32_t>(L) * 100u + 6u);
    }
}

TEST(PlayerMonsterPoint, ParseTabAndCRLFTolerant) {
    std::string text;
    for (std::uint16_t lv = 1; lv <= mxh::server::MAX_PLAYER_LEVEL_NUM; ++lv) {
        for (int col = 0; col < static_cast<int>(mxh::server::PLAYER_MONSTER_POINT_COLUMN_COUNT); ++col) {
            text += std::to_string(lv * 100u + col);
            text += "\t";
        }
        text += "\r\n";
    }
    const auto t = mxh::server::PlayerMonsterPointTable::load_from_text(text);
    EXPECT_EQ(t.get(1, 0), 106u);
    EXPECT_EQ(t.get(50, 9), 5015u);
}

TEST(PlayerMonsterPoint, GetPlayerPointJustBelowMaxIsLegit) {
    const auto t = mxh::server::PlayerMonsterPointTable::load_from_text(table_text());
    // Just below MAX level (120) should still return real values, not 0.
    EXPECT_NE(t.get_player_point(120, 0), 0u);
    EXPECT_EQ(t.get_player_point(120, 0), t.get(120, 0));  // gap=0 is in-range
    EXPECT_EQ(t.get_player_point(121, 0), 0u);  // at MAX, returns 0
}
}  // namespace

TEST(PlayerMonsterPoint, LoadsAllRowsAndColumns) {
    const auto table = mxh::server::PlayerMonsterPointTable::load_from_text(table_text());
    EXPECT_EQ(table.get(1, -6), 100u);
    EXPECT_EQ(table.get(1, 0), 106u);
    EXPECT_EQ(table.get(121, 9), 12115u);
}

TEST(PlayerMonsterPoint, GetPlayerPointClampsGapAndMaxLevel) {
    const auto table = mxh::server::PlayerMonsterPointTable::load_from_text(table_text());
    EXPECT_EQ(table.get_player_point(50, -100), table.get(50, -6));
    EXPECT_EQ(table.get_player_point(50, 100), table.get(50, 9));
    EXPECT_EQ(table.get_player_point(121, 0), 0u);
}

TEST(PlayerMonsterPoint, RejectsMissingOrInvalidIndexes) {
    const auto short_table = mxh::server::PlayerMonsterPointTable::load_from_text("1 2");
    EXPECT_EQ(short_table.get(1, -6), 1u);
    EXPECT_EQ(short_table.get(1, -5), 2u);
    EXPECT_EQ(short_table.get(1, -4), 0u);
    EXPECT_THROW(mxh::server::PlayerMonsterPointTable::load_from_text("x"), std::runtime_error);
    const auto table = mxh::server::PlayerMonsterPointTable::load_from_text(table_text());
    EXPECT_THROW(table.get(0, 0), std::out_of_range);
    EXPECT_THROW(table.get(1, 10), std::out_of_range);
}

TEST(PlayerMonsterPoint, LoadsDeployBinaryWhenAvailable) {
    const auto path = deploy_table_path();
    if (path.empty()) GTEST_SKIP() << "deploy PlayerxMonsterPoint.bin not available";
    const auto table = mxh::server::PlayerMonsterPointTable::load_from_bin(path);
    EXPECT_EQ(table.get(1, -6), 0u);
    EXPECT_EQ(table.get(1, 0), 15u);
    EXPECT_EQ(table.get(100, 0), 6259u);
    EXPECT_EQ(table.get(101, 0), 0u);
}
