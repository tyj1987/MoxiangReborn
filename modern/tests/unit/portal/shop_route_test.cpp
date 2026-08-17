// modern/tests/unit/portal/shop_route_test.cpp
// M5.6: ContentLoader shop catalog tests.

#include "portal/content_loader.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace mxh::portal;

namespace {

class ShopTest : public ::testing::Test {
protected:
    std::filesystem::path tmp_;

    void SetUp() override {
        tmp_ = std::filesystem::temp_directory_path() / "mox_portal_shop_test";
        std::filesystem::create_directories(tmp_ / "news");
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(tmp_, ec);
    }

    void write_catalog(const std::string& content) {
        std::ofstream(tmp_ / "catalog.json") << content;
    }
};

}  // namespace

TEST_F(ShopTest, LoadsEmptyCatalog) {
    write_catalog(R"({"items": []})");
    ContentLoader loader(tmp_, tmp_ / "catalog.json");
    loader.reload();
    EXPECT_TRUE(loader.shop_loaded());
    EXPECT_EQ(loader.list_shop("", 1, 24).size(), 0u);
}

TEST_F(ShopTest, LoadsMultipleCategories) {
    write_catalog(R"({
      "items": [
        {"idx": 1, "name_zh": "Sword",  "category": "weapon",    "price_points": 100},
        {"idx": 2, "name_zh": "Armor",  "category": "armor",     "price_points": 200},
        {"idx": 3, "name_zh": "Potion", "category": "consumable","price_points": 5}
      ]
    })");
    ContentLoader loader(tmp_, tmp_ / "catalog.json");
    loader.reload();

    auto all = loader.list_shop("", 1, 24);
    EXPECT_EQ(all.size(), 3u);

    auto weapons = loader.list_shop("weapon", 1, 24);
    EXPECT_EQ(weapons.size(), 1u);
    EXPECT_EQ(weapons[0].name_zh, "Sword");

    auto consumables = loader.list_shop("consumable", 1, 24);
    EXPECT_EQ(consumables.size(), 1u);
}

TEST_F(ShopTest, MissingCatalogReturnsNotLoaded) {
    ContentLoader loader(tmp_, tmp_ / "does-not-exist.json");
    loader.reload();
    EXPECT_FALSE(loader.shop_loaded());
}

TEST_F(ShopTest, CategoryFilterIsCaseInsensitive) {
    write_catalog(R"({"items": [{"idx": 1, "name_zh": "X", "category": "Weapon", "price_points": 1}]})");
    ContentLoader loader(tmp_, tmp_ / "catalog.json");
    loader.reload();
    auto upper = loader.list_shop("WEAPON", 1, 24);
    auto lower = loader.list_shop("weapon", 1, 24);
    EXPECT_EQ(upper.size(), 1u);
    EXPECT_EQ(lower.size(), 1u);
}
