#include <gtest/gtest.h>

#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/compat/monster_catalog.hpp"

#include <filesystem>
#include <fstream>

TEST(MonsterCatalog, ParsesVisualColumns) {
    const std::string text =
        "1\tMonster\tMonster\t7\t-1\t0\tL001.chx\t1.3\t30\t1\t1\t1000\t50\t90\t2"
        "\t12\t20\t3\t4\t0\t0\t0\t0\t0\t50\t100\t120\t0\t0\t0\t0\t0\t0\t0"
        "\t1000\t0\t0\t1\t750\t0\t500\t360\t0\t2\t301\t302\t70\t30"
        "\t10\t20\t5400\t660\t679\t1\t3260\t313\t314\t315\t1\r\n";
    const auto catalog = mxh::compat::MonsterCatalog::parse_text(
        {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
    ASSERT_TRUE(catalog.has_value());
    const auto* visual = catalog->find(1);
    ASSERT_NE(visual, nullptr);
    EXPECT_EQ(visual->chx_name, "L001.chx");
    EXPECT_FLOAT_EQ(visual->scale, 1.3f);
    EXPECT_EQ(visual->life, 1000u);
    EXPECT_EQ(visual->attack_min, 12u);
    EXPECT_EQ(visual->attack_max, 20u);
    EXPECT_TRUE(visual->aggressive);
    EXPECT_FLOAT_EQ(visual->search_period_ms, 750.0f);
    EXPECT_EQ(visual->attack_count, 2u);
    EXPECT_EQ(visual->attack_skills[0], 301u);
    EXPECT_EQ(visual->attack_rates[1], 30u);
    EXPECT_EQ(visual->min_drop_money, 10u);
    EXPECT_EQ(visual->max_drop_money, 20u);
    EXPECT_EQ(visual->drop_rates,
              (std::array<std::uint32_t, 5>{5400u, 660u, 679u, 1u, 3260u}));
    EXPECT_EQ(visual->item_list_ids,
              (std::array<std::uint16_t, 3>{313u, 314u, 315u}));
    EXPECT_EQ(visual->max_drop_item_num, 1u);
    EXPECT_EQ(catalog->find(2), nullptr);
}

TEST(MonsterCatalog, ParsesCanonicalPlayDh) {
    const auto file = std::filesystem::path(MXH_SOURCE_DIR) /
        "data/PlayDH/Resource/MonsterList.bin";
    ASSERT_TRUE(std::filesystem::is_regular_file(file));
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
    for (const auto kind : {73u, 102u, 103u, 104u, 105u, 219u}) {
        const auto* monster = catalog->find(static_cast<std::uint16_t>(kind));
        ASSERT_NE(monster, nullptr) << "Map10 kind=" << kind;
        EXPECT_GT(monster->life, 0u);
        EXPECT_LE(monster->attack_min, monster->attack_max);
        EXPECT_LE(monster->attack_count, 2u);
        if (monster->attack_count != 0)
            EXPECT_NE(monster->attack_skills[0], 0u);
    }
    const auto* active = catalog->find(73u);
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active->level, 55u);
    EXPECT_EQ(active->life, 5436u);
    EXPECT_EQ(active->attack_min, 108u);
    EXPECT_EQ(active->attack_max, 135u);
    EXPECT_EQ(active->defense, 480u);
    EXPECT_TRUE(active->aggressive);
    EXPECT_FLOAT_EQ(active->search_period_ms, 1000.0f);
    EXPECT_EQ(active->attack_skills[0], 1444u);

    const auto* passive = catalog->find(105u);
    ASSERT_NE(passive, nullptr);
    EXPECT_EQ(passive->level, 53u);
    EXPECT_EQ(passive->life, 5096u);
    EXPECT_EQ(passive->attack_min, 105u);
    EXPECT_EQ(passive->attack_max, 131u);
    EXPECT_EQ(passive->defense, 451u);
    EXPECT_FALSE(passive->aggressive);
    EXPECT_EQ(passive->attack_skills[0], 1508u);
    EXPECT_EQ(passive->min_drop_money, 542u);
    EXPECT_EQ(passive->max_drop_money, 704u);
    EXPECT_EQ(passive->drop_rates,
              (std::array<std::uint32_t, 5>{5400u, 660u, 679u, 1u, 3260u}));
    EXPECT_EQ(passive->item_list_ids,
              (std::array<std::uint16_t, 3>{313u, 314u, 315u}));
    EXPECT_EQ(passive->max_drop_item_num, 1u);
    const auto* no_entries = catalog->find(219u);
    ASSERT_NE(no_entries, nullptr);
    EXPECT_EQ(no_entries->drop_rates,
              (std::array<std::uint32_t, 5>{0u, 10000u, 0u, 0u, 0u}));
    EXPECT_EQ(no_entries->item_list_ids,
              (std::array<std::uint16_t, 3>{655u, 656u, 657u}));
    struct Map10DropGolden {
        std::uint16_t kind;
        std::array<std::uint16_t, 3> lists;
        std::array<std::uint32_t, 5> rates;
    };
    constexpr std::array<Map10DropGolden, 6> expected{{
        {73u, {217u, 218u, 219u}, {5400u, 660u, 679u, 1u, 3260u}},
        {102u, {304u, 305u, 306u}, {4800u, 650u, 669u, 1u, 3880u}},
        {103u, {307u, 308u, 309u}, {4800u, 650u, 669u, 1u, 3880u}},
        {104u, {310u, 311u, 312u}, {4200u, 640u, 659u, 1u, 4500u}},
        {105u, {313u, 314u, 315u}, {5400u, 660u, 679u, 1u, 3260u}},
        {219u, {655u, 656u, 657u}, {0u, 10000u, 0u, 0u, 0u}},
    }};
    for (const auto& golden : expected) {
        const auto* monster = catalog->find(golden.kind);
        ASSERT_NE(monster, nullptr) << golden.kind;
        EXPECT_EQ(monster->item_list_ids, golden.lists) << golden.kind;
        EXPECT_EQ(monster->drop_rates, golden.rates) << golden.kind;
        EXPECT_EQ(monster->max_drop_item_num, 1u) << golden.kind;
    }
}

TEST(MonsterCatalog, RejectsTruncatedDropColumns) {
    const std::string text =
        "1\tMonster\tMonster\t7\t-1\t0\tL001.chx\t1.3\t30\t1\t1\t1000\t50\t90\t2"
        "\t12\t20\t3\t4\t0\t0\t0\t0\t0\t50\t100\t120\t0\t0\t0\t0\t0\t0\t0"
        "\t1000\t0\t0\t1\t750\t0\t500\t360\t0\t2\t301\t302\t70\t30\r\n";
    EXPECT_FALSE(mxh::compat::MonsterCatalog::parse_text(
        {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()}));
}
