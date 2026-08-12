#include "mxh/compat/quest_npc_catalog.hpp"

#include <gtest/gtest.h>

TEST(QuestNpcCatalog, ParsesLegacyTabSeparatedRow) {
    const auto result = mxh::compat::parse_quest_npc_text(
        "12\t25\tNpcName\t501\t21571\t27440\t0\t\r\n");
    ASSERT_EQ(result.parse_errors, 0u);
    ASSERT_EQ(result.entries.size(), 1u);
    const auto& npc = result.entries.front();
    EXPECT_EQ(npc.map_num, 12u);
    EXPECT_EQ(npc.npc_kind, 25u);
    EXPECT_EQ(npc.name, "NpcName");
    EXPECT_EQ(npc.npc_index, 501u);
    EXPECT_EQ(npc.position_x, 21571u);
    EXPECT_EQ(npc.position_z, 27440u);
}

TEST(QuestNpcCatalog, LoadsRealResourceWithoutErrors) {
    std::filesystem::path path;
    for (const auto& item : std::filesystem::recursive_directory_iterator(
             std::filesystem::current_path(),
             std::filesystem::directory_options::skip_permission_denied)) {
        if (item.is_regular_file() && item.path().filename() == "questnpclist.bin") {
            path = item.path();
            break;
        }
    }
    if (path.empty()) GTEST_SKIP() << "PlayDH unavailable";
    const auto result = mxh::compat::load_quest_npc_catalog(path);
    EXPECT_TRUE(result.error_message.empty()) << result.error_message;
    EXPECT_EQ(result.parse_errors, 0u);
    EXPECT_GT(result.entries.size(), 50u);
    EXPECT_EQ(mxh::compat::quest_npc_count_for_map(result, 1), 5u);
    EXPECT_EQ(mxh::compat::quest_npc_count_for_map(result, 13), 12u);
    EXPECT_EQ(mxh::compat::quest_npc_count_for_map(result, 12), 0u);
}
