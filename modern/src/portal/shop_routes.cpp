// modern/src/portal/shop_routes.cpp
// M5.6: /api/shop/items?category=&page=&size=

#include "portal/shop_routes.hpp"
#include "portal/portal_log.hpp"

#include <nlohmann/json.hpp>
#include <string>

namespace mxh::portal {

namespace {
nlohmann::json shop_json(const ShopItem& s) {
    return {
        {"idx", s.idx},
        {"name", s.name_zh.empty() ? s.name_en : s.name_zh},
        {"name_zh", s.name_zh},
        {"name_en", s.name_en},
        {"category", s.category},
        {"price_points", s.price_points},
        {"image_url", s.image_url},
        {"description", s.description_zh.empty() ? s.description_en : s.description_zh},
        {"description_zh", s.description_zh},
        {"description_en", s.description_en},
    };
}

int parse_int(const std::string& s, int fallback) {
    if (s.empty()) return fallback;
    try { return std::stoi(s); } catch (...) { return fallback; }
}
}  // namespace

void register_shop_routes(HttpServer& server, ContentLoader& content) {
    server.get_json("/api/shop/items", [&content]() {
        if (!content.shop_loaded()) {
            // 503-ish — return wrapped error; the gateway will set status 200.
            // The frontend treats this as empty items.
            return nlohmann::json{
                {"items", nlohmann::json::array()},
                {"page", 1}, {"total", 0},
                {"warning", "shop catalog not loaded"},
            };
        }
        auto items = content.list_shop("", 1, 24);
        nlohmann::json arr = nlohmann::json::array();
        for (auto& s : items) arr.push_back(shop_json(s));
        return nlohmann::json{{"items", arr}, {"page", 1}, {"total", items.size()}};
    });

    // Category filter — dynamic path.
    server.get_public_dynamic(R"(/api/shop/items/([^/]+))",
        [&content](const std::string& path) -> nlohmann::json {
            auto pos = path.rfind('/');
            std::string cat = (pos == std::string::npos) ? path : path.substr(pos + 1);
            auto items = content.list_shop(cat, 1, 24);
            nlohmann::json arr = nlohmann::json::array();
            for (auto& s : items) arr.push_back(shop_json(s));
            return nlohmann::json{{"items", arr}, {"page", 1}, {"total", items.size()},
                                  {"category", cat}};
        });

    MLOG_INFO("[portal] /api/shop/items routes registered");
}

}  // namespace mxh::portal
