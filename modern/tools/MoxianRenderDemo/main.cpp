// MoxianRenderDemo: minimal smoke-test for mxh_render.
// Opens a window, creates the DX11 renderer, clears to a color, draws a sprite,
// then keeps the window responsive until the user closes it or N seconds pass.
#include <windows.h>

#include <cstdio>
#include <cstdlib>

#include "mxh/render/IRenderer.hpp"
#include "mxh/render/IFileStorage.hpp"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace mxh::gx;

// Minimal stub I4DyuchiFileStorage — Phase 5 demo doesn't actually load files.
class StubFileStorage : public I4DyuchiFileStorage {
public:
    StubFileStorage() {}
    virtual ~StubFileStorage() {}

    // IUnknown
    ULONG refCount_ = 1;
    STDMETHODIMP QueryInterface(REFIID, void**) override { return E_NOINTERFACE; }
    STDMETHODIMP_(ULONG) AddRef() override { return ++refCount_; }
    STDMETHODIMP_(ULONG) Release() override { ULONG r = --refCount_; if (r==0) delete this; return r; }

    BOOL __stdcall Initialize(std::uint32_t, std::uint32_t, std::uint32_t, FILE_ACCESS_METHOD) override { return TRUE; }
    void* __stdcall MapPackFile(char*) override { return nullptr; }
    void __stdcall UnmapPackFile(void*) override {}
    std::uint32_t __stdcall GetFileNum(void*) override { return 0; }
    std::uint32_t __stdcall CreateFileInfoList(void*, FSFILE_ATOM_INFO**, std::uint32_t) override { return 0; }
    void __stdcall DeleteFileInfoList(void*, FSFILE_ATOM_INFO*) override {}
    BOOL __stdcall IsExistInFileStorage(char*) override { return FALSE; }
    BOOL __stdcall LockPackFile(void*, std::uint32_t) override { return TRUE; }
    BOOL __stdcall InsertFileToPackFile(void*, char*) override { return FALSE; }
    BOOL __stdcall DeleteFileFromPackFile(char*) override { return FALSE; }
    BOOL __stdcall UnlockPackFile(void*, LOAD_CALLBACK_FUNC) override { return TRUE; }
    BOOL __stdcall ExtractFile(char*) override { return FALSE; }
    BOOL __stdcall ExtractAllFiles() override { return FALSE; }
    std::uint32_t __stdcall ExtractAllFilesFromPackFile(void*) override { return 0; }
    void* __stdcall FSOpenFile(char*, std::uint32_t) override { return nullptr; }
    int __stdcall FSScanf(void*, char*, ...) override { return 0; }
    std::uint32_t __stdcall FSRead(void*, void*, std::uint32_t) override { return 0; }
    std::uint32_t __stdcall FSSeek(void*, std::uint32_t, FSFILE_SEEK) override { return 0; }
    BOOL __stdcall FSCloseFile(void*) override { return TRUE; }
    BOOL __stdcall GetPackFileInfo(void*, FSPACK_FILE_INFO*) override { return FALSE; }
    BOOL __stdcall BeginLogging(char*, std::uint32_t) override { return FALSE; }
    BOOL __stdcall EndLogging() override { return FALSE; }
};

// Window proc.
static I4DyuchiGXRenderer* g_renderer = nullptr;
static bool g_running = true;

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CLOSE:
    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        return 0;
    case WM_PAINT: {
        if (g_renderer) {
            // 1. Clear to dark blue.
            g_renderer->BeginRender(nullptr, 0xff202080, 0);

            // 2. Draw a red square in the middle.
            VECTOR2 trans{ 320.0f, 240.0f };
            VECTOR2 scale{ 200.0f, 200.0f };
            // Phase 5: CreateEmptySpriteObject needs an actual texture; for now
            // we just exercise the box primitive to validate the pipeline.
            VECTOR3 oct[8] = {
                { 100, 0, 100 }, { 540, 0, 100 }, { 540, 0, 380 }, { 100, 0, 380 },
                { 100, 1, 100 }, { 540, 1, 100 }, { 540, 1, 380 }, { 100, 1, 380 },
            };
            g_renderer->RenderBox(oct, 0xff00ff00);

            g_renderer->EndRender();
            g_renderer->Present(h);
        }
        return 0;
    }
    default:
        return DefWindowProc(h, m, w, l);
    }
}

int main(int argc, char** argv) {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    WNDCLASSW wc{};
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"MoxianRenderDemoWnd";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    if (!RegisterClassW(&wc)) {
        std::fprintf(stderr, "RegisterClass failed (err=%lu)\n", GetLastError());
        return 1;
    }

    HWND hwnd = CreateWindowW(L"MoxianRenderDemoWnd", L"Moxian Render Demo",
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                               CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                               nullptr, nullptr, hInst, nullptr);
    if (!hwnd) {
        std::fprintf(stderr, "CreateWindow failed\n");
        return 1;
    }

    StubFileStorage* storage = new StubFileStorage();

    I4DyuchiGXRenderer* renderer = nullptr;
    CreateGXRendererInstance(reinterpret_cast<void**>(&renderer));

    DISPLAY_INFO info{};
    info.dispType     = WINDOW_WITH_BLT;
    info.dwWidth      = 800;
    info.dwHeight     = 600;
    info.dwBPS        = 32;
    info.dwRefreshRate = 60;

    if (!renderer->Create(hwnd, &info, storage, nullptr)) {
        std::fprintf(stderr, "renderer->Create failed\n");
        return 1;
    }
    g_renderer = renderer;

    MSG msg{};
    int  frame = 0;
    auto startTick = GetTickCount();
    while (g_running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) g_running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Trigger a repaint each ~16 ms.
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
        Sleep(16);
        if (++frame > 600) g_running = false;  // ~10 s cap
    }

    renderer->Release();
    storage->Release();
    std::printf("Demo ran for %lu ms, %d frames.\n", GetTickCount() - startTick, frame);
    return 0;
}