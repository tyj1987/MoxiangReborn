#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace mxh::client {

struct ClientSettingsV1 {
    std::uint32_t schema_version = 1;
    std::string resource_profile_id = "playdh-current";
    std::string locale = "default";
    std::uint32_t post_login_width = 1024;
    std::uint32_t post_login_height = 768;
    bool borderless = false;
    bool vsync = true;
    float bgm_volume = 1.0f;
    float sfx_volume = 1.0f;
    float ui_volume = 1.0f;
    float ambient_volume = 1.0f;
    std::string last_account;
};

class ClientSettingsStore {
public:
    static ClientSettingsV1 defaults() noexcept;
    static std::filesystem::path default_path();

    static ClientSettingsV1 load(const std::filesystem::path& path,
                                 std::string* warning = nullptr);
    static bool save_atomic(const std::filesystem::path& path,
                            const ClientSettingsV1& settings,
                            std::string* error = nullptr);
};

} // namespace mxh::client
