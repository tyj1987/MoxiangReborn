// MoxianClient: modern Moxian (DarkStory) client entry point.
//
// Bootstrap order follows the legacy client while using the modern runtime:
// create the Win32 host surface, mount the selected PlayDH profile, initialise
// DX11/UI/audio services, register the complete client state table, and drive
// the state machine from the message loop.
//
// 1:1 quirks preserved:
//   - The 800x600 default window size matches the legacy MHClient default.
//   - The legacy WinMain order — instance handle → class register → window
//     create → renderer init → message loop — is preserved.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <array>
#include <string>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
#include <chrono>
#include <cmath>
#include <vector>
#include <unordered_map>

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
#include "mxh/client/CharacterPreviewController.hpp"
#include "mxh/compat/map_change_catalog.hpp"
#include "mxh/render/render_typedef.hpp"
#include "mxh/game/npc_role.hpp"  // M-NPC1: per-role NPC marker slot + quest indicator
#include "mxh/ui/cImage.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/ui/cResourceManager.hpp"
#include "mxh/ui/cSpriteAtlas.hpp"
#include "mxh/ui/coptiondialog.hpp"
#include "mxh/ui/cWindowManager.hpp"
#include "TextRender.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/audio/bgm_player.hpp"
#include "mxh/audio/sfx_player.hpp"
#include "mxh/compat/bmhm_map.hpp"
#include "mxh/compat/ttb_tile_table.hpp"
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

// Version string mirrors the legacy g_CLIENTVERSION[32].  CMainTitle loads
// MHVerInfo.ver when it is present and retains this safe fallback otherwise.
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
    std::string username_env;
    std::string password_env;
    bool auto_login = false;
    bool auto_create = false;
    bool release_automation_requested = false;
    bool exit_after_gamein = false;
    std::uint32_t smoke_settle_frames = 0;
    bool follow_camera = false;
    bool debug_ui_bounds = false;
    std::string character_name = "ModernHero";
    std::string resource_profile_id = "playdh-current";
    std::filesystem::path resource_root;
    std::string save_frame;
    std::string state_frames_dir;
    std::string evidence_dir;
    // M-R7 (G3) 物理 GPU 段: --width/--height 控制 swap chain + 截屏尺寸.
    // 默认 0 = 用 kDefaultWindowWidth/Height (800x600). 4 档: 800/1024/1920/2560.
    std::uint32_t window_width = 0;
    std::uint32_t window_height = 0;
    std::uint32_t post_login_width = 1024;
    std::uint32_t post_login_height = 768;
    bool borderless = false;
    bool vsync = true;
    bool resource_profile_overridden = false;
    bool post_login_width_overridden = false;
    bool post_login_height_overridden = false;
    bool borderless_overridden = false;
    bool vsync_overridden = false;
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
#if defined(MXH_DEV_AUTOMATION)
        else if (arg == L"--username") take(options.username);
        else if (arg == L"--password") take(options.password);
        else if (arg == L"--username-env") take(options.username_env);
        else if (arg == L"--password-env") take(options.password_env);
        else if (arg == L"--auto-login") options.auto_login = true;
        else if (arg == L"--auto-create") options.auto_create = true;
        else if (arg == L"--exit-after-gamein") options.exit_after_gamein = true;
        else if (arg == L"--smoke-settle-frames" && i + 1 < argc)
            options.smoke_settle_frames = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        else if (arg == L"--follow-camera") options.follow_camera = true;
        else if (arg == L"--debug-ui-bounds") options.debug_ui_bounds = true;
        else if (arg == L"--character-name") take(options.character_name);
#endif
        else if (arg == L"--resource-root" && i + 1 < argc) {
            options.resource_root = argv[++i];
        }
        else if (arg == L"--resource-profile" && i + 1 < argc) {
            take(options.resource_profile_id);
            options.resource_profile_overridden = true;
        }
#if defined(MXH_DEV_AUTOMATION)
        else if (arg == L"--save-frame") take(options.save_frame);
        else if (arg == L"--state-frames-dir") take(options.state_frames_dir);
        else if (arg == L"--evidence-dir") take(options.evidence_dir);
#endif
        else if (arg == L"--width" && i + 1 < argc)
            options.window_width = static_cast<std::uint32_t>(std::wcstoul(argv[++i], nullptr, 10));
        else if (arg == L"--height" && i + 1 < argc)
            options.window_height = static_cast<std::uint32_t>(std::wcstoul(argv[++i], nullptr, 10));
        else if (arg == L"--login-width" && i + 1 < argc)
            options.window_width = static_cast<std::uint32_t>(std::wcstoul(argv[++i], nullptr, 10));
        else if (arg == L"--login-height" && i + 1 < argc)
            options.window_height = static_cast<std::uint32_t>(std::wcstoul(argv[++i], nullptr, 10));
        else if (arg == L"--post-width" && i + 1 < argc)
        {
            options.post_login_width = static_cast<std::uint32_t>(std::wcstoul(argv[++i], nullptr, 10));
            options.post_login_width_overridden = true;
        }
        else if (arg == L"--post-height" && i + 1 < argc)
        {
            options.post_login_height = static_cast<std::uint32_t>(std::wcstoul(argv[++i], nullptr, 10));
            options.post_login_height_overridden = true;
        }
        else if (arg == L"--borderless") {
            options.borderless = true;
            options.borderless_overridden = true;
        }
        else if (arg == L"--no-vsync") { options.vsync = false; options.vsync_overridden = true; }
        else if (arg == L"--vsync") { options.vsync = true; options.vsync_overridden = true; }
#if !defined(MXH_DEV_AUTOMATION)
        else if (arg == L"--auto-login" || arg == L"--auto-create" ||
                 arg == L"--username" || arg == L"--password" ||
                 arg == L"--username-env" || arg == L"--password-env" ||
                 arg == L"--character-name" || arg == L"--exit-after-gamein" ||
                 arg == L"--smoke-settle-frames" || arg == L"--follow-camera" ||
                 arg == L"--debug-ui-bounds" || arg == L"--save-frame" ||
                 arg == L"--state-frames-dir" || arg == L"--evidence-dir") {
            options.release_automation_requested = true;
            if ((arg == L"--username" || arg == L"--password" ||
                 arg == L"--username-env" || arg == L"--password-env" ||
                 arg == L"--character-name" || arg == L"--smoke-settle-frames" ||
                 arg == L"--save-frame" || arg == L"--state-frames-dir" ||
                 arg == L"--evidence-dir") && i + 1 < argc) {
                ++i;
            }
        }
#endif
    }
    LocalFree(argv);
    const auto read_env = [](const std::string& name) {
        if (name.empty()) return std::string{};
        const auto wide_name = std::wstring(name.begin(), name.end());
        wchar_t value[4096]{};
        const auto length = GetEnvironmentVariableW(
            wide_name.c_str(), value, static_cast<DWORD>(std::size(value)));
        return length == 0 || length >= std::size(value)
            ? std::string{}
            : narrow_ascii(value);
    };
    if (!options.username_env.empty()) options.username = read_env(options.username_env);
    if (!options.password_env.empty()) options.password = read_env(options.password_env);
    return options;
}

std::filesystem::path find_playdh_root() {
    std::error_code ec;
    auto base = std::filesystem::current_path(ec);
    for (int depth = 0; !base.empty() && depth < 8; ++depth) {
        // Only accept canonical runtime locations.  Never probe an arbitrary
        // `PlayDH` child directory: compatibility junctions and recovered
        // reference trees must be selected explicitly through a profile/root.
        const std::filesystem::path candidates[] = {
            base / "modern" / "data" / "PlayDH",
            base / "data" / "PlayDH"
        };
        for (const auto& candidate : candidates) {
            if (std::filesystem::is_directory(candidate, ec)) return candidate;
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
        // The current PlayDH profile ships the production character-creation
        // tree as CharMakeNewDlg.bin; CharMakeDlg.bin is a historical name
        // from a different resource generation and must not block startup.
        "Image/InterfaceScript/CharMakeNewDlg.bin",
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

#if 0 // historical null storage retained only as an audit reference; never compiled
/* Legacy null storage removed: production mounts FilesystemFileStorage. */
class StubFileStorage_REMOVED : public I4DyuchiFileStorage {
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
#endif

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
// Character-select uses the same real model/catalog pipeline as GameIn, but
// with a dedicated camera and no map dependency.  Keeping it separate avoids
// a RenderBox/debug avatar and lets the map scene be replaced transactionally.
std::unique_ptr<mxh::gx::EntityScene> g_charPreviewScene;
mxh::client::CharacterPreviewController g_charPreviewController;
bool g_renderTerrain = false;
std::string g_captureTerrainFrame;
bool g_overviewCamera = false;
bool g_debugUiBounds = false;
std::string __g_stateFramesDir;
int __g_currentState = -1;
std::string __g_pendingStateFrame;
std::string g_evidenceDir;
std::uint64_t g_evidenceFrameSequence = 0;

// Active in-game input target. The WndProc forwards keyboard/mouse events
// to the current game state (only CInGameState consumes input today).
mxh::client::CInGameState* g_inputTarget = nullptr;
std::optional<mxh::compat::MapChangeCatalog> g_mapChangeCatalog;
mxh::client::CCharSelectState* g_charSelectState = nullptr;
mxh::client::CCharMake*        g_charMakeState   = nullptr;  // M-R7.1 (2026-08-20)
mxh::client::CGameLoading*     g_gameLoadingState = nullptr;
mxh::client::CMapChange*       g_mapChangeState = nullptr;
mxh::client::GameLoadingCoordinator* g_loadingCoordinator = nullptr;
mxh::client::CMainTitle*       g_mainTitle       = nullptr;
mxh::client::LogicalViewport   g_logicalViewport;
mxh::audio::SfxPlayer* g_sfxPlayer = nullptr;
mxh::audio::BgmPlayer* g_bgmPlayer = nullptr;
struct EffectVisualOverlay;
EffectVisualOverlay* g_effectVisuals = nullptr;
std::uint16_t g_uiClickSound = 0xffffu;
std::uint16_t g_attackSound = 0xffffu;
std::uint16_t g_skillSound = 0xffffu;
std::uint16_t g_pickupSound = 0xffffu;
float g_uiVolume = 1.0f;
float g_bgmVolume = 1.0f;
float g_sfxVolume = 1.0f;
bool g_audioFocused = true;

struct OptionRuntimeContext {
    mxh::client::ClientSettingsV1* settings = nullptr;
    mxh::audio::BgmPlayer* bgm = nullptr;
    mxh::audio::SfxPlayer* sfx = nullptr;
};

void option_default_callback(mxh::ui::sGAMEOPTION* option, void* user) {
    const auto* ctx = static_cast<const OptionRuntimeContext*>(user);
    if (!option || !ctx || !ctx->settings) return;
    option->bSoundBGM = ctx->settings->bgm_volume > 0.0f;
    option->bSoundEnvironment = ctx->settings->ambient_volume > 0.0f;
    option->nVolumnBGM = static_cast<int>(std::lround(ctx->settings->bgm_volume * 100.0f));
    option->nVolumnEnvironment = static_cast<int>(std::lround(ctx->settings->ambient_volume * 100.0f));
}

void option_apply_callback(mxh::ui::sGAMEOPTION* option, void* user) {
    auto* ctx = static_cast<OptionRuntimeContext*>(user);
    if (!option || !ctx || !ctx->settings) return;
    const auto normalized = [](int value) {
        return std::clamp(static_cast<float>(value) / 100.0f, 0.0f, 1.0f);
    };
    ctx->settings->bgm_volume = normalized(option->nVolumnBGM);
    ctx->settings->ambient_volume = normalized(option->nVolumnEnvironment);
    g_bgmVolume = option->bSoundBGM ? ctx->settings->bgm_volume : 0.0f;
    if (ctx->bgm) ctx->bgm->setVolume(g_audioFocused ? g_bgmVolume : 0.0f);
    g_sfxVolume = option->bSoundEnvironment ? ctx->settings->sfx_volume : 0.0f;
    if (ctx->sfx) ctx->sfx->setVolume(g_audioFocused ? g_sfxVolume : 0.0f);
}

void configureCharacterPreviewCamera(I4DyuchiGXRenderer* renderer,
                                     float aspect,
                                     const mxh::client::CharacterPreviewController& controller) {
    if (!renderer) return;
    mxh::gx::CAMERA_DESC camera{};
    const float yaw = controller.yaw();
    const float distance = controller.distance();
    camera.v3From = {std::sin(yaw) * distance, 1.8f,
                     -std::cos(yaw) * distance};
    camera.v3To = {0.0f, 1.0f, 0.0f};
    camera.v3Up = {0.0f, 1.0f, 0.0f};
    camera.fFovY = mxh::gx::PI / 3.0f;
    camera.fAspect = aspect > 0.01f ? aspect : (4.0f / 3.0f);
    camera.fNear = 0.1f;
    camera.fFar = 100.0f;
    mxh::gx::VECTOR3 forward{
        camera.v3To.x - camera.v3From.x,
        camera.v3To.y - camera.v3From.y,
        camera.v3To.z - camera.v3From.z};
    const float length = std::sqrt(forward.x * forward.x +
                                   forward.y * forward.y +
                                   forward.z * forward.z);
    if (length <= 0.001f) return;
    forward.x /= length; forward.y /= length; forward.z /= length;
    mxh::gx::MATRIX4 view{}, projection{}, billboard{};
    mxh::gx::MatrixLookAtLH(&view, &camera.v3From, &camera.v3To,
                             &camera.v3Up);
    const float f = 1.0f / std::tan(camera.fFovY * 0.5f);
    projection = mxh::gx::MatrixIdentity();
    projection._11 = f / camera.fAspect;
    projection._22 = f;
    projection._33 = camera.fFar / (camera.fFar - camera.fNear);
    projection._43 = -camera.fNear * camera.fFar /
                     (camera.fFar - camera.fNear);
    projection._34 = 1.0f;
    billboard = mxh::gx::MatrixIdentity();
    mxh::gx::VIEW_VOLUME volume{};
    volume.From = camera.v3From;
    volume.fFar = camera.fFar;
    renderer->SetViewFrusturm(&volume, &camera, &view, &projection,
                              &billboard);
}

void clear_secret(std::string& value) noexcept {
    volatile char* bytes = value.empty() ? nullptr : value.data();
    for (std::size_t i = 0; bytes && i < value.size(); ++i) bytes[i] = '\0';
    value.clear();
}

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

// Main-thread staged world activation.  CPU descriptors and GPU resources
// are intentionally committed one dependency stage per frame so the real
// loading dialog keeps receiving/process­ing window messages.  The old
// scene remains live until the final commit, which is required for MapChange
// rollback and avoids exposing half-built render state.
class GameWorldLoadSession {
public:
    enum class Result { Pending, Complete, Failed };

    GameWorldLoadSession(const ClientOptions& options,
                         I4DyuchiGXRenderer* renderer,
                         I4DyuchiFileStorage* storage,
                         mxh::audio::BgmPlayer& bgm,
                         std::uint16_t map_num)
        : options_(options), renderer_(renderer), storage_(storage),
          bgm_(bgm), map_num_(map_num),
          previous_render_terrain_(g_renderTerrain),
          previous_capture_frame_(g_captureTerrainFrame),
          previous_bgm_id_(bgm.currentSoundId()) {
        g_renderTerrain = false;
        g_captureTerrainFrame.clear();
    }

    ~GameWorldLoadSession() {
        if (!complete_ && !failed_) cancel();
    }

    Result advance(const std::function<void(std::uint32_t)>& progress,
                   std::string& error) {
        error.clear();
        if (failed_) {
            error = error_;
            return Result::Failed;
        }
        if (stage_ == 0) {
            const auto descriptor_path = options_.resource_root / "Resource" / "Map" /
                ("Map" + std::to_string(map_num_) + ".bmhm");
            descriptor_ = mxh::compat::BmhmMap::load(descriptor_path);
            if (!descriptor_) return fail("Map descriptor unavailable: " + descriptor_path.string(), error);
            mark(progress, 1);
            ++stage_;
            return Result::Pending;
        }
        if (stage_ == 1) {
            if (descriptor_->desc().tile_file_name[0] == '\0')
                return fail("Map descriptor has no tile table", error);
            const auto tile_path = options_.resource_root / "Resource" /
                descriptor_->desc().tile_file_name;
            tile_table_ = mxh::compat::TtbTileTable::load(tile_path);
            if (!tile_table_ || tile_table_->tiles.empty())
                return fail("Tile table unavailable: " + tile_path.string(), error);
            MLOG_INFO("mxh_client: loaded TTB %s (%ux%u tiles)",
                      tile_path.string().c_str(),
                      static_cast<unsigned>(tile_table_->width),
                      static_cast<unsigned>(tile_table_->height));
            mark(progress, 2);
            ++stage_;
            return Result::Pending;
        }
        if (stage_ == 2) {
            terrain_ = std::make_unique<mxh::gx::TerrainScene>();
            const auto name = std::to_string(map_num_) + ".hfl";
            if (!terrain_->load(renderer_, storage_, name.c_str(), &stage_error_))
                return fail("Terrain load failed (" + name + "): " + stage_error_, error);
            if (terrain_->unresolvedTextureCount() != 0 && !g_debugUiBounds)
                return fail("Terrain contains " + std::to_string(terrain_->unresolvedTextureCount()) + " unresolved textures", error);
            mark(progress, 3);
            ++stage_;
            return Result::Pending;
        }
        if (stage_ == 3) {
            static_scene_ = std::make_unique<mxh::gx::StaticScene>();
            const auto name = std::to_string(map_num_) + ".stm";
            if (!static_scene_->load(renderer_, storage_, name.c_str(), &stage_error_))
                return fail("Static scene load failed (" + name + "): " + stage_error_, error);
            if (static_scene_->unresolvedTextureCount() != 0 && !g_debugUiBounds)
                return fail("Static scene contains " + std::to_string(static_scene_->unresolvedTextureCount()) + " unresolved textures", error);
            mark(progress, 5);
            ++stage_;
            return Result::Pending;
        }
        if (stage_ == 4) {
            if (descriptor_->desc().sky_mod[0]) {
                sky_scene_ = std::make_unique<mxh::gx::SkyScene>();
                if (!sky_scene_->load(renderer_, storage_, descriptor_->desc().sky_mod, &stage_error_))
                    return fail("Sky scene load failed (" + std::string(descriptor_->desc().sky_mod) + "): " + stage_error_, error);
            }
            mark(progress, 6);
            ++stage_;
            return Result::Pending;
        }
        if (stage_ == 5) {
            entity_scene_ = std::make_unique<mxh::gx::EntityScene>();
            if (!entity_scene_->load(renderer_, storage_, &stage_error_))
                return fail("Entity scene load failed: " + stage_error_, error);
            entity_scene_->setPlaceholderRenderingEnabled(g_debugUiBounds);
            mark(progress, 8);
            ++stage_;
            return Result::Pending;
        }
        if (stage_ == 6) {
            std::string audio_error;
            if (!bgm_.play(descriptor_->desc().bgm_sound_num, &audio_error)) {
                if (!g_debugUiBounds) {
                    return fail("Map BGM unavailable (sound " +
                                    std::to_string(descriptor_->desc().bgm_sound_num) +
                                    "): " + audio_error, error);
                }
                MLOG_WARN("mxh_client: map BGM unavailable in debug mode: %s",
                          audio_error.c_str());
            }
            mark(progress, 9);
            ++stage_;
            return Result::Pending;
        }

        g_terrain = std::move(terrain_);
        if (g_inputTarget) {
            g_inputTarget->set_world_bounds(g_terrain->worldWidth(), g_terrain->worldHeight());
        }
        g_staticScene = std::move(static_scene_);
        g_skyScene = std::move(sky_scene_);
        g_entityScene = std::move(entity_scene_);
        if (g_inputTarget && g_staticScene) {
            auto* static_scene = g_staticScene.get();
            g_inputTarget->set_collision_query(
                [static_scene](float x, float z, float radius) {
                    return static_scene->blocksPoint(x, z, radius);
                });
        }
        g_renderTerrain = true;
        if (g_overviewCamera) g_captureTerrainFrame = options_.save_frame;
        mark(progress, 10);
        complete_ = true;
        MLOG_INFO("mxh_client: staged GameLoading complete map=%u", static_cast<unsigned>(map_num_));
        return Result::Complete;
    }

    void cancel() noexcept {
        if (complete_) return;
        failed_ = true;
        error_ = "world loading cancelled";
        restore_previous_bgm();
        g_renderTerrain = previous_render_terrain_;
        g_captureTerrainFrame = previous_capture_frame_;
    }

private:
    void mark(const std::function<void(std::uint32_t)>& progress, std::uint32_t value) {
        if (progress) progress(value);
    }

    Result fail(std::string message, std::string& error) {
        failed_ = true;
        error_ = std::move(message);
        restore_previous_bgm();
        g_renderTerrain = previous_render_terrain_;
        g_captureTerrainFrame = previous_capture_frame_;
        error = error_;
        return Result::Failed;
    }

    void restore_previous_bgm() noexcept {
        if (bgm_.currentSoundId() == previous_bgm_id_) return;
        if (previous_bgm_id_ == 0xffffu) {
            bgm_.stop();
            return;
        }
        std::string ignored;
        if (!bgm_.play(previous_bgm_id_, &ignored)) {
            MLOG_WARN("mxh_client: failed to restore previous BGM id=%u after map rollback: %s",
                      static_cast<unsigned>(previous_bgm_id_), ignored.c_str());
        }
    }

    const ClientOptions& options_;
    I4DyuchiGXRenderer* renderer_ = nullptr;
    I4DyuchiFileStorage* storage_ = nullptr;
    mxh::audio::BgmPlayer& bgm_;
    std::uint16_t map_num_ = 0;
    std::uint32_t stage_ = 0;
    bool complete_ = false;
    bool failed_ = false;
    bool previous_render_terrain_ = false;
    std::string previous_capture_frame_;
    std::string stage_error_;
    std::string error_;
    std::uint16_t previous_bgm_id_ = 0xffffu;
    std::optional<mxh::compat::BmhmMap> descriptor_;
    std::optional<mxh::compat::TtbTileTable> tile_table_;
    std::unique_ptr<mxh::gx::TerrainScene> terrain_;
    std::unique_ptr<mxh::gx::StaticScene> static_scene_;
    std::unique_ptr<mxh::gx::SkyScene> sky_scene_;
    std::unique_ptr<mxh::gx::EntityScene> entity_scene_;
};

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

struct ItemIconEntry {
    IDISpriteObject* sprite = nullptr;
    RECT source{0, 0, 0, 0};
};
std::unordered_map<std::uint64_t, ItemIconEntry> g_atlas_icons;

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

bool drawSpriteRegion(I4DyuchiGXRenderer* r, IDISpriteObject* sprite,
                      const RECT& source, float x, float y, float w, float h,
                      std::uint32_t color) {
    if (!r || !sprite || source.right <= source.left || source.bottom <= source.top) {
        return false;
    }
    IMAGE_HEADER header{};
    if (!sprite->GetImageHeader(&header, 0)) return false;
    const auto geometry = mxh::client::compute_sprite_render_geometry(
        header.dwWidth, header.dwHeight, w, h,
        static_cast<float>(source.left) / static_cast<float>(header.dwWidth),
        static_cast<float>(source.top) / static_cast<float>(header.dwHeight),
        static_cast<float>(source.right) / static_cast<float>(header.dwWidth),
        static_cast<float>(source.bottom) / static_cast<float>(header.dwHeight));
    if (!geometry.has_value()) return false;
    VECTOR2 scale{geometry->scale_x, geometry->scale_y};
    VECTOR2 trans{x, y};
    RECT rc{geometry->source_left, geometry->source_top,
            geometry->source_right, geometry->source_bottom};
    return r->RenderSprite(sprite, &scale, 0.0f, &trans, &rc,
                           color, /*iZOrder=*/1, /*dwFlag=*/0) != FALSE;
}

ItemIconEntry* loadAtlasIcon(I4DyuchiGXRenderer* renderer,
                             std::int32_t icon_id,
                             mxh::ui::PathFileType path_type) {
    if (!renderer || icon_id <= 0) return nullptr;
    const auto cache_key = (static_cast<std::uint64_t>(path_type) << 32) |
                           static_cast<std::uint32_t>(icon_id);
    if (auto it = g_atlas_icons.find(cache_key); it != g_atlas_icons.end()) {
        return it->second.sprite ? &it->second : nullptr;
    }
    ItemIconEntry entry;
    const auto hard = mxh::ui::cResourceManager::getInstance().getHardPath(
        icon_id, path_type);
    if (!hard) {
        g_atlas_icons.emplace(cache_key, entry);
        return nullptr;
    }
    const auto atlas = mxh::ui::cSpriteAtlas::getInstance().getInfo(hard->atlas_idx);
    if (!atlas) {
        g_atlas_icons.emplace(cache_key, entry);
        return nullptr;
    }
    const auto path = mxh::ui::cSpriteAtlas::getInstance().resolvePath(*atlas);
    if (path.empty() || !std::filesystem::exists(path)) {
        g_atlas_icons.emplace(cache_key, entry);
        return nullptr;
    }
    auto* sprite = renderer->CreateSpriteObject(const_cast<char*>(path.string().c_str()), 0);
    if (!sprite) {
        g_atlas_icons.emplace(cache_key, entry);
        return nullptr;
    }
    entry.sprite = sprite;
    entry.source = RECT{hard->left, hard->top, hard->right, hard->bottom};
    MLOG_INFO("mxh_client: atlas icon loaded type=%u id=%d atlas=%d rect=%ld,%ld,%ld,%ld",
              static_cast<unsigned>(path_type), icon_id, hard->atlas_idx,
              entry.source.left, entry.source.top,
              entry.source.right, entry.source.bottom);
    auto [it, inserted] = g_atlas_icons.emplace(cache_key, entry);
    if (!inserted && entry.sprite) entry.sprite->Release();
    return it->second.sprite ? &it->second : nullptr;
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

// Asset-backed BEFF billboard consumer.  This is deliberately separate from
// the debug HUD: it only creates a sprite when the decoded BEFF event names a
// real texture/object asset, and it fails closed when that asset cannot be
// created.  The authoritative effect clock remains owned by CInGameState.
struct EffectVisualOverlay {
    static constexpr std::size_t kMaxActiveVisuals = 512;
    struct Instance {
        std::string key;
        std::uint32_t source_id = 0;
        std::uint32_t target_id = 0;
        IDISpriteObject* sprite = nullptr;
        std::string texture_name;
        std::string chx_name;
        std::array<float, 3> position{};
        float radius = 0.0f;
        std::uint32_t color_index = 0;
        bool light = false;
        std::size_t motion_index = 0;
        bool has_motion_index = false;
        std::array<float, 3> move_offset{};
        std::uint64_t move_start_ms = 0;
        std::uint32_t move_duration_ms = 0;
    };
    std::vector<Instance> active;

    ~EffectVisualOverlay() { clear(); }

    void clear() noexcept {
        for (auto& item : active) {
            if (item.sprite) item.sprite->Release();
        }
        active.clear();
    }

    void consume(const mxh::client::RuntimeEffectEvent& event,
                 I4DyuchiGXRenderer* renderer) {
        if (event.unit_kind == "SOUND") return;
        const bool light = event.unit_kind == "LIGHT";
        if (event.unit_kind == "ANIMATION" &&
            event.trigger.trigger.kind == "ON") {
            for (auto& item : active) {
                if (item.source_id == event.source_object_id &&
                    item.target_id == event.target_object_id &&
                    !item.chx_name.empty()) {
                    item.motion_index = event.motion_index;
                    item.has_motion_index = true;
                }
            }
            return;
        }
        if (event.unit_kind == "MOVE") {
            for (auto& item : active) {
                if (item.source_id != event.source_object_id ||
                    item.target_id != event.target_object_id ||
                    item.chx_name.empty()) continue;
                if (event.trigger.trigger.kind == "OFF") {
                    item.move_offset = {};
                    item.move_start_ms = 0;
                    item.move_duration_ms = 0;
                } else if (event.trigger.trigger.kind == "ON") {
                    item.move_offset = event.position;
                    item.move_start_ms = event.trigger.due_ms;
                    item.move_duration_ms = event.duration_ms;
                }
            }
            return;
        }
        const bool mesh = event.unit_kind == "OBJECT" &&
                          !event.object_name.empty();
        if ((!renderer && !mesh) || (!mesh && !light && event.texture_name.empty())) return;
        const std::string key = std::to_string(event.instance_id) + ":" +
            event.effect_name + ":" +
            std::to_string(event.source_object_id) + ":" +
            std::to_string(event.target_object_id) + ":" +
            std::to_string(event.trigger.trigger.unit) + ":" +
            (mesh ? event.object_name : (light ? "LIGHT" : event.texture_name));
        if (event.trigger.trigger.kind == "OFF") {
            for (auto it = active.begin(); it != active.end(); ++it) {
                if (it->key == key) {
                    if (it->sprite) it->sprite->Release();
                    active.erase(it);
                    return;
                }
            }
            return;
        }
        if (event.trigger.trigger.kind != "ON") return;
        if (std::find_if(active.begin(), active.end(),
                         [&key](const Instance& item) { return item.key == key; })
                != active.end()) return;
        if (active.size() >= kMaxActiveVisuals) {
            MLOG_WARN("mxh_client: BEFF visual capacity reached (%zu); evicting oldest visual",
                      kMaxActiveVisuals);
            if (active.front().sprite) active.front().sprite->Release();
            active.erase(active.begin());
        }
        IDISpriteObject* sprite = nullptr;
        if (!mesh) {
            if (light) {
                active.push_back({key, event.source_object_id, event.target_object_id,
                                  nullptr, {}, {}, event.position, event.radius,
                                  event.color_index, true, 0u, false,
                                  {}, 0u, 0u});
                return;
            }
            sprite = renderer->CreateSpriteObject(
                const_cast<char*>(event.texture_name.c_str()), 0);
            if (!sprite) {
                MLOG_WARN("mxh_client: BEFF visual asset unavailable effect=%s texture=%s",
                          event.effect_name.c_str(), event.texture_name.c_str());
                return;
            }
        }
        active.push_back({key, event.source_object_id, event.target_object_id,
                          sprite, event.texture_name,
                          mesh ? event.object_name : std::string{},
                          event.position, event.radius, event.color_index, false,
                          0u, false, {}, 0u, 0u});
    }

    void synchronizeLights(const mxh::client::CInGameState& game,
                           I4DyuchiGXRenderer* renderer) const {
        if (!renderer) return;
        std::uint32_t slot = 0;
        const auto now = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        const auto& info = game.game_info();
        for (const auto& item : active) {
            if (!item.light || slot >= 8u) continue;
            float world_x = static_cast<float>(info.position_x);
            float world_z = static_cast<float>(info.position_z);
            bool found = item.target_id == info.player_id || item.source_id == info.player_id;
            if (!found) {
                for (const auto& monster : game.monsters()) {
                    if (monster.object_id != item.target_id &&
                        monster.object_id != item.source_id) continue;
                    world_x = static_cast<float>(monster.position_x);
                    world_z = static_cast<float>(monster.position_z);
                    found = true;
                    break;
                }
                if (!found) {
                    for (const auto& [remote_id, remote] : game.remote_players()) {
                        if (remote_id != item.target_id && remote_id != item.source_id) continue;
                        world_x = static_cast<float>(remote.position_x);
                        world_z = static_cast<float>(remote.position_z);
                        found = true;
                        break;
                    }
                }
            }
            if (!found) continue;
            float move_progress = 1.0f;
            if (item.move_duration_ms != 0u && now >= item.move_start_ms) {
                move_progress = std::clamp(
                    static_cast<float>(now - item.move_start_ms) /
                        static_cast<float>(item.move_duration_ms), 0.0f, 1.0f);
            }
            const auto move_x = item.move_offset[0] * move_progress;
            const auto move_y = item.move_offset[1] * move_progress;
            const auto move_z = item.move_offset[2] * move_progress;
            LIGHT_DESC light{};
            light.dwDiffuse = item.color_index == 0u ? 0xffffffffu : 0u;
            light.dwAmbient = 0u;
            light.dwSpecular = 0u;
            light.v3Point = {world_x * mxh::gx::kEntitySceneScale -
                             mxh::gx::kEntityMapCenter + (item.position[0] + move_x) *
                             mxh::gx::kEntitySceneScale,
                             (item.position[1] + move_y) * mxh::gx::kEntitySceneScale,
                             world_z * mxh::gx::kEntitySceneScale -
                             mxh::gx::kEntityMapCenter + (item.position[2] + move_z) *
                             mxh::gx::kEntitySceneScale};
            light.fRs = item.radius > 0.0f ? item.radius *
                        mxh::gx::kEntitySceneScale : 2.0f;
            renderer->SetRTLight(&light, slot++, 0);
        }
        for (; slot < 8u; ++slot) {
            LIGHT_DESC clear{};
            renderer->SetRTLight(&clear, slot, 0);
        }
    }

    static void clearLights(I4DyuchiGXRenderer* renderer) {
        if (!renderer) return;
        for (std::uint32_t slot = 0; slot < 8u; ++slot) {
            LIGHT_DESC clear{};
            renderer->SetRTLight(&clear, slot, 0);
        }
    }

    void synchronizeMeshes(const mxh::client::CInGameState& game,
                           const mxh::gx::TerrainScene& terrain,
                           mxh::gx::EntityScene* scene) const {
        if (!scene) return;
        const auto now = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        std::vector<mxh::gx::EffectObject> objects;
        const auto& info = game.game_info();
        for (const auto& item : active) {
            if (item.chx_name.empty()) continue;
            mxh::gx::EffectObject object;
            object.object_id = item.source_id ^ item.target_id ^
                               static_cast<std::uint32_t>(
                                   std::hash<std::string>{}(item.key));
            object.chx_name = item.chx_name;
            object.motion_index = item.motion_index;
            object.has_motion_index = item.has_motion_index;
            if (item.target_id == info.player_id || item.source_id == info.player_id) {
                object.world_x = info.position_x;
                object.world_z = info.position_z;
                object.world_y = terrain.heightAt(info.position_x, info.position_z);
            } else {
                bool found = false;
                for (const auto& monster : game.monsters()) {
                    if (monster.object_id != item.target_id &&
                        monster.object_id != item.source_id) continue;
                    object.world_x = monster.position_x;
                    object.world_z = monster.position_z;
                    object.world_y = terrain.heightAt(monster.position_x, monster.position_z);
                    found = true;
                    break;
                }
                if (!found) {
                    for (const auto& [remote_id, remote] : game.remote_players()) {
                        if (remote_id != item.target_id && remote_id != item.source_id) continue;
                        object.world_x = remote.position_x;
                        object.world_z = remote.position_z;
                        object.world_y = terrain.heightAt(remote.position_x, remote.position_z);
                        found = true;
                        break;
                    }
                }
                if (!found) continue;
            }
            float progress = 1.0f;
            if (item.move_duration_ms != 0u && now >= item.move_start_ms) {
                progress = std::clamp(
                    static_cast<float>(now - item.move_start_ms) /
                        static_cast<float>(item.move_duration_ms), 0.0f, 1.0f);
            }
            object.world_x += item.move_offset[0] * progress;
            object.world_y += item.move_offset[1] * progress;
            object.world_z += item.move_offset[2] * progress;
            objects.push_back(std::move(object));
        }
        scene->synchronizeEffects(objects);
    }

    static bool project(const mxh::gx::MATRIX4& matrix,
                        float world_x, float world_y, float world_z,
                        float& screen_x, float& screen_y) noexcept {
        const float x = world_x * mxh::gx::kEntitySceneScale -
                        mxh::gx::kEntityMapCenter;
        const float y = world_y * mxh::gx::kEntitySceneScale;
        const float z = world_z * mxh::gx::kEntitySceneScale -
                        mxh::gx::kEntityMapCenter;
        const float clip_x = x * matrix._11 + y * matrix._21 +
                             z * matrix._31 + matrix._41;
        const float clip_y = x * matrix._12 + y * matrix._22 +
                             z * matrix._32 + matrix._42;
        const float clip_w = x * matrix._14 + y * matrix._24 +
                             z * matrix._34 + matrix._44;
        if (clip_w <= 0.001f) return false;
        screen_x = (clip_x / clip_w + 1.0f) * 400.0f;
        screen_y = (1.0f - clip_y / clip_w) * 300.0f;
        return screen_x >= -128.0f && screen_x <= 928.0f &&
               screen_y >= -128.0f && screen_y <= 728.0f;
    }

    void render(const mxh::client::CInGameState& game,
                const mxh::gx::TerrainScene& terrain,
                I4DyuchiGXRenderer* renderer) const {
        if (!renderer) return;
        renderer->SetScreenSpaceProjection();
        const auto now = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        for (const auto& item : active) {
            float move_progress = 1.0f;
            if (item.move_duration_ms != 0u && now >= item.move_start_ms) {
                move_progress = std::clamp(
                    static_cast<float>(now - item.move_start_ms) /
                        static_cast<float>(item.move_duration_ms), 0.0f, 1.0f);
            }
            const auto move_x = item.move_offset[0] * move_progress;
            const auto move_y = item.move_offset[1] * move_progress;
            const auto move_z = item.move_offset[2] * move_progress;
            float x = 0.0f, y = 0.0f;
            bool found = false;
            const auto& info = game.game_info();
            if (item.target_id == info.player_id || item.source_id == info.player_id) {
                const float world_x = info.position_x + item.position[0] + move_x;
                const float world_z = info.position_z + item.position[2] + move_z;
                found = project(terrain.viewProj(), world_x,
                                terrain.heightAt(info.position_x, info.position_z) +
                                    120.0f + item.position[1] + move_y,
                                world_z, x, y);
            }
            if (!found) {
                for (const auto& monster : game.monsters()) {
                    if (monster.object_id != item.target_id &&
                        monster.object_id != item.source_id) continue;
                    const float world_x = monster.position_x + item.position[0] + move_x;
                    const float world_z = monster.position_z + item.position[2] + move_z;
                    found = project(terrain.viewProj(), world_x,
                                    terrain.heightAt(monster.position_x, monster.position_z) +
                                        120.0f + item.position[1] + move_y,
                                    world_z, x, y);
                    break;
                }
            }
            if (!found) {
                for (const auto& [remote_id, remote] : game.remote_players()) {
                    if (remote_id != item.target_id && remote_id != item.source_id) continue;
                    found = project(terrain.viewProj(), remote.position_x + item.position[0] + move_x,
                                    terrain.heightAt(remote.position_x, remote.position_z) +
                                        120.0f + item.position[1] + move_y,
                                    remote.position_z + item.position[2] + move_z, x, y);
                    break;
                }
            }
            if (!found) continue;
            drawSpriteQuad(renderer, item.sprite, x - 32.0f, y - 64.0f,
                           64.0f, 64.0f, 0xD0FFFFFFu);
        }
    }
};

struct DamageFeedback {
    std::uint32_t target_id = 0;
    std::int32_t amount = 0;
    std::uint64_t start_ms = 0;
    std::uint32_t duration_ms = 900;
};

std::vector<DamageFeedback> g_damageFeedback;

struct DisplayTransitionResult {
    bool committed = false;
    DWORD win32_error = ERROR_SUCCESS;
    std::uint32_t client_width = 0;
    std::uint32_t client_height = 0;
};

// Apply a client-size change as one transaction.  Login must not leave the
// process with a resized HWND and an old swap-chain/viewport if any step
// fails.  The 4:3 logical canvas remains owned by LogicalViewport; physical
// client pixels are never used as logical UI coordinates.
DisplayTransitionResult applyDisplayTransition(
    HWND hwnd, I4DyuchiGXRenderer* renderer,
    mxh::client::LogicalViewport& viewport,
    std::uint32_t requested_width, std::uint32_t requested_height,
    bool borderless) {
    DisplayTransitionResult result{};
    if (!hwnd || requested_width < 800 || requested_height < 600) {
        result.win32_error = ERROR_INVALID_PARAMETER;
        return result;
    }

    RECT old_window{};
    RECT old_client{};
    if (!GetWindowRect(hwnd, &old_window) || !GetClientRect(hwnd, &old_client)) {
        result.win32_error = GetLastError();
        return result;
    }
    const LONG old_outer_width = old_window.right - old_window.left;
    const LONG old_outer_height = old_window.bottom - old_window.top;
    const LONG old_client_width = old_client.right - old_client.left;
    const LONG old_client_height = old_client.bottom - old_client.top;
    const LONG_PTR old_style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    const LONG_PTR new_style = borderless
        ? (old_style & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU))
        : (old_style | WS_OVERLAPPEDWINDOW);
    if (new_style != old_style) SetWindowLongPtrW(hwnd, GWL_STYLE, new_style);

    RECT desired{0, 0, static_cast<LONG>(requested_width),
                 static_cast<LONG>(requested_height)};
    // The login/post-login dimensions are client-area dimensions.  On a
    // per-monitor-DPI process the non-client frame is DPI-scaled, so using
    // AdjustWindowRectEx without the current monitor DPI produces a client
    // area smaller than the requested logical canvas (and shifts hitboxes).
    // Keep the Win10+ API behind a runtime lookup for older supported hosts.
    using AdjustWindowRectExForDpiFn = BOOL (WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
    static const auto adjust_for_dpi = []() -> AdjustWindowRectExForDpiFn {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        return user32 ? reinterpret_cast<AdjustWindowRectExForDpiFn>(
            GetProcAddress(user32, "AdjustWindowRectExForDpi")) : nullptr;
    }();
    const UINT dpi = GetDpiForWindow(hwnd);
    const DWORD frame_style = static_cast<DWORD>(new_style);
    const bool adjusted = adjust_for_dpi
        ? adjust_for_dpi(&desired, frame_style, FALSE, 0, dpi)
        : AdjustWindowRectEx(&desired, frame_style, FALSE, 0);
    if (!adjusted) {
        result.win32_error = GetLastError();
        // The style mutation happened before frame calculation.  Restore it
        // even when AdjustWindowRectEx* fails; otherwise a failed transition
        // can leave the login window borderless while the viewport remains
        // at 800x600, violating the transaction contract.
        if (new_style != old_style) {
            SetWindowLongPtrW(hwnd, GWL_STYLE, old_style);
            SetWindowPos(hwnd, nullptr, old_window.left, old_window.top,
                         old_outer_width, old_outer_height,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
        return result;
    }
    const LONG desired_outer_width = desired.right - desired.left;
    const LONG desired_outer_height = desired.bottom - desired.top;
    if (!SetWindowPos(hwnd, nullptr, old_window.left, old_window.top,
                      desired_outer_width, desired_outer_height,
                      SWP_NOZORDER | SWP_NOACTIVATE |
                          (new_style != old_style ? SWP_FRAMECHANGED : 0))) {
        result.win32_error = GetLastError();
        if (new_style != old_style) {
            SetWindowLongPtrW(hwnd, GWL_STYLE, old_style);
            SetWindowPos(hwnd, nullptr, old_window.left, old_window.top,
                         old_outer_width, old_outer_height,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
        return result;
    }

    RECT actual_client{};
    const bool sized = GetClientRect(hwnd, &actual_client);
    const LONG actual_width = sized ? actual_client.right - actual_client.left : 0;
    const LONG actual_height = sized ? actual_client.bottom - actual_client.top : 0;
    if (!sized || actual_width <= 0 || actual_height <= 0) {
        result.win32_error = GetLastError();
    } else {
        viewport.update(actual_width, actual_height);
        if (renderer && !renderer->TryUpdateWindowSize()) {
            result.win32_error = ERROR_GEN_FAILURE;
        } else {
            result.client_width = static_cast<std::uint32_t>(actual_width);
            result.client_height = static_cast<std::uint32_t>(actual_height);
            result.committed = true;
            return result;
        }
    }

    // Roll back the outer window and the logical viewport.  A failed
    // post-login transition must remain a login-screen failure, never a
    // partially applied display mode.
    SetWindowPos(hwnd, nullptr, old_window.left, old_window.top,
                 old_outer_width, old_outer_height,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    if (new_style != old_style) SetWindowLongPtrW(hwnd, GWL_STYLE, old_style);
    SetWindowPos(hwnd, nullptr, old_window.left, old_window.top,
                 old_outer_width, old_outer_height,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    viewport.update(old_client_width, old_client_height);
    if (renderer) renderer->UpdateWindowSize();
    return result;
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
        if (g_inputTarget) {
            g_terrain->setCameraDistance(g_inputTarget->camera_distance());
            g_terrain->setCameraYaw(g_inputTarget->camera_yaw());
        }
        g_terrain->configureCamera(800.0f / 600.0f);
        if (!g_overviewCamera && g_skyScene) g_skyScene->render();
        g_terrain->render();
        if (g_staticScene) g_staticScene->render();
        if (g_entityScene) {
            if (g_effectVisuals && g_inputTarget && g_inputTarget->is_in_game()) {
                g_effectVisuals->synchronizeMeshes(*g_inputTarget, *g_terrain,
                                                   g_entityScene.get());
                g_effectVisuals->synchronizeLights(*g_inputTarget, g_renderer);
            } else {
                g_entityScene->clearEffects();
                EffectVisualOverlay::clearLights(g_renderer);
            }
            // Push the terrain's view-projection as a Frustum so the
            // entity scene can cull NPCs whose world AABB is outside the
            // view volume. G5 M-R5: see mxh/render/frustum.hpp for the
            // Gribb-Hartmann plane extraction.
            g_entityScene->setCameraFrustum(mxh::gx::Frustum(g_terrain->viewProj()));
            g_entityScene->render();
        }
        if (g_effectVisuals && g_inputTarget && g_inputTarget->is_in_game()) {
            g_effectVisuals->render(*g_inputTarget, *g_terrain, g_renderer);
        }
        if (g_inputTarget && g_inputTarget->is_in_game() && !g_damageFeedback.empty()) {
            const auto now = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
            const auto& info = g_inputTarget->game_info();
            for (const auto& feedback : g_damageFeedback) {
                if (now < feedback.start_ms ||
                    now - feedback.start_ms >= feedback.duration_ms) continue;
                float sx = 0.0f, sy = 0.0f;
                bool found = false;
                if (feedback.target_id == info.player_id) {
                    found = EffectVisualOverlay::project(
                        g_terrain->viewProj(), info.position_x,
                        g_terrain->heightAt(info.position_x, info.position_z) + 140.0f,
                        info.position_z, sx, sy);
                } else {
                    for (const auto& monster : g_inputTarget->monsters()) {
                        if (monster.object_id != feedback.target_id) continue;
                        found = EffectVisualOverlay::project(
                            g_terrain->viewProj(), monster.position_x,
                            g_terrain->heightAt(monster.position_x, monster.position_z) + 140.0f,
                            monster.position_z, sx, sy);
                        break;
                    }
                }
                if (!found) continue;
                const auto age = static_cast<float>(now - feedback.start_ms);
                const auto text = std::to_string(feedback.amount);
                mxh::ui::renderText(mxh::ui::TextRenderRequest{
                    text, static_cast<std::int32_t>(sx - 32.0f),
                    static_cast<std::int32_t>(sy - 70.0f - age * 0.02f),
                    64, 24, 0, 0,
                    feedback.amount < 0 ? 0xFF70FF70u : 0xFFFF6060u,
                    0, mxh::ui::TextRenderAlign::Center, false});
            }
            g_damageFeedback.erase(
                std::remove_if(g_damageFeedback.begin(), g_damageFeedback.end(),
                    [now](const DamageFeedback& value) {
                        return now >= value.start_ms &&
                               now - value.start_ms >= value.duration_ms;
                    }), g_damageFeedback.end());
        }

        // Ground drops are live world objects, not a log-only event.  Use the
        // original ItemList name and the same camera projection as entity
        // labels so the player can see and click the item before picking it
        // up.  There is deliberately no synthetic icon or coloured quad
        // fallback: an unknown item is shown by its authoritative numeric ID.
        if (g_inputTarget && g_inputTarget->is_in_game() && g_hudFont &&
            g_entityScene && g_terrain) {
            for (const auto& drop : g_inputTarget->ground_drops()) {
                float sx = 0.0f, sy = 0.0f;
                if (!EffectVisualOverlay::project(
                        g_terrain->viewProj(), drop.position_x,
                        g_terrain->heightAt(drop.position_x, drop.position_z) +
                            100.0f,
                        drop.position_z, sx, sy)) {
                    continue;
                }
                auto name = g_entityScene->itemDisplayName(drop.item_id);
                if (name.empty()) {
                    name = "Item#" + std::to_string(drop.item_id);
                }
                if (drop.count > 1) {
                    name += " x" + std::to_string(drop.count);
                }
                RECT rc{static_cast<LONG>(sx - 90.0f),
                        static_cast<LONG>(sy - 18.0f),
                        static_cast<LONG>(sx + 90.0f),
                        static_cast<LONG>(sy + 4.0f)};
                g_renderer->RenderFont(
                    g_hudFont, name.data(),
                    static_cast<std::uint32_t>(name.size()), &rc,
                    0xFFFFE080u, CHAR_CODE_TYPE_ASCII, 1, 0);
            }
        }

        // GameIn UI is the original InterfaceScript tree.  The old geometric
        // placeholder HUD remains available only with --debug-ui-bounds.
        if (g_inputTarget && g_inputTarget->is_in_game()) {
            g_renderer->SetScreenSpaceProjection();
            const auto& info = g_inputTarget->game_info();
            // The shipped InterfaceScript tree is the release HUD.  The
            // solid 1x1 sprites below are diagnostic geometry only; keeping
            // them behind the explicit bounds flag prevents a debug overlay
            // from masquerading as original UI in normal builds.
            if (g_debugUiBounds) {
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
                            const auto item_icon = g_entityScene
                                ? g_entityScene->itemIconIndex(
                                      inventory[idx].wIconIdx)
                                : std::nullopt;
                            if (item_icon.has_value()) {
                                if (auto* icon = loadAtlasIcon(
                                        g_renderer,
                                        static_cast<std::int32_t>(*item_icon),
                                    mxh::ui::PathFileType::ItemPath)) {
                                    (void)drawSpriteRegion(g_renderer, icon->sprite,
                                                           icon->source, x, y,
                                                           kCell, kCell,
                                                           0xFFFFFFFFu);
                                } else {
                                    drawSpriteQuad(g_renderer, g_hud.barBg, x, y,
                                                   kCell, kCell, 0xFFFFFFFFu);
                                }
                            } else {
                                // Keep the slot chrome visible while making
                                // a missing profile mapping explicit; no
                                // synthetic item art is generated.
                                drawSpriteQuad(g_renderer, g_hud.barBg, x, y,
                                               kCell, kCell, 0xFFFFFFFFu);
                            }
                            if (g_hudFont) {
                                auto t = g_entityScene
                                    ? g_entityScene->itemDisplayName(
                                          inventory[idx].wIconIdx)
                                    : std::string{};
                                if (t.empty()) {
                                    t = "#" + std::to_string(
                                        inventory[idx].wIconIdx);
                                }
                                if (inventory[idx].ItemParam > 1) {
                                    t += " x" + std::to_string(
                                        inventory[idx].ItemParam);
                                }
                                const auto wide = mxh::compat::big5_to_utf16(t);
                                RECT rc{static_cast<LONG>(x),
                                        static_cast<LONG>(y),
                                        static_cast<LONG>(x) + static_cast<LONG>(kCell),
                                        static_cast<LONG>(y) + 16};
                                if (!wide.empty()) {
                                    g_renderer->RenderFont(
                                        g_hudFont,
                                        reinterpret_cast<TCHAR*>(const_cast<wchar_t*>(wide.data())),
                                        static_cast<std::uint32_t>(wide.size()), &rc,
                                        0xFFFFFFFFu, CHAR_CODE_TYPE_UNICODE, 1, 0);
                                }
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
                        auto itemName = g_entityScene
                            ? g_entityScene->itemDisplayName(shopItems[i].item_id)
                            : std::string{};
                        if (itemName.empty()) {
                            itemName = "Item#" +
                                       std::to_string(shopItems[i].item_id);
                        }
                        itemName += "   $" + std::to_string(shopItems[i].price);
                        const auto wide = mxh::compat::big5_to_utf16(itemName);
                        RECT rc{static_cast<LONG>(mxh::client::kShopPanelX) + 8,
                                static_cast<LONG>(rowY) + 4,
                                static_cast<LONG>(mxh::client::kShopPanelX) + 380,
                                static_cast<LONG>(rowY) + 22};
                        if (!wide.empty()) {
                            g_renderer->RenderFont(
                                g_hudFont,
                                reinterpret_cast<TCHAR*>(const_cast<wchar_t*>(wide.data())),
                                static_cast<std::uint32_t>(wide.size()), &rc,
                                0xFFFFFFFFu, CHAR_CODE_TYPE_UNICODE, 1, 0);
                        }
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
            }
            g_inputTarget->ui_runtime().render();
        }
    }

    // Inventory is a player-facing window, not diagnostic geometry.  Keep it
    // outside the debug overlay branch so a normal player can open I and see
    // the live item cells and atlas-backed icons.
    if (g_renderTerrain && g_inputTarget && g_inputTarget->is_in_game() &&
        g_inputTarget->inventory_open()) {
        constexpr float kCell = 34.0f;
        constexpr float kGap = 3.0f;
        constexpr float kX = 20.0f;
        constexpr float kY = 84.0f;
        const auto& inventory = g_inputTarget->game_info().items.Inventory;
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 10; ++col) {
                const int idx = row * 10 + col;
                const float x = kX + static_cast<float>(col) * (kCell + kGap);
                const float y = kY + static_cast<float>(row) * (kCell + kGap);
                drawSpriteQuad(g_renderer, g_hud.barBg, x, y,
                               kCell, kCell, 0xFFFFFFFFu);
                const auto& item = inventory[idx];
                if (mxh::game::is_empty_slot(item)) continue;
                const auto icon_id = g_entityScene
                    ? g_entityScene->itemIconIndex(item.wIconIdx)
                    : std::nullopt;
                if (!icon_id) continue;
                if (auto* icon = loadAtlasIcon(
                        g_renderer, static_cast<std::int32_t>(*icon_id),
                        mxh::ui::PathFileType::ItemPath)) {
                    (void)drawSpriteRegion(g_renderer, icon->sprite,
                                           icon->source, x, y,
                                           kCell, kCell, 0xFFFFFFFFu);
                }
            }
        }
    }

    // The quick-slot bar is part of the normal player HUD, not debug
    // geometry.  It consumes the authoritative GameIn mugong snapshot and
    // resolves each icon through the canonical MugongPath atlas.
    if (g_renderTerrain && g_inputTarget && g_inputTarget->is_in_game()) {
        constexpr float kSlotW = 44.0f;
        constexpr float kSlotH = 44.0f;
        constexpr float kGap = 6.0f;
        const float totalW = static_cast<float>(mxh::client::kQuickSlotCount) *
            kSlotW + (static_cast<float>(mxh::client::kQuickSlotCount) - 1.0f) * kGap;
        const float barX = (800.0f - totalW) * 0.5f;
        const float barY = 470.0f;
        const auto& info = g_inputTarget->game_info();
        for (std::size_t i = 0; i < mxh::client::kQuickSlotCount; ++i) {
            const float x = barX + static_cast<float>(i) * (kSlotW + kGap);
            drawSpriteQuad(g_renderer, g_hud.barBg, x, barY,
                           kSlotW, kSlotH, 0xFFFFFFFFu);
            if (!g_hudFont) continue;
            const auto skill = mxh::client::quick_skill_for_slot(info, i);
            if (skill != 0) {
                if (auto* icon = loadAtlasIcon(
                        g_renderer, static_cast<std::int32_t>(skill),
                        mxh::ui::PathFileType::MugongPath)) {
                    (void)drawSpriteRegion(g_renderer, icon->sprite,
                                           icon->source, x + 3.0f,
                                           barY + 3.0f, kSlotW - 6.0f,
                                           kSlotH - 6.0f, 0xFFFFFFFFu);
                }
            }
            const std::string key = "F" + std::to_string(i + 1);
            RECT rcKey{static_cast<LONG>(x), static_cast<LONG>(barY) + 26,
                       static_cast<LONG>(x) + static_cast<LONG>(kSlotW),
                       static_cast<LONG>(barY) + static_cast<LONG>(kSlotH)};
            g_renderer->RenderFont(g_hudFont, const_cast<char*>(key.data()),
                                   static_cast<std::uint32_t>(key.size()),
                                   &rcKey, 0xFFFFFFFFu, CHAR_CODE_TYPE_ASCII, 1, 0);
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
                if (cur_state == static_cast<int>(mxh::client::GameStateId::CharSelect) ||
                    cur_state == static_cast<int>(mxh::client::GameStateId::CharMake)) {
                    g_charPreviewController.reset();
                }
                last_state_logged = cur_state;
            }
            if (cur_state == static_cast<int>(mxh::client::GameStateId::CharSelect)) {
                if (g_charPreviewScene && g_charSelectState) {
                    const auto& slots = g_charSelectState->character_list();
                    const mxh::client::CharacterSlot* selected = nullptr;
                    for (const auto& slot : slots) {
                        if (slot.valid && slot.chrid == g_charSelectState->selected_chrid()) {
                            selected = &slot;
                            break;
                        }
                    }
                    if (!selected) {
                        for (const auto& slot : slots) {
                            if (slot.valid) { selected = &slot; break; }
                        }
                    }
                    if (selected) {
                        if (auto preview = mxh::client::make_character_preview(*selected)) {
                            mxh::gx::WorldSnapshot snapshot;
                            snapshot.local_player = *preview;
                            g_charPreviewScene->synchronize(snapshot);
                            configureCharacterPreviewCamera(
                                g_renderer, 800.0f / 600.0f,
                                g_charPreviewController);
                            g_charPreviewScene->render();
                            g_renderer->SetScreenSpaceProjection();
                            static std::uint32_t logged_preview = 0;
                            if (logged_preview != selected->chrid) {
                                MLOG_INFO("mxh_client: character preview rendered chrid=%u gender=%u face=%u hair=%u equipment=%zu models=%u failures=%u",
                                          selected->chrid,
                                          static_cast<unsigned>(selected->gender),
                                          static_cast<unsigned>(selected->face_type),
                                          static_cast<unsigned>(selected->hair_type),
                                          selected->weared_item_idx.size(),
                                          g_charPreviewScene->loadedModelCount(),
                                          g_charPreviewScene->failedModelCount());
                                logged_preview = selected->chrid;
                            }
                        }
                    }
                }
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
                if (g_charPreviewScene && g_charMakeState) {
                    const auto& params = g_charMakeState->form_model().params();
                    mxh::client::CharacterSlot preview_slot;
                    preview_slot.valid = true;
                    preview_slot.chrid = 0xFFFFFFFEu;
                    preview_slot.gender = params.sex_type;
                    preview_slot.face_type = params.face_type;
                    preview_slot.hair_type = params.hair_type;
                    preview_slot.weared_item_idx = params.weared_item_idx;
                    if (auto preview = mxh::client::make_character_preview(preview_slot)) {
                        mxh::gx::WorldSnapshot snapshot;
                        snapshot.local_player = *preview;
                        g_charPreviewScene->synchronize(snapshot);
                        configureCharacterPreviewCamera(g_renderer, 800.0f / 600.0f,
                                                        g_charPreviewController);
                        g_charPreviewScene->render();
                        g_renderer->SetScreenSpaceProjection();
                        static std::uint32_t logged_make_preview =
                            std::numeric_limits<std::uint32_t>::max();
                        const auto appearance_key =
                            (static_cast<std::uint32_t>(params.sex_type) << 16) |
                            (static_cast<std::uint32_t>(params.face_type) << 8) |
                            params.hair_type;
                        if (logged_make_preview != appearance_key) {
                            MLOG_INFO("mxh_client: character make preview rendered sex=%u face=%u hair=%u equipment=%zu failures=%u",
                                      static_cast<unsigned>(params.sex_type),
                                      static_cast<unsigned>(params.face_type),
                                      static_cast<unsigned>(params.hair_type),
                                      params.weared_item_idx.size(),
                                      g_charPreviewScene->failedModelCount());
                            logged_make_preview = appearance_key;
                        }
                    }
                }
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
                if (auto* loading = g_gameLoadingState) {
                    loading->ui_runtime().render();
                    if (g_debugUiBounds) {
                        const auto& dialogs = loading->ui_dialogs();
                        for (const auto& d : dialogs) {
                            if (!d) continue;
                            drawHudBar(g_renderer, g_hud.barBg, g_hud.barBg,
                                       static_cast<float>(d->absX()),
                                       static_cast<float>(d->absY()),
                                       static_cast<float>(d->width()),
                                       static_cast<float>(d->height()), 0.85f);
                        }
                        drawText("Loading progress: " +
                                     std::to_string(static_cast<int>(loading->progress() * 100.0f)) + "%",
                                 290, 540, 0xFF80FF80u);
                    }
                }
            } else if (cur_state == static_cast<int>(mxh::client::GameStateId::MapChange)) {
                if (g_mapChangeState) {
                    g_mapChangeState->ui_runtime().render();
                    if (g_debugUiBounds) {
                        for (const auto& d : g_mapChangeState->ui_dialogs()) {
                            if (!d) continue;
                            drawHudBar(g_renderer, g_hud.barBg, g_hud.barBg,
                                       static_cast<float>(d->absX()),
                                       static_cast<float>(d->absY()),
                                       static_cast<float>(d->width()),
                                       static_cast<float>(d->height()), 0.85f);
                        }
                        drawText("Changing map...", 290, 540, 0xFF80FF80u);
                    }
                }
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
void captureHumanEvidenceFrame() {
    if (g_evidenceDir.empty() || g_renderer == nullptr) return;
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(g_evidenceDir), ec);
    if (ec) {
        MLOG_WARN("mxh_client: evidence directory unavailable path=%s error=%s",
                  g_evidenceDir.c_str(), ec.message().c_str());
        return;
    }
    ++g_evidenceFrameSequence;
    const auto path = std::filesystem::path(g_evidenceDir) /
        ("screenshot-" + std::to_string(g_evidenceFrameSequence) + ".tga");
    auto mutablePath = path.string();
    if (g_renderer->CaptureScreen(mutablePath.data())) {
        MLOG_INFO("mxh_client: human evidence frame saved path=%s", mutablePath.c_str());
    } else {
        MLOG_WARN("mxh_client: human evidence frame capture failed path=%s", mutablePath.c_str());
    }
}

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
    case WM_DPICHANGED:
        // Per-monitor DPI changes (including moving the window between
        // displays) alter the non-client frame and client-area mapping.  Use
        // the system's suggested outer rectangle, then let WM_SIZE refresh
        // the logical viewport and DX11 swap-chain dimensions.
        if (l != 0) {
            const auto* suggested = reinterpret_cast<const RECT*>(l);
            SetWindowPos(h, nullptr,
                         suggested->left, suggested->top,
                         suggested->right - suggested->left,
                         suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        if (g_renderer) g_renderer->UpdateWindowSize();
        return 0;
    case WM_DISPLAYCHANGE: {
        // Display mode/topology changes can invalidate the current client
        // metrics without a matching resize message (notably during monitor
        // hot-plug and remote-desktop transitions).  Re-read the actual
        // client rectangle instead of trusting the message payload.
        RECT client{};
        if (GetClientRect(h, &client)) {
            g_logicalViewport.update(client.right - client.left,
                                     client.bottom - client.top);
        }
        if (g_renderer) g_renderer->UpdateWindowSize();
        return 0;
    }
    case WM_ACTIVATEAPP:
        g_audioFocused = w != FALSE;
        if (g_bgmPlayer) g_bgmPlayer->setVolume(g_audioFocused ? g_bgmVolume : 0.0f);
        if (g_sfxPlayer) g_sfxPlayer->setVolume(g_audioFocused ? g_sfxVolume : 0.0f);
        if (!g_audioFocused) {
            // Windows may not deliver KeyUp for keys held while the window
            // loses activation.  Release the movement bindings explicitly so
            // Alt-Tab/minimize cannot leave the avatar walking indefinitely.
            constexpr std::array<std::uint32_t, 10> kMovementKeys{
                0x57u, 0x53u, 0x51u, 0x45u, 0x41u, 0x44u,
                VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT};
            if (g_inputTarget) {
                for (const auto key : kMovementKeys)
                    g_inputTarget->OnKeyEvent(false, key);
                g_inputTarget->OnMouseButton(false, false, 0, 0);
            }
            // WM_ACTIVATEAPP does not guarantee a matching mouse-up for a
            // drag that was interrupted by Alt-Tab/minimize.  Release the
            // title, character-selection and character-creation surfaces as
            // well, including the shared 3D preview controller.
            if (g_mainTitle) (void)g_mainTitle->OnMouseButton(true, false, 0, 0);
            if (g_charSelectState) (void)g_charSelectState->OnMouseButton(true, false, 0, 0);
            if (g_charMakeState) (void)g_charMakeState->OnMouseButton(true, false, 0, 0);
            if (g_charPreviewController.onMouseButton(true, false, 0, 0)) {
                InvalidateRect(h, nullptr, FALSE);
            }
            if (GetCapture() == h) ReleaseCapture();
        }
        MLOG_INFO("mxh_client: audio focus=%s", g_audioFocused ? "active" : "muted");
        return 0;
    case WM_KEYDOWN:
        if (w == VK_F12) {
            captureHumanEvidenceFrame();
            return 0;
        }
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
        if (w == VK_ESCAPE && g_loadingCoordinator &&
            (__g_currentState == static_cast<int>(mxh::client::GameStateId::GameLoading) ||
             __g_currentState == static_cast<int>(mxh::client::GameStateId::MapChange))) {
            g_loadingCoordinator->cancel();
            InvalidateRect(h, nullptr, FALSE);
            return 0;
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
        // Keep drag/rotation gestures owned by this window until the button
        // is released.  Without capture, moving across the client edge (or
        // a pillarbox boundary) strands the pressed state and the next click
        // is interpreted as a fresh gesture by the legacy-style controls.
        if (m == WM_LBUTTONDOWN) {
            SetCapture(h);
        } else if (GetCapture() == h) {
            ReleaseCapture();
        }
        const auto logical = g_logicalViewport.to_logical(
            static_cast<std::int32_t>(static_cast<short>(LOWORD(l))),
            static_cast<std::int32_t>(static_cast<short>(HIWORD(l))));
        if (!logical.has_value()) {
            // WM capture keeps delivering the release after the cursor has
            // left the 4:3 content rect.  Do not drop that event merely
            // because it cannot be mapped to a logical coordinate: dialogs
            // and the in-game state still need it to clear pressed/dragging
            // state (inventory drags, camera gestures, etc.).
            if (m == WM_LBUTTONUP) {
                if (g_mainTitle) (void)g_mainTitle->OnMouseButton(true, false, 0, 0);
                if (g_charSelectState) (void)g_charSelectState->OnMouseButton(true, false, 0, 0);
                if (g_charMakeState) (void)g_charMakeState->OnMouseButton(true, false, 0, 0);
                if (g_inputTarget) (void)g_inputTarget->OnMouseButton(true, false, 0, 0);
            }
            return 0;
        }
        const auto x = static_cast<std::int32_t>(logical->x);
        const auto y = static_cast<std::int32_t>(logical->y);
        const auto playUiClick = [&]() {
            if (m != WM_LBUTTONUP || !g_sfxPlayer || g_uiClickSound == 0xffffu) return;
            std::string audio_error;
            (void)g_sfxPlayer->playOnBus(g_uiClickSound, g_uiVolume, &audio_error);
        };
        if (g_mainTitle &&
            g_mainTitle->OnMouseButton(true, m == WM_LBUTTONDOWN, x, y)) {
            playUiClick();
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }
        if (g_charSelectState &&
            g_charSelectState->OnMouseButton(
                true, m == WM_LBUTTONDOWN, x, y)) {
            playUiClick();
            return 0;
        }
        if (g_charMakeState &&
            g_charMakeState->OnMouseButton(
                true, m == WM_LBUTTONDOWN, x, y)) {
            playUiClick();
            return 0;
        }
        if (g_inputTarget) {
            if (g_inputTarget->OnMouseButton(
                true, m == WM_LBUTTONDOWN,
                x, y)) {
                playUiClick();
            }
        }
        return 0;
        }
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        {
        if (m == WM_RBUTTONDOWN) {
            SetCapture(h);
        } else if (GetCapture() == h) {
            ReleaseCapture();
        }
        const auto logical = g_logicalViewport.to_logical(
            static_cast<std::int32_t>(static_cast<short>(LOWORD(l))),
            static_cast<std::int32_t>(static_cast<short>(HIWORD(l))));
        if (!logical.has_value()) {
            if (m == WM_RBUTTONUP) {
                // See the left-button path above: a captured release outside
                // the pillarboxed content still must terminate camera/model
                // rotation in every active state.
                if (g_charSelectState) (void)g_charSelectState->OnMouseButton(false, false, 0, 0);
                if (g_charMakeState) (void)g_charMakeState->OnMouseButton(false, false, 0, 0);
                if (g_charPreviewController.onMouseButton(true, false, 0, 0)) {
                    InvalidateRect(h, nullptr, FALSE);
                }
                if (g_inputTarget) (void)g_inputTarget->OnMouseButton(false, false, 0, 0);
            }
            return 0;
        }
        if (g_charSelectState &&
            g_charSelectState->OnMouseButton(
                false, m == WM_RBUTTONDOWN,
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y))) {
            if (m == WM_RBUTTONUP) g_charPreviewController.onMouseButton(
                true, false, static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y));
            return 0;
        }
        if (g_charMakeState &&
            g_charMakeState->OnMouseButton(
                false, m == WM_RBUTTONDOWN,
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y))) {
            if (m == WM_RBUTTONUP) g_charPreviewController.onMouseButton(
                true, false, static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y));
            return 0;
        }
        if ((g_charSelectState || g_charMakeState) &&
            g_charPreviewController.onMouseButton(
                true, m == WM_RBUTTONDOWN,
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y))) {
            InvalidateRect(h, nullptr, FALSE);
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
        if (g_charPreviewController.onMouseMove(
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y))) {
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }
        if (g_inputTarget) {
            g_inputTarget->OnMouseMove(
                static_cast<std::int32_t>(logical->x),
                static_cast<std::int32_t>(logical->y));
        }
        return 0;
        }
    case WM_MOUSEWHEEL:
        // WM_MOUSEWHEEL coordinates are screen-relative.  Convert them to
        // the client viewport before dispatching so pillarbox side bars stay
        // inert just like mouse clicks and movement.  This matters in 16:9
        // mode where the legacy 4:3 canvas is intentionally not stretched.
        {
        POINT cursor{
            static_cast<LONG>(static_cast<short>(LOWORD(l))),
            static_cast<LONG>(static_cast<short>(HIWORD(l)))};
        if (!ScreenToClient(h, &cursor) ||
            !g_logicalViewport.to_logical(cursor.x, cursor.y).has_value()) {
            return 0;
        }
        if ((__g_currentState == static_cast<int>(mxh::client::GameStateId::CharSelect) ||
             __g_currentState == static_cast<int>(mxh::client::GameStateId::CharMake)) &&
            g_charPreviewController.onMouseWheel(
                static_cast<std::int32_t>(static_cast<short>(HIWORD(w))))) {
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }
        if (g_inputTarget) {
            g_inputTarget->OnMouseWheel(
                static_cast<std::int32_t>(static_cast<short>(HIWORD(w))));
            InvalidateRect(h, nullptr, FALSE);
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
    auto persisted_settings = mxh::client::ClientSettingsStore::load(
        settings_path, &settings_warning);
    g_uiVolume = persisted_settings.ui_volume;
    if (!settings_warning.empty()) MLOG_WARN("mxh_client: %s", settings_warning.c_str());
    // Launcher-owned settings are the source of truth for a normal start.
    // Command-line overrides remain available for development, but the
    // production client must carry the selected profile and borderless mode
    // into the LoginAck display transition instead of silently reverting to
    // hard-coded defaults.
    if (!options.resource_profile_overridden) {
        options.resource_profile_id = persisted_settings.resource_profile_id;
    }
    if (!options.borderless_overridden) options.borderless = persisted_settings.borderless;
    if (!options.vsync_overridden) options.vsync = persisted_settings.vsync;
    if (!options.post_login_width_overridden) options.post_login_width = persisted_settings.post_login_width;
    if (!options.post_login_height_overridden) options.post_login_height = persisted_settings.post_login_height;
#if !defined(MXH_DEV_AUTOMATION)
    if (options.release_automation_requested) {
        std::fprintf(stderr,
                     "mxh_client: automation and credential command-line options are disabled in release builds\n");
        return 2;
    }
#endif
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
    if (options.resource_profile_id == "sworking-2008-reference") {
        std::fprintf(stderr, "mxh_client: reference resource profiles are development-only\n");
        return 2;
    }
#endif
    if (options.resource_profile_id != "playdh-current" &&
        options.resource_profile_id != "sworking-2008-reference") {
        std::fprintf(stderr, "mxh_client: unknown resource profile '%s'\n",
                     options.resource_profile_id.c_str());
        return 2;
    }
    g_overviewCamera = !options.save_frame.empty() && !options.follow_camera;
    g_debugUiBounds = options.debug_ui_bounds;
    __g_stateFramesDir = options.state_frames_dir;
    g_evidenceDir = options.evidence_dir;
    if (!__g_stateFramesDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(__g_stateFramesDir), ec);
    }
    MLOG_INFO("mxh_client: booting version %s", mxh::client::g_CLIENTVERSION);
    MLOG_INFO("mxh_client: login=%s:%u map-port=%u user=%s",
              options.login_host.c_str(), options.login_port,
              options.map_port, options.username.c_str());
    if (!g_evidenceDir.empty()) {
        MLOG_INFO("mxh_client: F12 human evidence capture directory=%s", g_evidenceDir.c_str());
    }

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
    {
        std::error_code profile_ec;
        const auto canonical_root = std::filesystem::weakly_canonical(
            options.resource_root, profile_ec).generic_string();
        const bool is_reference = canonical_root.find("reference/legacy-source/") !=
            std::string::npos;
        if ((options.resource_profile_id == "playdh-current" && is_reference) ||
            (options.resource_profile_id == "sworking-2008-reference" && !is_reference)) {
            std::fprintf(stderr,
                         "mxh_client: resource root does not match profile '%s'\n",
                         options.resource_profile_id.c_str());
            return 1;
        }
    }
    std::string resource_error;
    if (!validate_runtime_resources(options.resource_root, &resource_error)) {
        std::fprintf(stderr, "mxh_client: %s\n", resource_error.c_str());
        return 1;
    }
    g_mapChangeCatalog = mxh::compat::load_map_change_bin(
        options.resource_root / "Resource" / "MapChange.bin");
    if (!g_mapChangeCatalog) {
        if (!g_debugUiBounds) {
            std::fprintf(stderr,
                         "mxh_client: MapChange.bin unavailable or invalid\n");
            return 1;
        }
        MLOG_WARN("mxh_client: MapChange.bin unavailable in debug mode; NPC warp interaction disabled");
    } else {
        MLOG_INFO("mxh_client: MapChange.bin loaded entries=%u",
                  static_cast<unsigned>(g_mapChangeCatalog->entries.size()));
    }
    auto* storage = new mxh::gx::FilesystemFileStorage(options.resource_root);
    if (!storage->Initialize(0, 0, 0, FILE_ACCESS_METHOD_ONLY_FILE)) {
        std::fprintf(stderr, "mxh_client: invalid resource root\n");
        storage->Release();
        return 1;
    }
    MLOG_INFO("mxh_client: resource profile=%s root loaded",
              options.resource_profile_id.c_str());

    mxh::audio::BgmPlayer bgm;
    mxh::audio::SfxPlayer sfx;
    OptionRuntimeContext optionContext{&persisted_settings, &bgm, &sfx};
    g_bgmPlayer = &bgm;
    g_sfxPlayer = &sfx;
    g_bgmVolume = persisted_settings.bgm_volume;
    g_sfxVolume = persisted_settings.sfx_volume;
    std::string audio_error;
    if (bgm.initialize(options.resource_root / "Sound", &audio_error)) {
        bgm.setVolume(persisted_settings.bgm_volume);
        // 1667 is the original login theme in SoundList.bin.
        if (!bgm.play(1667, &audio_error) && !g_debugUiBounds) {
            MLOG_ERROR("mxh_client: required login BGM unavailable: %s",
                       audio_error.c_str());
            storage->Release();
            return 1;
        } else if (!audio_error.empty()) {
            MLOG_WARN("mxh_client: login BGM unavailable: %s", audio_error.c_str());
        }
    } else {
        if (!g_debugUiBounds) {
            MLOG_ERROR("mxh_client: required SoundList unavailable: %s",
                       audio_error.c_str());
            storage->Release();
            return 1;
        }
        MLOG_WARN("mxh_client: SoundList unavailable: %s", audio_error.c_str());
    }
    if (sfx.initialize(options.resource_root / "Sound", &audio_error)) {
        sfx.setVolume(persisted_settings.sfx_volume);
        // Prefer a named UI/button entry when the profile provides one; use
        // the first real WAV otherwise.  The choice is data-driven and never
        // invents a sound ID or touches resource bytes.
        for (const auto& entry : sfx.manifest().entries) {
            if (!entry.available || entry.streaming) continue;
            const auto name = entry.file_name;
            std::string lower_name = name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            const bool ui = lower_name.find("click") != std::string::npos ||
                lower_name.find("button") != std::string::npos || lower_name.find("ui") != std::string::npos;
            const bool attack = lower_name.find("attack") != std::string::npos ||
                lower_name.find("swing") != std::string::npos || lower_name.find("weapon") != std::string::npos;
            const bool skill = lower_name.find("skill") != std::string::npos ||
                lower_name.find("magic") != std::string::npos || lower_name.find("mugong") != std::string::npos;
            const bool pickup = lower_name.find("pickup") != std::string::npos ||
                lower_name.find("loot") != std::string::npos || lower_name.find("item") != std::string::npos;
            if (ui && g_uiClickSound == 0xffffu) {
                g_uiClickSound = entry.index;
            }
            if (attack && g_attackSound == 0xffffu) g_attackSound = entry.index;
            if (skill && g_skillSound == 0xffffu) g_skillSound = entry.index;
            if (pickup && g_pickupSound == 0xffffu) g_pickupSound = entry.index;
        }
        const auto wav_count = static_cast<std::size_t>(std::count_if(
            sfx.manifest().entries.begin(), sfx.manifest().entries.end(),
            [](const auto& entry) { return entry.available && !entry.streaming; }));
        MLOG_INFO("mxh_client: SFX manifest ready entries=%zu wav=%zu ui=%u attack=%u skill=%u pickup=%u",
                  sfx.manifest().entries.size(), wav_count,
                  static_cast<unsigned>(g_uiClickSound),
                  static_cast<unsigned>(g_attackSound),
                  static_cast<unsigned>(g_skillSound),
                  static_cast<unsigned>(g_pickupSound));
        if (!g_debugUiBounds && (g_uiClickSound == 0xffffu ||
                                 g_attackSound == 0xffffu ||
                                 g_skillSound == 0xffffu ||
                                 g_pickupSound == 0xffffu)) {
            MLOG_ERROR("mxh_client: required semantic SFX mapping is incomplete");
            storage->Release();
            return 1;
        }
    } else {
        if (!g_debugUiBounds) {
            MLOG_ERROR("mxh_client: required SFX subsystem unavailable: %s",
                       audio_error.c_str());
            storage->Release();
            return 1;
        }
        MLOG_WARN("mxh_client: SFX unavailable: %s", audio_error.c_str());
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
    renderer->SetVerticalSync(options.vsync ? TRUE : FALSE);
    g_renderer = renderer;
    {
        auto preview = std::make_unique<mxh::gx::EntityScene>();
        std::string preview_error;
        if (preview->load(renderer, storage, &preview_error)) {
            // A missing appearance is a hard, visible failure in release;
            // placeholder boxes remain disabled for this product path.
            preview->setPlaceholderRenderingEnabled(false);
            g_charPreviewScene = std::move(preview);
            MLOG_INFO("mxh_client: character preview scene ready");
        } else {
            MLOG_ERROR("mxh_client: character preview scene unavailable: %s",
                       preview_error.c_str());
        }
    }
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
        if (!cResourceManager::getInstance().loadedFrom(image_dir)) {
            if (!cResourceManager::getInstance().InitScriptManager(image_dir)) {
                std::fprintf(stderr,
                             "mxh_client: playdh-current image path tables are incomplete; refusing to start\n");
                MLOG_ERROR("mxh_client: M-R1 cResourceManager init failed; resource profile rejected");
                return 3;
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
            if (!cSpriteAtlas::getInstance().Init(options.resource_root)) {
                std::fprintf(stderr,
                             "mxh_client: playdh-current sprite atlas is incomplete; refusing to start\n");
                MLOG_ERROR("mxh_client: M-R2 cSpriteAtlas init failed; resource profile rejected");
                return 3;
            }
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

    // All visible dialogs are owned by the active state/UI runtime.  The
    // product path never creates synthetic dialogs or debug-only roots.

    // -------------------------------------------------------------------------
    // Wire CMainGame + CEngine and the complete nine-state client table.
    //
    // CEngine gets the HWND and the IRenderer so future states can
    // look them up. CMainGame owns the state table; we register each
    // concrete state implementation. The boot transition
    // (Engine -> CMainTitle) goes through CMainGame::SetGameState so
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
    mainGame.GetEngine()->SetAudioEventFn(
        [&sfx](mxh::client::CEngine::AudioCue cue) {
            // Skill sounds are emitted by the decoded BEFF timeline below;
            // keeping this generic cue would play a guessed fallback in
            // parallel with the authoritative SoundList id.
            if (cue == mxh::client::CEngine::AudioCue::UiClick ||
                cue == mxh::client::CEngine::AudioCue::Skill) return;
            const auto sound = cue == mxh::client::CEngine::AudioCue::Attack
                ? g_attackSound
                : (cue == mxh::client::CEngine::AudioCue::Pickup ? g_pickupSound : g_skillSound);
            if (sound == 0xffffu) return;
            std::string audio_error;
            (void)sfx.play(sound, &audio_error);
        });
    mainGame.GetEngine()->SetSpatialAudioEventFn(
        [&sfx](mxh::client::CEngine::AudioCue cue, float distance) {
            if (cue == mxh::client::CEngine::AudioCue::Skill) return;
            const auto sound = cue == mxh::client::CEngine::AudioCue::Attack
                ? g_attackSound
                : (cue == mxh::client::CEngine::AudioCue::Pickup ? g_pickupSound : g_skillSound);
            if (sound == 0xffffu) return;
            std::string audio_error;
            (void)sfx.playAt(sound, distance, &audio_error);
        });
    mainGame.GetEngine()->SetSoundEventFn(
        [&sfx](std::uint32_t sound_id, float distance) {
            if (sound_id == 0 || sound_id > 0xffffu) return;
            std::string audio_error;
            (void)sfx.playAt(static_cast<std::uint16_t>(sound_id), distance, &audio_error);
        });
    mainGame.RegisterState(mxh::client::GameStateId::Intro,      std::make_unique<mxh::client::CIntroReplay>());
    mainGame.RegisterState(mxh::client::GameStateId::Connect,    std::make_unique<mxh::client::CLoginState>());
    mainGame.RegisterState(mxh::client::GameStateId::Title,      std::make_unique<mxh::client::CMainTitle>());
    mainGame.RegisterState(mxh::client::GameStateId::CharSelect, std::make_unique<mxh::client::CCharSelectState>());
    mainGame.RegisterState(mxh::client::GameStateId::CharMake,   std::make_unique<mxh::client::CCharMake>());
    mainGame.RegisterState(mxh::client::GameStateId::GameLoading,std::make_unique<mxh::client::CGameLoading>());
    mainGame.RegisterState(mxh::client::GameStateId::GameIn,     std::make_unique<mxh::client::CInGameState>());
    mainGame.RegisterState(mxh::client::GameStateId::MapChange,  std::make_unique<mxh::client::CMapChange>());
    mainGame.RegisterState(mxh::client::GameStateId::MurimNet,   std::make_unique<mxh::client::CMurimNet>());

    // Normal launches enter the real title/login UI. Explicit automation
    // mode may enter the login state directly, but never changes release
    // behavior or injects credentials into the UI.
    mainGame.SetGameState(options.auto_login ? mxh::client::GameStateId::Connect
                                             : mxh::client::GameStateId::Title);

    MLOG_INFO("mxh_client: CMainGame initialised, 9 states registered, initial state=%s",
              options.auto_login ? "Connect" : "Title");

    // Phase B.2.1: kick off the CLoginState connect (host-driven; the
    // state can't self-start because it doesn't know the login address
    // until MHVerInfo.ver parsing lands in B.2.5).
    if (options.auto_login) {
        if (auto* login = dynamic_cast<mxh::client::CLoginState*>(
                mainGame.GetGameState(mxh::client::GameStateId::Connect))) {
            login->Start(mainGame.GetEngine(), options.login_host,
                         options.login_port, options.username, options.password);
            clear_secret(options.password);
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
    EffectVisualOverlay effectVisuals;
    g_effectVisuals = &effectVisuals;
    std::uint32_t pending_character_id = 0;
    std::uint16_t pending_map_num = 0;
    std::string pending_loading_error;
    mxh::client::GameLoadingCoordinator loadingCoordinator;
    g_loadingCoordinator = &loadingCoordinator;
    std::unique_ptr<GameWorldLoadSession> worldLoadSession;
    bool map_change_from_game = false;
    bool game_loading_frame_presented = false;
    bool post_login_display_applied = false;
    bool login_failure_presented = false;
    bool charselect_failure_presented = false;
    bool charmake_failure_presented = false;
    bool auto_create_requested = false;
    bool loading_failure_latched = false;
    bool follow_frame_captured = false;
    bool smoke_inventory_opened = false;
    bool smoke_skill_logged = false;
    bool smoke_appearance_rotated = false;
    unsigned smoke_inventory_settle_frames = 0;
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
        // A headless/GUI smoke run must be able to finish immediately after
        // the authoritative GameInAck.  The first in-game render can lazily
        // resolve hundreds of legacy model dependencies; do not let that
        // presentation work mask a successful network/state transition.
        const bool smoke_open_inventory = std::getenv("MXH_GUI_SMOKE_OPEN_INVENTORY") != nullptr;
        if (smoke_open_inventory && smoke_inventory_opened) {
            ++smoke_inventory_settle_frames;
        }
        if (options.exit_after_gamein && !options.follow_camera &&
            (!smoke_open_inventory ||
             (smoke_inventory_opened && smoke_inventory_settle_frames >= 5u)) &&
            mainGame.GetCurStateNum() == mxh::client::GameStateId::GameIn) {
            if (auto* game_in = dynamic_cast<mxh::client::CInGameState*>(
                    mainGame.GetGameState(mxh::client::GameStateId::GameIn));
                game_in && game_in->is_in_game() && game_in->smoke_exit_ready()) {
                MLOG_INFO("mxh_client: GUI_SMOKE_PASS player_id=%u map=%u",
                          game_in->player_id(), game_in->map_num());
                mxh::client::g_running = false;
                break;
            }
        }
            if (g_mainTitle && g_mainTitle->consumeSubmit()) {
                options.username = g_mainTitle->username();
                options.password = g_mainTitle->password();
                loading_failure_latched = false;
                auto_create_requested = false;
                mainGame.SetGameState(mxh::client::GameStateId::Connect);
                if (auto* login = dynamic_cast<mxh::client::CLoginState*>(
                        mainGame.GetGameState(mxh::client::GameStateId::Connect))) {
                    login->Start(mainGame.GetEngine(), options.login_host,
                                 options.login_port, options.username, options.password);
                    clear_secret(options.password);
                    login_failure_presented = false;
                }
            }
            // Idle: drive CMainGame + render a frame.
            mainGame.Process();
            if (!smoke_appearance_rotated &&
                std::getenv("MXH_GUI_SMOKE_ROTATE_APPEARANCE") != nullptr &&
                mainGame.GetCurStateNum() == mxh::client::GameStateId::CharMake) {
                if (auto* make = dynamic_cast<mxh::client::CCharMake*>(
                        mainGame.GetGameState(mxh::client::GameStateId::CharMake));
                    make) {
                    const bool sex = make->RotateAppearanceOption(mxh::client::CharMakeOptionCategory::Sex, 1);
                    const bool face = make->RotateAppearanceOption(mxh::client::CharMakeOptionCategory::MaleFace, 1);
                    const bool hair = make->RotateAppearanceOption(mxh::client::CharMakeOptionCategory::MaleHair, 1);
                    const bool cloth = make->RotateAppearanceOption(mxh::client::CharMakeOptionCategory::Cloth, 1);
                    const bool weapon = make->RotateAppearanceOption(mxh::client::CharMakeOptionCategory::Weapon, 1);
                    const auto& p = make->form_model().params();
                    MLOG_INFO("mxh_client: GUI_SMOKE_APPEARANCE_ROTATED sex=%u face=%u hair=%u cloth=%u weapon=%u applied=%d%d%d%d%d",
                              static_cast<unsigned>(p.sex_type),
                              static_cast<unsigned>(p.face_type),
                              static_cast<unsigned>(p.hair_type),
                              static_cast<unsigned>(p.weared_item_idx[2]),
                              static_cast<unsigned>(p.weared_item_idx[5]),
                              sex ? 1 : 0, face ? 1 : 0, hair ? 1 : 0,
                              cloth ? 1 : 0, weapon ? 1 : 0);
                    smoke_appearance_rotated = sex || face || hair || cloth || weapon;
                }
            }
            if (!smoke_inventory_opened &&
                std::getenv("MXH_GUI_SMOKE_OPEN_INVENTORY") != nullptr &&
                mainGame.GetCurStateNum() == mxh::client::GameStateId::GameIn) {
                if (auto* smoke_game = dynamic_cast<mxh::client::CInGameState*>(
                        mainGame.GetGameState(mxh::client::GameStateId::GameIn));
                    smoke_game && smoke_game->is_in_game()) {
                    smoke_game->OnKeyEvent(true, 0x49u);
                    smoke_game->OnKeyEvent(false, 0x49u);
                    g_inputTarget = smoke_game;
                    smoke_inventory_opened = smoke_game->inventory_open();
                    std::size_t occupied_slots = 0;
                    std::uint16_t first_item_icon = 0;
                    for (const auto& item : smoke_game->game_info().items.Inventory) {
                        if (!mxh::game::is_empty_slot(item)) {
                            ++occupied_slots;
                            if (first_item_icon == 0) first_item_icon = item.wIconIdx;
                        }
                    }
                    MLOG_INFO("mxh_client: GUI_SMOKE_INVENTORY_OPEN=%s",
                              smoke_inventory_opened ? "true" : "false");
                    MLOG_INFO("mxh_client: GUI_SMOKE_INVENTORY_ITEMS=%zu first_item=%u",
                              occupied_slots, static_cast<unsigned>(first_item_icon));
                    if (std::getenv("MXH_GUI_SMOKE_SKILLS") != nullptr) {
                        MLOG_INFO("mxh_client: GUI_SMOKE_SKILLS=%u,%u,%u,%u",
                                  static_cast<unsigned>(quick_skill_for_slot(smoke_game->game_info(), 0)),
                                  static_cast<unsigned>(quick_skill_for_slot(smoke_game->game_info(), 1)),
                                  static_cast<unsigned>(quick_skill_for_slot(smoke_game->game_info(), 2)),
                                  static_cast<unsigned>(quick_skill_for_slot(smoke_game->game_info(), 3)));
                    }
                }
            }
            if (!smoke_skill_logged &&
                std::getenv("MXH_GUI_SMOKE_SKILLS") != nullptr &&
                mainGame.GetCurStateNum() == mxh::client::GameStateId::GameIn) {
                if (auto* smoke_game = dynamic_cast<mxh::client::CInGameState*>(
                        mainGame.GetGameState(mxh::client::GameStateId::GameIn));
                    smoke_game && smoke_game->is_in_game()) {
                    MLOG_INFO("mxh_client: GUI_SMOKE_SKILLS=%u,%u,%u,%u",
                              static_cast<unsigned>(quick_skill_for_slot(smoke_game->game_info(), 0)),
                              static_cast<unsigned>(quick_skill_for_slot(smoke_game->game_info(), 1)),
                              static_cast<unsigned>(quick_skill_for_slot(smoke_game->game_info(), 2)),
                              static_cast<unsigned>(quick_skill_for_slot(smoke_game->game_info(), 3)));
                    smoke_skill_logged = true;
                }
            }
            if (auto* smoke_game = dynamic_cast<mxh::client::CInGameState*>(
                    mainGame.GetGameState(mxh::client::GameStateId::GameIn));
                smoke_game && !options.follow_camera &&
                (!smoke_open_inventory || smoke_inventory_opened) &&
                (!smoke_open_inventory || smoke_inventory_settle_frames >= 5u) &&
                smoke_game->smoke_exit_ready()) {
                mxh::client::g_running = false;
            }
            if (!mxh::client::g_running) break;
            // Phase B.2.2: on state-change rising edge, Start() the
            // states that have an external Start() hook.
            const auto cur_state = mainGame.GetCurStateNum();
            if (cur_state == mxh::client::GameStateId::Connect) {
                if (auto* login = dynamic_cast<mxh::client::CLoginState*>(
                        mainGame.GetGameState(cur_state));
                    login && login->is_failed() && !login_failure_presented) {
                    if (auto* title = dynamic_cast<mxh::client::CMainTitle*>(
                            mainGame.GetGameState(mxh::client::GameStateId::Title))) {
                        title->clearPassword();
                    }
                    clear_secret(options.password);
                    pending_loading_error = login->failure_reason();
                    login_failure_presented = true;
                    mainGame.SetGameState(mxh::client::GameStateId::Title);
                }
            }
            if (cur_state != prev_state) {
                if ((prev_state == mxh::client::GameStateId::GameLoading ||
                     prev_state == mxh::client::GameStateId::MapChange) &&
                    cur_state != mxh::client::GameStateId::GameLoading &&
                    cur_state != mxh::client::GameStateId::MapChange) {
                    // A disconnect, cancel or external state transition can
                    // leave a staged session between frames. Resetting the
                    // owner here invokes its rollback destructor before the
                    // next state renders, so no half-built scene survives.
                    worldLoadSession.reset();
                }
                if (prev_state == mxh::client::GameStateId::GameIn) {
                    map_change_from_game =
                        cur_state == mxh::client::GameStateId::MapChange;
                    g_inputTarget = nullptr;
                    if (g_effectVisuals) g_effectVisuals->clear();
                    g_damageFeedback.clear();
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
                if (cur_state == mxh::client::GameStateId::GameLoading ||
                    cur_state == mxh::client::GameStateId::MapChange) {
                    game_loading_frame_presented = false;
                    g_renderTerrain = false;
                }
                if (cur_state == mxh::client::GameStateId::MapChange) {
                    g_mapChangeState = dynamic_cast<mxh::client::CMapChange*>(
                        mainGame.GetGameState(cur_state));
                    g_renderTerrain = false;
                } else {
                    g_mapChangeState = nullptr;
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
                    const auto transition = applyDisplayTransition(
                        hwnd, renderer, g_logicalViewport,
                        post_login_w, post_login_h, options.borderless);
                    if (!transition.committed) {
                        display_transition_ok = false;
                        MLOG_ERROR("mxh_client: post-login display transition failed error=%lu",
                                   static_cast<unsigned long>(transition.win32_error));
                    } else {
                        post_login_display_applied = true;
                        const auto ui_mode = mxh::ui::detect_from_screen_size(
                            static_cast<int>(post_login_w),
                            static_cast<int>(post_login_h));
                        mainGame.GetEngine()->SetUiResolutionMode(ui_mode);
                        if (g_mainTitle) {
                            g_mainTitle->ui_runtime().onResolutionChange(ui_mode);
                        }
                        InvalidateRect(hwnd, nullptr, FALSE);
                        MLOG_INFO("mxh_client: post-login display transition client=%ux%u",
                                  transition.client_width, transition.client_height);
                    }
                }
                if (display_transition_ok) {
                    if (auto* title = dynamic_cast<mxh::client::CMainTitle*>(
                            mainGame.GetGameState(mxh::client::GameStateId::Title))) {
                        title->clearPassword();
                    }
                    clear_secret(options.password);
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
                        if (!pending_loading_error.empty()) {
                            title->ui_runtime().showMessage(0x4D4C4552,
                                                            pending_loading_error);
                            pending_loading_error.clear();
                        }
                        g_mainTitle = title;
                    }
                } else if (cur_state == mxh::client::GameStateId::CharSelect) {
                    if (auto* cs = dynamic_cast<mxh::client::CCharSelectState*>(
                            mainGame.GetGameState(cur_state))) {
                        charselect_failure_presented = false;
                        cs->set_auto_select_for_test(
                            options.auto_create && !loading_failure_latched);
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
                        charmake_failure_presented = false;
                        cm->Start(mainGame.GetEngine());
                    }
                } else if (cur_state == mxh::client::GameStateId::GameLoading) {
                    if (auto* loading = dynamic_cast<mxh::client::CGameLoading*>(
                            mainGame.GetGameState(cur_state))) {
                        loading->Start(mainGame.GetEngine());
                        g_gameLoadingState = loading;
                    }
                } else if (cur_state == mxh::client::GameStateId::MapChange) {
                    if (auto* change = dynamic_cast<mxh::client::CMapChange*>(
                            mainGame.GetGameState(cur_state))) {
                        change->Start(mainGame.GetEngine());
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
                        if (auto* option = g->option_dialog()) {
                            option->SetDefaultCallbackForTest(&option_default_callback,
                                                               &optionContext);
                            option->SetApplyCallbackForTest(&option_apply_callback,
                                                             &optionContext);
                            option->gameOption().nVolumnBGM = static_cast<int>(
                                std::lround(persisted_settings.bgm_volume * 100.0f));
                            option->gameOption().nVolumnEnvironment = static_cast<int>(
                                std::lround(persisted_settings.ambient_volume * 100.0f));
                            option->UpdateData(false);
                        }
                        g_inputTarget = g;
                        // World loading commits before the new GameIn state
                        // is constructed. Rebind authoritative terrain bounds
                        // and static collision to this fresh input target;
                        // otherwise MapChange would leave the new player with
                        // the protocol-wide default movement limits.
                        if (g_terrain) {
                            g->set_world_bounds(g_terrain->worldWidth(),
                                                g_terrain->worldHeight());
                        }
                        if (g_staticScene) {
                            auto* static_scene = g_staticScene.get();
                            g->set_collision_query(
                                [static_scene](float x, float z, float radius) {
                                    return static_scene->blocksPoint(x, z, radius);
                                });
                        }
                        g->set_map_change_target_resolver(
                            [](std::uint32_t npc_id, std::uint16_t current_map)
                                -> std::optional<std::uint16_t> {
                                if (!g_mapChangeCatalog || !g_inputTarget) return std::nullopt;
                                const auto npc = std::find_if(
                                    g_inputTarget->npcs().begin(),
                                    g_inputTarget->npcs().end(),
                                    [npc_id](const auto& value) {
                                        return value.npc_id == npc_id;
                                    });
                                if (npc == g_inputTarget->npcs().end()) return std::nullopt;
                                // MapChange.bin stores the legacy object name;
                                // match it against the live NPC name and current
                                // map so the destination comes from profile data.
            const auto* route = g_mapChangeCatalog->find_object_destination(
                current_map, npc->name);
            return route ? std::optional<std::uint16_t>(route->move_map_num)
                         : std::nullopt;
        });
                    }
                }
                prev_state = cur_state;
            }
            if (cur_state == mxh::client::GameStateId::CharSelect) {
                if (auto* cs = dynamic_cast<mxh::client::CCharSelectState*>(
                        mainGame.GetGameState(cur_state));
                    cs && cs->is_failed() && !charselect_failure_presented) {
                    charselect_failure_presented = true;
                    pending_loading_error = cs->failure_reason();
                    MLOG_ERROR("CharSelect: %s", pending_loading_error.c_str());
                    mainGame.SetGameState(mxh::client::GameStateId::Title);
                }
            }
            if (cur_state == mxh::client::GameStateId::CharMake) {
                if (auto* cm = dynamic_cast<mxh::client::CCharMake*>(
                        mainGame.GetGameState(cur_state));
                    cm && cm->is_failed() && !charmake_failure_presented) {
                    charmake_failure_presented = true;
                    pending_loading_error = cm->failure_reason();
                    MLOG_ERROR("CharMake: %s", pending_loading_error.c_str());
                    mainGame.GetEngine()->SetPendingTransfer(cm->login_result());
                    mainGame.SetGameState(mxh::client::GameStateId::CharSelect);
                }
            }
            if (cur_state == mxh::client::GameStateId::GameIn) {
                if (auto* game_in = dynamic_cast<mxh::client::CInGameState*>(
                        mainGame.GetGameState(cur_state));
                    game_in && game_in->is_failed()) {
                    pending_loading_error = game_in->failure_reason();
                    if (pending_loading_error.empty()) {
                        pending_loading_error = "游戏连接已断开";
                    }
                    MLOG_ERROR("GameIn: %s", pending_loading_error.c_str());
                    mainGame.SetGameState(mxh::client::GameStateId::Title);
                }
            }
            // GameLoading consumer must run every frame when the state is
            // GameLoading, because the CCharSelectState TCP recv thread may
            // set the GameEntryRequest transfer AFTER the state-transition
            // edge fires. Restricting to the rising-edge check loses the
            // transfer and the state machine stalls.
            if (cur_state == mxh::client::GameStateId::GameLoading ||
                cur_state == mxh::client::GameStateId::MapChange) {
                if (!game_loading_frame_presented) {
                    game_loading_frame_presented = true;
                } else {
                    std::string transferError;
                    const auto state_start_failure = [&]() -> std::optional<std::string> {
                        if (cur_state == mxh::client::GameStateId::GameLoading) {
                            if (const auto* loading = dynamic_cast<const mxh::client::CGameLoading*>(
                                    mainGame.GetGameState(cur_state));
                                loading && loading->failed()) {
                                return loading->error();
                            }
                        } else if (const auto* change = dynamic_cast<const mxh::client::CMapChange*>(
                                       mainGame.GetGameState(cur_state));
                                   change && change->failed()) {
                            return change->error();
                        }
                        return std::nullopt;
                    }();
                    if (!worldLoadSession && state_start_failure) {
                        pending_loading_error = *state_start_failure;
                        MLOG_ERROR("GameLoading: %s", pending_loading_error.c_str());
                        const bool restore_existing_game =
                            mxh::client::can_restore_previous_game_after_map_change(
                                map_change_from_game,
                                static_cast<bool>(g_terrain),
                                static_cast<bool>(g_staticScene),
                                static_cast<bool>(g_entityScene));
                        mainGame.SetGameState(
                            restore_existing_game
                                ? mxh::client::GameStateId::GameIn
                                : mxh::client::GameStateId::CharSelect);
                        map_change_from_game = false;
                        continue;
                    }
                    if (!worldLoadSession && mainGame.GetEngine()->has_pending_transfer() &&
                        loadingCoordinator.consume_pending_transfer(
                            *mainGame.GetEngine(), &transferError)) {
                        if (cur_state == mxh::client::GameStateId::GameLoading) {
                            if (auto* loading = dynamic_cast<mxh::client::CGameLoading*>(
                                    mainGame.GetGameState(cur_state))) {
                                loading->set_context(&loadingCoordinator.context());
                            }
                        } else if (auto* change = dynamic_cast<mxh::client::CMapChange*>(
                                       mainGame.GetGameState(cur_state))) {
                            change->set_context(&loadingCoordinator.context());
                        }
                        worldLoadSession = std::make_unique<GameWorldLoadSession>(
                            options, renderer, storage, bgm,
                            loadingCoordinator.request().map_num);
                    } else if (!worldLoadSession && !transferError.empty() &&
                               transferError != "waiting for GameEntryRequest") {
                        pending_loading_error = transferError;
                        MLOG_ERROR("GameLoading: %s", pending_loading_error.c_str());
                        mainGame.SetGameState(
                            mxh::client::GameStateId::CharSelect);
                    }
                    if (worldLoadSession) {
                        std::string loadingError;
                        if (loadingCoordinator.context().cancelled) {
                            pending_loading_error = "Map loading cancelled";
                            mainGame.SetGameState(
                                map_change_from_game
                                    ? mxh::client::GameStateId::GameIn
                                    : mxh::client::GameStateId::CharSelect);
                            worldLoadSession.reset();
                            map_change_from_game = false;
                            continue;
                        }
                        const auto result = worldLoadSession->advance(
                            [&loadingCoordinator](std::uint32_t step) {
                                loadingCoordinator.mark_completed(step);
                            }, loadingError);
                        if (result == GameWorldLoadSession::Result::Complete) {
                            pending_character_id = loadingCoordinator.request().character_id;
                            pending_map_num = loadingCoordinator.request().map_num;
                            worldLoadSession.reset();
                            mainGame.SetGameState(mxh::client::GameStateId::GameIn);
                        } else if (result == GameWorldLoadSession::Result::Failed) {
                            pending_loading_error = "Unable to enter Map " +
                                std::to_string(loadingCoordinator.request().map_num) + ": " + loadingError;
                            loadingCoordinator.mark_failed(pending_loading_error);
                            loading_failure_latched = true;
                            MLOG_ERROR("GameLoading: %s", pending_loading_error.c_str());
                            const bool restore_existing_game =
                                mxh::client::can_restore_previous_game_after_map_change(
                                    map_change_from_game,
                                    static_cast<bool>(g_terrain),
                                    static_cast<bool>(g_staticScene),
                                    static_cast<bool>(g_entityScene));
                            if (restore_existing_game) {
                                if (auto* previous_game =
                                        dynamic_cast<mxh::client::CInGameState*>(
                                            mainGame.GetGameState(
                                                mxh::client::GameStateId::GameIn))) {
                                    g_inputTarget = previous_game;
                                }
                            }
                            mainGame.SetGameState(
                                restore_existing_game
                                    ? mxh::client::GameStateId::GameIn
                                    : mxh::client::GameStateId::CharSelect);
                            map_change_from_game = false;
                            worldLoadSession.reset();
                            if (options.exit_after_gamein) {
                                // Automated smoke runs must report the
                                // authoritative failure immediately instead
                                // of entering the manual-recovery loop.
                                mxh::client::g_running = false;
                            }
                        }
                    }
                }
            }
            if (cur_state == mxh::client::GameStateId::CharSelect &&
                options.auto_create && !auto_create_requested &&
                !loading_failure_latched) {
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
                    !cm->is_failed() &&
                    (std::getenv("MXH_GUI_SMOKE_ROTATE_APPEARANCE") == nullptr ||
                     smoke_appearance_rotated)) {
                    mxh::client::CharacterMakeParams params =
                        cm->form_model().params();
                    params.name = options.character_name;
                    (void)cm->SubmitCharacter(params);
                }
            } else if (cur_state == mxh::client::GameStateId::GameIn &&
                       options.exit_after_gamein) {
                if (auto* game_in = dynamic_cast<mxh::client::CInGameState*>(
                        mainGame.GetGameState(cur_state));
                    game_in && game_in->is_in_game() &&
                    ([&] {
                        const char* value = std::getenv("MXH_GUI_SMOKE_EXIT");
                        const bool require_entities = value && *value == '1';
                        if (require_entities && game_in->map_num() == 10)
                            return game_in->monsters().size() >= 228;
                        return !require_entities && !options.follow_camera
                            ? true
                            : !game_in->monsters().empty() || !game_in->npcs().empty();
                    }())) {
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
                    for (const auto& effect : game_in->drain_runtime_effect_events()) {
                        if (g_effectVisuals) {
                            g_effectVisuals->consume(effect, renderer);
                        }
                    }
                    for (const auto& effect : game_in->drain_effect_events()) {
                        if (effect.kind != mxh::client::EffectEventKind::Hit ||
                            effect.damage == 0) continue;
                        if (g_damageFeedback.size() >= 256u)
                            g_damageFeedback.erase(g_damageFeedback.begin());
                        g_damageFeedback.push_back({effect.target_object_id,
                                                    effect.damage,
                                                    effect.timestamp_ms,
                                                    900u});
                    }
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
                    const bool gui_smoke_exit = [] {
                        const char* value = std::getenv("MXH_GUI_SMOKE_EXIT");
                        const char* render = std::getenv("MXH_GUI_SMOKE_RENDER_ENTITIES");
                        return value && *value == '1' && !(render && *render == '1');
                    }();
                    if (g_entityScene && g_terrain && !gui_smoke_exit) {
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
                        static std::uint32_t last_unresolved_textures = ~0u;
                        const auto loaded = g_entityScene->loadedModelCount();
                        const auto failed = g_entityScene->failedModelCount();
                        const auto ph = g_entityScene->placeholderCount();
                        const auto unresolvedTextures =
                            g_entityScene->unresolvedTextureCount();
                        if (loaded != last_loaded || failed != last_failed ||
                            ph != last_ph ||
                            unresolvedTextures != last_unresolved_textures) {
                            last_loaded = loaded;
                            last_failed = failed;
                            last_ph = ph;
                            last_unresolved_textures = unresolvedTextures;
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
                            if (unresolvedTextures != 0 && !g_debugUiBounds) {
                                MLOG_ERROR(
                                    "mxh_client: release visual gate failed: "
                                    "unresolvedEntityTextures=%u",
                                    unresolvedTextures);
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
    mainGame.PrepareForProcessExit();
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
    for (auto& [cache_key, icon] : g_atlas_icons) {
        if (icon.sprite) icon.sprite->Release();
        icon.sprite = nullptr;
    }
    g_atlas_icons.clear();
    for (auto& e : g_sprites) {
        if (e.sprite) e.sprite->Release();
        e.sprite = nullptr;
    }
    g_sprites = {};
    storage->Release();
    g_loadingCoordinator = nullptr;
    g_bgmPlayer = nullptr;
    g_sfxPlayer = nullptr;

    return 0;
}
