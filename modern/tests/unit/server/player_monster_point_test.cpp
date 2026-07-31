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
