#include "mxh/compat/quest_string_catalog.hpp"

#include <gtest/gtest.h>
#include "../../support/playdh_env.hpp"

TEST(QuestStringCatalog, ParsesMainAndSubquestText) {
    const auto result = mxh::compat::parse_quest_string_text(
        "$SUBQUESTSTR 1 0\n{\n#TITLE Main Quest\n#DESC\n{\nOverview\n}\n}\n"
        "$SUBQUESTSTR 1 1\n{\n#TITLE Hunt\n#DESC\n{\nKill ten monsters\n}\n}\n");
    ASSERT_EQ(result.entries.size(), 2u);
    ASSERT_NE(result.find(1, 0), nullptr);
    EXPECT_EQ(result.find(1, 0)->title, "Main Quest");
    ASSERT_NE(result.find(1, 1), nullptr);
    EXPECT_EQ(result.find(1, 1)->description.front(), "Kill ten monsters");
    ASSERT_EQ(result.main_quests().size(), 1u);
}

TEST(QuestStringCatalog, ConvertsKnownBig5TitleToUnicode) {
    const std::string bytes{"\xA6\xAC\xB6\xB0", 4};
    EXPECT_EQ(mxh::compat::big5_to_utf16(bytes), L"收集");
}

TEST(QuestStringCatalog, LoadsRealQuestStringResource) {
    std::filesystem::path path;
    if (const auto playdh_override = mxh::test::playdh_dir_override()) {
        path = *playdh_override / "Resource" / "QuestScript" / "QuestString.bin";
    }
    auto root = std::filesystem::current_path();
    for (int depth = 0; path.empty() && depth < 8 && !root.empty(); ++depth) {
        const auto candidate = root / "modern" / "data" / "PlayDH" /
                               "Resource" / "QuestScript" / "QuestString.bin";
        if (std::filesystem::is_regular_file(candidate)) path = candidate;
        root = root.parent_path();
    }
    if (!std::filesystem::exists(path)) GTEST_SKIP() << "PlayDH unavailable";
    const auto result = mxh::compat::load_quest_string_catalog(path);
    EXPECT_TRUE(result.error_message.empty()) << result.error_message;
    EXPECT_GT(result.entries.size(), 100u);
    ASSERT_NE(result.find(1, 0), nullptr);
    EXPECT_FALSE(result.find(1, 0)->title.empty());
}
