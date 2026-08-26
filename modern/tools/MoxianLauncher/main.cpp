#include <windows.h>
#include <shellapi.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct LauncherSettings {
    std::wstring profile = L"playdh-current";
    int postWidth = 1024;
    int postHeight = 768;
    bool borderless = false;
};

static fs::path settingsPath() {
    wchar_t buffer[MAX_PATH]{};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    fs::path root = n ? fs::path(buffer) : fs::temp_directory_path();
    return root / L"Moxian" / L"settings.json";
}

static LauncherSettings loadSettings() {
    LauncherSettings s;
    std::ifstream in(settingsPath(), std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(in)), {});
    std::smatch match;
    if (std::regex_search(text, match, std::regex(R"REGEX("resourceProfileId"\s*:\s*"([^"]+)")REGEX")) ||
        std::regex_search(text, match, std::regex(R"REGEX("profile"\s*:\s*"([^"]+)")REGEX")))
        s.profile = std::wstring(match[1].str().begin(), match[1].str().end());
    if (std::regex_search(text, match, std::regex(R"REGEX("postLoginWidth"\s*:\s*(\d+))REGEX"))) s.postWidth = std::stoi(match[1].str());
    if (std::regex_search(text, match, std::regex(R"REGEX("postLoginHeight"\s*:\s*(\d+))REGEX"))) s.postHeight = std::stoi(match[1].str());
    if (std::regex_search(text, match, std::regex(R"REGEX("borderless"\s*:\s*(true|false))REGEX"))) s.borderless = match[1].str() == "true";
    if (s.profile != L"playdh-current") s.profile = L"playdh-current";
    if (s.postWidth < 640 || s.postHeight < 480) { s.postWidth = 1024; s.postHeight = 768; }
    return s;
}

static void saveSettings(const LauncherSettings& s) {
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
    replace_or_insert("resourceProfileId", "\"" + profile + "\"");
    replace_or_insert("postLoginWidth", std::to_string(s.postWidth));
    replace_or_insert("postLoginHeight", std::to_string(s.postHeight));
    replace_or_insert("borderless", s.borderless ? "true" : "false");
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    if (!out) return;
    out << text;
    out.close();
    MoveFileExW(temp.c_str(), settingsPath().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}

static fs::path locateResourceRoot(const fs::path& executable) {
    const fs::path bin = executable.parent_path();
    const fs::path roots[] = {
        bin / L"data" / L"PlayDH",
        bin.parent_path() / L"data" / L"PlayDH",
        bin.parent_path() / L"modern" / L"data" / L"PlayDH"
    };
    for (const auto& root : roots) {
        if (fs::is_regular_file(root / L"Map.pak") &&
            fs::is_regular_file(root / L"Image" / L"InterfaceScript" / L"IDDlg.bin")) {
            return root;
        }
    }
    return {};
}

class LauncherWindow {
public:
    bool create(HINSTANCE instance) {
        settings_ = loadSettings();
        WNDCLASSW wc{}; wc.hInstance = instance; wc.lpfnWndProc = &LauncherWindow::proc;
        wc.lpszClassName = L"MoxianLauncherWindow"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        RegisterClassW(&wc);
        hwnd_ = CreateWindowW(wc.lpszClassName, L"墨香启动器", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                              CW_USEDEFAULT, CW_USEDEFAULT, 480, 250, nullptr, nullptr, instance, this);
        return hwnd_ != nullptr;
    }
    int run() { ShowWindow(hwnd_, SW_SHOW); UpdateWindow(hwnd_); MSG msg{}; while (GetMessageW(&msg, nullptr, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); } return static_cast<int>(msg.wParam); }
private:
    static constexpr int kPostWidthEdit = 10;
    static constexpr int kPostHeightEdit = 11;
    static constexpr int kBorderlessCheck = 12;
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
            return 0;
        }
        if (msg == WM_COMMAND && LOWORD(wp) == 1) {
            wchar_t module[MAX_PATH]{};
            GetModuleFileNameW(nullptr, module, MAX_PATH);
            const auto root = locateResourceRoot(fs::path(module));
            if (root.empty()) {
                MessageBoxW(hwnd_, L"资源检查失败：缺少 Map.pak 或 Image\\InterfaceScript\\IDDlg.bin。未执行任何删除或下载。", L"检查/修复", MB_ICONERROR);
            } else {
                MessageBoxW(hwnd_, (L"本地资源基础文件检查通过：\n" + root.wstring()).c_str(), L"检查/修复", MB_ICONINFORMATION);
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
            saveSettings(settings_);
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
        const fs::path resourceRoot = locateResourceRoot(fs::path(module));
        if (resourceRoot.empty()) {
            MessageBoxW(hwnd_, L"playdh-current 资源不完整：缺少 Map.pak 或 Image\\InterfaceScript\\IDDlg.bin。请先检查/修复资源。", L"启动失败", MB_ICONERROR);
            return;
        }
        std::wstring command = L"\"" + client.wstring() + L"\" --resource-profile playdh-current --login-width 800 --login-height 600 --post-width " + std::to_wstring(settings_.postWidth) + L" --post-height " + std::to_wstring(settings_.postHeight);
        if (settings_.borderless) command += L" --borderless";
        command += L" --resource-root \"" + resourceRoot.wstring() + L"\"";
        STARTUPINFOW si{sizeof(si)}; PROCESS_INFORMATION pi{}; std::vector<wchar_t> mutableCommand(command.begin(), command.end()); mutableCommand.push_back(L'\0');
        if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, 0, nullptr, client.parent_path().c_str(), &si, &pi)) MessageBoxW(hwnd_, L"无法启动客户端，请先完成客户端安装。", L"启动失败", MB_ICONERROR); else { CloseHandle(pi.hThread); CloseHandle(pi.hProcess); }
    }
    HWND hwnd_{}; LauncherSettings settings_{};
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) { LauncherWindow window; return window.create(instance) ? window.run() : 1; }
