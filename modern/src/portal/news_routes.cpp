// modern/src/portal/news_routes.cpp
// M5.5: /api/news (list) and /api/news/:slug (detail)

#include "portal/news_routes.hpp"
#include "portal/portal_log.hpp"

#include <nlohmann/json.hpp>
#include <string_view>

namespace mxh::portal {

namespace {
nlohmann::json news_summary(const NewsItem& n) {
    return {
        {"id", n.id},
        {"slug", n.slug},
        {"title", n.title_zh.empty() ? n.title_en : n.title_zh},
        {"title_zh", n.title_zh},
        {"title_en", n.title_en},
        {"summary", n.summary_zh.empty() ? n.summary_en : n.summary_zh},
        {"summary_zh", n.summary_zh},
        {"summary_en", n.summary_en},
        {"hero_image", n.hero_image},
        {"published_at", n.published_at},
    };
}

nlohmann::json news_detail(const NewsItem& n) {
    auto j = news_summary(n);
    j["body"]     = n.body_zh.empty() ? n.body_en : n.body_zh;
    j["body_zh"]  = n.body_zh;
    j["body_en"]  = n.body_en;
    return j;
}

// Extract the slug from "/api/news/<slug>".
std::string extract_slug(std::string_view path) {
    constexpr std::string_view prefix = "/api/news/";
    if (!path.starts_with(prefix)) return {};
    auto rest = path.substr(prefix.size());
    auto slash = rest.find('/');
    if (slash != std::string_view::npos) rest = rest.substr(0, slash);
    return std::string(rest);
}

int parse_int(const std::string& s, int fallback) {
    if (s.empty()) return fallback;
    try { return std::stoi(s); } catch (...) { return fallback; }
}
}  // namespace

void register_news_routes(HttpServer& server, ContentLoader& content) {
    // GET /api/news?page=1&size=10
    server.get_json("/api/news", [&content]() {
        return nlohmann::json{{"items", nlohmann::json::array()}, {"page", 1}, {"total", 0}};
    });

    // GET /api/news/<slug>  — note: this is a regex pattern in cpp-httplib.
    server.get_public_dynamic(R"(/api/news/([^/]+))",
        [&content](const std::string& path) -> nlohmann::json {
            auto slug = extract_slug(path);
            if (slug.empty()) {
                return nlohmann::json{{"error", "missing slug"}};
            }
            auto n = content.find_news(slug);
            if (!n) {
                return nlohmann::json{{"error", "not found"}, {"slug", slug}};
            }
            return news_detail(*n);
        });

    // GET /api/news/page/<n>  — optional paginated view.
    server.get_public_dynamic(R"(/api/news/page/(\d+))",
        [&content](const std::string& path) -> nlohmann::json {
            auto pos = path.rfind('/');
            int page = parse_int(path.substr(pos + 1), 1);
            auto items = content.list_news(page, 10);
            nlohmann::json arr = nlohmann::json::array();
            for (auto& n : items) arr.push_back(news_summary(n));
            return nlohmann::json{{"items", arr}, {"page", page}, {"total", items.size()}};
        });

    MLOG_INFO("[portal] /api/news routes registered (list + dynamic :slug)");
}

}  // namespace mxh::portal
