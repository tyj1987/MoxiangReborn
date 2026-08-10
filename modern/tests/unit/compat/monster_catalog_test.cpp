#include <gtest/gtest.h>

#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/compat/monster_catalog.hpp"

#include <filesystem>
#include <fstream>

TEST(MonsterCatalog, ParsesVisualColumns) {
    const std::string text = "1\tMonster\t0\t1\t-1\t0\tL001.chx\t1.3\t30\r\n";
    const auto catalog = mxh::compat::MonsterCatalog::parse_text(
        {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
    ASSERT_TRUE(catalog.has_value());
    const auto* visual = catalog->find(1);
    ASSERT_NE(visual, nullptr);
    EXPECT_EQ(visual->chx_name, "L001.chx");
    EXPECT_FLOAT_EQ(visual->scale, 1.3f);
    EXPECT_EQ(catalog->find(2), nullptr);
}

TEST(MonsterCatalog, ParsesRealPlayDhWhenAvailable) {
    auto root = std::filesystem::current_path();
    std::filesystem::path file;
    for (int depth = 0; depth < 8 && !root.empty(); ++depth, root = root.parent_path()) {
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            if (!entry.is_directory()) continue;
            const auto candidate = entry.path() / L"PlayDH" / L"Resource" / L"MonsterList.bin";
            const auto direct = entry.path() / L"Resource" / L"MonsterList.bin";
            if (std::filesystem::is_regular_file(candidate)) { file = candidate; break; }
            if (std::filesystem::is_regular_file(direct)) { file = direct; break; }
        }
        if (!file.empty()) break;
    }
    if (file.empty()) GTEST_SKIP() << "PlayDH MonsterList.bin unavailable";
    std::ifstream input(file, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(input.good());
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(input.tellg()));
    input.seekg(0); input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    const auto catalog = mxh::compat::MonsterCatalog::parse_bin(bytes);
    ASSERT_TRUE(catalog.has_value());
    ASSERT_GT(catalog->entries().size(), 100u);
    const auto* first = catalog->find(1);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->chx_name, "L001.chx");
    EXPECT_FLOAT_EQ(first->scale, 1.0f);
}
