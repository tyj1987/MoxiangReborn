#include <gtest/gtest.h>

#include "CharMakeOptions.hpp"

namespace {

const std::filesystem::path kPlaydhRoot =
    "C:/moxiang/modern/data/PlayDH";

} // namespace

TEST(CharMakeOptions, LoadsExactChinaResourceValues) {
    std::string error;
    const auto catalog = mxh::client::CharMakeOptionCatalog::load(
        kPlaydhRoot, &error);
    ASSERT_TRUE(catalog.has_value()) << error;
    ASSERT_TRUE(catalog->hasChinaBaseline());

    using Category = mxh::client::CharMakeOptionCategory;
    const auto* sex = catalog->find(Category::Sex);
    const auto* male_hair = catalog->find(Category::MaleHair);
    const auto* female_hair = catalog->find(Category::FemaleHair);
    const auto* male_face = catalog->find(Category::MaleFace);
    const auto* female_face = catalog->find(Category::FemaleFace);
    const auto* cloth = catalog->find(Category::Cloth);
    const auto* boot = catalog->find(Category::Boot);
    const auto* weapon = catalog->find(Category::Weapon);
    const auto* area = catalog->find(Category::StartArea);
    ASSERT_NE(sex, nullptr);
    ASSERT_NE(male_hair, nullptr);
    ASSERT_NE(female_hair, nullptr);
    ASSERT_NE(male_face, nullptr);
    ASSERT_NE(female_face, nullptr);
    ASSERT_NE(cloth, nullptr);
    ASSERT_NE(boot, nullptr);
    ASSERT_NE(weapon, nullptr);
    ASSERT_NE(area, nullptr);
    EXPECT_EQ(sex->options.size(), 2u);
    EXPECT_EQ(male_hair->options.size(), 5u);
    EXPECT_EQ(female_hair->options.size(), 5u);
    EXPECT_EQ(male_face->options.size(), 5u);
    EXPECT_EQ(female_face->options.size(), 5u);
    ASSERT_EQ(cloth->options.size(), 2u);
    ASSERT_EQ(boot->options.size(), 2u);
    ASSERT_EQ(weapon->options.size(), 6u);
    ASSERT_EQ(area->options.size(), 1u);
    EXPECT_EQ(cloth->options[0].value, 23000u);
    EXPECT_EQ(cloth->options[1].value, 23010u);
    EXPECT_EQ(boot->options[0].value, 27000u);
    EXPECT_EQ(boot->options[1].value, 27010u);
    EXPECT_EQ(weapon->options[0].value, 11000u);
    EXPECT_EQ(weapon->options[5].value, 21000u);
    EXPECT_EQ(area->options[0].value, 17u);
}

TEST(CharMakeOptions, RejectsTruncatedOptionGroup) {
    std::string error;
    const auto catalog = mxh::client::CharMakeOptionCatalog::parse(
        "2 CMID_SexType male 0 0\n", &error);
    EXPECT_FALSE(catalog.has_value());
    EXPECT_FALSE(error.empty());
}
