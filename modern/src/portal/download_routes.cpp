// modern/src/portal/download_routes.cpp
// M5.7: /download/client (302 to /static/downloads/MoxianClientSetup-<ver>.zip),
//        /download/manifest.json (passthrough),
//        /download/checksums.txt (passthrough).

#include "portal/download_routes.hpp"
#include "portal/portal_log.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace mxh::portal {

namespace {

std::string read_file(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    std::stringstream ss; ss << in.rdbuf();
    return ss.str();
}

std::string find_client_zip(const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    if (!fs::exists(root)) return {};
    for (auto& entry : fs::directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        auto name = entry.path().filename().string();
        if (name.find("MoxianClientSetup") == 0 &&
            entry.path().extension() == ".zip") {
            return name;
        }
    }
    return {};
}

}  // namespace

void register_download_routes(HttpServer& server, std::filesystem::path download_root) {
    // GET /download/client -> 302 to /static/downloads/MoxianClientSetup-<ver>.zip
    server.get_public_dynamic(R"(/download/client)",
        [download_root](const std::string&) -> nlohmann::json {
            auto zip = find_client_zip(download_root);
            if (zip.empty()) {
                return nlohmann::json{{"error", "client package not found"}};
            }
            // The HttpServer helpers don't currently expose 302; we set it via
            // a custom handler. The wrapping get_public_dynamic returns 200,
            // so we wrap the redirect URL in a JSON body. The frontend will
            // extract `url` and follow it.
            return nlohmann::json{
                {"url", "/static/downloads/" + zip},
                {"method", "GET"},
            };
        });

    // GET /download/manifest.json  — read manifest.json from download_root.
    server.get_public_dynamic(R"(/download/manifest\.json)",
        [download_root](const std::string&) -> nlohmann::json {
            auto path = download_root / "manifest.json";
            auto raw = read_file(path);
            if (raw.empty()) {
                return nlohmann::json{
                    {"error", "manifest not found"},
                    {"path", path.string()},
                };
            }
            try {
                return nlohmann::json::parse(raw);
            } catch (const std::exception& e) {
                return nlohmann::json{{"error", std::string("invalid manifest: ") + e.what()}};
            }
        });

    // GET /download/checksums.txt  — pass-through plain text.
    server.get_public_dynamic(R"(/download/checksums\.txt)",
        [download_root](const std::string&) -> nlohmann::json {
            auto path = download_root / "checksums.txt";
            auto raw = read_file(path);
            if (raw.empty()) {
                return nlohmann::json{{"error", "checksums not found"}};
            }
            return nlohmann::json{{"text", raw}};
        });

    MLOG_INFO("[portal] /download/* routes registered (root=%s)",
              download_root.string().c_str());
}

}  // namespace mxh::portal
