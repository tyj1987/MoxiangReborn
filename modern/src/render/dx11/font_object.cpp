// mxh/render/dx11/font_object.cpp
// IDIFontObject DX11 implementation. See font_object.hpp for design notes.
#include "font_object.hpp"

#include "device.hpp"
#include "primitives.hpp"

#include "mxh/log/mlog.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>   // After <d3d11.h>: GDI types, GetGlyphOutlineA, LOGFONTA

namespace mxh::gx::dx11 {

namespace {

constexpr std::uint32_t kAtlasWidth  = 512;
constexpr std::uint32_t kAtlasHeight = 512;

// Default LOGFONT when caller passes nullptr. 12 px Arial is present on
// every Windows install and gives clean Latin glyph metrics; the legacy
// engine used Dotum/Gulim for Korean via the .TTB pre-baked atlas.
LOGFONT makeDefaultLogFont() {
    LOGFONT lf{};
    lf.lfHeight         = -12;                          // negative = char height in pixels
    lf.lfWidth          = 0;
    lf.lfEscapement     = 0;
    lf.lfOrientation    = 0;
    lf.lfWeight         = FW_NORMAL;
    lf.lfItalic         = FALSE;
    lf.lfUnderline      = FALSE;
    lf.lfStrikeOut      = FALSE;
    lf.lfCharSet        = DEFAULT_CHARSET;
    lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    lf.lfQuality        = ANTIALIASED_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    std::strncpy(lf.lfFaceName, "Arial", LF_FACESIZE - 1);
    lf.lfFaceName[LF_FACESIZE - 1] = '\0';
    return lf;
}

} // namespace

FontObject::~FontObject() {
    releaseAtlas();
    releaseFont();
}

STDMETHODIMP FontObject::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown) {
        *ppv = static_cast<IDIFontObject*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) FontObject::AddRef() {
    return ++m_refCount;
}

STDMETHODIMP_(ULONG) FontObject::Release() {
    ULONG r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

bool FontObject::initialize(Device* dev, const LOGFONT* lf, std::uint32_t /*dwFlag*/) {
    m_dev = dev;

    LOGFONT effective = lf ? *lf : makeDefaultLogFont();
    m_hFont = CreateFontIndirect(&effective);
    if (!m_hFont) {
        MLOG_ERROR("[font] CreateFontIndirect failed");
        return false;
    }

    D3D11_TEXTURE2D_DESC td{};
    td.Width              = kAtlasWidth;
    td.Height             = kAtlasHeight;
    td.MipLevels          = 1;
    td.ArraySize          = 1;
    td.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count   = 1;
    td.Usage              = D3D11_USAGE_DEFAULT;  // UpdateSubresource path
    td.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags     = 0;

    if (!m_dev || FAILED(m_dev->rawDevice()->CreateTexture2D(&td, nullptr, &m_atlasTex))) {
        MLOG_ERROR("[font] CreateTexture2D atlas failed");
        releaseFont();
        return false;
    }
    m_atlasWidth  = kAtlasWidth;
    m_atlasHeight = kAtlasHeight;

    if (auto* ctx = m_dev->rawContext()) {
        std::vector<std::uint32_t> clear(m_atlasWidth * m_atlasHeight, 0x00000000u);
        D3D11_BOX box{};
        box.left = box.top = box.front = 0;
        box.right  = m_atlasWidth;
        box.bottom = m_atlasHeight;
        box.back   = 1;
        ctx->UpdateSubresource(m_atlasTex.Get(), 0, &box, clear.data(),
                               m_atlasWidth * 4, 0);
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
    srvd.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    if (FAILED(m_dev->rawDevice()->CreateShaderResourceView(m_atlasTex.Get(), &srvd, &m_atlasSRV))) {
        MLOG_ERROR("[font] CreateShaderResourceView atlas failed");
        releaseAtlas();
        releaseFont();
        return false;
    }

    HDC screen = GetDC(nullptr);
    if (!screen) {
        MLOG_ERROR("[font] GetDC(nullptr) failed");
        releaseAtlas();
        releaseFont();
        return false;
    }
    m_memDC = CreateCompatibleDC(screen);
    ReleaseDC(nullptr, screen);
    if (!m_memDC) {
        MLOG_ERROR("[font] CreateCompatibleDC failed");
        releaseAtlas();
        releaseFont();
        return false;
    }
    SelectObject(m_memDC, m_hFont);

    m_packCursorX   = 0;
    m_packCursorY   = 0;
    m_packRowHeight = 0;
    m_glyphs.clear();

    MLOG_INFO("[font] FontObject ready (face='%s' size=%d)",
              effective.lfFaceName, effective.lfHeight);
    return true;
}

void FontObject::releaseAtlas() {
    m_atlasSRV.Reset();
    m_atlasTex.Reset();
    m_atlasWidth = m_atlasHeight = 0;
    m_glyphs.clear();
    m_packCursorX = m_packCursorY = m_packRowHeight = 0;
}

void FontObject::releaseFont() {
    if (m_memDC) {
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }
    if (m_hFont) {
        DeleteObject(m_hFont);
        m_hFont = nullptr;
    }
}

FontObject* FontObject::create(Device* dev, const LOGFONT* lf, std::uint32_t dwFlag) {
    auto* f = new FontObject();
    if (!f->initialize(dev, lf, dwFlag)) {
        f->Release();
        return nullptr;
    }
    return f;
}

const GlyphEntry* FontObject::findGlyph(std::uint8_t cp) const {
    auto it = m_glyphs.find(cp);
    return it != m_glyphs.end() ? &it->second : nullptr;
}

std::uint32_t FontObject::packGlyph(std::uint16_t w, std::uint16_t h,
                                    GlyphEntry* outEntry) {
    if (w == 0 || h == 0) return 0;
    if (w > m_atlasWidth || h > m_atlasHeight) return 0;  // glyph too large

    if (m_packCursorX + w > m_atlasWidth) {
        m_packCursorY += m_packRowHeight + 1;       // 1-px gap to avoid bleed
        m_packCursorX   = 0;
        m_packRowHeight = 0;
    }
    if (m_packCursorY + h > m_atlasHeight) {
        m_glyphs.clear();
        m_packCursorX   = 0;
        m_packCursorY   = 0;
        m_packRowHeight = 0;
    }

    outEntry->atlas_x = static_cast<std::uint16_t>(m_packCursorX);
    outEntry->atlas_y = static_cast<std::uint16_t>(m_packCursorY);
    outEntry->width   = w;
    outEntry->height  = h;

    m_packCursorX += w + 1;
    if (h > m_packRowHeight) m_packRowHeight = h;
    return 1;
}

const GlyphEntry* FontObject::cacheGlyph(std::uint8_t cp) {
    if (!m_memDC || !m_hFont) return nullptr;
    auto it = m_glyphs.find(cp);
    if (it != m_glyphs.end()) return &it->second;

    GLYPHMETRICS gm{};
    MAT2 identity = { {0,1}, {0,0}, {0,0}, {0,1} };
    DWORD needed = GetGlyphOutlineA(m_memDC, static_cast<UINT>(cp),
                                    GGO_GRAY8_BITMAP, &gm, 0, nullptr, &identity);
    if (needed == GDI_ERROR || needed == 0) {
        // Empty glyph (space, etc.) — still cache an entry so we don't re-query.
        GlyphEntry e{};
        e.advance = gm.gmCellIncX;
        if (e.advance <= 0) e.advance = (gm.gmBlackBoxX > 0) ? gm.gmBlackBoxX : 4;
        auto [insIt, _] = m_glyphs.emplace(cp, e);
        return &insIt->second;
    }

    std::vector<std::uint8_t> gray(needed);
    if (GetGlyphOutlineA(m_memDC, static_cast<UINT>(cp), GGO_GRAY8_BITMAP, &gm,
                         needed, gray.data(), &identity) == GDI_ERROR) {
        return nullptr;
    }

    GlyphEntry e{};
    e.width    = static_cast<std::uint16_t>(gm.gmBlackBoxX);
    e.height   = static_cast<std::uint16_t>(gm.gmBlackBoxY);
    e.bearing_x = gm.gmptGlyphOrigin.x;
    e.bearing_y = gm.gmptGlyphOrigin.y;
    e.advance   = gm.gmCellIncX;

    if (e.width > 0 && e.height > 0) {
        if (packGlyph(e.width, e.height, &e) == 0) {
            return nullptr;
        }
    }

    auto [insIt, _2] = m_glyphs.emplace(cp, e);

    if (m_dev && m_atlasTex && e.width > 0 && e.height > 0) {
        uploadGlyph(e, gray.data(),
                    static_cast<std::int32_t>((e.width + 3) & ~3));
    }
    return &insIt->second;
}

bool FontObject::uploadGlyph(const GlyphEntry& e, const std::uint8_t* gray,
                             std::int32_t pitch) {
    if (!m_dev || !m_atlasTex || !gray) return false;
    auto* ctx = m_dev->rawContext();
    if (!ctx) return false;

    // Pack 8-bit coverage into BGRA so the lit-textured PS shader does
    // sampled.a * color — white channel + coverage alpha.
    // DXGI_FORMAT_B8G8R8A8_UNORM byte layout: B, G, R, A.
    std::vector<std::uint32_t> bgra(e.width * e.height);
    for (std::uint32_t y = 0; y < e.height; ++y) {
        const std::uint8_t* srcRow = gray + y * pitch;
        std::uint32_t*       dstRow = bgra.data() + y * e.width;
        for (std::uint32_t x = 0; x < e.width; ++x) {
            // GGO_GRAY8_BITMAP coverage is 0..64 (4-bit AA), not 0..255.
            const std::uint8_t cov = srcRow[x];
            const std::uint8_t alpha = cov >= 64
                ? 255u : static_cast<std::uint8_t>(cov * 4u);
            dstRow[x] = static_cast<std::uint32_t>(alpha) << 24
                        | 0x00FFFFFFu;
        }
    }

    D3D11_BOX box{};
    box.left   = e.atlas_x;
    box.top    = e.atlas_y;
    box.front  = 0;
    box.right  = e.atlas_x + e.width;
    box.bottom = e.atlas_y + e.height;
    box.back   = 1;
    ctx->UpdateSubresource(m_atlasTex.Get(), 0, &box, bgra.data(),
                           e.width * 4, 0);
    return true;
}

void __stdcall FontObject::BeginRender() {
    m_inRender = true;
}

void __stdcall FontObject::EndRender() {
    m_inRender = false;
}

BOOL __stdcall FontObject::DrawText(TCHAR* str, std::uint32_t dwLen, RECT* pRect,
                                    std::uint32_t dwColor, CHAR_CODE_TYPE /*type*/,
                                    std::uint32_t /*dwFlag*/) {
    if (!m_dev || !m_atlasSRV || !str || dwLen == 0 || !pRect) return FALSE;

    PrimitiveDrawer drawer;
    drawer.initialize(m_dev);
    drawer.setViewProj(m_dev->viewProjMatrix());

    int penX = pRect->left;
    int penY = pRect->top;

    for (std::uint32_t i = 0; i < dwLen; ++i) {
        std::uint8_t cp = static_cast<std::uint8_t>(str[i]);
        const GlyphEntry* g = cacheGlyph(cp);
        if (!g) continue;

        if (g->width > 0 && g->height > 0) {
            float dx = static_cast<float>(penX + g->bearing_x);
            float dy = static_cast<float>(penY - g->bearing_y);
            float dw = static_cast<float>(g->width);
            float dh = static_cast<float>(g->height);

            float u0 = static_cast<float>(g->atlas_x)             / static_cast<float>(m_atlasWidth);
            float v0 = static_cast<float>(g->atlas_y)             / static_cast<float>(m_atlasHeight);
            float u1 = static_cast<float>(g->atlas_x + g->width)  / static_cast<float>(m_atlasWidth);
            float v1 = static_cast<float>(g->atlas_y + g->height) / static_cast<float>(m_atlasHeight);

            drawer.drawTexturedQuad(m_atlasSRV.Get(), dx, dy, dw, dh,
                                    u0, v0, u1, v1, dwColor, 0.0f, 0);
        }
        penX += g->advance;
    }
    return TRUE;
}

} // namespace mxh::gx::dx11
