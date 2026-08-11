// MoxianClient: modern Moxian (DarkStory) client main entry.
//
// Phase A.1 â€” minimal skeleton that exercises the entire UI â†” GPU seam
// end-to-end so the Phase 6.4 cImage::bindRenderer adapter gets a real
// render path instead of a no-op stub. Subsequent phases (A.1.6+) layer
// CMainGame + eGAMESTATE on top of this skeleton.
//
// Bootstrap order (matches what the legacy MHClient.cpp does at WinMain):
//   1. Register window class + create HWND (the host surface for DX11).
//   2. Mount the original PlayDH resource tree through I4DyuchiFileStorage.
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
//   - The legacy WinMain order â€” instance handle â†’ class register â†’
//     window create â†’ renderer init â†’ message loop â€” is preserved.

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <array>
#include <string>
#include <filesystem>
#include <memory>

#include <windows.h>
#include <shellapi.h>

#include <d3d11.h>
#include <wrl/client.h>

#include "mxh/render/IRenderer.hpp"
#include "mxh/render/IFileStorage.hpp"
#include "mxh/render/FilesystemFileStorage.hpp"
#include "mxh/render/TerrainScene.hpp"
#include "mxh/render/StaticScene.hpp"
#include "mxh/render/SkyScene.hpp"
#include "mxh/render/EntityScene.hpp"
#include "mxh/render/render_typedef.hpp"
#include "mxh/ui/cImage.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/audio/bgm_player.hpp"
#include "mxh/compat/bmhm_map.hpp"
#include "CMainGame.hpp"
#include "CEngine.hpp"
#include "CCharMake.hpp"
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
// Client host configuration and PlayDH discovery.
// ---------------------------------------------------------------------------
namespace {

struct ClientOptions {
    std::string login_host = "127.0.0.1";
    std::uint16_t login_port = 16001;
    std::uint16_t map_port = 18001;
    std::string username = "test";
    std::string password = "test";
    bool auto_create = true;  // default true: see message-pump CharSelect->CharMake branch
    bool exit_after_gamein = false;
    bool follow_camera = false;
    std::string character_name = "ModernHero";
    std::filesystem::path resource_root;
    std::string save_frame;
    std::string state_frames_dir;
};

std::string narrow_ascii(const wchar_t* value) {
    std::string out;
    if (!value) return out;
    while (*value) out.push_back(static_cast<char>(*value++ & 0x7f));
    return out;
}

ClientOptions parse_client_options() {
    ClientOptions options;
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return options;
    for (int i = 1; i < argc; ++i) {
        const std::wstring_view arg(argv[i]);
        const auto take = [&](std::string& target) {
            if (i + 1 < argc) target = narrow_ascii(argv[++i]);
        };
        const auto take_port = [&](std::uint16_t& target) {
            if (i + 1 < argc) {
                const unsigned long value = std::wcstoul(argv[++i], nullptr, 10);
                if (value > 0 && value <= 65535) {
                    target = static_cast<std::uint16_t>(value);
                }
            }
        };
        if (arg == L"--login-host") take(options.login_host);
        else if (arg == L"--login-port") take_port(options.login_port);
        else if (arg == L"--map-port") take_port(options.map_port);
        else if (arg == L"--username") take(options.username);
        else if (arg == L"--password") take(options.password);
        else if (arg == L"--auto-create") options.auto_create = true;
        else if (arg == L"--exit-after-gamein") options.exit_after_gamein = true;
        else if (arg == L"--follow-camera") options.follow_camera = true;
        else if (arg == L"--character-name") take(options.character_name);
        else if (arg == L"--resource-root" && i + 1 < argc) {
            options.resource_root = argv[++i];
        }
        else if (arg == L"--save-frame") take(options.save_frame);
        else if (arg == L"--state-frames-dir") take(options.state_frames_dir);
    }
    LocalFree(argv);
    return options;
}

std::filesystem::path find_playdh_root() {
    std::error_code ec;
    auto base = std::filesystem::current_path(ec);
    for (int depth = 0; !base.empty() && depth < 8; ++depth) {
        const auto direct = base / "PlayDH";
        if (std::filesystem::is_directory(direct, ec)) return direct;
        for (std::filesystem::directory_iterator it(base, ec), end;
             !ec && it != end; it.increment(ec)) {
            if (!it->is_directory(ec)) continue;
            const auto nested = it->path() / "PlayDH";
            if (std::filesystem::is_directory(nested, ec)) return nested;
        }
        const auto parent = base.parent_path();
        if (parent == base) break;
        base = parent;
    }
    return {};
}

class StubFileStorage : public I4DyuchiFileStorage {
public:
    StubFileStorage() = default;
    // I4DyuchiFileStorage has no virtual destructor (legacy COM-style
    // interface), so we don't add one â€” Release() owns the deletion
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
// Render adapter â€” bridges cImage (Phase 6.4) to IDISpriteObject (Phase 5).
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
std::unique_ptr<mxh::gx::TerrainScene> g_terrain;
std::unique_ptr<mxh::gx::StaticScene> g_staticScene;
std::unique_ptr<mxh::gx::SkyScene> g_skyScene;
std::unique_ptr<mxh::gx::EntityScene> g_entityScene;
bool g_renderTerrain = false;
std::string g_captureTerrainFrame;
bool g_overviewCamera = false;
std::string __g_stateFramesDir;
int __g_currentState = -1;
std::string __g_pendingStateFrame;

// Active in-game input target. The WndProc forwards keyboard/mouse events
// to the current game state (only CInGameState consumes input today).
mxh::client::CInGameState* g_inputTarget = nullptr;

// In-game HUD sprites (solid-color quads; original InterfaceScript art is
// wired in a later phase once the UI runtime lands).
struct HudSprites {
    IDISpriteObject* barBg  = nullptr;
    IDISpriteObject* hpFill = nullptr;
    IDISpriteObject* mpFill = nullptr;
};
HudSprites g_hud;
IDIFontObject* g_hudFont = nullptr;

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

// "Moxian-flavored" gradient (dark navy â†’ bright cyan) used as the
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
// 3 small "dialog tile" sprites laid out in a row â€” enough to visually
// confirm per-cImage sprite binding (different colors, different sprites)
// on screen. CMainGame's Process() replaces this in A.1.6.
// ---------------------------------------------------------------------------
namespace {

void drawSpriteQuad(I4DyuchiGXRenderer* r, IDISpriteObject* sprite,
                    float x, float y, float w, float h,
                    std::uint32_t color) {
    if (!r || !sprite) return;
    VECTOR2 scale{w, h};
    VECTOR2 trans{x, y};
    RECT    rc{0, 0, 1, 1};
    r->RenderSprite(sprite, &scale, 0.0f, &trans, &rc,
                    color, /*iZOrder=*/1, /*dwFlag=*/0);
}

void drawHudBar(I4DyuchiGXRenderer* r, IDISpriteObject* bg,
                IDISpriteObject* fill, float x, float y,
                float w, float h, float fraction) {
    drawSpriteQuad(r, bg, x, y, w, h, 0xFFFFFFFFu);
    const float fillW = (w - 2.0f) * std::clamp(fraction, 0.0f, 1.0f);
    if (fillW > 0.5f) {
        drawSpriteQuad(r, fill, x + 1.0f, y + 1.0f, fillW, h - 2.0f,
                       0xFFFFFFFFu);
    }
}

void renderFrame(HWND h) {
    if (!g_renderer) return;

    g_renderer->BeginRender(nullptr, 0xff101830, 0);

    if (g_renderTerrain && g_terrain) {
        g_terrain->configureCamera(800.0f / 600.0f);
        if (!g_overviewCamera && g_skyScene) g_skyScene->render();
        g_terrain->render();
        if (g_staticScene) g_staticScene->render();
        if (g_entityScene) g_entityScene->render();

        // In-game HUD: HP / MP bars fed from the GameInAck totalinfo.
        if (g_inputTarget && g_inputTarget->is_in_game() && g_hud.barBg &&
            g_hud.hpFill && g_hud.mpFill) {
            g_renderer->SetScreenSpaceProjection();
            const auto& info = g_inputTarget->game_info();
            const float hpFrac = info.max_life == 0
                ? 0.0f : static_cast<float>(info.life) /
                         static_cast<float>(info.max_life);
            const float mpFrac = info.max_mp == 0
                ? 0.0f : static_cast<float>(info.mp) /
                         static_cast<float>(info.max_mp);
            drawHudBar(g_renderer, g_hud.barBg, g_hud.hpFill,
                       20.0f, 44.0f, 180.0f, 12.0f, hpFrac);
            drawHudBar(g_renderer, g_hud.barBg, g_hud.mpFill,
                       20.0f, 62.0f, 180.0f, 12.0f, mpFrac);

            // Chat log (last 6 lines) + input line.
            if (g_hudFont) {
                const auto& lines = g_inputTarget->chat_lines();
                int y = 556;
                const std::size_t start =
                    lines.size() > 6 ? lines.size() - 6 : 0;
                for (std::size_t i = start; i < lines.size(); ++i) {
                    const std::string& line = lines[i];
                    if (line.empty()) continue;
                    RECT rc{12, y, 780, y + 20};
                    g_renderer->RenderFont(
                        g_hudFont, const_cast<char*>(line.data()),
                        static_cast<std::uint32_t>(line.size()), &rc,
                        0xFFFFFFFFu, CHAR_CODE_TYPE_ASCII, 1, 0);
                    y -= 18;
                }
                if (g_inputTarget->chat_open()) {
                    const std::string prompt =
                        "> " + g_inputTarget->chat_buffer() + "_";
                    RECT rc{12, y, 780, y + 20};
                    g_renderer->RenderFont(
                        g_hudFont, const_cast<char*>(prompt.data()),
                        static_cast<std::uint32_t>(prompt.size()), &rc,
                        0xFFFFFFFFu, CHAR_CODE_TYPE_ASCII, 1, 0);
                }
            }

            // Quick slot bar (F1..F8). Skill idx from parsed mugong data,
            // falling back to the level-1 starter set.
            {
                constexpr float kSlotW = 44.0f;
                constexpr float kSlotH = 44.0f;
                constexpr float kGap   = 6.0f;
                const float totalW = static_cast<float>(mxh::client::kQuickSlotCount) *
                                     kSlotW + (static_cast<float>(mxh::client::kQuickSlotCount) - 1.0f) * kGap;
                const float barX = (800.0f - totalW) * 0.5f;
                const float barY = 470.0f;
                for (std::size_t i = 0; i < mxh::client::kQuickSlotCount; ++i) {
                    const float x = barX + static_cast<float>(i) * (kSlotW + kGap);
                    drawSpriteQuad(g_renderer, g_hud.barBg, x, barY,
                                   kSlotW, kSlotH, 0xFFFFFFFFu);
                    if (!g_hudFont) continue;
                    const auto skill = mxh::client::quick_skill_for_slot(info, i);
                    const std::string label =
                        skill == 0 ? "-" : std::to_string(skill);
                    RECT rc{static_cast<LONG>(x) + 4, static_cast<LONG>(barY) + 4,
                            static_cast<LONG>(x) + static_cast<LONG>(kSlotW) - 4,
                            static_cast<LONG>(barY) + 20};
                    g_renderer->RenderFont(
                        g_hudFont, const_cast<char*>(label.data()),
                        static_cast<std::uint32_t>(label.size()), &rc,
                        0xFFFFFFFFu, CHAR_CODE_TYPE_ASCII, 1, 0);
                    const std::string key = "F" + std::to_string(i + 1);
                    RECT rcKey{static_cast<LONG>(x), static_cast<LONG>(barY) + 26,
                               static_cast<LONG>(x) + static_cast<LONG>(kSlotW),
                               static_cast<LONG>(barY) + static_cast<LONG>(kSlotH)};
                    g_renderer->RenderFont(
                        g_hudFont, const_cast<char*>(key.data()),
                        static_cast<std::uint32_t>(key.size()), &rcKey,
                        0xFFFFFFFFu, CHAR_CODE_TYPE_ASCII, 1, 0);
                }
            }

            // Inventory panel (I key toggles).
            if (g_inputTarget->inventory_open()) {
                constexpr float kCell = 34.0f;
                constexpr float kInvGap = 3.0f;
                constexpr float invX = 20.0f;
                constexpr float invY = 84.0f;
                const auto& inventory = info.items.Inventory;
                for (int row = 0; row < 8; ++row) {
                    for (int col = 0; col < 10; ++col) {
                        const int idx = row * 10 + col;
                        const float x = invX + static_cast<float>(col) * (kCell + kInvGap);
                        const float y = invY + static_cast<float>(row) * (kCell + kInvGap);
                        if (!mxh::game::is_empty_slot(inventory[idx])) {
                            drawSpriteQuad(g_renderer, g_hud.mpFill, x, y,
                                           kCell, kCell, 0xFFFFFFFFu);
                            if (g_hudFont) {
                                const std::string t =
                                    std::to_string(inventory[idx].wIconIdx);
                                RECT rc{static_cast<LONG>(x),
                                        static_cast<LONG>(y),
                                        static_cast<LONG>(x) + static_cast<LONG>(kCell),
                                        static_cast<LONG>(y) + 16};
                                g_renderer->RenderFont(
                                    g_hudFont, const_cast<char*>(t.data()),
                                    static_cast<std::uint32_t>(t.size()), &rc,
                                    0xFFFFFFFFu, CHAR_CODE_TYPE_ASCII, 1, 0);
                            }
                        } else {
                            drawSpriteQuad(g_renderer, g_hud.barBg, x, y,
                                           kCell, kCell, 0xFFFFFFFFu);
                        }
                    }
                }
            }

            // NPC shop panel (opened with 'B', click a row to buy).
            if (g_inputTarget->shop_open()) {
                const auto& shopItems = g_inputTarget->shop_items();
                constexpr float kTitleH = 28.0f;
                drawSpriteQuad(g_renderer, g_hud.barBg,
                               mxh::client::kShopPanelX - 8.0f,
                               mxh::client::kShopPanelY - kTitleH,
                               mxh::client::kShopPanelW + 16.0f,
                               kTitleH, 0xFFFFFFFFu);
                drawSpriteQuad(g_renderer, g_hud.barBg,
                               mxh::client::kShopPanelX,
                               mxh::client::kShopPanelY,
                               mxh::client::kShopPanelW,
                               static_cast<float>(shopItems.size()) *
                                   mxh::client::kShopRowH,
                               0xFFFFFFFFu);
                if (g_hudFont) {
                    const std::string title =
                        "NPC Shop  (click to buy, B close)";
                    RECT rcTitle{static_cast<LONG>(mxh::client::kShopPanelX) - 4,
                                 static_cast<LONG>(mxh::client::kShopPanelY) - kTitleH + 6,
                                 static_cast<LONG>(mxh::client::kShopPanelX) + 300,
                                 static_cast<LONG>(mxh::client::kShopPanelY)};
                    g_renderer->RenderFont(
                        g_hudFont, const_cast<char*>(title.data()),
                        static_cast<std::uint32_t>(title.size()), &rcTitle,
                        0xFFFFFFFFu, CHAR_CODE_TYPE_ASCII, 1, 0);
                    for (std::size_t i = 0; i < shopItems.size() && i < 12; ++i) {
                        const float rowY = mxh::client::kShopPanelY +
                            static_cast<float>(i) * mxh::client::kShopRowH;
                        const std::string line =
                            std::to_string(shopItems[i].item_id) +
                            "   $" + std::to_string(shopItems[i].price);
                        RECT rc{static_cast<LONG>(mxh::client::kShopPanelX) + 8,
                                static_cast<LONG>(rowY) + 4,
                                static_cast<LONG>(mxh::client::kShopPanelX) + 380,
                                static_cast<LONG>(rowY) + 22};
                        g_renderer->RenderFont(
                            g_hudFont, const_cast<char*>(line.data()),
                            static_cast<std::uint32_t>(line.size()), &rc,
                            0xFFFFFFFFu, CHAR_CODE_TYPE_ASCII, 1, 0);
                    }
                }
            }
        }
    }

    // Background tile during login/character states.
    if (!g_renderTerrain && g_sprites[0].sprite) {
        IMAGE_HEADER image{};
        g_sprites[0].sprite->GetImageHeader(&image, 0);
        VECTOR2 scale{
            image.dwWidth ? 800.0f / static_cast<float>(image.dwWidth) : 1.0f,
            image.dwHeight ? 600.0f / static_cast<float>(image.dwHeight) : 1.0f };
        VECTOR2 trans{ 0.0f, 0.0f };
        RECT     rc{ 0, 0, static_cast<LONG>(image.dwWidth),
                     static_cast<LONG>(image.dwHeight) };
        { BOOL _bg_ok = g_renderer->RenderSprite(g_sprites[0].sprite, &scale, 0.0f, &trans,
                                 &rc, 0xFFFFFFFFu, 0, 0); MLOG_DEBUG("mxh_client: bg draw sprite=%p ok=%d", (void*)g_sprites[0].sprite, (int)_bg_ok); }
    }

    // 3 demo cImages at the bottom.  In A.1.5 these come from a real
    // cDialog tree (the CMainTitle login form); for now we draw them
    // directly through the renderer's HUD pass using the per-sprite
    // binding we just registered.
    constexpr float kTileW = 120.0f, kTileH = 80.0f;
    constexpr float kTileY = 480.0f;
    constexpr float kGapX  = 20.0f;
    constexpr float kStartX = (800.0f - (3.0f * kTileW + 2.0f * kGapX)) * 0.5f;
    for (std::uint32_t i = 1; !g_renderTerrain && i <= 3; ++i) {
        if (!g_sprites[i].sprite) continue;
        IMAGE_HEADER image{};
        g_sprites[i].sprite->GetImageHeader(&image, 0);
        VECTOR2 scale{
            image.dwWidth ? kTileW / static_cast<float>(image.dwWidth) : 1.0f,
            image.dwHeight ? kTileH / static_cast<float>(image.dwHeight) : 1.0f };
        VECTOR2 trans{ kStartX + (i - 1) * (kTileW + kGapX), kTileY };
        RECT     rc{ 0, 0, static_cast<LONG>(image.dwWidth),
                     static_cast<LONG>(image.dwHeight) };
        g_renderer->RenderSprite(g_sprites[i].sprite, &scale, 0.0f, &trans,
                                 &rc, 0xFFFFFFFFu,
                                 /*iZOrder=*/static_cast<int>(i), 0);
    }

    g_renderer->EndRender();
    if (!__g_stateFramesDir.empty() && __g_currentState >= 0 &&
        __g_pendingStateFrame.empty()) {
        // Build per-state frame path on first frame in this state.
        static const char* kStateNames[] = {
            "end", "intro", "connect", "login", "charselect",
            "charmake", "gameloading", "gamein", "mapchange", "murimnet"
        };
        const auto idx = static_cast<std::size_t>(__g_currentState);
        const char* name = (idx < std::size(kStateNames))
                                ? kStateNames[idx] : "unknown";
        std::string p = __g_stateFramesDir + "/state-" + name + ".tga";
        __g_pendingStateFrame = p;
    }
    if (!__g_pendingStateFrame.empty()) {
        std::string mutable_fname = __g_pendingStateFrame;
        __g_pendingStateFrame.clear();
        if (g_renderer->CaptureScreen(mutable_fname.data())) {
            MLOG_INFO("mxh_client: state frame saved state=%d path=%s",
                      __g_currentState, mutable_fname.c_str());
        }
    }
    if (g_renderTerrain && !g_captureTerrainFrame.empty()) {
        if (g_renderer->CaptureScreen(g_captureTerrainFrame.data()))
            MLOG_INFO("mxh_client: terrain frame saved");
        g_captureTerrainFrame.clear();
    }
    g_renderer->Present(h);
}

} // namespace

// ---------------------------------------------------------------------------
// Window procedure. Mirrors the legacy MHClient.cpp WndProc surface â€” only
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
        PAINTSTRUCT paint{};
        BeginPaint(h, &paint);
        renderFrame(h);
        EndPaint(h, &paint);
        return 0;
    }
    case WM_KEYDOWN:
        // ESC quits in A.1 (the legacy engine uses ESC to open the menu;
        // for the skeleton we use it as the "exit" hotkey).
        if (w == VK_ESCAPE) {
            mxh::client::g_running = false;
            PostQuitMessage(0);
            return 0;
        }
        if (g_inputTarget) {
            g_inputTarget->OnKeyEvent(true, static_cast<std::uint32_t>(w));
        }
        return 0;
    case WM_KEYUP:
        if (g_inputTarget) {
            g_inputTarget->OnKeyEvent(false, static_cast<std::uint32_t>(w));
        }
        return 0;
    case WM_CHAR:
        if (g_inputTarget) {
            g_inputTarget->OnChar(static_cast<std::uint32_t>(w));
        }
        return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        if (g_inputTarget) {
            g_inputTarget->OnMouseButton(
                true, m == WM_LBUTTONDOWN,
                static_cast<std::int32_t>(static_cast<short>(LOWORD(l))),
                static_cast<std::int32_t>(static_cast<short>(HIWORD(l))));
        }
        return 0;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        if (g_inputTarget) {
            g_inputTarget->OnMouseButton(
                false, m == WM_RBUTTONDOWN,
                static_cast<std::int32_t>(static_cast<short>(LOWORD(l))),
                static_cast<std::int32_t>(static_cast<short>(HIWORD(l))));
        }
        return 0;
    case WM_MOUSEMOVE:
        if (g_inputTarget) {
            g_inputTarget->OnMouseMove(
                static_cast<std::int32_t>(static_cast<short>(LOWORD(l))),
                static_cast<std::int32_t>(static_cast<short>(HIWORD(l))));
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
    ClientOptions options = parse_client_options();
    g_overviewCamera = !options.save_frame.empty() && !options.follow_camera;
    __g_stateFramesDir = options.state_frames_dir;
    if (!__g_stateFramesDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(__g_stateFramesDir), ec);
    }
    MLOG_INFO("mxh_client: booting version %s", mxh::client::g_CLIENTVERSION);
    MLOG_INFO("mxh_client: login=%s:%u map-port=%u user=%s",
              options.login_host.c_str(), options.login_port,
              options.map_port, options.username.c_str());

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

    if (options.resource_root.empty()) options.resource_root = find_playdh_root();
    if (options.resource_root.empty()) {
        std::fprintf(stderr, "mxh_client: PlayDH resource root not found\n");
        return 1;
    }
    auto* storage = new mxh::gx::FilesystemFileStorage(options.resource_root);
    if (!storage->Initialize(0, 0, 0, FILE_ACCESS_METHOD_ONLY_FILE)) {
        std::fprintf(stderr, "mxh_client: invalid resource root\n");
        storage->Release();
        return 1;
    }
    MLOG_INFO("mxh_client: PlayDH root loaded");

    mxh::audio::BgmPlayer bgm;
    std::string audio_error;
    if (bgm.initialize(options.resource_root / "Sound", &audio_error)) {
        // 1667 is the original login theme in SoundList.bin.
        if (!bgm.play(1667, &audio_error))
            MLOG_WARN("mxh_client: login BGM unavailable: %s", audio_error.c_str());
    } else {
        MLOG_WARN("mxh_client: SoundList unavailable: %s", audio_error.c_str());
    }

    // Renderer. The factory is implemented in modern/src/render (DX11
    // backend). The pointer is borrowed â€” the factory retains ownership.
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
    g_hud.barBg  = renderer->CreateSolidSpriteObject(0xAA181010u, 1, 1);
    g_hud.hpFill = renderer->CreateSolidSpriteObject(0xFF4040FFu, 1, 1);
    g_hud.mpFill = renderer->CreateSolidSpriteObject(0xFFFF9040u, 1, 1);
    LOGFONT hudLf{};
    hudLf.lfHeight = -14;
    hudLf.lfWeight = FW_NORMAL;
    hudLf.lfCharSet = DEFAULT_CHARSET;
    hudLf.lfQuality = ANTIALIASED_QUALITY;
    std::strncpy(hudLf.lfFaceName, "Arial", LF_FACESIZE - 1);
    g_hudFont = renderer->CreateFontObject(&hudLf, 0);
    MLOG_INFO("mxh_client: hud font=%p", (void*)g_hudFont);

    // Build per-cImage sprite registry.  A.1.4 ships with 4 demo
    // sprites: one gradient background + three solid-colour "dialog
    // tiles" laid out in a row.  The IDs are stable so cImage wrappers
    // can be created with SetSpriteObject(sprite) at the same slot.
    ID3D11Device* dev = nullptr;
    if (renderer->GetD3DDevice(__uuidof(IUnknown),
                              reinterpret_cast<void**>(&dev))) {
        g_sprites[0].sprite = renderer->CreateSpriteObject(
            const_cast<char*>("Image/2D/login.dds"), 0);
        MLOG_INFO("mxh_client: sprite[0] login.dds sprite=%p", (void*)g_sprites[0].sprite);
        g_sprites[1].sprite = renderer->CreateSpriteObject(
            const_cast<char*>("Image/MunpaMark/02_1000.tga"), 0);
        g_sprites[2].sprite = renderer->CreateSpriteObject(
            const_cast<char*>("Image/MunpaMark/02_1003.tga"), 0);
        g_sprites[3].sprite = renderer->CreateSpriteObject(
            const_cast<char*>("Image/MunpaMark/02_1008.tga"), 0);
        MLOG_INFO("mxh_client: %u sprites registered (1 background + 3 tiles)",
                  static_cast<unsigned>(g_sprites.size()));
    } else {
        MLOG_WARN("mxh_client: GetD3DDevice failed, no sprites created");
    }

    // Install the cImage â†” renderer adapter.  After this call every
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
    // Phase A.1.6 â€” wire CMainGame + CEngine + the 9 eGAMESTATE stubs.
    //
    // CEngine gets the HWND and the IRenderer so future states can
    // look them up. CMainGame owns the state table; we register the
    // 9 concrete stubs from GameStateStubs.hpp.  The boot transition
    // (Engine â†’ CMainTitle) goes through CMainGame::SetGameState so
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
              "boot â†’ GameStateId::Connect");

    // Phase B.2.1: kick off the CLoginState connect (host-driven; the
    // state can't self-start because it doesn't know the login address
    // until MHVerInfo.ver parsing lands in B.2.5).
    if (auto* login = dynamic_cast<mxh::client::CLoginState*>(
            mainGame.GetGameState(mxh::client::GameStateId::Connect))) {
        login->Start(mainGame.GetEngine(), options.login_host,
                     options.login_port, options.username, options.password);
    } else {
        MLOG_ERROR("Connect slot is not a CLoginState; cannot start login");
    }

    MLOG_INFO("mxh_client: entering message loop");

    // Standard Win32 message pump + CMainGame driver.  The legacy
    // engine did "Process() â†’ render() â†’ flip" once per frame; we
    // mirror that here.  CMainGame::BeforeRender/AfterRender fan out
    // to the current state and respect the pause-render flag.
    //
    // Phase B.2.2: the host has to Start() each new state once it
    // becomes current.  CMainGame's transition is delayed (next
    // Process() call after SetGameState), so we track the previous
    // state number and drive Start() on the rising edge.
    auto prev_state = mxh::client::GameStateId::End;
    std::uint32_t pending_character_id = 0;
    std::uint16_t pending_map_num = 0;
    bool auto_create_requested = false;
    bool follow_frame_captured = false;
    unsigned follow_settle_frames = 0;
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
                if (prev_state == mxh::client::GameStateId::GameIn) {
                    g_inputTarget = nullptr;
                }
                // Phase B.2.5: skip past the manual login form (CMainTitle)
            // when running in headless smoke mode. The 1:1 flow goes
            // Connect -> Distribute -> Title(login form) -> CharSelect;
            // CLoginState now requests Title so the GUI smoke test can
            // capture state-login.tga, and the host main loop
            // immediately auto-redirects Title to CharSelect on the
            // rising edge. The LoginResult transfer slot (set by
            // CLoginState::dispatch_login_ack) is consumed by
            // CCharSelectState::Init() when CharSelect is entered.
            if (cur_state == mxh::client::GameStateId::Title) {
                mainGame.SetGameState(mxh::client::GameStateId::CharSelect);
            } else if (cur_state == mxh::client::GameStateId::CharSelect) {
                    if (auto* cs = dynamic_cast<mxh::client::CCharSelectState*>(
                            mainGame.GetGameState(cur_state))) {
                        cs->Start(mainGame.GetEngine());
                    }
                } else if (cur_state == mxh::client::GameStateId::CharMake) {
                    if (auto* cm = dynamic_cast<mxh::client::CCharMake*>(
                            mainGame.GetGameState(cur_state))) {
                        cm->Start(mainGame.GetEngine());
                    }
                } else if (cur_state == mxh::client::GameStateId::GameLoading) {
                    auto transfer = mainGame.GetEngine()->TakePendingTransfer();
                    if (transfer.type() == typeid(mxh::client::GameEntryRequest)) {
                        const auto request =
                            std::any_cast<mxh::client::GameEntryRequest>(transfer);
                        pending_character_id = request.character_id;
                        pending_map_num = request.map_num;
                        mainGame.SetGameState(mxh::client::GameStateId::GameIn);
                    } else {
                        MLOG_ERROR("GameLoading: missing GameEntryRequest");
                    }
                } else if (cur_state == mxh::client::GameStateId::GameIn) {
                    // Phase B.2.3: dev-mode direct-connect to MapServer.
                    // The full CharSelect â†’ GameLoading â†’ GameIn path
                    // requires a character in character_info (Phase B.4+);
                    // for now we jump straight to MapServer with the
                    // login ack user_idx as chrid.
                    if (auto* g = dynamic_cast<mxh::client::CInGameState*>(
                            mainGame.GetGameState(cur_state))) {
                        const auto descriptorPath = options.resource_root / "Resource" / "Map" /
                            ("Map" + std::to_string(pending_map_num) + ".bmhm");
                        if (const auto descriptor = mxh::compat::BmhmMap::load(descriptorPath)) {
                            audio_error.clear();
                            if (!bgm.play(descriptor->desc().bgm_sound_num, &audio_error))
                                MLOG_WARN("mxh_client: map BGM unavailable: %s", audio_error.c_str());
                            if (!g_terrain) g_terrain = std::make_unique<mxh::gx::TerrainScene>();
                            const std::string hflName = std::to_string(pending_map_num) + ".hfl";
                            std::string terrainError;
                            if (g_terrain->load(renderer, storage, hflName.c_str(), &terrainError)) {
                                g_renderTerrain = true;
                                if (g_overviewCamera)
                                    g_captureTerrainFrame = options.save_frame;
                                if (!g_staticScene) g_staticScene = std::make_unique<mxh::gx::StaticScene>();
                                const std::string stmName = std::to_string(pending_map_num) + ".stm";
                                std::string staticError;
                                if (!g_staticScene->load(renderer, storage, stmName.c_str(), &staticError))
                                    MLOG_WARN("mxh_client: static scene unavailable: %s", staticError.c_str());
                                if (descriptor->desc().sky_mod[0]) {
                                    if (!g_skyScene) g_skyScene = std::make_unique<mxh::gx::SkyScene>();
                                    std::string skyError;
                                    if (!g_skyScene->load(renderer, storage,
                                                          descriptor->desc().sky_mod, &skyError))
                                        MLOG_WARN("mxh_client: sky scene unavailable: %s", skyError.c_str());
                                }
                                if (!g_entityScene) {
                                    g_entityScene = std::make_unique<mxh::gx::EntityScene>();
                                    std::string entityError;
                                    if (!g_entityScene->load(renderer, storage, &entityError))
                                        MLOG_WARN("mxh_client: entity scene unavailable: %s", entityError.c_str());
                                }
                            } else {
                                MLOG_ERROR("mxh_client: terrain load failed: %s", terrainError.c_str());
                            }
                        } else {
                            MLOG_WARN("mxh_client: map descriptor unavailable for map=%u", pending_map_num);
                        }
                        g->Start(mainGame.GetEngine(), options.login_host,
                                 options.map_port, pending_character_id,
                                 pending_map_num);
                        g_inputTarget = g;
                    }
                }
                prev_state = cur_state;
            }
            if (cur_state == mxh::client::GameStateId::CharSelect &&
                options.auto_create && !auto_create_requested) {
                if (auto* cs = dynamic_cast<mxh::client::CCharSelectState*>(
                        mainGame.GetGameState(cur_state));
                    cs && cs->has_character_list()) {
                    bool has_character = false;
                    for (const auto& slot : cs->character_list()) {
                        has_character = has_character || slot.valid;
                    }
                    if (!has_character) {
                        mainGame.GetEngine()->SetPendingTransfer(cs->login_result());
                        mainGame.SetGameState(mxh::client::GameStateId::CharMake);
                        auto_create_requested = true;
                    }
                }
            } else if (cur_state == mxh::client::GameStateId::CharMake &&
                       options.auto_create) {
                if (auto* cm = dynamic_cast<mxh::client::CCharMake*>(
                        mainGame.GetGameState(cur_state));
                    cm && cm->is_connected() && !cm->is_submitted() &&
                    !cm->is_failed()) {
                    mxh::client::CharacterMakeParams params;
                    params.name = options.character_name;
                    (void)cm->SubmitCharacter(params);
                }
            } else if (cur_state == mxh::client::GameStateId::GameIn &&
                       options.exit_after_gamein) {
                if (auto* game_in = dynamic_cast<mxh::client::CInGameState*>(
                        mainGame.GetGameState(cur_state));
                    game_in && game_in->is_in_game() &&
                    (!options.follow_camera || !game_in->monsters().empty())) {
                    if (!options.follow_camera || ++follow_settle_frames >= 20u) {
                        MLOG_INFO("mxh_client: GUI_SMOKE_PASS player_id=%u map=%u",
                                  game_in->player_id(), game_in->map_num());
                        mxh::client::g_running = false;
                    }
                }
            }
            if (!g_overviewCamera && cur_state == mxh::client::GameStateId::GameIn) {
                if (auto* game_in = dynamic_cast<mxh::client::CInGameState*>(
                        mainGame.GetGameState(cur_state)); game_in && game_in->is_in_game()) {
                    const auto& info = game_in->game_info();
                    if (g_terrain) {
                        g_terrain->followPlayer(info.position_x, info.position_z);
                        g_terrain->setCameraYaw(game_in->camera_yaw());
                        if (!follow_frame_captured && !options.save_frame.empty() &&
                            (!options.follow_camera || follow_settle_frames >= 20u)) {
                            g_captureTerrainFrame = options.save_frame;
                            follow_frame_captured = true;
                        }
                    }
                    if (g_entityScene && g_terrain) {
                        std::vector<mxh::gx::SceneEntity> entities;
                        entities.reserve(game_in->monsters().size());
                        for (const auto& monster : game_in->monsters()) {
                            entities.push_back({monster.object_id, monster.monster_kind,
                                static_cast<float>(monster.position_x),
                                g_terrain->heightAt(monster.position_x, monster.position_z),
                                static_cast<float>(monster.position_z)});
                        }
                        g_entityScene->synchronize(entities);
                        g_entityScene->synchronizePlayer({info.player_id, info.gender,
                            info.face_type, info.hair_type, info.weared_item_idx,
                            static_cast<float>(info.position_x),
                            g_terrain->heightAt(info.position_x, info.position_z),
                            static_cast<float>(info.position_z)});
                    }
                }
            }
            __g_currentState = static_cast<int>(mainGame.GetCurStateNum());
            if (cur_state != prev_state) {
                __g_pendingStateFrame.clear();
            }
            mainGame.BeforeRender();
            renderFrame(hwnd);
            mainGame.AfterRender();
            Sleep(16);
        }
    }

    MLOG_INFO("mxh_client: shutting down");

    g_inputTarget = nullptr;
    mainGame.Release();
    g_renderTerrain = false;
    g_terrain.reset();
    g_staticScene.reset();
    g_skyScene.reset();
    g_entityScene.reset();

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


