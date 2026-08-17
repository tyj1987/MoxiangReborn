// modern/src/portal/content_loader.cpp
// M5.5: scan content_root/news/*.md and parse simple front-matter.

#include "portal/content_loader.hpp"
#include "portal/portal_log.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

namespace mxh::portal {

namespace {

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

// Trim leading/trailing whitespace.
std::string trim(const std::string& s) {
    auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Parse minimal YAML front-matter `key: value` lines (1-level, no nesting).
std::string parse_front_matter(const std::string& md, std::string& body) {
    std::string fm;
    body = md;
    if (md.size() < 4 || md.substr(0, 4) != "---\n") return fm;
    auto end = md.find("\n---", 4);
    if (end == std::string::npos) return fm;
    fm = md.substr(4, end - 4);
    body = md.substr(end + 4);
    // Strip leading newline after closing ---
    if (!body.empty() && body[0] == '\n') body.erase(body.begin());
    return fm;
}

std::string fm_value(const std::string& fm, const std::string& key) {
    auto pos = fm.find(key + ":");
    if (pos == std::string::npos) return {};
    auto eol = fm.find('\n', pos);
    return trim(fm.substr(pos + key.size() + 1,
                          eol == std::string::npos ? std::string::npos : eol - pos - key.size() - 1));
}

std::string slugify(const std::filesystem::path& p) {
    auto s = p.stem().string();
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return c == ' ' ? '-' : std::tolower(c); });
    return s;
}

}  // namespace

ContentLoader::ContentLoader(std::filesystem::path content_root,
                             std::filesystem::path shop_catalog)
    : content_root_(std::move(content_root)), shop_catalog_(std::move(shop_catalog)) {}

std::size_t ContentLoader::reload() {
    std::lock_guard<std::mutex> g(mu_);
    news_.clear();

    namespace fs = std::filesystem;
    if (fs::exists(content_root_ / "news")) {
        for (auto& entry : fs::directory_iterator(content_root_ / "news")) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".md") continue;
            std::ifstream in(entry.path());
            if (!in) continue;
            std::stringstream ss; ss << in.rdbuf();
            std::string md = ss.str();
            std::string body;
            auto fm = parse_front_matter(md, body);
            NewsItem n;
            n.slug = slugify(entry.path());
            n.title_zh = fm_value(fm, "title_zh");
            n.title_en = fm_value(fm, "title_en");
            if (n.title_zh.empty() && n.title_en.empty()) {
                n.title_zh = n.slug;
            }
            n.summary_zh = fm_value(fm, "summary_zh");
            n.summary_en = fm_value(fm, "summary_en");
            n.hero_image = fm_value(fm, "hero_image");
            n.published_at = fm_value(fm, "published_at");
            if (n.published_at.empty()) {
                auto t = std::time(nullptr);
                std::tm tm{};
#if defined(_WIN32)
                gmtime_s(&tm, &t);
#else
                gmtime_r(&tm, &t);
#endif
                char buf[32];
                std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
                n.published_at = buf;
            }
            // Body: first line is title_zh, second line is title_en, rest is body_zh;
            // blank-line-separated section "EN:" becomes body_en.
            auto en_marker = body.find("\n\nEN:\n");
            if (en_marker != std::string::npos) {
                n.body_zh = trim(body.substr(0, en_marker));
                n.body_en = trim(body.substr(en_marker + 6));
            } else {
                n.body_zh = trim(body);
            }
            n.id = n.slug;
            news_.push_back(std::move(n));
        }
    }
    std::sort(news_.begin(), news_.end(),
              [](const NewsItem& a, const NewsItem& b) {
                  return a.published_at > b.published_at;
              });

    // Shop catalog
    shop_.clear();
    shop_loaded_ = false;
    if (fs::exists(shop_catalog_)) {
        try {
            std::ifstream in(shop_catalog_);
            nlohmann::json j; in >> j;
            for (auto& it : j.value("items", nlohmann::json::array())) {
                ShopItem s;
                s.idx = it.value("idx", 0);
                s.name_zh = it.value("name_zh", "");
                s.name_en = it.value("name_en", "");
                s.category = it.value("category", "consumable");
                s.price_points = it.value("price_points", 0);
                s.image_url = it.value("image_url", "");
                s.description_zh = it.value("description_zh", "");
                s.description_en = it.value("description_en", "");
                shop_.push_back(std::move(s));
            }
            shop_loaded_ = true;
        } catch (const std::exception& e) {
            MLOG_WARN("[portal] failed to load shop catalog: %s", e.what());
        }
    }

    MLOG_INFO("[portal] content reload: %zu news, %zu shop items",
              news_.size(), shop_.size());
    return news_.size() + shop_.size();
}

std::vector<NewsItem> ContentLoader::list_news(int page, int size) const {
    std::lock_guard<std::mutex> g(mu_);
    if (page < 1) page = 1;
    if (size < 1) size = 10;
    int begin = (page - 1) * size;
    if (begin >= static_cast<int>(news_.size())) return {};
    int end = std::min(begin + size, static_cast<int>(news_.size()));
    return std::vector<NewsItem>(news_.begin() + begin, news_.begin() + end);
}

std::optional<NewsItem> ContentLoader::find_news(const std::string& slug) const {
    std::lock_guard<std::mutex> g(mu_);
    for (auto& n : news_) {
        if (n.slug == slug) return n;
    }
    return std::nullopt;
}

std::vector<ShopItem> ContentLoader::list_shop(const std::string& category, int page, int size) const {
    std::lock_guard<std::mutex> g(mu_);
    std::vector<ShopItem> filtered;
    auto cat = to_lower(category);
    for (auto& s : shop_) {
        if (cat.empty() || to_lower(s.category) == cat) {
            filtered.push_back(s);
        }
    }
    if (page < 1) page = 1;
    if (size < 1) size = 24;
    int begin = (page - 1) * size;
    if (begin >= static_cast<int>(filtered.size())) return {};
    int end = std::min(begin + size, static_cast<int>(filtered.size()));
    return std::vector<ShopItem>(filtered.begin() + begin, filtered.begin() + end);
}

bool ContentLoader::shop_loaded() const {
    std::lock_guard<std::mutex> g(mu_);
    return shop_loaded_;
}

}  // namespace mxh::portal
