// modern/src/portal/shop_routes.hpp
// M5.6: /api/shop/items with category filter and pagination.

#pragma once

#include "portal/content_loader.hpp"
#include "portal/http_server.hpp"

namespace mxh::portal {

void register_shop_routes(HttpServer& server, ContentLoader& content);

}  // namespace mxh::portal
