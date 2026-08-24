// mxh/ui/cImage.hpp
// Phase 6.4 — modern C++ cImage: bridges cWindow (6.0) to mxh_render's
// SpriteObject. This is the Phase 5 -> Phase 6 GPU seam: every UI widget
// that wants a real image on screen now points its basicImage / overImage
// / pressImage at a cImage instead of an opaque void*.
//
// Design intent: a cImage is a value-semantic handle (matches the legacy
// cImage pattern) with shared ownership of the underlying SpriteObject
// via the IDISpriteObject IUnknown refcount. Default copy / move are
// enabled; the cImage just holds a pointer + image source rect.
//
// Source rect (m_srcImageRect): the legacy engine used this to draw
// sub-regions of a sprite atlas. We keep the same contract: an empty
// rect (right == left) means "draw the whole sprite", otherwise clip to
// the rect's UVs.
//
// Render path: cImage::render(x, y, w, h, color) computes the UVs from
// the source rect (or full sprite) and forwards to mxh_render's
// drawTexturedQuad via the global renderer singleton. The 2D HUD pass
// the renderer already sets up in BeginRender() takes care of the blend
// state and the projection; this is the same path MoxianRenderDemo uses
// for its sprite.
#pragma once

#include <cstdint>
#include <string>

#include "cObject.hpp"

namespace mxh::ui {

// Source image sub-rect. (0,0,0,0) means "use the full sprite"; any
// non-empty rect clips to the UV range. The legacy cImage used
// cImageRect with int fields; we keep the same shape.
struct ImageRect {
    std::int32_t left   = 0;
    std::int32_t top    = 0;
    std::int32_t right  = 0;
    std::int32_t bottom = 0;
    bool isEmpty() const noexcept { return right == 0 && bottom == 0; }
};

// Source image size. (0,0) means "unspecified — use the sprite's own
// width/height at draw time". The legacy cImage uses this together with
// ImageRect to scale the sprite into the target rect.
struct ImageSize {
    std::int32_t width  = 0;
    std::int32_t height = 0;
};

class cImage : public cObject {
public:
    cImage() = default;
    explicit cImage(std::int32_t id) : cObject(id) {}
    ~cImage() override = default;

    // Default copy / move: the underlying SpriteObject is refcounted via
    // IUnknown so multiple cImage instances can share the same GPU
    // resource safely. cObject's copy/move are deleted; we re-enable here
    // because cImage only stores POD + a borrowed pointer — no owned
    // resources.
    cImage(const cImage&)                = default;
    cImage& operator=(const cImage&)     = default;
    cImage(cImage&&) noexcept            = default;
    cImage& operator=(cImage&&) noexcept = default;

    // -------------------------------------------------------------------------
    // Sprite binding. cImage stores a borrowed pointer to a SpriteObject
    // (IDISpriteObject*); the framework does not own it. The caller is
    // responsible for keeping the sprite alive as long as this cImage
    // references it (in practice: the legacy engine's cResourceManager
    // owns the sprite cache).
    // -------------------------------------------------------------------------
    void SetSpriteObject(void* sprite) noexcept;
    void* spriteObject() const noexcept { return m_pSurface; }
    bool IsNull() const noexcept         { return m_pSurface == nullptr; }

    // -------------------------------------------------------------------------
    // Source-rect + size. Used by render() to compute the UVs.
    // -------------------------------------------------------------------------
    void SetImageSrcSize(const ImageSize& sz) noexcept  { m_srcImageSize = sz; }
    void SetImageSrcRect(const ImageRect& rt) noexcept  { m_srcImageRect = rt; }
    const ImageSize& srcImageSize() const noexcept      { return m_srcImageSize; }
    const ImageRect& srcImageRect() const noexcept      { return m_srcImageRect; }

    // Convenience: configure the source rect from (left, top, right, bottom)
    // and size in one call. This matches the legacy cImage::SetImageSrcRect
    // / SetImageSrcSize pair.
    void SetSource(std::int32_t left, std::int32_t top, std::int32_t right,
                   std::int32_t bottom, std::int32_t srcW, std::int32_t srcH) noexcept;

    // -------------------------------------------------------------------------
    // Width / height. If m_srcImageSize is set we use that; otherwise we
    // fall back to the sprite's own dimensions (queried via the cached
    // m_cachedSpriteW / m_cachedSpriteH, set by SetSpriteObject).
    // -------------------------------------------------------------------------
    std::int32_t srcWidth()  const noexcept;
    std::int32_t srcHeight() const noexcept;

    // -------------------------------------------------------------------------
    // Render at screen position (x, y) with target size (w, h) and tint
    // color (0xAARRGGBB; alpha < 0xFF triggers alpha blend). zOrder is
    // forwarded to the renderer's HUD pass. Returns true if the image
    // was actually drawn (sprite present + renderer hooked up).
    //
    // Phase 6.4 wiring: this delegates to mxh_render's PrimitiveDrawer
    // via a small adapter (mxh::ui::bindRenderer) installed at app
    // startup. If no adapter is bound, render() is a safe no-op.
    // -------------------------------------------------------------------------
    bool render(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h,
                std::uint32_t dwColor = 0xFFFFFFFFu, int zOrder = 0) const;

    // -------------------------------------------------------------------------
    // Test accessors.
    // -------------------------------------------------------------------------
    void setCachedSpriteSizeForTest(std::int32_t w, std::int32_t h) noexcept {
        m_cachedSpriteW = w;
        m_cachedSpriteH = h;
    }

private:
    void*     m_pSurface      = nullptr;       // IDISpriteObject*, borrowed
    ImageRect m_srcImageRect{};                 // empty = use whole sprite
    ImageSize m_srcImageSize{};                 // (0,0) = unknown, query later
    std::int32_t m_cachedSpriteW = 0;
    std::int32_t m_cachedSpriteH = 0;
};

// The host (MoxianRenderDemo / MoxianClient) calls this at startup. The
// adapter receives a borrowed sprite pointer (typed as void* to keep
// cImage free of mxh_render's #include) and the UVs derived from the
// image's source rect / size. Returns true on success.
using RenderAdapterFn = bool (*)(void* ctx, void* sprite,
                                  float x, float y, float w, float h,
                                  float u0, float v0, float u1, float v1,
                                  std::uint32_t color, int zOrder);
void bindRenderer(RenderAdapterFn drawFn, void* ctx);

} // namespace mxh::ui
