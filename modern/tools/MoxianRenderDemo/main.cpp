// MoxianRenderDemo: 3D mesh + 2D HUD + Effect/Material smoke test.
// Opens a window, creates the DX11 renderer, draws a lit textured cube
// (CreateMeshObject + RenderMeshObject path) plus a wireframe ground grid
// (RenderBox + RenderGrid paths), and overlays a 2D HUD via
// CreateEmptySpriteObject (RenderSprite path) and CreateFontObject
// (RenderFont path). Also exercises the Effect Shader Palette
// (CreateEffectShaderPalette → buildFromDesc) and Material Set
// (CreateMaterialSet) CPU-side init paths.
//
// Phase 5.10 / Phase 6+ / Phase 7:
//   - 3D path: cube + grid + fog (Phase 5.10 markers: feature level,
//     device, CoD3DDeviceDX11, Fog enabled)
//   - 2D path: sprite + font (Phase 6+ markers: SpriteObject created,
//     FontObject ready)
//   - Effect+Material path (Phase 7): effect palette + material set
//     markers: "[effect] palette built", "[material] MaterialSet created"
// The smoke harness (run_demo_smoke.py) verifies all three init paths.
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>

#include <d3d11.h>
#include <wrl/client.h>

#include "mxh/render/IRenderer.hpp"
#include "mxh/render/IFileStorage.hpp"
#include "mxh/render/render_typedef.hpp"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace mxh::gx;
using Microsoft::WRL::ComPtr;

namespace {

// Minimal stub I4DyuchiFileStorage — demo doesn't actually load files.
class StubFileStorage : public I4DyuchiFileStorage {
public:
    StubFileStorage() {}
    virtual ~StubFileStorage() {}
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

// Build a 2x2 RGB checker texture and create an SRV on the given device.
ComPtr<ID3D11ShaderResourceView> makeCheckerSRV(ID3D11Device* dev, std::uint32_t size) {
    std::vector<std::uint32_t> pixels(size * size);
    for (std::uint32_t y = 0; y < size; ++y) {
        for (std::uint32_t x = 0; x < size; ++x) {
            bool on = ((x / (size / 2)) + (y / (size / 2))) % 2 == 0;
            pixels[y * size + x] = on ? 0xffffffffu : 0xff404040u;  // BGR? no, RGBA8 -> ABGR? DX is BGRA
        }
    }
    D3D11_TEXTURE2D_DESC td{};
    td.Width = size; td.Height = size; td.MipLevels = 1;
    td.ArraySize = 1; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = pixels.data(); sd.SysMemPitch = size * 4;
    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(dev->CreateTexture2D(&td, &sd, &tex))) return nullptr;
    ComPtr<ID3D11ShaderResourceView> srv;
    dev->CreateShaderResourceView(tex.Get(), nullptr, &srv);
    return srv;
}

// Build a 4x4 MATRIX4 from a column-major 4x4 float[16]. We need this because
// D3DX / XMMATRIX-style matrices are column-major, but our MATRIX4 struct is
// row-major. So when copying in, transpose.
MATRIX4 fromColMajor(const float c[16]) {
    MATRIX4 m{};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m.m[i][j] = c[j * 4 + i];
    return m;
}

void buildViewProj(MATRIX4& view, MATRIX4& proj) {
    // Camera at (0, 1.5, -4) looking at (0, 0, 0), up = +Y.
    VECTOR3 eye{ 0.0f, 1.5f, -4.0f };
    VECTOR3 tgt{ 0.0f, 0.0f,  0.0f };
    VECTOR3 up { 0.0f, 1.0f,  0.0f };
    VECTOR3 z{ eye.x - tgt.x, eye.y - tgt.y, eye.z - tgt.z };  // forward
    float zl = std::sqrt(z.x*z.x + z.y*z.y + z.z*z.z);
    z = { z.x/zl, z.y/zl, z.z/zl };
    VECTOR3 x{ up.y*z.z - up.z*z.y, up.z*z.x - up.x*z.z, up.x*z.y - up.y*z.x };
    float xl = std::sqrt(x.x*x.x + x.y*x.y + x.z*x.z);
    x = { x.x/xl, x.y/xl, x.z/xl };
    VECTOR3 y{ z.y*x.z - z.z*x.y, z.z*x.x - z.x*x.z, z.x*x.y - z.y*x.x };
    float vc[16] = {
        x.x, x.y, x.z, -(x.x*eye.x + x.y*eye.y + x.z*eye.z),
        y.x, y.y, y.z, -(y.x*eye.x + y.y*eye.y + y.z*eye.z),
        z.x, z.y, z.z, -(z.x*eye.x + z.y*eye.y + z.z*eye.z),
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    view = fromColMajor(vc);

    // Perspective FOV = π/3, aspect = 4/3, near = 0.1, far = 100.
    float fov = 3.14159265f / 3.0f;
    float f   = 1.0f / std::tan(fov * 0.5f);
    float aspect = 800.0f / 600.0f;
    float zn = 0.1f, zf = 100.0f;
    float pc[16] = {
        f/aspect, 0.0f, 0.0f,             0.0f,
        0.0f,     f,    0.0f,             0.0f,
        0.0f,     0.0f, zf/(zf-zn),       1.0f,
        0.0f,     0.0f, -zn*zf/(zf-zn),   0.0f,
    };
    proj = fromColMajor(pc);
}

} // namespace

static I4DyuchiGXRenderer* g_renderer  = nullptr;
static bool                g_running    = true;
static IDIMeshObject*      g_cube       = nullptr;
static IDISpriteObject*    g_hudSprite  = nullptr;  // 2D HUD sprite (Phase 6+)
static IDIFontObject*      g_hudFont    = nullptr;  // 2D HUD font   (Phase 6+)
static ComPtr<ID3D11ShaderResourceView> g_cubeSRV;
static float               g_angle      = 0.0f;
static std::uint32_t       g_frames     = 0;

void renderFrame(HWND h) {
    if (!g_renderer) return;
    g_angle += 0.01f;
    if (g_angle > 6.2831853f) g_angle -= 6.2831853f;

    MATRIX4 view, proj;
    buildViewProj(view, proj);
    VIEW_VOLUME vv{};
    CAMERA_DESC cam{};
    cam.v3From = { 0.0f, 1.5f, -4.0f };
    cam.v3To   = { 0.0f, 0.0f,  0.0f };
    cam.v3Up   = { 0.0f, 1.0f,  0.0f };
    cam.fFovY  = 3.14159265f / 3.0f;
    cam.fAspect = 800.0f / 600.0f;
    cam.fNear = 0.1f; cam.fFar = 100.0f;
    MATRIX4 bb{};
    bb._11 = bb._22 = bb._33 = bb._44 = 1.0f;
    g_renderer->SetViewFrusturm(&vv, &cam, &view, &proj, &bb);

    // 1. Clear to dark blue.
    g_renderer->BeginRender(nullptr, 0xff202080, 0);

    // 2. Draw a wireframe ground grid (ortho-ish top-down).
    VECTOR3 grid[4] = {
        { -3.0f, -1.0f, -3.0f }, {  3.0f, -1.0f, -3.0f },
        {  3.0f, -1.0f,  3.0f }, { -3.0f, -1.0f,  3.0f },
    };
    g_renderer->RenderGrid(grid, 0xff00ff00);

    // 3. Draw the lit textured cube.
    if (g_cube) {
        g_renderer->RenderMeshObject(g_cube, 0, 0.0f, 0xff,
                                     nullptr, 0, nullptr, 0,
                                     0, 0, 0);
    }

    // 4. 2D HUD overlay (Phase 6+). Sprite + font live in screen space, so
    // they don't need a view/proj — RenderSprite/RenderFont are pixel-coord
    // calls that go through the SpriteObject/FontObject internal pipeline.
    if (g_hudSprite) {
        VECTOR2 scale{ 1.0f, 1.0f };
        VECTOR2 trans{ 10.0f, 10.0f };
        RECT     rc  { 0, 0, 128, 64 };
        g_renderer->RenderSprite(g_hudSprite, &scale, 0.0f, &trans, &rc,
                                 0xffffffffu, /*iZOrder=*/0, /*dwFlag=*/0);
    }
    if (g_hudFont) {
        const char* text = "Moxian Render Demo (HUD)";
        RECT trc{ 150, 10, 800, 50 };
        g_renderer->RenderFont(g_hudFont,
                               reinterpret_cast<TCHAR*>(const_cast<char*>(text)),
                               static_cast<std::uint32_t>(std::strlen(text)),
                               &trc, 0xffffffffu,
                               CHAR_CODE_TYPE_ASCII, /*iZOrder=*/0, /*dwFlag=*/0);
    }

    g_renderer->EndRender();
    g_renderer->Present(h);
    ++g_frames;
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CLOSE:
    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        return 0;
    case WM_PAINT: {
        renderFrame(h);
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

    // Build a cube via CreateMeshObject.
    g_cube = renderer->CreateMeshObject(CMeshFlag());
    if (g_cube) {
        // Get the underlying DX11 device via the test surface.
        ID3D11Device* dev = nullptr;
        // The renderer doesn't expose the raw device publicly, but we can
        // build a checker texture by re-using the ID3D11Device queried via
        // GetD3DDevice.
        if (renderer->GetD3DDevice(__uuidof(IUnknown), reinterpret_cast<void**>(&dev))) {
            g_cubeSRV = makeCheckerSRV(dev, 64);
            // Texture binding: rely on RenderMeshObject's default white/no-texture path
            // (the real game uses mtrl system; demo skips it for simplicity)
        }
        // Initialize cube via MESH_DESC path.
        MESH_DESC md{};
        constexpr std::uint32_t N = 24;  // 6 faces × 4 verts
        std::vector<VECTOR3> positions(N);
        std::vector<TVERTEX> tex(N);
        std::vector<VECTOR3> normals(N);
        // 6 faces, 4 verts each, face normals (matches MeshObject::initializeCube).
        struct Face { float nx, ny, nz; };
        Face faces[6] = {
            {  0,  0, -1 }, {  0,  0,  1 }, { -1,  0,  0 },
            {  1,  0,  0 }, {  0,  1,  0 }, {  0, -1,  0 },
        };
        float corners[8][3] = {
            {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
            {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        };
        int faceQuads[6][4] = {
            { 0, 1, 2, 3 }, { 5, 4, 7, 6 }, { 4, 0, 3, 7 },
            { 1, 5, 6, 2 }, { 3, 2, 6, 7 }, { 4, 5, 1, 0 },
        };
        for (int f = 0; f < 6; ++f) {
            for (int v = 0; v < 4; ++v) {
                int idx = f * 4 + v;
                int ci  = faceQuads[f][v];
                positions[idx] = { corners[ci][0], corners[ci][1], corners[ci][2] };
                tex[idx] = { (v == 1 || v == 2) ? 1.0f : 0.0f,
                             (v == 2 || v == 3) ? 1.0f : 0.0f };
                normals[idx] = { faces[f].nx, faces[f].ny, faces[f].nz };
            }
        }
        md.dwVertexNum     = N;
        md.pv3WorldList    = positions.data();
        md.dwTexVertexNum  = N;
        md.ptvTexCoordList = tex.data();
        md.pv3NormalLocal  = normals.data();
        md.meshFlag        = CMeshFlag();

        g_cube->StartInitialize(&md, nullptr, nullptr);
        std::vector<std::uint16_t> idx(36);
        for (std::uint32_t i = 0; i < 36; ++i) idx[i] = static_cast<std::uint16_t>(i);
        FACE_DESC fd{};
        fd.pIndex     = idx.data();
        fd.dwFacesNum = 12;
        fd.dwMtlIndex = 0;
        g_cube->InsertFaceGroup(&fd);
        g_cube->EndInitialize();
        // Checker texture created but not bound to mesh face groups directly.
        // RenderMeshObject will use the default lighting path (no texture needed for demo).
        (void)g_cubeSRV;
    }

    // Optional fog.
    renderer->EnableFog(3.0f, 8.0f, 1.0f, 0xff101030, 0);

    // === Phase 6+: 2D HUD pipeline (sprite + font) ===========================
    // Create a 128x64 empty sprite (no pixels — the smoke only verifies the
    // sprite went through CreateEmptySpriteObject + RenderSprite; visible
    // content is out of scope here).
    g_hudSprite = renderer->CreateEmptySpriteObject(128, 64,
                                                    TEXTURE_FORMAT_A8R8G8B8, 0);
    if (g_hudSprite) {
        std::fprintf(stderr, "[sprite] SpriteObject created (128x64 A8R8G8B8)\n");
    } else {
        std::fprintf(stderr, "[sprite] CreateEmptySpriteObject returned null\n");
    }

    // Create a default Arial 24pt font. The font init logs "[font] FontObject
    // ready (face=...)" internally (font_object.cpp:150) which the smoke
    // harness matches against.
    LOGFONT lf{};
    std::strncpy(lf.lfFaceName, "Arial", sizeof(lf.lfFaceName) - 1);
    lf.lfHeight = 24;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    g_hudFont = renderer->CreateFontObject(&lf, 0);
    if (!g_hudFont) {
        std::fprintf(stderr, "[font] CreateFontObject returned null (see renderer warn)\n");
    }
    // ==========================================================================

    // === Phase 7: Effect Shader Palette + Material Set ========================
    // Build a 2-entry effect palette (one WAVE + one SPHEREMAP). Both use empty
    // texture names so the palette builds without hitting the file system (the
    // demo's StubFileStorage returns nothing anyway). The renderer's
    // effect_shader.cpp:53 logs "[effect] palette built: %u entries"
    // automatically — that log line is the smoke marker.
    CUSTOM_EFFECT_DESC effectDescs[2]{};
    std::strncpy(effectDescs[0].szEffectShaderName, "demo_wave",
                 sizeof(effectDescs[0].szEffectShaderName) - 1);
    effectDescs[0].method         = TEXGEN_METHOD_WAVE;
    effectDescs[0].bDisableSrcTex = TRUE;
    std::strncpy(effectDescs[1].szEffectShaderName, "demo_sphere",
                 sizeof(effectDescs[1].szEffectShaderName) - 1);
    effectDescs[1].method         = TEXGEN_METHOD_REFLECT_SPHEREMAP;
    effectDescs[1].bDisableSrcTex = FALSE;
    BOOL effOk = renderer->CreateEffectShaderPalette(effectDescs, 2);
    if (!effOk) {
        std::fprintf(stderr, "[effect] CreateEffectShaderPalette returned FALSE\n");
    }

    // Build a 1-entry material set (no textures → no file IO). Demo logs the
    // handle itself since the renderer-side CreateMaterialSet is silent. The
    // return value is a 1-based handle (0 = invalid).
    MATERIAL demoMtl{};
    demoMtl.dwTextureNum    = 0;
    demoMtl.dwDiffuse       = 0xFF808080u;   // grey
    demoMtl.dwAmbient       = 0xFF404040u;
    demoMtl.dwSpecular      = 0xFFFFFFFFu;
    demoMtl.fTransparency   = 0.0f;
    demoMtl.fShine          = 32.0f;
    demoMtl.fShineStrength  = 1.0f;
    demoMtl.dwFlag          = 0x00000001u;
    MATERIAL_TABLE demoTable{};
    demoTable.pMtl       = &demoMtl;
    demoTable.dwMtlIndex = 1;
    std::uint32_t mtlHandle = renderer->CreateMaterialSet(&demoTable, 1);
    if (mtlHandle == 0) {
        std::fprintf(stderr, "[material] CreateMaterialSet returned 0 (invalid handle)\n");
    } else {
        std::fprintf(stderr, "[material] MaterialSet created (1 entries, handle=%u)\n", mtlHandle);
    }
    // ==========================================================================

    MSG msg{};
    auto startTick = GetTickCount();
    // Cap check runs BEFORE the render call so the loop can exit cleanly even
    // if InvalidateRect/UpdateWindow happens to take a long time (e.g. when
    // a window message sits in the queue and DispatchMessage runs user code
    // for a long time). The original ordering (cap check after UpdateWindow)
    // meant the cap never fired on hosts where the render call blocks.
    //
    // Note: on headless / Session 0 build hosts (no real display server)
    // UpdateWindow can still block indefinitely even with this reordering,
    // because the window message loop has no source of WM_PAINT events.
    // The smoke harness accounts for that with PASS-B (force-kill after
    // 10s, verify DX11 init markers) — see run_demo_smoke.py.
    while (g_running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) g_running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (GetTickCount() - startTick > 5000) {
            g_running = false;  // 5 s cap — checked BEFORE any render call
            break;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
        Sleep(16);
    }

    if (g_cube) { g_cube->Release(); g_cube = nullptr; }
    if (g_hudSprite) { g_hudSprite->Release(); g_hudSprite = nullptr; }
    if (g_hudFont)   { g_hudFont->Release();   g_hudFont   = nullptr; }
    if (mtlHandle) { renderer->DeleteMaterialSet(mtlHandle); }
    renderer->DeleteEffectShaderPalette();
    renderer->Release();
    storage->Release();
    std::printf("Demo ran for %lu ms, %u frames.\n", GetTickCount() - startTick, g_frames);
    return 0;
}