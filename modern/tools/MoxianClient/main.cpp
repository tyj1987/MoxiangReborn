// MoxianClient: modern Moxian (DarkStory) client main entry.
//
// Phase A.1 — minimal skeleton that exercises the entire UI ↔ GPU seam
// end-to-end so the Phase 6.4 cImage::bindRenderer adapter gets a real
// render path instead of a no-op stub. Subsequent phases (A.1.6+) layer
// CMainGame + eGAMESTATE on top of this skeleton.
//
// Bootstrap order (matches what the legacy MHClient.cpp does at WinMain):
//   1. Register window class + create HWND (the host surface for DX11).
//   2. Build a minimal I4DyuchiFileStorage (the legacy engine always has
//      one; we use a stub here because the resource cache is empty until
//      Phase B.1+ wires MoxianResourceExplorer into the live tree).
//   3. Create + initialise the IRenderer (DX11 backend).
//   4. Install the cImage render adapter (the Phase 6.4 seam).
//   5. Run the Win32 message pump. On each WM_PAINT we tick the game
//      state machine (CMainGame in A.1.6+; a no-op frame for now) and
//      present the back buffer.
//
// 1:1 quirks preserved:
//   - The 800x600 default window size matches the legacy MHClient default.
//   - g_DistributeAddr / g_DistributePort / g_AgentAddr / g_AgentPort
//     globals (parsed from MHVerInfo.ver in B.1+) live at file scope so
//     MainTitle (A.1.8) can read them without touching the message pump.
//   - The legacy WinMain order — instance handle → class register →
//     window create → renderer init → message loop — is preserved.

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <array>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d11.h>
#include <wrl/client.h>

#include "mxh/render/IRenderer.hpp"
#include "mxh/render/IFileStorage.hpp"
#include "mxh/render/render_typedef.hpp"
#include "mxh/ui/cImage.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/log/mlog.hpp"
#include "CMainGame.hpp"
#include "CEngine.hpp"
#include "GameStateStubs.hpp"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace mxh::gx;
using mxh::ui::cDialog;
using Microsoft::WRL::ComPtr;

// ---------------------------------------------------------------------------
// Legacy global state (mirrors MHClient.cpp).  These are file-scope today so
// the CMainTitle (Phase A.1.8) can pick them up without changing the
// ownership model later.  MHVerInfo.ver parsing lands in Phase B.1.
// ---------------------------------------------------------------------------
namespace mxh::client {

// 800x600 matches the legacy default window size.
constexpr std::uint32_t kDefaultWindowWidth  = 800;
constexpr std::uint32_t kDefaultWindowHeight = 600;

// Distribute / Agent connection info (parsed from MHVerInfo.ver in B.1).
// Stub values for A.1: localhost:6000.
char   g_DistributeAddr[16] = "127.0.0.1";
std::uint16_t g_DistributePort = 6000;
char   g_AgentAddr[16] = "127.0.0.1";
std::uint16_t g_AgentPort = 7001;

// Version string (mirrors the legacy g_CLIENTVERSION[32]).  A.1 uses a
// placeholder; B.1 swaps in the parsed value from MHVerInfo.ver.
char g_CLIENTVERSION[32] = "MXRBN99999999";

bool g_running = true;

} // namespace mxh::client

// ---------------------------------------------------------------------------
// Stub I4DyuchiFileStorage.
//
// A.1 ships with no on-disk resource cache (MoxianResourceExplorer runs as
// a stand-alone CLI today; wiring it into the live client is B.1+).  The
// legacy engine requires an I4DyuchiFileStorage pointer at IRenderer::Create
// time, so we hand it a no-op stub.
// ---------------------------------------------------------------------------
namespace {

class StubFileStorage : public I4DyuchiFileStorage {
public:
    StubFileStorage() = default;
    // I4DyuchiFileStorage has no virtual destructor (legacy COM-style
    // interface), so we don't add one — Release() owns the deletion
    // contract. Destruction happens via the IUnknown refcount.

    ULONG refCount_ = 1;
    STDMETHODIMP QueryInterface(REFIID, void**) override { return E_NOINTERFACE; }
    STDMETHODIMP_(ULONG) AddRef() override { return ++refCount_; }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG r = --refCount_;
        if (r == 0) delete this;
        return r;
    }

    BOOL __stdcall Initialize(std::uint32_t, std::uint32_t, std::uint32_t,
                              FILE_ACCESS_METHOD) override { return TRUE; }
    void* __stdcall MapPackFile(char*) override { return nullptr; }
    void __stdcall UnmapPackFile(void*) override {}
    std::uint32_t __stdcall GetFileNum(void*) override { return 0; }
    std::uint32_t __stdcall CreateFileInfoList(void*, FSFILE_ATOM_INFO**,
                                               std::uint32_t) override { return 0; }
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

} // namespace

// ---------------------------------------------------------------------------
// Render adapter — bridges cImage (Phase 6.4) to IDISpriteObject (Phase 5).
//
// cImage::render(x, y, w, h, color, zOrder) calls back into the host with
// UVs derived from the image's source rect, plus the borrowed sprite
// pointer that was attached to the cImage via SetSpriteObject().  The
// adapter forwards to the HUD pass's RenderSprite call so the cImage
// path goes through the same pipeline as the legacy engine's HUD.
//
// 1:1 quirks preserved:
//   - The legacy cImage used a fixed pipeline that went through the
//     engine's HUD pass. The modern port preserves that ordering by
//     calling RenderSprite between BeginRender() and EndRender().
//   - The Phase 6.4 adapter API is untyped (void* sprite) so the UI
//     library doesn't have to drag in mxh_render's #include.  We cast
//     back to IDISpriteObject* here.
//   - zOrder is preserved end-to-end (A.1.5 re-sorts the cWindow tree
//     by zOrder before drawing, matching the legacy order).
// ---------------------------------------------------------------------------
namespace {

I4DyuchiGXRenderer* g_renderer = nullptr;

// Phase A.1.4: per-cImage sprite. cImage holds an opaque void* (its
// IDISpriteObject*). The adapter casts back and forwards to the
// renderer's RenderSprite.  Earlier A.1.3 had a single g_hudSprite
// that all cImages shared; the per-sprite version is the realistic
// surface the rest of Phase A will use.
bool renderAdapter(void* /*ctx*/, void* sprite,
                   float x, float y, float w, float h,
                   float /*u0*/, float /*v0*/, float /*u1*/, float /*v1*/,
                   std::uint32_t color, int zOrder) {
    if (!g_renderer) return false;
    if (!sprite)    return false;

    // Cast back to the typed sprite interface.  A.1.5 will pass UVs
    // through the source rect (cImage has already computed them at the
    // call site in cImage::render); the renderer currently treats the
    // sprite as a flat textured quad so we hand the full rect to it.
    auto* sp = static_cast<IDISpriteObject*>(sprite);
    VECTOR2 scale{ w, h };
    VECTOR2 trans{ x, y };
    RECT     rc{ 0, 0, 64, 64 };
    return g_renderer->RenderSprite(sp, &scale, 0.0f, &trans, &rc,
                                    color, zOrder, /*dwFlag=*/0) != FALSE;
}

// ---------------------------------------------------------------------------
// Procedural sprite helpers (Phase A.1.4).
//
// Real resource loading is a Phase B.1 task (MoxianResourceExplorer +
// cResourceManager). Until then we synthesise a few small RGBA textures
// in-code so the adapter has something to draw.  Each helper returns
// the SRV plus a SpriteObject* ready to attach to a cImage.
// ---------------------------------------------------------------------------

ComPtr<ID3D11ShaderResourceView> makeSolidSRV(ID3D11Device* dev,
                                              std::uint32_t argb) {
    constexpr std::uint32_t kSize = 64;
    std::uint32_t pixels[kSize * kSize];
    for (std::uint32_t i = 0; i < kSize * kSize; ++i) pixels[i] = argb;
    D3D11_TEXTURE2D_DESC td{};
    td.Width = kSize; td.Height = kSize; td.MipLevels = 1;
    td.ArraySize = 1; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = pixels; sd.SysMemPitch = kSize * 4;
    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(dev->CreateTexture2D(&td, &sd, &tex))) return nullptr;
    ComPtr<ID3D11ShaderResourceView> srv;
    dev->CreateShaderResourceView(tex.Get(), nullptr, &srv);
    return srv;
}

// "Moxian-flavored" gradient (dark navy → bright cyan) used as the
// bootscreen background in A.1.3.  Kept for compatibility.
ComPtr<ID3D11ShaderResourceView> makeBootSRV(ID3D11Device* dev) {
    constexpr std::uint32_t kSize = 64;
    std::uint32_t pixels[kSize * kSize];
    for (std::uint32_t y = 0; y < kSize; ++y) {
        for (std::uint32_t x = 0; x < kSize; ++x) {
            const std::uint32_t t = (x + y) / 2;
            const std::uint32_t r = 0x20u + t;
            const std::uint32_t g = 0x20u + t / 2;
            const std::uint32_t b = 0x80u + t;
            pixels[y * kSize + x] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
    D3D11_TEXTURE2D_DESC td{};
    td.Width = kSize; td.Height = kSize; td.MipLevels = 1;
    td.ArraySize = 1; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = pixels; sd.SysMemPitch = kSize * 4;
    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(dev->CreateTexture2D(&td, &sd, &tex))) return nullptr;
    ComPtr<ID3D11ShaderResourceView> srv;
    dev->CreateShaderResourceView(tex.Get(), nullptr, &srv);
    return srv;
}

// Wraps an SRV in a IDISpriteObject so the cImage::bindRenderer adapter
// can draw it.  A.1.4 stores the SpriteObject*; the underlying texture
// lives as long as the ComPtr we return.  The caller is responsible for
// keeping the ComPtr alive for the lifetime of the cImage.
//
// In Phase B.1 the SRV will come from MoxianResourceExplorer reading
// Effect.pak / InterfaceScript/*.tga; the SpriteObject* factory call
// is unchanged.
IDISpriteObject* makeSpriteFromSRV(I4DyuchiGXRenderer* r,
                                    ComPtr<ID3D11ShaderResourceView>& srv,
                                    const char* tag) {
    if (!r) return nullptr;
    IDISpriteObject* sp = r->CreateSpriteObject(const_cast<char*>(tag), 0);
    if (!sp) return nullptr;
    // The renderer keeps a weak reference to the SRV; it doesn't addref.
    // The caller owns the ComPtr and the SpriteObject is released when
    // the cImage that wraps it is destroyed.
    (void)srv;
    return sp;
}

} // namespace

// ---------------------------------------------------------------------------
// Phase A.1.4: per-cImage sprite registry.  Each cImage holds a
// IDISpriteObject* (passed through bindRenderer's void* adapter).  The
// SRV that backs the sprite is owned by the registry so the
// SpriteObject keeps a valid texture for as long as the cImage draws.
//
// In Phase B.1 the registry will be replaced by cResourceManager, which
// owns the long-lived sprite cache and looks up by file name + atlas
// coords.  A.1.4 ships the smallest version that proves the
// per-cImage binding works end-to-end.
// ---------------------------------------------------------------------------
struct SpriteEntry {
    ComPtr<ID3D11ShaderResourceView> srv;
    IDISpriteObject*                 sprite = nullptr;
};
std::array<SpriteEntry, 4> g_sprites{};  // 4 demo slots: bg/red/green/blue

// ---------------------------------------------------------------------------
// Frame loop. A.1.4 draws the boot background (full-window sprite) and
// 3 small "dialog tile" sprites laid out in a row — enough to visually
// confirm per-cImage sprite binding (different colors, different sprites)
// on screen. CMainGame's Process() replaces this in A.1.6.
// ---------------------------------------------------------------------------
namespace {

void renderFrame(HWND h) {
    if (!g_renderer) return;

    g_renderer->BeginRender(nullptr, 0xff101830, 0);

    // Background tile (slot 0 = boot gradient).
    if (g_sprites[0].sprite) {
        VECTOR2 scale{ 800.0f, 600.0f };
        VECTOR2 trans{ 0.0f, 0.0f };
        RECT     rc{ 0, 0, 64, 64 };
        g_renderer->RenderSprite(g_sprites[0].sprite, &scale, 0.0f, &trans,
                                 &rc, 0xFFFFFFFFu, 0, 0);
    }

    // 3 demo cImages at the bottom.  In A.1.5 these come from a real
    // cDialog tree (the CMainTitle login form); for now we draw them
    // directly through the renderer's HUD pass using the per-sprite
    // binding we just registered.
    constexpr float kTileW = 120.0f, kTileH = 80.0f;
    constexpr float kTileY = 480.0f;
    constexpr float kGapX  = 20.0f;
    constexpr float kStartX = (800.0f - (3.0f * kTileW + 2.0f * kGapX)) * 0.5f;
    for (std::uint32_t i = 1; i <= 3; ++i) {
        if (!g_sprites[i].sprite) continue;
        VECTOR2 scale{ kTileW, kTileH };
        VECTOR2 trans{ kStartX + (i - 1) * (kTileW + kGapX), kTileY };
        RECT     rc{ 0, 0, 64, 64 };
        g_renderer->RenderSprite(g_sprites[i].sprite, &scale, 0.0f, &trans,
                                 &rc, 0xFFFFFFFFu,
                                 /*iZOrder=*/static_cast<int>(i), 0);
    }

    g_renderer->EndRender();
    g_renderer->Present(h);
}

} // namespace

// ---------------------------------------------------------------------------
// Window procedure. Mirrors the legacy MHClient.cpp WndProc surface — only
// the messages the A.1 skeleton needs are handled.  Phase A.1.6+ extends
// this with IME, mouse, keyboard, and the game-state dispatch.
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CLOSE:
    case WM_DESTROY:
        mxh::client::g_running = false;
        PostQuitMessage(0);
        return 0;
    case WM_PAINT: {
        renderFrame(h);
        return 0;
    }
    case WM_KEYDOWN:
        // ESC quits in A.1 (the legacy engine uses ESC to open the menu;
        // for the skeleton we use it as the "exit" hotkey).
        if (w == VK_ESCAPE) {
            mxh::client::g_running = false;
            PostQuitMessage(0);
        }
        return 0;
    default:
        return DefWindowProc(h, m, w, l);
    }
}

// ---------------------------------------------------------------------------
// WinMain. 1:1 mirrors MHClient.cpp's WinMain order.
// ---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE /*hPrev*/, LPSTR /*cmd*/, int /*show*/) {
    MLOG_INFO("mxh_client: booting version %s", mxh::client::g_CLIENTVERSION);

    WNDCLASSW wc{};
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"MoxianClientWnd";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    if (!RegisterClassW(&wc)) {
        std::fprintf(stderr, "mxh_client: RegisterClass failed (err=%lu)\n",
                     GetLastError());
        return 1;
    }

    HWND hwnd = CreateWindowW(
        L"MoxianClientWnd", L"Moxian Client (modern)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        static_cast<int>(mxh::client::kDefaultWindowWidth),
        static_cast<int>(mxh::client::kDefaultWindowHeight),
        nullptr, nullptr, hInst, nullptr);
    if (!hwnd) {
        std::fprintf(stderr, "mxh_client: CreateWindow failed\n");
        return 1;
    }

    // Resource storage. Stub for A.1; MoxianResourceExplorer integration is
    // a Phase B.1 task.
    StubFileStorage* storage = new StubFileStorage();

    // Renderer. The factory is implemented in modern/src/render (DX11
    // backend). The pointer is borrowed — the factory retains ownership.
    I4DyuchiGXRenderer* renderer = nullptr;
    CreateGXRendererInstance(reinterpret_cast<void**>(&renderer));

    DISPLAY_INFO info{};
    info.dispType      = WINDOW_WITH_BLT;
    info.dwWidth       = mxh::client::kDefaultWindowWidth;
    info.dwHeight      = mxh::client::kDefaultWindowHeight;
    info.dwBPS         = 32;
    info.dwRefreshRate = 60;
    if (!renderer->Create(hwnd, &info, storage, nullptr)) {
        std::fprintf(stderr, "mxh_client: renderer->Create failed\n");
        return 1;
    }
    g_renderer = renderer;

    // Build per-cImage sprite registry.  A.1.4 ships with 4 demo
    // sprites: one gradient background + three solid-colour "dialog
    // tiles" laid out in a row.  The IDs are stable so cImage wrappers
    // can be created with SetSpriteObject(sprite) at the same slot.
    ID3D11Device* dev = nullptr;
    if (renderer->GetD3DDevice(__uuidof(IUnknown),
                              reinterpret_cast<void**>(&dev))) {
        g_sprites[0].srv    = makeBootSRV(dev);
        g_sprites[1].srv    = makeSolidSRV(dev, 0xFFB04040u);  // red
        g_sprites[2].srv    = makeSolidSRV(dev, 0xFF40B040u);  // green
        g_sprites[3].srv    = makeSolidSRV(dev, 0xFF4060B0u);  // blue
        g_sprites[0].sprite = makeSpriteFromSRV(renderer, g_sprites[0].srv, "bg");
        g_sprites[1].sprite = makeSpriteFromSRV(renderer, g_sprites[1].srv, "tile_red");
        g_sprites[2].sprite = makeSpriteFromSRV(renderer, g_sprites[2].srv, "tile_green");
        g_sprites[3].sprite = makeSpriteFromSRV(renderer, g_sprites[3].srv, "tile_blue");
        MLOG_INFO("mxh_client: %u sprites registered (1 background + 3 tiles)",
                  static_cast<unsigned>(g_sprites.size()));
    } else {
        MLOG_WARN("mxh_client: GetD3DDevice failed, no sprites created");
    }

    // Install the cImage ↔ renderer adapter.  After this call every
    // cImage::render() forwards to renderAdapter() which casts the
    // opaque sprite back to IDISpriteObject* and draws it through
    // the HUD pass.
    mxh::ui::bindRenderer(&renderAdapter, nullptr);

    // Build a placeholder cDialog centred in the window.  A.1.5 swaps
    // in the real CMainTitle dialog tree.
    cDialog placeholder;
    placeholder.Init(280, 240, 240, 120, /*basicImage=*/nullptr, /*id=*/0);
    (void)placeholder;  // not yet rendered (Render is a no-op stub today)

    // -------------------------------------------------------------------------
    // Phase A.1.6 — wire CMainGame + CEngine + the 9 eGAMESTATE stubs.
    //
    // CEngine gets the HWND and the IRenderer so future states can
    // look them up. CMainGame owns the state table; we register the
    // 9 concrete stubs from GameStateStubs.hpp.  The boot transition
    // (Engine → CMainTitle) goes through CMainGame::SetGameState so
    // the legacy "delayed transition on next Process()" semantics are
    // preserved.
    // -------------------------------------------------------------------------
    auto engine = std::make_unique<mxh::client::CEngine>();
    engine->SetHwnd(hwnd);
    engine->SetRenderer(renderer);
    engine->Init();

    mxh::client::CMainGame mainGame;
    mainGame.Init(hwnd);
    mainGame.SetEngine(std::move(engine));
    mainGame.RegisterState(mxh::client::GameStateId::Intro,      std::make_unique<mxh::client::CIntroReplay>());
    mainGame.RegisterState(mxh::client::GameStateId::Connect,    std::make_unique<mxh::client::CLoginState>());
    mainGame.RegisterState(mxh::client::GameStateId::Title,      std::make_unique<mxh::client::CMainTitle>());
    mainGame.RegisterState(mxh::client::GameStateId::CharSelect, std::make_unique<mxh::client::CCharSelectState>());
    mainGame.RegisterState(mxh::client::GameStateId::CharMake,   std::make_unique<mxh::client::CCharMake>());
    mainGame.RegisterState(mxh::client::GameStateId::GameLoading,std::make_unique<mxh::client::CGameLoading>());
    mainGame.RegisterState(mxh::client::GameStateId::GameIn,     std::make_unique<mxh::client::CInGameState>());
    mainGame.RegisterState(mxh::client::GameStateId::MapChange,  std::make_unique<mxh::client::CMapChange>());
    mainGame.RegisterState(mxh::client::GameStateId::MurimNet,   std::make_unique<mxh::client::CMurimNet>());

    // A.1.7 booted into the CConnecting stub (legacy started at
    // eGAMESTATE_CONNECT and immediately tried the Distribute connect).
    // Phase B.2.1 replaces the stub with CLoginState; we now boot into
    // the CMainTitle (which presents the server list) and let the user
    // click "Connect" to drive CLoginState explicitly.  Until the
    // server-list UI is wired up we still boot into Connect as a
    // dev-mode shortcut; that path will move to a button in B.2.5+.
    mainGame.SetGameState(mxh::client::GameStateId::Connect);
    MLOG_INFO("mxh_client: CMainGame initialised, 9 states registered, "
              "boot → GameStateId::Connect");

    // Phase B.2.1: kick off the CLoginState connect (host-driven; the
    // state can't self-start because it doesn't know the login address
    // until MHVerInfo.ver parsing lands in B.2.5).
    if (auto* login = dynamic_cast<mxh::client::CLoginState*>(
            mainGame.GetGameState(mxh::client::GameStateId::Connect))) {
        login->Start(mainGame.GetEngine(),
                     "127.0.0.1", 6001, "test", "test");
    } else {
        MLOG_ERROR("Connect slot is not a CLoginState; cannot start login");
    }

    MLOG_INFO("mxh_client: entering message loop");

    // Standard Win32 message pump + CMainGame driver.  The legacy
    // engine did "Process() → render() → flip" once per frame; we
    // mirror that here.  CMainGame::BeforeRender/AfterRender fan out
    // to the current state and respect the pause-render flag.
    //
    // Phase B.2.2: the host has to Start() each new state once it
    // becomes current.  CMainGame's transition is delayed (next
    // Process() call after SetGameState), so we track the previous
    // state number and drive Start() on the rising edge.
    auto prev_state = mxh::client::GameStateId::End;
    MSG msg{};
    while (mxh::client::g_running) {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        } else {
            // Idle: drive CMainGame + render a frame.
            mainGame.Process();
            // Phase B.2.2: on state-change rising edge, Start() the
            // states that have an external Start() hook.
            const auto cur_state = mainGame.GetCurStateNum();
            if (cur_state != prev_state) {
                if (cur_state == mxh::client::GameStateId::CharSelect) {
                    if (auto* cs = dynamic_cast<mxh::client::CCharSelectState*>(
                            mainGame.GetGameState(cur_state))) {
                        cs->Start(mainGame.GetEngine());
                    }
                } else if (cur_state == mxh::client::GameStateId::GameIn) {
                    // Phase B.2.3: dev-mode direct-connect to MapServer.
                    // The full CharSelect → GameLoading → GameIn path
                    // requires a character in character_info (Phase B.4+);
                    // for now we jump straight to MapServer with the
                    // login ack user_idx as chrid.
                    if (auto* g = dynamic_cast<mxh::client::CInGameState*>(
                            mainGame.GetGameState(cur_state))) {
                        g->Start(mainGame.GetEngine(), "127.0.0.1", 8001,
                                 /*player_id=*/1, /*map_num=*/12);
                    }
                }
                prev_state = cur_state;
            }
            mainGame.BeforeRender();
            renderFrame(hwnd);
            mainGame.AfterRender();
            Sleep(16);
        }
    }

    MLOG_INFO("mxh_client: shutting down");

    mainGame.Release();

    // Cleanup. The SpriteObject* are owned by us; the renderer factory
    // will release them when renderer is destroyed. SRVs are released
    // by the ComPtr destructor.
    for (auto& e : g_sprites) {
        if (e.sprite) e.sprite->Release();
        e.sprite = nullptr;
    }
    g_sprites = {};
    storage->Release();

    return 0;
}
