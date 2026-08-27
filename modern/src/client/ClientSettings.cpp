#include "ClientSettings.hpp"

#include <windows.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <sstream>

namespace mxh::client {
namespace {

bool directory_writable(const std::filesystem::path& directory) {
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) return false;
    wchar_t probe[MAX_PATH]{};
    if (GetTempFileNameW(directory.wstring().c_str(), L"mxh", 0, probe) == 0)
        return false;
    DeleteFileW(probe);
    return true;
}

std::string escape_json(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 4);
    constexpr char hex[] = "0123456789abcdef";
    for (const char ch : value) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20u) {
                const auto byte = static_cast<unsigned char>(ch);
                out += "\\u00";
                out.push_back(hex[(byte >> 4) & 0x0f]);
                out.push_back(hex[byte & 0x0f]);
            } else {
                out.push_back(ch);
            }
            break;
        }
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
    std::string value;
    value.reserve(32);
    for (std::size_t cursor = value_start; cursor < text.size(); ++cursor) {
        const char ch = text[cursor];
        if (ch == '"') return value;
        if (static_cast<unsigned char>(ch) < 0x20) return fallback;
        if (ch != '\\') {
            value.push_back(ch);
            continue;
        }
        if (++cursor >= text.size()) return fallback;
        switch (text[cursor]) {
        case '"': value.push_back('"'); break;
        case '\\': value.push_back('\\'); break;
        case '/': value.push_back('/'); break;
        case 'b': value.push_back('\b'); break;
        case 'f': value.push_back('\f'); break;
        case 'n': value.push_back('\n'); break;
        case 'r': value.push_back('\r'); break;
        case 't': value.push_back('\t'); break;
        case 'u': {
            if (cursor + 4 >= text.size() || text[cursor + 1] != '0' ||
                text[cursor + 2] != '0') return fallback;
            const auto hex_value = [](char digit) -> int {
                if (digit >= '0' && digit <= '9') return digit - '0';
                if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
                if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
                return -1;
            };
            const int hi = hex_value(text[cursor + 3]);
            const int lo = hex_value(text[cursor + 4]);
            if (hi < 0 || lo < 0) return fallback;
            value.push_back(static_cast<char>((hi << 4) | lo));
            cursor += 4;
            break;
        }
        default: return fallback;
        }
    }
    return fallback;
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
    if (!std::isfinite(value)) return 1.0f;
    return std::clamp(value, 0.0f, 1.0f);
}

void backup_corrupt_settings(const std::filesystem::path& path,
                             std::string* warning) {
    std::error_code ec;
    for (std::uint32_t index = 0; index < 1000; ++index) {
        auto backup = path;
        backup += ".corrupt";
        if (index != 0) backup += "." + std::to_string(index);
        if (std::filesystem::exists(backup, ec)) {
            ec.clear();
            continue;
        }
        std::filesystem::rename(path, backup, ec);
        if (!ec) {
            if (warning) *warning = "settings file was invalid; moved to " + backup.string();
            return;
        }
        ec.clear();
    }
    if (warning) *warning = "settings file was invalid; defaults were used (backup failed)";
}

} // namespace

ClientSettingsV1 ClientSettingsStore::defaults() noexcept { return {}; }

std::filesystem::path ClientSettingsStore::default_path() {
    wchar_t buffer[32768]{};
    const auto length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, 32768);
    if (length == 0 || length >= 32768) return {};
    const auto local_dir = std::filesystem::path(buffer) / "Moxian";
    if (directory_writable(local_dir)) return local_dir / "settings.json";

    // Some managed/portable installations expose LOCALAPPDATA as read-only.
    // Keep settings persistence functional without writing beside the game or
    // silently dropping the user's display/audio choices.
    wchar_t temp_buffer[MAX_PATH]{};
    const auto temp_length = GetTempPathW(MAX_PATH, temp_buffer);
    if (temp_length > 0 && temp_length < MAX_PATH) {
        const auto fallback_dir = std::filesystem::path(temp_buffer) / "Moxian";
        if (directory_writable(fallback_dir)) return fallback_dir / "settings.json";
    }
    return local_dir / "settings.json";
}

ClientSettingsV1 ClientSettingsStore::load(const std::filesystem::path& path,
                                           std::string* warning) {
    auto settings = defaults();
    std::ifstream input(path, std::ios::binary);
    if (!input) return settings;
    const std::string text((std::istreambuf_iterator<char>(input)), {});
    input.close();
    const bool has_object = text.find('{') != std::string::npos &&
                            text.rfind('}') != std::string::npos &&
                            text.find('{') < text.rfind('}');
    const bool has_required_keys = text.find("\"schemaVersion\"") != std::string::npos &&
                                   text.find("\"resourceProfileId\"") != std::string::npos &&
                                   text.find("\"postLoginWidth\"") != std::string::npos &&
                                   text.find("\"postLoginHeight\"") != std::string::npos;
    if (!has_object || !has_required_keys) {
        backup_corrupt_settings(path, warning);
        return settings;
    }
    settings.schema_version = read_uint(text, "schemaVersion", 1);
    if (settings.schema_version != 1) {
        backup_corrupt_settings(path, warning);
        if (warning) {
            if (!warning->empty()) *warning += "; ";
            *warning += "unsupported settings schema; defaults were used";
        }
        return defaults();
    }
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
    // Keep the temporary path as a filesystem::path all the way through the
    // Windows file APIs.  Converting LOCALAPPDATA to a narrow string first
    // can select the process ANSI code page and make an otherwise valid user
    // profile path unopenable on localized installations.
    auto temp_path = path;
    temp_path += ".tmp." +
                 std::to_string(static_cast<unsigned long>(GetCurrentProcessId()));
    std::ofstream output(temp_path, std::ios::binary | std::ios::trunc);
    if (!output) {
        if (error) *error = "cannot open temporary settings file: " + temp_path.string();
        return false;
    }
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

    // Make the temporary file durable before publishing it.  A successful
    // rename alone only protects atomicity; it does not guarantee that the
    // newly selected display/audio settings survive a power loss.
    const auto temp_wide = temp_path.wstring();
    HANDLE temp_handle = CreateFileW(
        temp_wide.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (temp_handle == INVALID_HANDLE_VALUE) {
        if (error) *error = "cannot reopen temporary settings file for flush";
        std::filesystem::remove(temp_path, ec);
        return false;
    }
    const bool flushed = FlushFileBuffers(temp_handle) != FALSE;
    const DWORD flush_error = flushed ? ERROR_SUCCESS : GetLastError();
    CloseHandle(temp_handle);
    if (!flushed) {
        if (error) *error = "FlushFileBuffers failed: " + std::to_string(flush_error);
        std::filesystem::remove(temp_path, ec);
        return false;
    }

    const auto path_wide = path.wstring();
    if (!MoveFileExW(temp_wide.c_str(), path_wide.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const DWORD move_error = GetLastError();
        if (error) *error = "MoveFileExW failed: " + std::to_string(move_error);
        std::filesystem::remove(temp_path, ec);
        return false;
    }
    return true;
}

} // namespace mxh::client
