// modern/src/portal/news_routes.hpp
// M5.5: /api/news and /api/news/:slug

#pragma once

#include "portal/content_loader.hpp"
#include "portal/http_server.hpp"

namespace mxh::portal {

void register_news_routes(HttpServer& server, ContentLoader& content);

}  // namespace mxh::portal
