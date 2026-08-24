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
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <array>
#include <string>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

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
#include "mxh/game/npc_role.hpp"  // M-NPC1: per-role NPC marker slot + quest indicator
#include "mxh/ui/cImage.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/ui/cResourceManager.hpp"
#include "mxh/ui/cSpriteAtlas.hpp"
#include "mxh/ui/cWindowManager.hpp"
#include "TextRender.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/audio/bgm_player.hpp"
#include "mxh/compat/bmhm_map.hpp"
#include "CMainGame.hpp"
#include "CEngine.hpp"
#include "CCharMake.hpp"
#include "GameStateStubs.hpp"
#include "ClientSettings.hpp"
#include "GameLoadingCoordinator.hpp"
#include "LogicalViewport.hpp"
#include "SpriteRenderGeometry.hpp"

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
    std::string username;
    std::string password;
    bool auto_login = false;
    bool auto_create = false;
    bool exit_after_gamein = false;
    std::uint32_t smoke_settle_frames = 0;
    bool follow_camera = false;
    bool debug_ui_bounds = false;
    std::string character_name = "ModernHero";
    std::filesystem::path resource_root;
    std::string save_frame;
    std::string state_frames_dir;
    // M-R7 (G3) 物理 GPU 段: --width/--height 控制 swap chain + 截屏尺寸.
    // 默认 0 = 用 kDefaultWindowWidth/Height (800x600). 4 档: 800/1024/1920/2560.
    std::uint32_t window_width = 0;
    std::uint32_t window_height = 0;
    std::uint32_t post_login_width = 1024;
    std::uint32_t post_login_height = 768;
};

std::uint32_t read_dimension_env(const wchar_t* name,
                                 std::uint32_t fallback,
                                 std::uint32_t minimum,
                                 std::uint32_t maximum) {
    wchar_t value[32]{};
    const auto length = GetEnvironmentVariableW(name, value, 32);
    if (length == 0 || length >= 32) return fallback;
    const auto parsed = std::wcstoul(value, nullptr, 10);
    if (parsed < minimum || parsed > maximum) return fallback;
    return static_cast<std::uint32_t>(parsed);
}

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
        else if (arg == L"--auto-login") options.auto_login = true;
        else if (arg == L"--auto-create") options.auto_create = true;
        else if (arg == L"--exit-after-gamein") options.exit_after_gamein = true;
        else if (arg == L"--smoke-settle-frames" && i + 1 < argc)
            options.smoke_settle_frames = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        else if (arg == L"--follow-camera") options.follow_camera = true;
        else if (arg == L"--debug-ui-bounds") options.debug_ui_bounds = true;
        else if (arg == L"--character-name") take(options.character_name);
        else if (arg == L"--resource-root" && i + 1 < argc) {
            options.resource_root = argv[++i];
        }
        else if (arg == L"--save-frame") take(options.save_frame);
        else if (arg == L"--state-frames-dir") take(options.state_frames_dir);
        else if (arg == L"--width" && i + 1 < argc)
            options.window_width = static_cast<std::uint32_t>(std::wcstoul(argv[++i], nullptr, 10));
        else if (arg == L"--height" && i + 1 < argc)
            options.window_height = static_cast<std::uint32_t>(std::wcstoul(argv[++i], nullptr, 10));
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

bool validate_runtime_resources(const std::filesystem::path& root,
                                std::string* error) {
    static constexpr const char* required[] = {
        "Resource/Map/Map10.bmhm",
        "Resource/MonsterList.bin",
        "Resource/Client/NpcChxList.bin",
        "Image/InterfaceScript/CharSelectDlg.bin",
        "Image/InterfaceScript/CharMakeDlg.bin",
        "Sound/SoundList.bin"
    };
    for (const auto* relative : required) {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(root / relative, ec) || ec) {
            if (error) *error = std::string("missing required runtime resource: ") + relative;
            return false;
        }
    }
    return true;
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
bool g_debugUiBounds = false;
std::string __g_stateFramesDir;
int __g_currentState = -1;
std::string __g_pendingStateFrame;

// Active in-game input target. The WndProc forwards keyboard/mouse events
// to the current game state (only CInGameState consumes input today).
mxh::client::CInGameState* g_inputTarget = nullptr;
mxh::client::CCharSelectState* g_charSelectState = nullptr;
mxh::client::CCharMake*        g_charMakeState   = nullptr;  // M-R7.1 (2026-08-20)
mxh::client::CMainTitle*       g_mainTitle       = nullptr;
mxh::client::LogicalViewport   g_logicalViewport;

// In-game HUD sprites (solid-color quads; original InterfaceScript art is
// wired in M-R3 via cDialogLoader::LoadAll — see below after bindRenderer()).
//
// M-NPC1: NPC markers are now per-role so the player can see at a glance
// whether a static NPC is a quest giver (yellow "!" above), a shop
// (cyan), a bounty (orange), or a warp (purple).  Wire order matches
// the modern NpcRole enum (mxh::game::NpcRole); see npc_role.hpp.
struct HudSprites {
    IDISpriteObject* barBg  = nullptr;
    IDISpriteObject* hpFill = nullptr;
    IDISpriteObject* mpFill = nullptr;
    // npcMarkers[0..4] = Talker, Dealer, Wanted, MapChange, Other
    // (see kNpcMarkerCount).  Indexed by npc_marker_slot(NpcRole).
    IDISpriteObject* npcMarkers[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    static constexpr std::size_t kNpcMarkerCount = 5;
    IDISpriteObject* questMark = nullptr;  // "!" indicator above quest NPCs
};
HudSprites g_hud;
IDIFontObject* g_hudFont = nullptr;
HFONT g_hudMeasureFont = nullptr;

// M-NPC1: choose the NPC marker sprite slot for a given wire role.
// Returns the slot in [0, kNpcMarkerCount).
static std::size_t npc_marker_slot(std::uint16_t kind) noexcept {
    using mxh::game::NpcRole;
    switch (mxh::game::role_from_wire(kind)) {
        case NpcRole::Talker:    return 0;  // quest giver — yellow
        case NpcRole::Dealer:    return 1;  // shop — cyan (was the old 0x00D7FF)
        case NpcRole::Wanted:    return 2;  // bounty — orange
        case NpcRole::MapChange: return 3;  // warp — purple
        default:                 return 4;  // everything else — gray
    }
}

bool loadGameWorld(const ClientOptions& options,
                   I4DyuchiGXRenderer* renderer,
                   I4DyuchiFileStorage* storage,
                   mxh::audio::BgmPlayer& bgm,
                   std::uint16_t mapNum,
                   std::string& error) {
    error.clear();
    g_renderTerrain = false;
    g_captureTerrainFrame.clear();

    const auto descriptorPath = options.resource_root / "Resource" / "Map" /
        ("Map" + std::to_string(mapNum) + ".bmhm");
    const auto descriptor = mxh::compat::BmhmMap::load(descriptorPath);
    if (!descriptor) {
        error = "Map descriptor unavailable: " + descriptorPath.string();
        return false;
    }

    auto terrain = std::make_unique<mxh::gx::TerrainScene>();
    const std::string hflName = std::to_string(mapNum) + ".hfl";
    std::string stageError;
    if (!terrain->load(renderer, storage, hflName.c_str(), &stageError)) {
        error = "Terrain load failed (" + hflName + "): " + stageError;
        return false;
    }
    if (terrain->unresolvedTextureCount() != 0 && !g_debugUiBounds) {
        error = "Terrain contains " +
            std::to_string(terrain->unresolvedTextureCount()) +
            " unresolved textures";
        MLOG_ERROR("mxh_client: %s", error.c_str());
        return false;
    }

    auto staticScene = std::make_unique<mxh::gx::StaticScene>();
    const std::string stmName = std::to_string(mapNum) + ".stm";
    stageError.clear();
    if (!staticScene->load(renderer, storage, stmName.c_str(), &stageError)) {
        error = "Static scene load failed (" + stmName + "): " + stageError;
        return false;
    }
    if (staticScene->unresolvedTextureCount() != 0 && !g_debugUiBounds) {
        error = "Static scene contains " +
            std::to_string(staticScene->unresolvedTextureCount()) +
            " unresolved textures";
        MLOG_ERROR("mxh_client: %s", error.c_str());
        return false;
    }

    std::unique_ptr<mxh::gx::SkyScene> skyScene;
    if (descriptor->desc().sky_mod[0]) {
        skyScene = std::make_unique<mxh::gx::SkyScene>();
        stageError.clear();
        if (!skyScene->load(renderer, storage, descriptor->desc().sky_mod,
                            &stageError)) {
            error = "Sky scene load failed (" +
                std::string(descriptor->desc().sky_mod) + "): " + stageError;
            return false;
        }
    }

    auto entityScene = std::make_unique<mxh::gx::EntityScene>();
    stageError.clear();
    if (!entityScene->load(renderer, storage, &stageError)) {
        error = "Entity scene load failed: " + stageError;
        return false;
    }

    std::string audioError;
    if (!bgm.play(descriptor->desc().bgm_sound_num, &audioError)) {
        MLOG_WARN("mxh_client: map BGM unavailable: %s", audioError.c_str());
    }

    g_terrain = std::move(terrain);
    g_staticScene = std::move(staticScene);
    g_skyScene = std::move(skyScene);
    g_entityScene = std::move(entityScene);
    g_renderTerrain = true;
    if (g_overviewCamera) g_captureTerrainFrame = options.save_frame;
    MLOG_INFO("mxh_client: GameLoading complete map=%u terrain=%s static=%s",
              static_cast<unsigned>(mapNum), hflName.c_str(), stmName.c_str());
    return true;
}

// Phase A.1.4: per-cImage sprite. cImage holds an opaque void* (its
// IDISpriteObject*). The adapter casts back and forwards to the
// renderer's RenderSprite.  Earlier A.1.3 had a single g_hudSprite
// that all cImages shared; the per-sprite version is the realistic
// surface the rest of Phase A will use.
bool renderAdapter(void* /*ctx*/, void* sprite,
                   float x, float y, float w, float h,
                   float u0, float v0, float u1, float v1,
                   std::uint32_t color, int zOrder) {
    if (!g_renderer) return false;
    if (!sprite)    return false;

    auto* sp = static_cast<IDISpriteObject*>(sprite);
    IMAGE_HEADER header{};
    if (!sp->GetImageHeader(&header, 0)) return false;
    const auto geometry = mxh::client::compute_sprite_render_geometry(
        header.dwWidth, header.dwHeight, w, h, u0, v0, u1, v1);
    if (!geometry.has_value()) return false;

    VECTOR2 scale{ geometry->scale_x, geometry->scale_y };
    VECTOR2 trans{ x, y };
    RECT rc{
        geometry->source_left,
        geometry->source_top,
        geometry->source_right,
        geometry->source_bottom,
    };
    return g_renderer->RenderSprite(sp, &scale, 0.0f, &trans, &rc,
                                    color, zOrder, /*dwFlag=*/0) != FALSE;
}

std::wstring decodeUiText(std::string_view text) {
    if (text.empty()) return {};
    auto decode = [&](UINT code_page, DWORD flags) -> std::wstring {
        const int count = MultiByteToWideChar(
            code_page, flags, text.data(), static_cast<int>(text.size()),
            nullptr, 0);
        if (count <= 0) return {};
        std::wstring result(static_cast<std::size_t>(count), L'\0');
        if (MultiByteToWideChar(code_page, flags, text.data(),
                                static_cast<int>(text.size()), result.data(),
                                count) <= 0) {
            return {};
        }
        return result;
    };

    // The CHINA PlayDH InterfaceScript and option tables use Big5 bytes.
    // ASCII is a strict subset, so the same path also handles account/name UI.
    auto result = decode(950, MB_ERR_INVALID_CHARS);
    if (result.empty()) result = decode(CP_ACP, 0);
    return result;
}

SIZE measureUiText(std::wstring_view text) {
    SIZE size{};
    if (text.empty()) return size;
    HDC dc = GetDC(nullptr);
    if (!dc) return size;
    HGDIOBJ previous = nullptr;
    if (g_hudMeasureFont) previous = SelectObject(dc, g_hudMeasureFont);
    GetTextExtentPoint32W(dc, text.data(), static_cast<int>(text.size()), &size);
    if (previous) SelectObject(dc, previous);
    ReleaseDC(nullptr, dc);
    return size;
}

bool textRenderAdapter(void* /*context*/,
                       const mxh::ui::TextRenderRequest& request) {
    if (!g_renderer || !g_hudFont) return false;
    if (request.text.empty() &&
        request.caret_byte == mxh::ui::TextRenderRequest::NoCaret) {
        return false;
    }

    const auto draw_line = [&](std::string_view bytes, std::int32_t line_y,
                               std::size_t caret_byte) {
        auto wide = decodeUiText(bytes);
        const SIZE extent = measureUiText(wide);
        LONG x = request.x + request.left_inset;
        if (request.align == mxh::ui::TextRenderAlign::Right) {
            x = request.x + request.width - request.right_inset - extent.cx;
        } else if (request.align == mxh::ui::TextRenderAlign::Center) {
            x = request.x + (request.width - extent.cx) / 2;
        }
        RECT rect{x, line_y, request.x + request.width,
                  line_y + request.height};
        BOOL drawn = FALSE;
        if (!wide.empty()) {
            drawn = g_renderer->RenderFont(
                g_hudFont,
                reinterpret_cast<TCHAR*>(wide.data()),
                static_cast<std::uint32_t>(wide.size()), &rect,
                request.color, CHAR_CODE_TYPE_UNICODE, 2, 0);
        }

        BOOL caret_drawn = FALSE;
        if (caret_byte != mxh::ui::TextRenderRequest::NoCaret) {
            const auto prefix = decodeUiText(
                bytes.substr(0, std::min(caret_byte, bytes.size())));
            const SIZE prefix_extent = measureUiText(prefix);
            wchar_t caret = L'|';
            RECT caret_rect{x + prefix_extent.cx - 2, line_y,
                            x + prefix_extent.cx + 8,
                            line_y + request.height};
            caret_drawn = g_renderer->RenderFont(
                g_hudFont, reinterpret_cast<TCHAR*>(&caret), 1, &caret_rect,
                0xFFFFFFFFu, CHAR_CODE_TYPE_UNICODE, 3, 0);
        }
        return drawn != FALSE || caret_drawn != FALSE;
    };

    if (!request.multiline) {
        return draw_line(request.text, request.y, request.caret_byte);
    }

    bool drew_any = false;
    std::size_t begin = 0;
    std::int32_t y = request.y;
    while (begin <= request.text.size()) {
        const auto end = request.text.find('\n', begin);
        auto line = request.text.substr(
            begin, end == std::string_view::npos ? request.text.size() - begin
                                                  : end - begin);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        drew_any = draw_line(line, y,
            mxh::ui::TextRenderRequest::NoCaret) || drew_any;
        if (end == std::string_view::npos) break;
        begin = end + 1;
        y += 16;
    }
    return drew_any;
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

    g_renderer->UpdateWindowSize();
    RECT clientRect{};
    GetClientRect(h, &clientRect);
    g_logicalViewport.update(clientRect.right - clientRect.left,
                             clientRect.bottom - clientRect.top);
    SetGXLogicalScreenSize(
        g_renderer,
        static_cast<std::uint16_t>(mxh::client::LogicalViewport::kLogicalWidth),
        static_cast<std::uint16_t>(mxh::client::LogicalViewport::kLogicalHeight));
    const auto& content = g_logicalViewport.content_rect();
    SHORT_RECT contentRect{
        static_cast<short>(content.x),
        static_cast<short>(content.y),
        static_cast<short>(content.x + content.width),
        static_cast<short>(content.y + content.height),
    };
    g_renderer->BeginRender(
        content.width > 0 && content.height > 0 ? &contentRect : nullptr,
        0xff000000, 0);

    if (g_renderTerrain && g_terrain) {
        g_terrain->configureCamera(800.0f / 600.0f);
        if (!g_overviewCamera && g_skyScene) g_skyScene->render();
        g_terrain->render();
        if (g_staticScene) g_staticScene->render();
        if (g_entityScene) {
            // Push the terrain's view-projection as a Frustum so the
            // entity scene can cull NPCs whose world AABB is outside the
            // view volume. G5 M-R5: see mxh/render/frustum.hpp for the
            // Gribb-Hartmann plane extraction.
            g_entityScene->setCameraFrustum(mxh::gx::Frustum(g_terrain->viewProj()));
            g_entityScene->render();
        }

        // GameIn UI is the original InterfaceScript tree.  The old geometric
        // placeholder HUD remains available only with --debug-ui-bounds.
        if (g_inputTarget && g_inputTarget->is_in_game()) {
            g_renderer->SetScreenSpaceProjection();
            const auto& info = g_inputTarget->game_info();
            if (g_debugUiBounds && g_hud.barBg && g_hud.hpFill &&
                g_hud.mpFill) {
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

            // Attack visual flash: semi-transparent red screen overlay
            // (g_hud.hpFill is a 1x1 solid-color sprite; tint it with ARGB 0x40FF0000).
            const auto flashAge = g_inputTarget->attack_flash_age_ms();
            if (flashAge > 0 && flashAge <= 200) {
                drawSpriteQuad(g_renderer, g_hud.hpFill,
                               0.0f, 0.0f, 800.0f, 600.0f,
                               0x40FF0000u);  // alpha=0x40 (~25%), red
            }

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
                                 static_cast<LONG>(mxh::client::kShopPanelY - kTitleH + 6.0f),
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

            if (g_inputTarget->quest_open() && g_hudFont) {
                drawSpriteQuad(g_renderer, g_hud.barBg, 500.0f, 90.0f,
                               270.0f, 120.0f, 0xFFFFFFFFu);
                const auto* selected = g_inputTarget->selected_quest();
                const auto wideTitle = selected
                    ? mxh::compat::big5_to_utf16(selected->title) : std::wstring{};
                RECT titleRect{515, 108, 755, 132};
                if (!wideTitle.empty()) {
                    g_renderer->RenderFont(g_hudFont,
                        reinterpret_cast<TCHAR*>(const_cast<wchar_t*>(wideTitle.data())),
                        static_cast<std::uint32_t>(wideTitle.size()), &titleRect,
                        0xFFFFD080u, CHAR_CODE_TYPE_UNICODE, 2, 0);
                }
                const std::array<std::string, 3> lines{
                    g_inputTarget->quest_status(),
                    "Up/Down select   J accept   K claim",
                    "Q close"};
                for (std::size_t i = 0; i < lines.size(); ++i) {
                    RECT rc{515, static_cast<LONG>(138 + i * 24), 755,
                            static_cast<LONG>(162 + i * 24)};
                    g_renderer->RenderFont(g_hudFont,
                        const_cast<char*>(lines[i].data()),
                        static_cast<std::uint32_t>(lines[i].size()), &rc,
                        0xFFFFFFFFu, CHAR_CODE_TYPE_ASCII, 2, 0);
                }
            }
            }

            // Static NPC markers (click to talk / open their shop).
            // M-NPC1: per-role colour + "!" quest indicator + Big5 name.
            for (const auto& npc : g_inputTarget->npcs()) {
                float sx = 0;
                float sy = 0;
                if (!mxh::client::project_npc_to_screen(
                        info.position_x, info.position_z,
                        g_inputTarget->camera_yaw(),
                        static_cast<float>(npc.position_x),
                        static_cast<float>(npc.position_z),
                        sx, sy)) {
                    continue;
                }
                if (sx < -20.0f || sx > 820.0f || sy < -20.0f ||
                    sy > 620.0f) {
                    continue;
                }
                const std::size_t slot = npc_marker_slot(npc.npc_kind);
                if (slot < HudSprites::kNpcMarkerCount && g_hud.npcMarkers[slot]) {
                    drawSpriteQuad(g_renderer, g_hud.npcMarkers[slot],
                                   sx - 6.0f, sy - 6.0f, 12.0f, 12.0f,
                                   0xFFFFFFFFu);
                }
                // Quest indicator: yellow "!" 6px above the marker for
                // TALKER_ROLE / WANTED_ROLE / SURYUN_ROLE NPCs.  Drawn as
                // a small filled quad + label so it's visible without
                // needing the original .tga sprite atlas.
                if (g_hud.questMark && g_hudFont &&
                    mxh::game::role_has_quest_indicator(
                        mxh::game::role_from_wire(npc.npc_kind))) {
                    drawSpriteQuad(g_renderer, g_hud.questMark,
                                   sx - 3.0f, sy - 18.0f, 6.0f, 6.0f,
                                   0xFFFFFFFFu);
                    constexpr const wchar_t kBang = L'!';
                    RECT bangRc{static_cast<LONG>(sx) - 6,
                                static_cast<LONG>(sy) - 32,
                                static_cast<LONG>(sx) + 6,
                                static_cast<LONG>(sy) - 18};
                    g_renderer->RenderFont(
                        g_hudFont,
                        reinterpret_cast<TCHAR*>(const_cast<wchar_t*>(&kBang)),
                        1u, &bangRc,
                        0xFFFFE040u, CHAR_CODE_TYPE_UNICODE, 1, 0);
                }
                if (g_hudFont && npc.name[0] != '\0') {
                    const std::string name(npc.name, 17);
                    const auto nul = name.find('\0');
                    const std::string label =
                        nul == std::string::npos ? name : name.substr(0, nul);
                    if (!label.empty()) {
                        // M-NPC1: NPC names in the legacy client are
                        // Big5-encoded MBCS.  The HUD font (g_hudFont) is
                        // created with CHINESEBIG5_CHARSET, so render
                        // through CHAR_CODE_TYPE_UNICODE after a Big5→UTF-16
                        // conversion; ASCII-only names also work since the
                        // first 128 codepoints are identical in Big5.
                        const auto wide = mxh::compat::big5_to_utf16(label);
                        if (!wide.empty()) {
                            RECT rc{static_cast<LONG>(sx) - 40,
                                    static_cast<LONG>(sy) - 20,
                                    static_cast<LONG>(sx) + 40,
                                    static_cast<LONG>(sy)};
                            g_renderer->RenderFont(
                                g_hudFont,
                                reinterpret_cast<TCHAR*>(const_cast<wchar_t*>(wide.data())),
                                static_cast<std::uint32_t>(wide.size()), &rc,
                                0xFFFFFFFFu, CHAR_CODE_TYPE_UNICODE, 1, 0);
                        }
                    }
                }
            }
            g_inputTarget->ui_runtime().render();
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

    if (!g_renderTerrain && g_hud.barBg && g_hudFont) {
        g_renderer->SetScreenSpaceProjection();
        const auto drawText = [&](const std::string& value, LONG left, LONG top,
                                  std::uint32_t color = 0xFFFFFFFFu) {
            RECT rc{left, top, 535, top + 24};
            g_renderer->RenderFont(g_hudFont, const_cast<char*>(value.data()),
                                   static_cast<std::uint32_t>(value.size()), &rc,
                                   color, CHAR_CODE_TYPE_ASCII, 2, 0);
        };
        if (g_mainTitle) {
            g_mainTitle->ui_runtime().render();
            if (g_debugUiBounds) {
                for (const auto& d : g_mainTitle->ui_dialogs()) {
                    if (!d) continue;
                    drawHudBar(g_renderer, g_hud.barBg, g_hud.barBg,
                               static_cast<float>(d->absX()),
                               static_cast<float>(d->absY()),
                               static_cast<float>(d->width()),
                               static_cast<float>(d->height()),
                               0.85f);
                }
            }
        } else {
            // CharSelect / CharMake / GameLoading view.
            const auto cur_state = __g_currentState;
            // Quadrant trace for debugging — emits at most once per state.
            static int last_state_logged = -2;
            if (cur_state != last_state_logged) {
                MLOG_INFO("mxh_client: renderFrame state=%d g_charSelectState=%p",
                          cur_state, (void*)g_charSelectState);
                last_state_logged = cur_state;
            }
            if (cur_state == static_cast<int>(mxh::client::GameStateId::CharSelect)) {
                if (g_debugUiBounds) {
                    drawHudBar(g_renderer, g_hud.barBg, g_hud.barBg,
                               145.0f, 110.0f, 510.0f, 380.0f, 1.0f);
                    drawText("SELECT CHARACTER", 320, 130, 0xFFFFD080u);
                }
                // M-R7.1 (2026-08-20): if CharSelectDlg.bin cDialog tree
                // is loaded (12 child widgets: 5 char slots, 4 buttons,
                // 3 statics), draw its bounding rect + child rects so
                // the user sees the 1:1 UI shape instead of an empty
                // HUD bar.  Headless 端 — no sprite yet, just outlines.
                if (g_charSelectState) {
                    g_charSelectState->ui_runtime().render();
                    const auto& dlgs_cs = g_charSelectState->ui_dialogs();
                    if (g_debugUiBounds && !dlgs_cs.empty()) {
                        for (const auto& d : dlgs_cs) {
                            if (!d) continue;
                            drawHudBar(g_renderer, g_hud.barBg, g_hud.barBg,
                                       static_cast<float>(d->absX()),
                                       static_cast<float>(d->absY()),
                                       static_cast<float>(d->width()),
                                       static_cast<float>(d->height()),
                                       0.85f);
                            for (std::size_t i = 0; i < d->childCount(); ++i) {
                                mxh::ui::cWindow* c = d->childAt(i);
                                if (!c) continue;
                                drawHudBar(g_renderer, g_hud.barBg, g_hud.barBg,
                                           static_cast<float>(c->absX()),
                                           static_cast<float>(c->absY()),
                                           static_cast<float>(c->width()),
                                           static_cast<float>(c->height()),
                                           0.4f);
                            }
                        }
                    }
                }
                if (g_debugUiBounds && g_charSelectState) {
                    const std::vector<mxh::client::CharacterSlot>& chars = g_charSelectState->character_list();
                    if (chars.empty()) {
                        drawText("Connecting to AgentServer...", 200, 200, 0xFFA0A0A0u);
                    } else {
                        LONG y = 200;
                        for (std::size_t i = 0; i < chars.size(); ++i) {
                            const auto& s = chars[i];
                            const std::string label = s.valid
                                ? ("  [" + std::to_string(i) + "] chrid=" + std::to_string(s.chrid))
                                : ("  [" + std::to_string(i) + "] (empty)");
                            const std::uint32_t color = s.valid
                                ? (s.chrid == g_charSelectState->selected_chrid()
                                    ? 0xFF80FF80u : 0xFFFFFFFFu)
                                : 0xFF606060u;
                            drawText(label, 200, y, color);
                            y += 28;
                        }
                        if (g_charSelectState->selected_chrid() != 0 &&
                            g_charSelectState->selected_map() == 0) {
                            drawText("-> awaiting CharacterSelectAck...", 200, y + 12,
                                     0xFFFFD080u);
                        }
                    }
                }
            } else if (cur_state == static_cast<int>(mxh::client::GameStateId::CharMake)) {
                if (g_debugUiBounds) {
                    drawHudBar(g_renderer, g_hud.barBg, g_hud.barBg,
                               145.0f, 110.0f, 510.0f, 380.0f, 1.0f);
                    drawText("CREATE CHARACTER", 320, 130, 0xFFFFD080u);
                }
                // M-R7.1 (2026-08-20): if CharMakeNewDlg.bin cDialog
                // tree is loaded (49 children: 5 class pages, 12
                // race toggles, 16 statics), draw its bounding rect +
                // child rects so the user sees the 1:1 UI shape
                // instead of an empty bar.
                if (g_charMakeState) {
                    g_charMakeState->ui_runtime().render();
                    const auto& dlgs = g_charMakeState->ui_dialogs();
                    if (g_debugUiBounds && !dlgs.empty()) {
                        for (const auto& d : dlgs) {
                            if (!d) continue;
                            drawHudBar(g_renderer, g_hud.barBg, g_hud.barBg,
                                       static_cast<float>(d->absX()),
                                       static_cast<float>(d->absY()),
                                       static_cast<float>(d->width()),
                                       static_cast<float>(d->height()),
                                       0.85f);
                            for (std::size_t i = 0; i < d->childCount(); ++i) {
                                mxh::ui::cWindow* c = d->childAt(i);
                                if (!c) continue;
                                drawHudBar(g_renderer, g_hud.barBg, g_hud.barBg,
                                           static_cast<float>(c->absX()),
                                           static_cast<float>(c->absY()),
                                           static_cast<float>(c->width()),
                                           static_cast<float>(c->height()),
                                           0.4f);
                            }
                        }
                    }
                }
                if (g_debugUiBounds) {
                    drawText("0 slots in use; auto-create kicked in", 200, 200,
                             0xFFFFFFFFu);
                }
            } else if (cur_state == static_cast<int>(mxh::client::GameStateId::GameLoading)) {
                drawText("Entering game...", 290, 240, 0xFF80FF80u);
            }
        }
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

    // Lightweight FPS probe: log every 60 frames (≈1 s at the 60 fps
    // target).  Used by the manual render benchmark for the title/login
    // path so we can verify the login.dds V-flip doesn't cost us frame
    // budget.
    static std::uint32_t kFpsLogInterval = 60u;
    static std::uint32_t s_frame_count = 0;
    static std::uint64_t s_fps_window_start_ms = 0;
    ++s_frame_count;
    if (s_fps_window_start_ms == 0) {
        s_fps_window_start_ms = GetTickCount64();
        return;
    }
    if (s_frame_count >= kFpsLogInterval) {
        const auto now = GetTickCount64();
        const auto elapsed_ms = now - s_fps_window_start_ms;
        const double fps = (elapsed_ms > 0)
            ? (1000.0 * static_cast<double>(s_frame_count) /
               static_cast<double>(elapsed_ms))
            : 0.0;
        MLOG_INFO("mxh_client: render fps=%.1f frames=%u elapsed_ms=%llu state=%d",
                  fps, s_frame_count,
                  static_cast<unsigned long long>(elapsed_ms),
                  __g_currentState);
        s_frame_count = 0;
        s_fps_window_start_ms = now;
    }
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
    case WM_SIZE:
        g_logicalViewport.update(
            static_cast<std::int32_t>(LOWORD(l)),
            static_cast<std::int32_t>(HIWORD(l)));
        if (g_renderer) g_renderer->UpdateWindowSize();
        return 0;
    case WM_KEYDOWN:
        if (g_mainTitle) {
            if (w == VK_ESCAPE) {
                mxh::client::g_running = false;
                PostQuitMessage(0);
                return 0;
            }
            if (g_mainTitle->OnKeyEvent(true, static_cast<std::uint32_t>(w))) {
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
        }
        if (g_charSelectState &&
            g_charSelectState->OnKeyEvent(true, static_cast<std::uint32_t>(w))) {
            return 0;
        }
        if (g_charMakeState &&
            g_charMakeState->OnKeyEvent(true, static_cast<std::uint32_t>(w))) {
            return 0;
        }
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
        if (g_charSelectState &&
            g_charSelectState->OnKeyEvent(false, static_cast<std::uint32_t>(w))) {
            return 0;
        }
        if (g_charMakeState &&
            g_charMakeState->OnKeyEvent(false, static_cast<std::uint32_t>(w))) {
            return 0;
        }
        if (g_inputTarget) {
            g_inputTarget->OnKeyEvent(false, static_cast<std::uint32_t>(w));
        }
        return 0;
    case WM_CHAR:
        if (g_mainTitle &&
            g_mainTitle->OnChar(static_cast<std::uint32_t>(w))) {
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }
        if (g_charSelectState &&
            g_charSelectState->OnChar(static_cast<std::uint32_t>(w))) {
            return 0;
        }
        if (g_charMakeState &&
            g_charMakeState->OnChar(static_cast<std::uint32_t>(w))) {
            return 0;
        }
        if (g_inputTarget) {
            g_inputTarget->OnChar(static_cast<std::uint32_t>(w));
        }
        return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        {
        const auto logical = g_logicalViewport.to_logical(
            static_cast<std::int32_t>(static_cast<short>(LOWORD(l))),
            static_cast<std::int32_t>(static_cast<short>(HIWORD(l))));
        if (!logical.has_value()) return 0;
        const auto x = static_cast<std::int32_t>(logical->x);
        const auto y = static_cast<std::int32_t>(logical->y);
        if (g_mainTitle &&
            g_mainTitle->OnMouseButton(true, m == WM_LBUTTONDOWN, x, y)) {
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }
        if (g_charSelectState &&
            g_charSelectState->OnMouseButton(
                true, m == WM_LBUTTONDOWN, x, y)) {
            return 0;
        }
        if (g_charMakeState &&
            g_charMakeState->OnMouseButton(
                true, m == WM_LBUTTONDOWN, x, y)) {
            return 0;
        }
        if (g_inputTarget) {
            g_inputTarget->OnMouseButton(
                true, m == WM_LBUTTONDOWN,
                x, y);
        }
        return 0;
        }
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        {
        const auto logical = g_logicalViewport.to_logical(
            static_cast<std::int32_t>(static_cast<short>(LOWORD(l))),
            static_cast<std::int32_t>(static_cast<short>(HIWORD(l))));
        if (!logical.has_value()) return 0;
        if (g_charSelectState &&
            g_charSelectState->OnMouseButton(
                false, m == WM_RBUTTONDOWN,
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y))) {
            return 0;
        }
        if (g_charMakeState &&
            g_charMakeState->OnMouseButton(
                false, m == WM_RBUTTONDOWN,
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y))) {
            return 0;
        }
        if (g_inputTarget) {
            g_inputTarget->OnMouseButton(
                false, m == WM_RBUTTONDOWN,
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y));
        }
        return 0;
        }
    case WM_MOUSEMOVE:
        {
        const auto logical = g_logicalViewport.to_logical(
            static_cast<std::int32_t>(static_cast<short>(LOWORD(l))),
            static_cast<std::int32_t>(static_cast<short>(HIWORD(l))));
        if (!logical.has_value()) return 0;
        if (g_mainTitle &&
            g_mainTitle->OnMouseMove(
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y))) {
            return 0;
        }
        if (g_charSelectState &&
            g_charSelectState->OnMouseMove(
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y))) {
            return 0;
        }
        if (g_charMakeState &&
            g_charMakeState->OnMouseMove(
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y))) {
            return 0;
        }
        if (g_inputTarget) {
            g_inputTarget->OnMouseMove(
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y));
        }
        return 0;
        }
    default:
        return DefWindowProc(h, m, w, l);
    }
}

// ---------------------------------------------------------------------------
// WinMain. 1:1 mirrors MHClient.cpp's WinMain order.
// ---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE /*hPrev*/, LPSTR /*cmd*/, int /*show*/) {
    ClientOptions options = parse_client_options();
    const auto settings_path = mxh::client::ClientSettingsStore::default_path();
    std::string settings_warning;
    const auto persisted_settings = mxh::client::ClientSettingsStore::load(
        settings_path, &settings_warning);
    if (!settings_warning.empty()) MLOG_WARN("mxh_client: %s", settings_warning.c_str());
    if (options.post_login_width == 1024) options.post_login_width = persisted_settings.post_login_width;
    if (options.post_login_height == 768) options.post_login_height = persisted_settings.post_login_height;
    if (options.auto_login && (options.username.empty() || options.password.empty())) {
        std::fprintf(stderr, "mxh_client: --auto-login requires --username and --password\n");
        return 2;
    }
#ifdef NDEBUG
    if (options.auto_login || options.auto_create || !options.username.empty() ||
        !options.password.empty() || options.exit_after_gamein ||
        options.smoke_settle_frames != 0 || !options.state_frames_dir.empty()) {
        std::fprintf(stderr,
                     "mxh_client: automation and credential command-line options are disabled in release builds\n");
        return 2;
    }
#endif
    g_overviewCamera = !options.save_frame.empty() && !options.follow_camera;
    g_debugUiBounds = options.debug_ui_bounds;
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

    const std::uint32_t win_w = options.window_width  ? options.window_width  : mxh::client::kDefaultWindowWidth;
    const std::uint32_t win_h = options.window_height ? options.window_height : mxh::client::kDefaultWindowHeight;
    const std::uint32_t post_login_w = read_dimension_env(
        L"MXH_POST_LOGIN_WIDTH", options.post_login_width, 800, 7680);
    const std::uint32_t post_login_h = read_dimension_env(
        L"MXH_POST_LOGIN_HEIGHT", options.post_login_height, 600, 4320);
    RECT initial_rect{0, 0, static_cast<LONG>(win_w), static_cast<LONG>(win_h)};
    AdjustWindowRectEx(&initial_rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    HWND hwnd = CreateWindowW(
        L"MoxianClientWnd", L"Moxian Client (modern)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        initial_rect.right - initial_rect.left,
        initial_rect.bottom - initial_rect.top,
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
    std::string resource_error;
    if (!validate_runtime_resources(options.resource_root, &resource_error)) {
        std::fprintf(stderr, "mxh_client: %s\n", resource_error.c_str());
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
    info.dwWidth       = win_w;
    info.dwHeight      = win_h;
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
    // M-NPC1: 4-role NPC palette (yellow / cyan / orange / purple) plus
    // a gray "other" slot.  Slot order = npc_marker_slot(NpcRole).
    g_hud.npcMarkers[0] = renderer->CreateSolidSpriteObject(0xFFE0E040u, 1, 1);  // Talker    — yellow
    g_hud.npcMarkers[1] = renderer->CreateSolidSpriteObject(0xFF00D7FFu, 1, 1);  // Dealer    — cyan  (matches old npcMark colour)
    g_hud.npcMarkers[2] = renderer->CreateSolidSpriteObject(0xFFFF9040u, 1, 1);  // Wanted    — orange
    g_hud.npcMarkers[3] = renderer->CreateSolidSpriteObject(0xFFB070FFu, 1, 1);  // MapChange — purple
    g_hud.npcMarkers[4] = renderer->CreateSolidSpriteObject(0xFFB0B0B0u, 1, 1);  // Other     — gray
    g_hud.questMark = renderer->CreateSolidSpriteObject(0xFFFFE040u, 1, 1);  // "!" tag above quest NPCs
    LOGFONT hudLf{};
    hudLf.lfHeight = -14;
    hudLf.lfWeight = FW_NORMAL;
    hudLf.lfCharSet = CHINESEBIG5_CHARSET;
    hudLf.lfQuality = ANTIALIASED_QUALITY;
    std::strncpy(hudLf.lfFaceName, "Microsoft JhengHei", LF_FACESIZE - 1);
    g_hudFont = renderer->CreateFontObject(&hudLf, 0);
    LOGFONTW measureLf{};
    measureLf.lfHeight = -14;
    measureLf.lfWeight = FW_NORMAL;
    measureLf.lfCharSet = CHINESEBIG5_CHARSET;
    measureLf.lfQuality = ANTIALIASED_QUALITY;
    std::wcsncpy(measureLf.lfFaceName, L"Microsoft JhengHei", LF_FACESIZE - 1);
    g_hudMeasureFont = CreateFontIndirectW(&measureLf);
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
    mxh::ui::bindTextRenderer(&textRenderAdapter, nullptr);

    // -------------------------------------------------------------------------
    // M-R1 / M-R2 / M-R3 — full legacy resource + UI load chain.
    //
    //   M-R1: cResourceManager 装载 7 张 hard path .bin (image_hard_path,
    //         image_item_path, image_mugong_path, image_ability_path,
    //         image_buff_path, image_minimap_path, image_jackpot_path).
    //   M-R2: cSpriteAtlas 装载 image_path.bin (184 entries, 老版老
    //         cResourceManager::Init(szImageListPath) 等价物).
    //   M-R3: cDialogLoader 装载 132 (实测 157) 个 InterfaceScript/*.bin
    //         → 解析 → 创建 cDialog → cWindowManager::AddDialog. 装载链
    //         是老版 cScriptManager::GetDlgInfoFromFile 的现代等价物。
    //
    // 装载全部在 renderer 启动之后、CMainGame 之前;cDialog 树装到全局
    // cWindowManager,后面 A.1.6 state machine 跟它交互。
    // -------------------------------------------------------------------------
    {
        using namespace mxh::ui;
        const auto image_dir = options.resource_root / "Image";
        if (!cResourceManager::getInstance().allLoaded()) {
            if (!cResourceManager::getInstance().InitScriptManager(image_dir)) {
                std::fprintf(stderr, "mxh_client: M-R1 cResourceManager init failed\n");
            } else {
                MLOG_INFO("mxh_client: M-R1 cResourceManager loaded (%zu total records)",
                          cResourceManager::getInstance().sizeOf(PathFileType::HardPath)
                          + cResourceManager::getInstance().sizeOf(PathFileType::ItemPath)
                          + cResourceManager::getInstance().sizeOf(PathFileType::MugongPath)
                          + cResourceManager::getInstance().sizeOf(PathFileType::AbilityPath)
                          + cResourceManager::getInstance().sizeOf(PathFileType::BuffPath)
                          + cResourceManager::getInstance().sizeOf(PathFileType::MiniMapPath)
                          + cResourceManager::getInstance().sizeOf(PathFileType::JackpotPath));
            }
        }
        if (!cSpriteAtlas::getInstance().loaded()) {
            cSpriteAtlas::getInstance().Init(options.resource_root);
        }
        // Sprite hook is registered here so CharSelect/CharMake/GameIn
        // ClientUiRuntime loads can CreateSpriteObject. Do NOT LoadAll
        // 157 InterfaceScript bins into an unused WindowManager — that
        // allocates one GPU texture per cImage and exhausts a 4GB Arc
        // B580 before the player reaches character select.
        struct SpriteLoaderCtx {
            I4DyuchiGXRenderer* renderer;
        };
        static SpriteLoaderCtx g_spriteCtx{renderer};
        auto spriteHook = [](void* ctx, const std::string& tif_path) -> void* {
            auto* lc = static_cast<SpriteLoaderCtx*>(ctx);
            if (!lc || !lc->renderer) return nullptr;
            IDISpriteObject* sp = lc->renderer->CreateSpriteObject(
                const_cast<char*>(tif_path.c_str()), 0);
            if (!sp) return nullptr;
            return static_cast<void*>(sp);
        };
        cDialogLoader::SetSpriteLoader(
            static_cast<mxh::ui::LoadSpriteFn>(spriteHook),
            static_cast<void*>(&g_spriteCtx));
        MLOG_INFO("mxh_client: M-R1/M-R2 ready; InterfaceScript loads on demand per state");
    }

    // All visible dialogs are owned by the active state/UI runtime.  Do not
    // create a synthetic placeholder dialog in the product path.

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
    if (!options.resource_root.empty()) {
        engine->SetPlaydhRoot(std::filesystem::path(options.resource_root));
    }
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
    mainGame.SetGameState(options.auto_login ? mxh::client::GameStateId::Connect
                                             : mxh::client::GameStateId::Title);

    MLOG_INFO("mxh_client: CMainGame initialised, 9 states registered, "
              "boot -> GameStateId::Connect");

    // Phase B.2.1: kick off the CLoginState connect (host-driven; the
    // state can't self-start because it doesn't know the login address
    // until MHVerInfo.ver parsing lands in B.2.5).
    if (options.auto_login) {
        if (auto* login = dynamic_cast<mxh::client::CLoginState*>(
                mainGame.GetGameState(mxh::client::GameStateId::Connect))) {
            login->Start(mainGame.GetEngine(), options.login_host,
                         options.login_port, options.username, options.password);
        } else {
            MLOG_ERROR("Connect slot is not a CLoginState; cannot start login");
        }
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
    std::string pending_loading_error;
    mxh::client::GameLoadingCoordinator loadingCoordinator;
    bool game_loading_frame_presented = false;
    bool post_login_display_applied = false;
    bool auto_create_requested = false;
    bool follow_frame_captured = false;
    unsigned follow_settle_frames = 0;
    MSG msg{};
    while (mxh::client::g_running) {
        bool quit_requested = false;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quit_requested = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (quit_requested) break;
            if (g_mainTitle && g_mainTitle->consumeSubmit()) {
                options.username = g_mainTitle->username();
                options.password = g_mainTitle->password();
                mainGame.SetGameState(mxh::client::GameStateId::Connect);
                if (auto* login = dynamic_cast<mxh::client::CLoginState*>(
                        mainGame.GetGameState(mxh::client::GameStateId::Connect))) {
                    login->Start(mainGame.GetEngine(), options.login_host,
                                 options.login_port, options.username, options.password);
                }
            }
            // Idle: drive CMainGame + render a frame.
            mainGame.Process();
            // Phase B.2.2: on state-change rising edge, Start() the
            // states that have an external Start() hook.
            const auto cur_state = mainGame.GetCurStateNum();
            if (cur_state != prev_state) {
                if (prev_state == mxh::client::GameStateId::GameIn) {
                    g_inputTarget = nullptr;
                }
                if (cur_state == mxh::client::GameStateId::Title) {
                    g_mainTitle = dynamic_cast<mxh::client::CMainTitle*>(
                        mainGame.GetGameState(cur_state));
                } else {
                    g_mainTitle = nullptr;
                }
                if (cur_state == mxh::client::GameStateId::CharSelect) {
                    g_charSelectState = dynamic_cast<mxh::client::CCharSelectState*>(
                        mainGame.GetGameState(cur_state));
                } else {
                    g_charSelectState = nullptr;
                }
                if (cur_state == mxh::client::GameStateId::CharMake) {
                    g_charMakeState = dynamic_cast<mxh::client::CCharMake*>(
                        mainGame.GetGameState(cur_state));
                } else {
                    g_charMakeState = nullptr;
                }
                if (cur_state == mxh::client::GameStateId::GameLoading) {
                    game_loading_frame_presented = false;
                    g_renderTerrain = false;
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
            if (cur_state == mxh::client::GameStateId::Title &&
                mainGame.GetEngine()->pending_transfer_is<
                    mxh::client::LoginResult>()) {
                // CLoginState routes through Title after LoginAck and
                // hands the LoginResult via the engine transfer slot.
                // The CharSelect state will pull it in its own Init().
                // Do NOT take it here or CharSelect sees an empty slot.
                bool display_transition_ok = true;
                if (!post_login_display_applied) {
                    RECT target{0, 0, static_cast<LONG>(post_login_w),
                                static_cast<LONG>(post_login_h)};
                    AdjustWindowRectEx(&target, WS_OVERLAPPEDWINDOW, FALSE, 0);
                    const auto resized = SetWindowPos(
                        hwnd, nullptr, 0, 0,
                        target.right - target.left,
                        target.bottom - target.top,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                    if (!resized) {
                        display_transition_ok = false;
                        MLOG_ERROR("mxh_client: post-login display transition failed error=%lu",
                                   GetLastError());
                    } else {
                        post_login_display_applied = true;
                        g_logicalViewport.update(
                            static_cast<std::int32_t>(post_login_w),
                            static_cast<std::int32_t>(post_login_h));
                        if (g_renderer) g_renderer->UpdateWindowSize();
                        InvalidateRect(hwnd, nullptr, FALSE);
                        MLOG_INFO("mxh_client: post-login display transition client=%ux%u",
                                  post_login_w, post_login_h);
                    }
                }
                if (display_transition_ok) {
                    g_mainTitle = nullptr;
                    mainGame.SetGameState(mxh::client::GameStateId::CharSelect);
                } else {
                    // Keep the LoginResult pending and remain on the login
                    // screen.  This is a recoverable display failure, not a
                    // successful login followed by a mismatched viewport.
                    pending_loading_error = "无法切换到保存的显示模式";
                }
                } else if (cur_state == mxh::client::GameStateId::Title) {
                    if (auto* title = dynamic_cast<mxh::client::CMainTitle*>(
                            mainGame.GetGameState(cur_state))) {
                        title->Start(mainGame.GetEngine(),
                                     options.username, options.password);
                        g_mainTitle = title;
                    }
                } else if (cur_state == mxh::client::GameStateId::CharSelect) {
                    if (auto* cs = dynamic_cast<mxh::client::CCharSelectState*>(
                            mainGame.GetGameState(cur_state))) {
                        cs->set_auto_select_for_test(options.auto_create);
                        cs->Start(mainGame.GetEngine());
                        if (!pending_loading_error.empty()) {
                            cs->ui_runtime().showMessage(
                                0x4D4C4552, pending_loading_error);
                            pending_loading_error.clear();
                        }
                    }
                } else if (cur_state == mxh::client::GameStateId::CharMake) {
                    if (auto* cm = dynamic_cast<mxh::client::CCharMake*>(
                            mainGame.GetGameState(cur_state))) {
                        cm->Start(mainGame.GetEngine());
                    }
                } else if (cur_state == mxh::client::GameStateId::GameIn) {
                    if (auto* g = dynamic_cast<mxh::client::CInGameState*>(
                            mainGame.GetGameState(cur_state))) {
                        auto questCatalog = mxh::compat::load_quest_string_catalog(
                            options.resource_root / "Resource" / "QuestScript" / "QuestString.bin");
                        if (!questCatalog.error_message.empty()) {
                            MLOG_WARN("mxh_client: QuestString unavailable: %s",
                                      questCatalog.error_message.c_str());
                        } else {
                            MLOG_INFO("mxh_client: original QuestString loaded entries=%zu main=%zu",
                                      questCatalog.entries.size(), questCatalog.main_quests().size());
                        }
                        g->set_quest_catalog(std::move(questCatalog));
                        g->Start(mainGame.GetEngine(), pending_character_id,
                                 pending_map_num);
                        g_inputTarget = g;
                    }
                }
                prev_state = cur_state;
            }
            // GameLoading consumer must run every frame when the state is
            // GameLoading, because the CCharSelectState TCP recv thread may
            // set the GameEntryRequest transfer AFTER the state-transition
            // edge fires. Restricting to the rising-edge check loses the
            // transfer and the state machine stalls.
            if (cur_state == mxh::client::GameStateId::GameLoading) {
                if (!game_loading_frame_presented) {
                    game_loading_frame_presented = true;
                } else if (!loadingCoordinator.has_request()) {
                    std::string transferError;
                    if (loadingCoordinator.consume_pending_transfer(
                            *mainGame.GetEngine(), &transferError)) {
                        if (auto* loading = dynamic_cast<mxh::client::CGameLoading*>(
                                mainGame.GetGameState(cur_state))) {
                            loading->set_context(&loadingCoordinator.context());
                        }
                        std::string loadingError;
                        if (loadGameWorld(options, renderer, storage, bgm,
                                          loadingCoordinator.request().map_num, loadingError)) {
                            loadingCoordinator.mark_completed(10);
                            pending_character_id = loadingCoordinator.request().character_id;
                            pending_map_num = loadingCoordinator.request().map_num;
                            mainGame.SetGameState(mxh::client::GameStateId::GameIn);
                        } else {
                            pending_loading_error = "Unable to enter Map " +
                                std::to_string(loadingCoordinator.request().map_num) + ": " + loadingError;
                            loadingCoordinator.mark_failed(pending_loading_error);
                            MLOG_ERROR("GameLoading: %s",
                                       pending_loading_error.c_str());
                            mainGame.SetGameState(
                                mxh::client::GameStateId::CharSelect);
                        }
                    } else if (transferError != "waiting for GameEntryRequest") {
                        pending_loading_error = transferError;
                        MLOG_ERROR("GameLoading: %s", pending_loading_error.c_str());
                        mainGame.SetGameState(
                            mxh::client::GameStateId::CharSelect);
                    }
                }
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
                    ++follow_settle_frames;
                    const auto required_frames = options.follow_camera
                        ? std::max<std::uint32_t>(20u, options.smoke_settle_frames)
                        : options.smoke_settle_frames;
                    if (follow_settle_frames >= required_frames) {
                        MLOG_INFO("mxh_client: GUI_SMOKE_PASS player_id=%u map=%u",
                                  game_in->player_id(), game_in->map_num());
                        mxh::client::g_running = false;
                    }
                }
            }
            if (cur_state == mxh::client::GameStateId::GameIn) {
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
                        mxh::gx::WorldSnapshot snapshot;
                        snapshot.entities.reserve(game_in->monsters().size() +
                                                  game_in->npcs().size());
                        for (const auto& monster : game_in->monsters()) {
                            snapshot.entities.push_back({monster.object_id, monster.monster_kind,
                                static_cast<float>(monster.position_x),
                                g_terrain->heightAt(monster.position_x, monster.position_z),
                                static_cast<float>(monster.position_z),
                                mxh::gx::SceneEntityType::Monster,
                                monster.facing_yaw, monster.current_life, 0,
                                monster.current_life == 0
                                    ? mxh::gx::SceneAction::Dead
                                    : (monster.moving
                                        ? mxh::gx::SceneAction::Moving
                                        : mxh::gx::SceneAction::Idle)});
                        }
                        for (const auto& npc : game_in->npcs()) {
                            snapshot.entities.push_back({npc.npc_id, npc.npc_kind,
                                static_cast<float>(npc.position_x),
                                g_terrain->heightAt(npc.position_x, npc.position_z),
                                static_cast<float>(npc.position_z),
                                mxh::gx::SceneEntityType::Npc, 0.0f, 0, 0,
                                mxh::gx::SceneAction::Idle});
                        }
                        snapshot.local_player = mxh::gx::ScenePlayer{
                            info.player_id, info.gender,
                            info.face_type, info.hair_type, info.weared_item_idx,
                            static_cast<float>(info.position_x),
                            g_terrain->heightAt(info.position_x, info.position_z),
                            static_cast<float>(info.position_z),
                            game_in->camera_yaw(), info.life, info.max_life,
                            info.life == 0
                                ? mxh::gx::SceneAction::Dead
                                : (game_in->is_moving()
                                    ? mxh::gx::SceneAction::Moving
                                    : mxh::gx::SceneAction::Idle)};
                        snapshot.remote_players.reserve(
                            game_in->remote_players().size());
                        for (const auto& [objectId, remote] :
                             game_in->remote_players()) {
                            if (!remote.appearance_known || !remote.visible) continue;
                            snapshot.remote_players.push_back({
                                objectId, remote.gender, remote.face_type,
                                remote.hair_type, remote.weared_item_idx,
                                static_cast<float>(remote.position_x),
                                g_terrain->heightAt(remote.position_x,
                                                    remote.position_z),
                                static_cast<float>(remote.position_z),
                                remote.facing_yaw, remote.life, remote.max_life,
                                remote.life == 0
                                    ? mxh::gx::SceneAction::Dead
                                    : (remote.moving
                                        ? mxh::gx::SceneAction::Moving
                                        : mxh::gx::SceneAction::Idle)});
                        }
                        g_entityScene->synchronize(snapshot);
                        static std::uint32_t last_loaded = ~0u;
                        static std::uint32_t last_failed = ~0u;
                        static std::uint32_t last_ph = ~0u;
                        const auto loaded = g_entityScene->loadedModelCount();
                        const auto failed = g_entityScene->failedModelCount();
                        const auto ph = g_entityScene->placeholderCount();
                        if (loaded != last_loaded || failed != last_failed ||
                            ph != last_ph) {
                            last_loaded = loaded;
                            last_failed = failed;
                            last_ph = ph;
                            MLOG_INFO(
                                "mxh_client: entity loaded=%u failed=%u "
                                "placeholders=%u monsters=%u npcs=%u players=%u",
                                loaded, failed, ph,
                                static_cast<unsigned>(game_in->monsters().size()),
                                static_cast<unsigned>(game_in->npcs().size()),
                                g_entityScene->playerInstanceCount());
                            if (ph != 0 && !g_debugUiBounds) {
                                MLOG_ERROR("mxh_client: release visual gate failed: "
                                           "placeholderCount=%u failedModelCount=%u",
                                           ph, failed);
                                mxh::client::g_running = false;
                            }
                        }
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

    MLOG_INFO("mxh_client: shutting down");

    if (!settings_path.empty()) {
        auto settings = persisted_settings;
        settings.post_login_width = post_login_w;
        settings.post_login_height = post_login_h;
        settings.last_account = options.username;
        std::string settings_error;
        if (!mxh::client::ClientSettingsStore::save_atomic(settings_path, settings,
                                                           &settings_error)) {
            MLOG_WARN("mxh_client: settings save failed: %s", settings_error.c_str());
        }
    }

    g_inputTarget = nullptr;
    mainGame.Release();
    g_renderTerrain = false;
    g_terrain.reset();
    g_staticScene.reset();
    g_skyScene.reset();
    g_entityScene.reset();
    mxh::ui::bindTextRenderer(nullptr, nullptr);
    if (g_hudMeasureFont) {
        DeleteObject(g_hudMeasureFont);
        g_hudMeasureFont = nullptr;
    }

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
