// modern/tests/unit/portal/news_route_test.cpp
// M5.5: ContentLoader + news route JSON shape tests.

#include "portal/content_loader.hpp"
#include "portal/news_routes.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace mxh::portal;

namespace {

class ContentLoaderTest : public ::testing::Test {
protected:
    std::filesystem::path tmp_;

    void SetUp() override {
        tmp_ = std::filesystem::temp_directory_path() / "mox_portal_news_test";
        std::filesystem::create_directories(tmp_ / "news");
        std::filesystem::create_directories(tmp_ / "shop");
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(tmp_, ec);
    }

    void write_news(const std::string& name, const std::string& content) {
        std::ofstream(tmp_ / "news" / name) << content;
    }
};

}  // namespace

TEST_F(ContentLoaderTest, LoadsYAMLFrontMatter) {
    write_news("welcome.md", R"(---
title_zh: 墨香归来
title_en: Moxiang Returns
summary_zh: portal 上线
published_at: 2026-08-01T10:00:00Z
---

正文中文内容。

EN:
English body content.
)");
    ContentLoader loader(tmp_, tmp_ / "shop" / "catalog.json");
    loader.reload();

    auto items = loader.list_news(1, 10);
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].slug, "welcome");
    EXPECT_EQ(items[0].title_zh, "墨香归来");
    EXPECT_EQ(items[0].title_en, "Moxiang Returns");
    EXPECT_FALSE(items[0].body_zh.empty());
    EXPECT_FALSE(items[0].body_en.empty());
}

TEST_F(ContentLoaderTest, FindNewsBySlug) {
    write_news("a.md", "---\ntitle_zh: A\n---\nA body\n");
    write_news("b.md", "---\ntitle_zh: B\n---\nB body\n");
    ContentLoader loader(tmp_, tmp_ / "shop" / "catalog.json");
    loader.reload();

    auto a = loader.find_news("a");
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(a->title_zh, "A");

    EXPECT_FALSE(loader.find_news("nonexistent").has_value());
}

TEST_F(ContentLoaderTest, Pagination) {
    for (int i = 0; i < 25; ++i) {
        write_news("item-" + std::to_string(i) + ".md",
                   "---\ntitle_zh: Item " + std::to_string(i) + "\npublished_at: 2026-08-01T00:00:0" + std::to_string(i % 10) + "Z\n---\nbody");
    }
    ContentLoader loader(tmp_, tmp_ / "shop" / "catalog.json");
    loader.reload();

    auto page1 = loader.list_news(1, 10);
    auto page2 = loader.list_news(2, 10);
    auto page3 = loader.list_news(3, 10);
    EXPECT_EQ(page1.size(), 10u);
    EXPECT_EQ(page2.size(), 10u);
    EXPECT_EQ(page3.size(), 5u);
}

TEST_F(ContentLoaderTest, ReloadIsIdempotent) {
    write_news("x.md", "---\ntitle_zh: X\n---\nbody");
    ContentLoader loader(tmp_, tmp_ / "shop" / "catalog.json");
    loader.reload();
    loader.reload();
    loader.reload();
    EXPECT_EQ(loader.list_news(1, 10).size(), 1u);
}
