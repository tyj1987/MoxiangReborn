// mxh/ui/cImage.cpp
// Phase 6.4 — implementation of the modern cImage widget. Bridges to
// mxh_render's SpriteObject + drawTexturedQuad via a small adapter
// the host app installs at startup (see bindRenderer below).
#include "cImage.hpp"

#include <cstdint>

namespace mxh::ui {

namespace {

// Adapter signature: the host app installs a function that knows how to
// draw a SpriteObject at the given position with the given UVs. We use
// a function pointer + void* ctx so the adapter doesn't have to know
// about mxh_render's full Device/PrimitiveDrawer surface.
struct RenderAdapter {
    bool (*draw)(void* ctx, void* sprite, float x, float y, float w, float h,
                 float u0, float v0, float u1, float v1,
                 std::uint32_t color, int zOrder) = nullptr;
    void* ctx = nullptr;
};
RenderAdapter& adapter() {
    static RenderAdapter a;
    return a;
}

} // anonymous namespace

// The host (MoxianRenderDemo / MoxianClient) calls this at startup. The
// adapter receives a borrowed sprite pointer (typed as void* to keep
// cImage free of mxh_render's #include) and the UVs derived from
// m_srcImageRect / m_srcImageSize. Returns true on success.
void bindRenderer(bool (*drawFn)(void*, void*, float, float, float, float,
                                 float, float, float, float, std::uint32_t, int),
                  void* ctx) {
    adapter().draw = drawFn;
    adapter().ctx  = ctx;
}

void cImage::SetSpriteObject(void* sprite) noexcept {
    m_pSurface = sprite;
    // Reset cached size; if the host's adapter is able to query the
    // sprite it will refresh this via setCachedSpriteSizeForTest.
    m_cachedSpriteW = 0;
    m_cachedSpriteH = 0;
}

void cImage::SetSource(std::int32_t left, std::int32_t top, std::int32_t right,
                       std::int32_t bottom, std::int32_t srcW,
                       std::int32_t srcH) noexcept {
    m_srcImageRect = {left, top, right, bottom};
    m_srcImageSize = {srcW, srcH};
}

std::int32_t cImage::srcWidth() const noexcept {
    if (m_srcImageSize.width > 0)  return m_srcImageSize.width;
    if (m_cachedSpriteW > 0)       return m_cachedSpriteW;
    return 0;
}

std::int32_t cImage::srcHeight() const noexcept {
    if (m_srcImageSize.height > 0) return m_srcImageSize.height;
    if (m_cachedSpriteH > 0)       return m_cachedSpriteH;
    return 0;
}

bool cImage::render(std::int32_t x, std::int32_t y, std::int32_t w,
                    std::int32_t h, std::uint32_t dwColor, int zOrder) const {
    if (m_pSurface == nullptr) return false;
    if (adapter().draw == nullptr) return false;

    // UV computation. m_srcImageRect empty => full sprite UVs [0,1].
    float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 1.f;
    if (!m_srcImageRect.isEmpty()) {
        const std::int32_t sw = (m_srcImageSize.width  > 0)
                                ? m_srcImageSize.width  : m_cachedSpriteW;
        const std::int32_t sh = (m_srcImageSize.height > 0)
                                ? m_srcImageSize.height : m_cachedSpriteH;
        if (sw > 0 && sh > 0) {
            u0 = static_cast<float>(m_srcImageRect.left)   / static_cast<float>(sw);
            v0 = static_cast<float>(m_srcImageRect.top)    / static_cast<float>(sh);
            u1 = static_cast<float>(m_srcImageRect.right)  / static_cast<float>(sw);
            v1 = static_cast<float>(m_srcImageRect.bottom) / static_cast<float>(sh);
            // Clamp into [0,1] to defend against misconfigured atlas rects.
            // Manual clamp (we avoid <algorithm> here because pulling it in
            // has triggered an MSVC 14.44 <xutility> ICE in the past).
            if (u0 < 0.f) u0 = 0.f; else if (u0 > 1.f) u0 = 1.f;
            if (v0 < 0.f) v0 = 0.f; else if (v0 > 1.f) v0 = 1.f;
            if (u1 < 0.f) u1 = 0.f; else if (u1 > 1.f) u1 = 1.f;
            if (v1 < 0.f) v1 = 0.f; else if (v1 > 1.f) v1 = 1.f;
        }
    }
    return adapter().draw(adapter().ctx, m_pSurface,
                          static_cast<float>(x), static_cast<float>(y),
                          static_cast<float>(w), static_cast<float>(h),
                          u0, v0, u1, v1, dwColor, zOrder);
}

} // namespace mxh::ui
