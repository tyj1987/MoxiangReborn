// modern/src/portal/content_loader.hpp
// M5.5: scan content_root/news/*.md (front-matter + body) and catalog.

#pragma once

#include <chrono>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace mxh::portal {

struct NewsItem {
    std::string id;
    std::string slug;
    std::string title_zh;
    std::string title_en;
    std::string summary_zh;
    std::string summary_en;
    std::string hero_image;
    std::string published_at;  // ISO8601
    std::string body_zh;
    std::string body_en;
};

struct ShopItem {
    int idx;
    std::string name_zh;
    std::string name_en;
    std::string category;       // hair | weapon | armor | consumable
    int price_points;
    std::string image_url;
    std::string description_zh;
    std::string description_en;
};

// Loads content from disk (markdown news + JSON shop catalog).
// Thread-safe snapshot reads; reload() rescans disk.
class ContentLoader {
public:
    explicit ContentLoader(std::filesystem::path content_root,
                           std::filesystem::path shop_catalog);

    // (Re)scan disk. Returns number of items loaded.
    std::size_t reload();

    std::vector<NewsItem> list_news(int page, int size) const;
    std::optional<NewsItem> find_news(const std::string& slug) const;

    std::vector<ShopItem> list_shop(const std::string& category, int page, int size) const;
    bool shop_loaded() const;

private:
    std::filesystem::path content_root_;
    std::filesystem::path shop_catalog_;
    mutable std::mutex mu_;
    std::vector<NewsItem> news_;
    std::vector<ShopItem> shop_;
    bool shop_loaded_ = false;
};

}  // namespace mxh::portal
