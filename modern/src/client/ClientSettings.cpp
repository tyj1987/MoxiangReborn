#include "ClientSettings.hpp"

#include <windows.h>

#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>

namespace mxh::client {
namespace {

std::string escape_json(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 4);
    for (const char ch : value) {
        if (ch == '\\' || ch == '"') out.push_back('\\');
        out.push_back(ch);
    }
    return out;
}

std::string read_string(const std::string& text, const char* key,
                        const std::string& fallback) {
    const std::string marker = std::string("\"") + key + "\":";
    const auto start = text.find(marker);
    if (start == std::string::npos) return fallback;
    auto value_start = start + marker.size();
    while (value_start < text.size() &&
           (text[value_start] == ' ' || text[value_start] == '\t')) ++value_start;
    if (value_start >= text.size() || text[value_start] != '\"') return fallback;
    ++value_start;
    const auto end = text.find('"', value_start);
    return end == std::string::npos ? fallback : text.substr(value_start, end - value_start);
}

std::uint32_t read_uint(const std::string& text, const char* key, std::uint32_t fallback) {
    const std::string marker = std::string("\"") + key + "\":";
    const auto start = text.find(marker);
    if (start == std::string::npos) return fallback;
    auto offset = start + marker.size();
    while (offset < text.size() && (text[offset] == ' ' || text[offset] == '\t')) ++offset;
    const auto begin = text.data() + offset;
    const auto end = text.data() + text.size();
    std::uint32_t value = fallback;
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} ? value : fallback;
}

bool read_bool(const std::string& text, const char* key, bool fallback) {
    const std::string marker = std::string("\"") + key + "\":";
    const auto start = text.find(marker);
    if (start == std::string::npos) return fallback;
    auto offset = start + marker.size();
    while (offset < text.size() && (text[offset] == ' ' || text[offset] == '\t')) ++offset;
    const auto value = text.substr(offset, 5);
    if (value.rfind("true", 0) == 0) return true;
    if (value.rfind("false", 0) == 0) return false;
    return fallback;
}

float read_float(const std::string& text, const char* key, float fallback) {
    const std::string marker = std::string("\"") + key + "\":";
    const auto start = text.find(marker);
    if (start == std::string::npos) return fallback;
    auto offset = start + marker.size();
    while (offset < text.size() && (text[offset] == ' ' || text[offset] == '\t')) ++offset;
    try { return std::stof(text.substr(offset)); } catch (...) { return fallback; }
}

float clamp_volume(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

} // namespace

ClientSettingsV1 ClientSettingsStore::defaults() noexcept { return {}; }

std::filesystem::path ClientSettingsStore::default_path() {
    wchar_t buffer[32768]{};
    const auto length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, 32768);
    if (length == 0 || length >= 32768) return {};
    return std::filesystem::path(buffer) / "Moxian" / "settings.json";
}

ClientSettingsV1 ClientSettingsStore::load(const std::filesystem::path& path,
                                           std::string* warning) {
    auto settings = defaults();
    std::ifstream input(path, std::ios::binary);
    if (!input) return settings;
    const std::string text((std::istreambuf_iterator<char>(input)), {});
    settings.schema_version = read_uint(text, "schemaVersion", 1);
    settings.resource_profile_id = read_string(text, "resourceProfileId", settings.resource_profile_id);
    settings.locale = read_string(text, "locale", settings.locale);
    settings.post_login_width = read_uint(text, "postLoginWidth", settings.post_login_width);
    settings.post_login_height = read_uint(text, "postLoginHeight", settings.post_login_height);
    settings.borderless = read_bool(text, "borderless", settings.borderless);
    settings.vsync = read_bool(text, "vsync", settings.vsync);
    settings.bgm_volume = clamp_volume(read_float(text, "bgmVolume", settings.bgm_volume));
    settings.sfx_volume = clamp_volume(read_float(text, "sfxVolume", settings.sfx_volume));
    settings.ui_volume = clamp_volume(read_float(text, "uiVolume", settings.ui_volume));
    settings.ambient_volume = clamp_volume(read_float(text, "ambientVolume", settings.ambient_volume));
    settings.last_account = read_string(text, "lastAccount", {});
    settings.post_login_width = std::clamp(settings.post_login_width, 800u, 7680u);
    settings.post_login_height = std::clamp(settings.post_login_height, 600u, 4320u);
    if (settings.resource_profile_id != "playdh-current" &&
        settings.resource_profile_id != "sworking-2008-reference") {
        if (warning) *warning = "unknown resource profile; reverted to playdh-current";
        settings.resource_profile_id = "playdh-current";
    }
    return settings;
}

bool ClientSettingsStore::save_atomic(const std::filesystem::path& path,
                                      const ClientSettingsV1& source,
                                      std::string* error) {
    auto settings = source;
    settings.bgm_volume = clamp_volume(settings.bgm_volume);
    settings.sfx_volume = clamp_volume(settings.sfx_volume);
    settings.ui_volume = clamp_volume(settings.ui_volume);
    settings.ambient_volume = clamp_volume(settings.ambient_volume);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) { if (error) *error = ec.message(); return false; }
    const auto temp = path.string() + ".tmp";
    std::ofstream output(temp, std::ios::binary | std::ios::trunc);
    if (!output) { if (error) *error = "cannot open temporary settings file"; return false; }
    output << "{\n"
           << "  \"schemaVersion\": " << settings.schema_version << ",\n"
           << "  \"resourceProfileId\": \"" << escape_json(settings.resource_profile_id) << "\",\n"
           << "  \"locale\": \"" << escape_json(settings.locale) << "\",\n"
           << "  \"postLoginWidth\": " << settings.post_login_width << ",\n"
           << "  \"postLoginHeight\": " << settings.post_login_height << ",\n"
           << "  \"borderless\": " << (settings.borderless ? "true" : "false") << ",\n"
           << "  \"vsync\": " << (settings.vsync ? "true" : "false") << ",\n"
           << "  \"bgmVolume\": " << settings.bgm_volume << ",\n"
           << "  \"sfxVolume\": " << settings.sfx_volume << ",\n"
           << "  \"uiVolume\": " << settings.ui_volume << ",\n"
           << "  \"ambientVolume\": " << settings.ambient_volume << ",\n"
           << "  \"lastAccount\": \"" << escape_json(settings.last_account) << "\"\n}\n";
    output.close();
    if (!output) { if (error) *error = "failed writing settings"; return false; }
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::filesystem::remove(path, ec);
        ec.clear();
        std::filesystem::rename(temp, path, ec);
    }
    if (ec && error) *error = ec.message();
    return !ec;
}

} // namespace mxh::client
