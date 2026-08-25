#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
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
    return root / L"Moxian" / L"launcher-settings.txt";
}

static LauncherSettings loadSettings() {
    LauncherSettings s;
    std::wifstream in(settingsPath());
    std::wstring key, value;
    while (in >> key >> value) {
        if (key == L"profile") s.profile = value;
        else if (key == L"post_width") s.postWidth = _wtoi(value.c_str());
        else if (key == L"post_height") s.postHeight = _wtoi(value.c_str());
        else if (key == L"borderless") s.borderless = value == L"1";
    }
    if (s.profile != L"playdh-current") s.profile = L"playdh-current";
    if (s.postWidth < 640 || s.postHeight < 480) { s.postWidth = 1024; s.postHeight = 768; }
    return s;
}

static void saveSettings(const LauncherSettings& s) {
    std::error_code ec;
    fs::create_directories(settingsPath().parent_path(), ec);
    fs::path temp = settingsPath(); temp += L".tmp";
    std::wofstream out(temp, std::ios::trunc);
    if (!out) return;
    out << L"profile " << s.profile << L"\npost_width " << s.postWidth
        << L"\npost_height " << s.postHeight << L"\nborderless " << (s.borderless ? 1 : 0) << L"\n";
    out.close();
    MoveFileExW(temp.c_str(), settingsPath().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
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
            CreateWindowW(L"STATIC", L"登录窗口 800×600；登录成功后切换为 1024×768。", WS_CHILD | WS_VISIBLE, 20, 140, 430, 30, hwnd_, nullptr, nullptr, nullptr);
            return 0;
        }
        if (msg == WM_COMMAND && LOWORD(wp) == 1) { MessageBoxW(hwnd_, L"未配置经过签名校验的补丁 manifest。为避免不安全更新，检查/修复已安全阻止。", L"检查/修复", MB_ICONWARNING); return 0; }
        if (msg == WM_COMMAND && LOWORD(wp) == 2) { saveSettings(settings_); launchClient(); return 0; }
        if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
        return DefWindowProcW(hwnd_, msg, wp, 0);
    }
    void launchClient() {
        wchar_t module[MAX_PATH]{}; GetModuleFileNameW(nullptr, module, MAX_PATH);
        fs::path client = fs::path(module).parent_path() / L"MoxianClient.exe";
        std::wstring command = L"\"" + client.wstring() + L"\" --resource-profile playdh-current --login-width 800 --login-height 600 --post-width " + std::to_wstring(settings_.postWidth) + L" --post-height " + std::to_wstring(settings_.postHeight);
        STARTUPINFOW si{sizeof(si)}; PROCESS_INFORMATION pi{}; std::vector<wchar_t> mutableCommand(command.begin(), command.end()); mutableCommand.push_back(L'\0');
        if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, 0, nullptr, client.parent_path().c_str(), &si, &pi)) MessageBoxW(hwnd_, L"无法启动客户端，请先完成客户端安装。", L"启动失败", MB_ICONERROR); else { CloseHandle(pi.hThread); CloseHandle(pi.hProcess); }
    }
    HWND hwnd_{}; LauncherSettings settings_{};
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) { LauncherWindow window; return window.create(instance) ? window.run() : 1; }
