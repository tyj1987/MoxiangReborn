// mxh/render/dx11/font_object.hpp
// IDIFontObject DX11 implementation backed by a GDI-rasterized glyph atlas.
//
// Project charset note: the Moxian source tree is built with MultiByte
// (not Unicode). TCHAR == char and LOGFONT == LOGFONTA in this build,
// matching the original 4Dyuchi engine's MBCS / Korean-EUC-KR configuration.
// We accept the engine's TCHAR* strings directly and index the glyph
// cache by single-byte code (0..255). Multi-byte CJK strings will be
// cached byte-by-byte — sufficient for Latin + Hangul Jamo; full CJK
// cluster rendering is out of scope for Phase 5 (the legacy engine
// pre-baked Hangul glyphs into .TTB atlases for that path).
//
// Strategy:
//   - CreateFontIndirect (A) from the LOGFONT (or a default).
//   - Maintain a CPU-side glyph cache keyed by 8-bit code point. Each
//     entry holds (atlas_x, atlas_y, w, h, advance_x, bearing_y).
//   - Glyphs are rasterized on demand via GetGlyphOutlineA (GGO_GRAY8_BITMAP)
//     and uploaded into a BGRA atlas texture via UpdateSubresource.
//   - drawText iterates the input string, resolves each glyph, and emits
//     a textured quad via PrimitiveDrawer::drawTexturedQuad (same path
//     SpriteObject uses) — the existing lit-textured shader is reused,
//     no new shader.
#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <unordered_map>

// Pull in Windows GDI types (LOGFONTA, MAT2, GLYPHMETRICS, etc.) BEFORE
// IRenderer.hpp so LOGFONT is consistently LOGFONTA in this MultiByte build.
#include <windows.h>

#include "mxh/render/IRenderer.hpp"
#include "mxh/render/render_typedef.hpp"

namespace mxh::gx::dx11 {

class Device;

struct GlyphEntry {
    std::uint16_t atlas_x   = 0;     // pixel offset into the atlas texture
    std::uint16_t atlas_y   = 0;
    std::uint16_t width     = 0;     // rasterized glyph width in pixels
    std::uint16_t height    = 0;     // rasterized glyph height in pixels
    int          bearing_x  = 0;     // offset from pen to glyph's left edge
    int          bearing_y  = 0;     // offset from baseline to glyph's top
    int          advance    = 0;     // pen advance in pixels (per glyph)
};

class FontObject : public IDIFontObject {
public:
    static FontObject* create(Device* dev, const LOGFONT* lf, std::uint32_t dwFlag);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void**) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IDIFontObject
    void __stdcall BeginRender() override;
    void __stdcall EndRender() override;
    BOOL __stdcall DrawText(TCHAR* str, std::uint32_t dwLen, RECT* pRect,
                            std::uint32_t dwColor, CHAR_CODE_TYPE type,
                            std::uint32_t dwFlag) override;

    // Accessors for tests / introspection.
    std::uint32_t                atlasWidth()  const { return m_atlasWidth; }
    std::uint32_t                atlasHeight() const { return m_atlasHeight; }
    std::uint32_t                glyphCount()  const { return static_cast<std::uint32_t>(m_glyphs.size()); }
    const GlyphEntry*             findGlyph(std::uint8_t cp) const;

    // Pure CPU helpers exposed for unit tests (no D3D11 device required).
    // Returns nullptr if the glyph can't fit (atlas full + can't reset).
    const GlyphEntry* cacheGlyph(std::uint8_t cp);
    std::uint32_t    packGlyph(std::uint16_t w, std::uint16_t h,
                                GlyphEntry* outEntry);

    // The atlas is laid out as a simple row-packer: rows of glyph_height px,
    // each row advances horizontally; when a row is full, wrap to the next.
    // Tests can drive this independently of any actual rasterization.

private:
    FontObject() = default;
    ~FontObject();

    bool initialize(Device* dev, const LOGFONT* lf, std::uint32_t dwFlag);
    void releaseAtlas();
    void releaseFont();
    bool uploadGlyph(const GlyphEntry& e, const std::uint8_t* grayBitmap,
                     std::int32_t pitch);

    Device*                                     m_dev = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>      m_atlasTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_atlasSRV;
    std::uint32_t                                m_atlasWidth  = 0;
    std::uint32_t                                m_atlasHeight = 0;

    HFONT                                        m_hFont   = nullptr;
    HDC                                          m_memDC   = nullptr;

    std::unordered_map<std::uint8_t, GlyphEntry>  m_glyphs;

    // Row-packing state.
    std::uint32_t                                m_packCursorX = 0;
    std::uint32_t                                m_packCursorY = 0;
    std::uint32_t                                m_packRowHeight = 0;

    std::uint32_t                                m_refCount = 1;
    bool                                         m_inRender  = false;
};

} // namespace mxh::gx::dx11