#include <windows.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <array>
#include <regex>
#include <string>
#include <string_view>
#include <cwchar>
#include <unordered_map>
#include <vector>
#include <utility>

namespace fs = std::filesystem;

struct LauncherSettings {
    std::wstring profile = L"playdh-current";
    int postWidth = 1024;
    int postHeight = 768;
    bool borderless = false;
    bool vsync = true;
    int bgmVolume = 100;
    int sfxVolume = 100;
};

struct LauncherEndpoints {
    int loginPort = 6001;
    int agentPort = 7001;
    int mapPort = 8001;
};

static int parsePort(const wchar_t* value, int fallback, bool* valid = nullptr) noexcept {
    if (valid) *valid = false;
    if (!value || !*value) return fallback;
    wchar_t* end = nullptr;
    const auto parsed = std::wcstol(value, &end, 10);
    if (end == value || *end != L'\0' || parsed < 1 || parsed > 65535) return fallback;
    if (valid) *valid = true;
    return static_cast<int>(parsed);
}

// Quote one argument using the CommandLineToArgvW/MSVC escaping rules.  The
// launcher passes only paths and numeric settings to the client, but paths
// may legally contain spaces, quotes, or trailing backslashes.  Hand-built
// `"path"` wrapping is not sufficient for those cases and can alter the
// resource root seen by the client.
static std::wstring quoteWindowsArg(std::wstring_view value) {
    std::wstring out;
    out.reserve(value.size() + 2);
    out.push_back(L'"');
    std::size_t backslashes = 0;
    for (const wchar_t ch : value) {
        if (ch == L'\\') {
            ++backslashes;
            continue;
        }
        if (ch == L'"') {
            out.append(backslashes * 2 + 1, L'\\');
            out.push_back(L'"');
            backslashes = 0;
            continue;
        }
        out.append(backslashes, L'\\');
        backslashes = 0;
        out.push_back(ch);
    }
    out.append(backslashes * 2, L'\\');
    out.push_back(L'"');
    return out;
}

static fs::path settingsPath() {
    wchar_t buffer[MAX_PATH]{};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    const fs::path localRoot = n ? fs::path(buffer) : fs::path{};
    const fs::path candidates[] = {
        localRoot / L"Moxian",
        fs::temp_directory_path() / L"Moxian"
    };
    for (const auto& directory : candidates) {
        if (directory.empty()) continue;
        std::error_code ec;
        fs::create_directories(directory, ec);
        if (ec) continue;
        wchar_t probe[MAX_PATH]{};
        if (GetTempFileNameW(directory.c_str(), L"mxh", 0, probe) != 0) {
            DeleteFileW(probe);
            return directory / L"settings.json";
        }
    }
    // Preserve a deterministic path for the error message if both locations
    // are unavailable; saveSettings will fail closed and refuse launch.
    return (localRoot.empty() ? fs::temp_directory_path() : localRoot) /
           L"Moxian" / L"settings.json";
}

static int parseBoundedInt(const std::string& value, int fallback) noexcept {
    try {
        const auto parsed = std::stoll(value);
        if (parsed < 0 || parsed > 100000) return fallback;
        return static_cast<int>(parsed);
    } catch (...) {
        return fallback;
    }
}

static int parseVolumePercent(const std::string& value, int fallback) noexcept {
    try {
        const auto parsed = std::stod(value);
        if (!std::isfinite(parsed) || parsed < 0.0 || parsed > 100.0) return fallback;
        // ClientSettings stores normalized volume (0.0..1.0), while the
        // launcher UI presents percentages. Accept both representations so
        // settings survive a launcher/client round trip.
        const auto percent = parsed <= 1.0 ? parsed * 100.0 : parsed;
        return static_cast<int>(std::lround(percent));
    } catch (...) {
        return fallback;
    }
}

static bool sha256File(const fs::path& path, std::string& hex) noexcept {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectLength = 0;
    DWORD resultLength = 0;
    bool ok = BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
                                          nullptr, 0)) &&
              BCRYPT_SUCCESS(BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                                reinterpret_cast<PUCHAR>(&objectLength),
                                sizeof(objectLength), &resultLength, 0));
    std::vector<UCHAR> object;
    std::array<UCHAR, 32> digest{};
    if (ok) {
        object.resize(objectLength);
        ok = BCRYPT_SUCCESS(BCryptCreateHash(algorithm, &hash, object.data(), objectLength,
                              nullptr, 0, 0));
    }
    std::array<char, 1024 * 1024> buffer{};
    while (ok && input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count > 0) {
            ok = BCRYPT_SUCCESS(BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()),
                                static_cast<ULONG>(count), 0));
        }
    }
    if (ok) ok = BCRYPT_SUCCESS(BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0));
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!ok) return false;
    static constexpr char digits[] = "0123456789abcdef";
    hex.clear();
    hex.reserve(64);
    for (const auto byte : digest) {
        hex.push_back(digits[(byte >> 4) & 0x0f]);
        hex.push_back(digits[byte & 0x0f]);
    }
    return true;
}

static fs::path locateResourceManifest(const fs::path& root) {
    // A release package carries the manifest beside its resource tree.  The
    // extra repository-layout candidates keep the developer build usable
    // without making an absolute machine path part of the product contract.
    const fs::path candidates[] = {
        root / L"resource-profile.sha256.json",
        root.parent_path().parent_path().parent_path() /
            L"reference/manifests/playdh-current.sha256.json",
        root.parent_path().parent_path().parent_path().parent_path() /
            L"reference/manifests/playdh-current.sha256.json"
    };
    for (const auto& candidate : candidates) {
        if (fs::is_regular_file(candidate)) return candidate;
    }
    return {};
}

static bool verifyResourceManifest(const fs::path& root,
                                   const fs::path& manifestPath,
                                   std::wstring& error) {
    std::ifstream input(manifestPath, std::ios::binary);
    if (!input) {
        error = L"无法读取资源清单：" + manifestPath.wstring();
        return false;
    }
    const std::string text((std::istreambuf_iterator<char>(input)), {});
    std::smatch profileMatch;
    if (!std::regex_search(text, profileMatch,
            std::regex(R"REGEX("profileId"\s*:\s*"([^"]+)")REGEX")) ||
        profileMatch[1].str() != "playdh-current") {
        error = L"资源清单 profileId 不是 playdh-current：" + manifestPath.wstring();
        return false;
    }
    std::smatch countMatch;
    std::smatch byteMatch;
    if (!std::regex_search(text, countMatch,
            std::regex(R"REGEX("fileCount"\s*:\s*(\d+))REGEX")) ||
        !std::regex_search(text, byteMatch,
            std::regex(R"REGEX("byteCount"\s*:\s*([0-9]+(?:\.[0-9]+)?))REGEX"))) {
        error = L"资源清单缺少完整 inventory：" + manifestPath.wstring();
        return false;
    }
    std::size_t expectedCount = 0;
    std::uint64_t expectedBytes = 0;
    try {
        expectedCount = static_cast<std::size_t>(std::stoull(countMatch[1].str()));
        expectedBytes = static_cast<std::uint64_t>(std::stold(byteMatch[1].str()));
    } catch (...) {
        error = L"资源清单 inventory 数值无效：" + manifestPath.wstring();
        return false;
    }

    struct Expected { std::uint64_t bytes; std::string sha256; };
    std::unordered_map<std::string, Expected> expected;
    const std::regex entryPattern(
        R"REGEX(\{\s*"path"\s*:\s*"([^"]+)"\s*,\s*"bytes"\s*:\s*(\d+)\s*,\s*"sha256"\s*:\s*"([0-9a-fA-F]{64})")REGEX");
    for (std::sregex_iterator it(text.begin(), text.end(), entryPattern), end;
         it != end; ++it) {
        const auto relative = (*it)[1].str();
        const auto normalized = fs::path(relative).lexically_normal();
        if (normalized.empty() || normalized.is_absolute() ||
            normalized != fs::path(relative) || relative.find('\\') != std::string::npos) {
            error = L"资源清单包含不安全路径：" + fs::path(relative).wstring();
            return false;
        }
        if (!expected.emplace(relative, Expected{
                static_cast<std::uint64_t>(std::stoull((*it)[2].str())),
                (*it)[3].str()}).second) {
            error = L"资源清单包含重复路径：" + fs::path(relative).wstring();
            return false;
        }
    }
    if (expected.size() != expectedCount) {
        error = L"资源清单 fileCount 与条目数不一致：" + manifestPath.wstring();
        return false;
    }
    const auto files = [&] {
        std::vector<fs::path> result;
        std::error_code ec;
        for (fs::recursive_directory_iterator it(root, ec), end; it != end && !ec; it.increment(ec)) {
            if (it->is_regular_file(ec)) result.push_back(it->path());
        }
        return result;
    }();
    if (files.size() != expectedCount) {
        error = L"资源文件数量与清单不一致：期望 " + std::to_wstring(expectedCount) +
                L"，实际 " + std::to_wstring(files.size());
        return false;
    }
    std::uint64_t actualBytes = 0;
    for (const auto& file : files) {
        const auto relative = file.lexically_relative(root).generic_string();
        const auto found = expected.find(relative);
        if (found == expected.end()) {
            error = L"资源目录包含清单外文件：" + fs::path(relative).wstring();
            return false;
        }
        const auto size = static_cast<std::uint64_t>(fs::file_size(file));
        actualBytes += size;
        if (size != found->second.bytes) {
            error = L"资源大小不匹配：" + file.wstring();
            return false;
        }
        std::string actual;
        if (!sha256File(file, actual) || _stricmp(actual.c_str(), found->second.sha256.c_str()) != 0) {
            error = L"资源 SHA-256 不匹配：" + file.wstring();
            return false;
        }
    }
    if (actualBytes != expectedBytes) {
        error = L"资源总字节数与清单不一致";
        return false;
    }
    return true;
}

static bool verifyRequiredResourceHashes(const fs::path& root, std::wstring& error) {
    const auto manifest = locateResourceManifest(root);
    if (manifest.empty()) {
        error = L"缺少 playdh-current 完整资源清单；拒绝使用不完整校验启动。";
        return false;
    }
    return verifyResourceManifest(root, manifest, error);
}

static LauncherSettings loadSettings() {
    LauncherSettings s;
    std::ifstream in(settingsPath(), std::ios::binary);
    if (!in) return s;
    std::string text((std::istreambuf_iterator<char>(in)), {});
    const auto open = text.find('{');
    const auto close = text.rfind('}');
    if (open == std::string::npos || close == std::string::npos || open > close) return s;
    std::smatch match;
    int schema = 1;
    if (std::regex_search(text, match, std::regex(R"REGEX("schemaVersion"\s*:\s*(\d+))REGEX"))) {
        try { schema = std::stoi(match[1].str()); } catch (...) { schema = 0; }
    }
    if (schema != 1) return s;
    if (std::regex_search(text, match, std::regex(R"REGEX("resourceProfileId"\s*:\s*"([^"]+)")REGEX")) ||
        std::regex_search(text, match, std::regex(R"REGEX("profile"\s*:\s*"([^"]+)")REGEX")))
        s.profile = std::wstring(match[1].str().begin(), match[1].str().end());
    if (std::regex_search(text, match, std::regex(R"REGEX("postLoginWidth"\s*:\s*(\d+))REGEX"))) s.postWidth = parseBoundedInt(match[1].str(), s.postWidth);
    if (std::regex_search(text, match, std::regex(R"REGEX("postLoginHeight"\s*:\s*(\d+))REGEX"))) s.postHeight = parseBoundedInt(match[1].str(), s.postHeight);
    if (std::regex_search(text, match, std::regex(R"REGEX("borderless"\s*:\s*(true|false))REGEX"))) s.borderless = match[1].str() == "true";
    if (std::regex_search(text, match, std::regex(R"REGEX("vsync"\s*:\s*(true|false))REGEX"))) s.vsync = match[1].str() == "true";
    if (std::regex_search(text, match, std::regex(R"REGEX("bgmVolume"\s*:\s*([0-9]+(?:\.[0-9]+)?))REGEX"))) s.bgmVolume = parseVolumePercent(match[1].str(), s.bgmVolume);
    if (std::regex_search(text, match, std::regex(R"REGEX("sfxVolume"\s*:\s*([0-9]+(?:\.[0-9]+)?))REGEX"))) s.sfxVolume = parseVolumePercent(match[1].str(), s.sfxVolume);
    if (s.profile != L"playdh-current") s.profile = L"playdh-current";
    if (s.postWidth < 800 || s.postHeight < 600) { s.postWidth = 1024; s.postHeight = 768; }
    s.postWidth = (s.postWidth > 7680) ? 7680 : s.postWidth;
    s.postHeight = (s.postHeight > 4320) ? 4320 : s.postHeight;
    s.bgmVolume = std::clamp(s.bgmVolume, 0, 100);
    s.sfxVolume = std::clamp(s.sfxVolume, 0, 100);
    return s;
}

static bool saveSettings(const LauncherSettings& s) {
    std::error_code ec;
    fs::create_directories(settingsPath().parent_path(), ec);
    fs::path temp = settingsPath(); temp += L".tmp";
    std::ifstream existing(settingsPath(), std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(existing)), {});
    if (text.empty()) text = "{\n}\n";
    std::string profile;
    profile.reserve(s.profile.size());
    for (const auto c : s.profile) profile.push_back(static_cast<char>(c));
    const auto replace_or_insert = [&text](const std::string& key, const std::string& value) {
        const std::regex pattern("(\\\"" + key + "\\\"\\s*:\\s*)[^,}]+", std::regex::ECMAScript);
        if (std::regex_search(text, pattern)) text = std::regex_replace(text, pattern, "$1" + value);
        else {
            const auto close = text.rfind('}');
            if (close == std::string::npos) { text = "{\n  \"" + key + "\": " + value + "\n}\n"; return; }
            auto insert_at = close;
            while (insert_at > 0 && std::isspace(static_cast<unsigned char>(text[insert_at - 1]))) --insert_at;
            const bool has_field = insert_at > 0 && text[insert_at - 1] != '{' && text[insert_at - 1] != ',';
            text.insert(insert_at, std::string(has_field ? ",\n  \"" : "  \"") + key + "\": " + value + "\n");
        }
    };
    replace_or_insert("schemaVersion", "1");
    replace_or_insert("resourceProfileId", "\"" + profile + "\"");
    replace_or_insert("postLoginWidth", std::to_string(s.postWidth));
    replace_or_insert("postLoginHeight", std::to_string(s.postHeight));
    replace_or_insert("borderless", s.borderless ? "true" : "false");
    replace_or_insert("vsync", s.vsync ? "true" : "false");
    replace_or_insert("bgmVolume", std::to_string(static_cast<double>(s.bgmVolume) / 100.0));
    replace_or_insert("sfxVolume", std::to_string(static_cast<double>(s.sfxVolume) / 100.0));
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << text;
    out.close();
    if (!out) {
        std::error_code ignored;
        fs::remove(temp, ignored);
        return false;
    }
    // Publish only after the temporary settings file has reached the OS
    // cache.  MOVEFILE_WRITE_THROUGH protects the rename itself, while this
    // explicit flush protects the newly written display settings.
    const auto tempHandle = CreateFileW(
        temp.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (tempHandle == INVALID_HANDLE_VALUE || !FlushFileBuffers(tempHandle)) {
        if (tempHandle != INVALID_HANDLE_VALUE) CloseHandle(tempHandle);
        std::error_code ignored;
        fs::remove(temp, ignored);
        return false;
    }
    CloseHandle(tempHandle);
    if (!MoveFileExW(temp.c_str(), settingsPath().c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::error_code ignored;
        fs::remove(temp, ignored);
        return false;
    }
    return true;
}

static fs::path locateResourceRoot(const fs::path& executable) {
    const fs::path bin = executable.parent_path();
    const fs::path roots[] = {
        bin / L"data" / L"PlayDH",
        bin.parent_path() / L"data" / L"PlayDH",
        bin.parent_path() / L"modern" / L"data" / L"PlayDH"
    };
    for (const auto& root : roots) {
        const fs::path required[] = {
            root / L"Map.pak",
            root / L"Resource" / L"Map" / L"Map10.bmhm",
            root / L"Resource" / L"MonsterList.bin",
            root / L"Resource" / L"Client" / L"NpcChxList.bin",
            root / L"Image" / L"InterfaceScript" / L"IDDlg.bin",
            root / L"Image" / L"InterfaceScript" / L"CharSelectDlg.bin",
            root / L"Image" / L"InterfaceScript" / L"CharMakeNewDlg.bin",
            root / L"Sound" / L"SoundList.bin"
        };
        if (std::all_of(std::begin(required), std::end(required),
                        [](const auto& path) { return fs::is_regular_file(path); })) {
            return root;
        }
    }
    return {};
}

class LauncherWindow {
public:
    LauncherWindow(LauncherEndpoints endpoints, fs::path resourceRoot,
                   fs::path evidenceDir)
        : endpoints_(endpoints), resourceRoot_(std::move(resourceRoot)),
          evidenceDir_(std::move(evidenceDir)) {}
    bool create(HINSTANCE instance) {
        settings_ = loadSettings();
        WNDCLASSW wc{}; wc.hInstance = instance; wc.lpfnWndProc = &LauncherWindow::proc;
        wc.lpszClassName = L"MoxianLauncherWindow"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        RegisterClassW(&wc);
        hwnd_ = CreateWindowW(wc.lpszClassName, L"墨香启动器", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                              CW_USEDEFAULT, CW_USEDEFAULT, 480, 335, nullptr, nullptr, instance, this);
        return hwnd_ != nullptr;
    }
    int run() { ShowWindow(hwnd_, SW_SHOW); UpdateWindow(hwnd_); MSG msg{}; while (GetMessageW(&msg, nullptr, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); } return static_cast<int>(msg.wParam); }
private:
    static constexpr int kPostWidthEdit = 10;
    static constexpr int kPostHeightEdit = 11;
    static constexpr int kBorderlessCheck = 12;
    static constexpr int kVsyncCheck = 13;
    static constexpr int kBgmVolumeEdit = 14;
    static constexpr int kSfxVolumeEdit = 15;
    static LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        auto* self = reinterpret_cast<LauncherWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) { self = static_cast<LauncherWindow*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams); SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self)); self->hwnd_ = hwnd; }
        return self ? self->handle(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
    }
    LRESULT handle(UINT msg, WPARAM wp, LPARAM) {
        if (msg == WM_CREATE) {
            CreateWindowW(L"STATIC", L"资源 profile: playdh-current\n账号密码在客户端登录界面输入，启动器不会接触凭据。", WS_CHILD | WS_VISIBLE, 20, 20, 430, 45, hwnd_, nullptr, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"检查/修复", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 85, 120, 32, hwnd_, reinterpret_cast<HMENU>(1), nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"启动游戏", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 155, 85, 120, 32, hwnd_, reinterpret_cast<HMENU>(2), nullptr, nullptr);
            CreateWindowW(L"STATIC", L"登录窗口 800×600；登录成功后切换为：", WS_CHILD | WS_VISIBLE, 20, 140, 300, 22, hwnd_, nullptr, nullptr, nullptr);
            CreateWindowW(L"STATIC", L"宽", WS_CHILD | WS_VISIBLE, 20, 168, 20, 22, hwnd_, nullptr, nullptr, nullptr);
            CreateWindowW(L"EDIT", std::to_wstring(settings_.postWidth).c_str(),
                          WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                          45, 165, 75, 24, hwnd_, reinterpret_cast<HMENU>(kPostWidthEdit), nullptr, nullptr);
            CreateWindowW(L"STATIC", L"高", WS_CHILD | WS_VISIBLE, 130, 168, 20, 22, hwnd_, nullptr, nullptr, nullptr);
            CreateWindowW(L"EDIT", std::to_wstring(settings_.postHeight).c_str(),
                          WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                          155, 165, 75, 24, hwnd_, reinterpret_cast<HMENU>(kPostHeightEdit), nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"无边框窗口", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                          250, 165, 120, 24, hwnd_, reinterpret_cast<HMENU>(kBorderlessCheck), nullptr, nullptr);
            if (settings_.borderless) {
                SendDlgItemMessageW(hwnd_, kBorderlessCheck, BM_SETCHECK, BST_CHECKED, 0);
            }
            CreateWindowW(L"BUTTON", L"垂直同步", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                          20, 200, 120, 24, hwnd_, reinterpret_cast<HMENU>(kVsyncCheck), nullptr, nullptr);
            if (settings_.vsync) {
                SendDlgItemMessageW(hwnd_, kVsyncCheck, BM_SETCHECK, BST_CHECKED, 0);
            }
            CreateWindowW(L"STATIC", L"BGM 音量(0-100)", WS_CHILD | WS_VISIBLE,
                          160, 200, 120, 22, hwnd_, nullptr, nullptr, nullptr);
            CreateWindowW(L"EDIT", std::to_wstring(settings_.bgmVolume).c_str(),
                          WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                          285, 197, 65, 24, hwnd_, reinterpret_cast<HMENU>(kBgmVolumeEdit), nullptr, nullptr);
            CreateWindowW(L"STATIC", L"SFX 音量(0-100)", WS_CHILD | WS_VISIBLE,
                          160, 230, 120, 22, hwnd_, nullptr, nullptr, nullptr);
            CreateWindowW(L"EDIT", std::to_wstring(settings_.sfxVolume).c_str(),
                          WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                          285, 227, 65, 24, hwnd_, reinterpret_cast<HMENU>(kSfxVolumeEdit), nullptr, nullptr);
            return 0;
        }
        if (msg == WM_COMMAND && LOWORD(wp) == 1) {
            wchar_t module[MAX_PATH]{};
            GetModuleFileNameW(nullptr, module, MAX_PATH);
            const auto root = resourceRoot_.empty()
                ? locateResourceRoot(fs::path(module)) : resourceRoot_;
            const auto validRoot = root.empty() ? fs::path{} :
                (fs::is_directory(root) ? root : fs::path{});
            if (validRoot.empty()) {
                MessageBoxW(hwnd_, L"资源检查失败：缺少运行所需的 Map10、怪物、NPC、登录、选角、建角或音频资源。未执行任何删除或下载。", L"检查/修复", MB_ICONERROR);
            } else {
                std::wstring hashError;
                if (!verifyRequiredResourceHashes(validRoot, hashError)) {
                    MessageBoxW(hwnd_, (L"资源校验失败：\n" + hashError + L"\n未执行任何删除或下载。请从正式 profile 修复资源。").c_str(), L"检查/修复", MB_ICONERROR);
                    return 0;
                }
                MessageBoxW(hwnd_, (L"本地资源基础检查和 SHA-256 校验通过：\n" + validRoot.wstring()).c_str(), L"检查/修复", MB_ICONINFORMATION);
            }
            return 0;
        }
        if (msg == WM_COMMAND && LOWORD(wp) == 2) {
            BOOL width_ok = FALSE;
            BOOL height_ok = FALSE;
            const auto width = GetDlgItemInt(hwnd_, kPostWidthEdit, &width_ok, FALSE);
            const auto height = GetDlgItemInt(hwnd_, kPostHeightEdit, &height_ok, FALSE);
            if (!width_ok || !height_ok || width < 800 || height < 600 || width > 7680 || height > 4320) {
                MessageBoxW(hwnd_, L"分辨率必须是 800×600 到 7680×4320 之间的整数。", L"设置无效", MB_ICONWARNING);
                return 0;
            }
            settings_.postWidth = static_cast<int>(width);
            settings_.postHeight = static_cast<int>(height);
            settings_.borderless = SendDlgItemMessageW(
                hwnd_, kBorderlessCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            settings_.vsync = SendDlgItemMessageW(
                hwnd_, kVsyncCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            BOOL bgm_ok = FALSE;
            BOOL sfx_ok = FALSE;
            const auto bgm = GetDlgItemInt(hwnd_, kBgmVolumeEdit, &bgm_ok, FALSE);
            const auto sfx = GetDlgItemInt(hwnd_, kSfxVolumeEdit, &sfx_ok, FALSE);
            if (!bgm_ok || !sfx_ok || bgm > 100 || sfx > 100) {
                MessageBoxW(hwnd_, L"BGM/SFX 音量必须是 0 到 100 之间的整数。", L"设置无效", MB_ICONWARNING);
                return 0;
            }
            settings_.bgmVolume = static_cast<int>(bgm);
            settings_.sfxVolume = static_cast<int>(sfx);
            if (!saveSettings(settings_)) {
                MessageBoxW(hwnd_, L"设置保存失败：无法写入用户配置。为避免启动后设置丢失，客户端未启动。", L"启动失败", MB_ICONERROR);
                return 0;
            }
            launchClient();
            return 0;
        }
        if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
        return DefWindowProcW(hwnd_, msg, wp, 0);
    }
    void launchClient() {
        wchar_t module[MAX_PATH]{}; GetModuleFileNameW(nullptr, module, MAX_PATH);
        const fs::path bin = fs::path(module).parent_path();
        // Release packages may rename the binary to MoxianClient.exe while
        // the modern build emits mxh_client.exe.  Probe only these two
        // explicit names; never search arbitrary executables or accept a
        // user-controlled command line.
        fs::path client = bin / L"MoxianClient.exe";
        if (!fs::is_regular_file(client)) client = bin / L"mxh_client.exe";
        if (!fs::is_regular_file(client)) {
            MessageBoxW(hwnd_, L"未找到 MoxianClient.exe 或 mxh_client.exe，请先完成客户端安装。", L"启动失败", MB_ICONERROR);
            return;
        }
        // A launcher must fail closed when the installed profile is
        // incomplete.  Starting with a missing IDDlg or Map.pak only
        // produces a misleading black/error screen in the client.
        const fs::path resourceRoot = resourceRoot_.empty()
            ? locateResourceRoot(fs::path(module)) : resourceRoot_;
        if (resourceRoot.empty()) {
            MessageBoxW(hwnd_, L"playdh-current 资源不完整：缺少 Map10、怪物、NPC、登录、选角、建角或音频资源。请先检查/修复资源。", L"启动失败", MB_ICONERROR);
            return;
        }
        std::wstring hashError;
        if (!verifyRequiredResourceHashes(resourceRoot, hashError)) {
            MessageBoxW(hwnd_, (L"playdh-current 资源校验失败：\n" + hashError).c_str(), L"启动失败", MB_ICONERROR);
            return;
        }
        std::wstring command = quoteWindowsArg(client.wstring()) + L" --resource-profile playdh-current --login-width 800 --login-height 600 --post-width " + std::to_wstring(settings_.postWidth) + L" --post-height " + std::to_wstring(settings_.postHeight) +
            L" --login-port " + std::to_wstring(endpoints_.loginPort) +
            L" --agent-port " + std::to_wstring(endpoints_.agentPort) +
            L" --map-port " + std::to_wstring(endpoints_.mapPort);
        if (settings_.borderless) command += L" --borderless";
        if (!settings_.vsync) command += L" --no-vsync";
        command += L" --resource-root " + quoteWindowsArg(resourceRoot.wstring());
        if (!evidenceDir_.empty()) {
            command += L" --evidence-dir " + quoteWindowsArg(evidenceDir_.wstring());
        }
        STARTUPINFOW si{sizeof(si)}; PROCESS_INFORMATION pi{}; std::vector<wchar_t> mutableCommand(command.begin(), command.end()); mutableCommand.push_back(L'\0');
        if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, 0, nullptr, client.parent_path().c_str(), &si, &pi)) MessageBoxW(hwnd_, L"无法启动客户端，请先完成客户端安装。", L"启动失败", MB_ICONERROR); else { CloseHandle(pi.hThread); CloseHandle(pi.hProcess); }
    }
    HWND hwnd_{}; LauncherSettings settings_{}; LauncherEndpoints endpoints_{};
    fs::path resourceRoot_{};
    fs::path evidenceDir_{};
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    LauncherEndpoints endpoints;
    fs::path resourceRoot;
    fs::path evidenceDir;
    bool verifyOnly = false;
    bool invalidEndpoint = false;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        const auto nextValue = [&](int& index) -> std::optional<std::wstring_view> {
            if (index + 1 >= argc || argv[index + 1][0] == L'-') return std::nullopt;
            ++index;
            return std::wstring_view(argv[index]);
        };
        for (int i = 1; i < argc; ++i) {
            const auto name = std::wstring_view(argv[i]);
            if (name == L"--login-port") { bool valid = false; const auto value = nextValue(i); endpoints.loginPort = value ? parsePort(value->data(), endpoints.loginPort, &valid) : endpoints.loginPort; invalidEndpoint |= !valid; }
            else if (name == L"--agent-port") { bool valid = false; const auto value = nextValue(i); endpoints.agentPort = value ? parsePort(value->data(), endpoints.agentPort, &valid) : endpoints.agentPort; invalidEndpoint |= !valid; }
            else if (name == L"--map-port") { bool valid = false; const auto value = nextValue(i); endpoints.mapPort = value ? parsePort(value->data(), endpoints.mapPort, &valid) : endpoints.mapPort; invalidEndpoint |= !valid; }
            else if (name == L"--resource-root") { const auto value = nextValue(i); if (value) resourceRoot = fs::path(*value); else invalidEndpoint = true; }
            else if (name == L"--evidence-dir") { const auto value = nextValue(i); if (value) evidenceDir = fs::path(*value); else invalidEndpoint = true; }
            else if (name == L"--verify-resources") verifyOnly = true;
        }
        LocalFree(argv);
    }
    if (invalidEndpoint) {
        MessageBoxW(nullptr, L"服务端口必须是 1 到 65535 之间的整数。", L"启动器配置错误", MB_ICONERROR);
        return 2;
    }
    if (endpoints.loginPort == endpoints.agentPort ||
        endpoints.loginPort == endpoints.mapPort ||
        endpoints.agentPort == endpoints.mapPort) {
        MessageBoxW(nullptr, L"登录、角色和地图服务端口必须互不相同。", L"启动器配置错误", MB_ICONERROR);
        return 2;
    }
    if (verifyOnly) {
        if (resourceRoot.empty()) {
            wchar_t module[MAX_PATH]{};
            GetModuleFileNameW(nullptr, module, MAX_PATH);
            resourceRoot = locateResourceRoot(fs::path(module));
        }
        std::wstring error;
        const bool ok = !resourceRoot.empty() &&
            verifyRequiredResourceHashes(resourceRoot, error);
        if (!ok) {
            OutputDebugStringW((L"MoxianLauncher resource verification failed: " +
                                error + L"\n").c_str());
            return 3;
        }
        OutputDebugStringW(L"MoxianLauncher resource verification passed\n");
        return 0;
    }
    LauncherWindow window(endpoints, std::move(resourceRoot), std::move(evidenceDir));
    return window.create(instance) ? window.run() : 1;
}
