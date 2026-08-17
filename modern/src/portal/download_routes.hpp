// modern/src/portal/download_routes.hpp
// M5.7: /download/{client, manifest.json, checksums.txt}

#pragma once

#include "portal/http_server.hpp"
#include <filesystem>

namespace mxh::portal {

// download_root: directory containing manifest.json + checksums.txt + MoxianClientSetup-*.zip
void register_download_routes(HttpServer& server, std::filesystem::path download_root);

}  // namespace mxh::portal
