// tests/unit/ui/cimage_test.cpp
// Phase 6.4 unit tests for the modern mxh::ui::cImage widget.
#include <gtest/gtest.h>

#include <atomic>

#include "cImage.hpp"
#include "cObject.hpp"

using mxh::ui::cImage;
using mxh::ui::cObject;

namespace {

// A test-only "sprite" stand-in. We don't need a real DX11 ID3D11Texture2D
// to verify the cImage's behavior — the framework passes the pointer as
// void* through a function-pointer adapter. We just need a non-null
// unique address per instance.
struct FakeSprite {
    int magic = 0xC1A0C1A0;
};
FakeSprite g_spriteA{};
FakeSprite g_spriteB{};

// Adapter that records every draw call. Returns true if the sprite and
// adapter arguments are both non-null.
struct DrawCall {
    void* sprite = nullptr;
    float x = 0, y = 0, w = 0, h = 0;
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    std::uint32_t color = 0;
    int zOrder = 0;
    int callCount = 0;
};
std::vector<DrawCall>& g_draws() { static std::vector<DrawCall> v; return v; }

bool testAdapter(void* /*ctx*/, void* sprite, float x, float y, float w, float h,
                 float u0, float v0, float u1, float v1,
                 std::uint32_t color, int zOrder) {
    if (!sprite) return false;
    g_draws().push_back({sprite, x, y, w, h, u0, v0, u1, v1, color, zOrder, 0});
    return true;
}

} // namespace

TEST(CImage, DefaultIsNull) {
    cImage img;
    EXPECT_TRUE(img.IsNull());
    EXPECT_EQ(img.spriteObject(), nullptr);
    EXPECT_EQ(img.srcWidth(), 0);
    EXPECT_EQ(img.srcHeight(), 0);
    EXPECT_FALSE(img.render(0, 0, 10, 10));   // no sprite => false
}

TEST(CImage, SetSpriteObjectStoresPointer) {
    cImage img;
    img.SetSpriteObject(&g_spriteA);
    EXPECT_FALSE(img.IsNull());
    EXPECT_EQ(img.spriteObject(), &g_spriteA);
}

TEST(CImage, SetSourceRecordsRectAndSize) {
    cImage img;
    img.SetSource(10, 20, 110, 120, 256, 256);
    EXPECT_EQ(img.srcImageRect().left,   10);
    EXPECT_EQ(img.srcImageRect().top,    20);
    EXPECT_EQ(img.srcImageRect().right,  110);
    EXPECT_EQ(img.srcImageRect().bottom, 120);
    EXPECT_EQ(img.srcImageSize().width,  256);
    EXPECT_EQ(img.srcImageSize().height, 256);
    EXPECT_EQ(img.srcWidth(),  256);
    EXPECT_EQ(img.srcHeight(), 256);
}

TEST(CImage, EmptyRectMeansFullSprite) {
    cImage img;
    EXPECT_TRUE(img.srcImageRect().isEmpty());
}

TEST(CImage, RenderFullSpriteUsesDefaultUVs) {
    mxh::ui::bindRenderer(&testAdapter, nullptr);
    cImage img;
    img.SetSpriteObject(&g_spriteA);
    g_draws().clear();
    EXPECT_TRUE(img.render(0, 0, 100, 50));
    ASSERT_EQ(g_draws().size(), 1u);
    const auto& d = g_draws()[0];
    EXPECT_EQ(d.sprite, &g_spriteA);
    EXPECT_FLOAT_EQ(d.x, 0.f);
    EXPECT_FLOAT_EQ(d.y, 0.f);
    EXPECT_FLOAT_EQ(d.w, 100.f);
    EXPECT_FLOAT_EQ(d.h, 50.f);
    EXPECT_FLOAT_EQ(d.u0, 0.f);
    EXPECT_FLOAT_EQ(d.v0, 0.f);
    EXPECT_FLOAT_EQ(d.u1, 1.f);
    EXPECT_FLOAT_EQ(d.v1, 1.f);
}

TEST(CImage, RenderClipsToSourceRect) {
    mxh::ui::bindRenderer(&testAdapter, nullptr);
    cImage img;
    img.SetSpriteObject(&g_spriteA);
    img.SetSource(32, 48, 96, 80, 128, 256);
    g_draws().clear();
    EXPECT_TRUE(img.render(0, 0, 100, 50));
    ASSERT_EQ(g_draws().size(), 1u);
    const auto& d = g_draws()[0];
    EXPECT_FLOAT_EQ(d.u0, 32.f / 128.f);
    EXPECT_FLOAT_EQ(d.v0, 48.f / 256.f);
    EXPECT_FLOAT_EQ(d.u1, 96.f / 128.f);
    EXPECT_FLOAT_EQ(d.v1, 80.f / 256.f);
}

TEST(CImage, RenderClampsOutOfRangeUVs) {
    mxh::ui::bindRenderer(&testAdapter, nullptr);
    cImage img;
    img.SetSpriteObject(&g_spriteA);
    img.SetSource(-10, 0, 200, 256, 100, 256);   // right=200 > srcW=100
    g_draws().clear();
    EXPECT_TRUE(img.render(0, 0, 10, 10));
    ASSERT_EQ(g_draws().size(), 1u);
    const auto& d = g_draws()[0];
    EXPECT_FLOAT_EQ(d.u0, 0.f);  // clamped
    EXPECT_FLOAT_EQ(d.v0, 0.f);
    EXPECT_FLOAT_EQ(d.u1, 1.f);  // clamped
    EXPECT_FLOAT_EQ(d.v1, 1.f);
}

TEST(CImage, RenderWithoutAdapterIsNoOp) {
    mxh::ui::bindRenderer(nullptr, nullptr);
    cImage img;
    img.SetSpriteObject(&g_spriteA);
    g_draws().clear();
    EXPECT_FALSE(img.render(0, 0, 10, 10));
    EXPECT_EQ(g_draws().size(), 0u);
}

TEST(CImage, RenderWithoutSpriteIsNoOp) {
    mxh::ui::bindRenderer(&testAdapter, nullptr);
    cImage img;
    EXPECT_FALSE(img.render(0, 0, 10, 10));
}

TEST(CImage, ColorAndZOrderForwarded) {
    mxh::ui::bindRenderer(&testAdapter, nullptr);
    cImage img;
    img.SetSpriteObject(&g_spriteA);
    g_draws().clear();
    EXPECT_TRUE(img.render(50, 60, 200, 100, 0x80FF0000u, 5));
    ASSERT_EQ(g_draws().size(), 1u);
    EXPECT_EQ(g_draws()[0].color, 0x80FF0000u);
    EXPECT_EQ(g_draws()[0].zOrder, 5);
    EXPECT_FLOAT_EQ(g_draws()[0].x, 50.f);
    EXPECT_FLOAT_EQ(g_draws()[0].y, 60.f);
}

TEST(CImage, CachedSpriteSizeFallback) {
    // If srcImageSize is not configured (0,0) but the host has cached the
    // sprite's native dimensions, srcWidth/Height should return those.
    cImage img;
    img.SetSpriteObject(&g_spriteA);
    img.setCachedSpriteSizeForTest(64, 32);
    EXPECT_EQ(img.srcWidth(),  64);
    EXPECT_EQ(img.srcHeight(), 32);
    // srcImageSize (0,0) takes precedence over a stale cache; here both
    // are 0 so we hit the cache.
}

TEST(CImage, CopyAndMoveSemantics) {
    cImage a;
    a.SetSpriteObject(&g_spriteA);
    a.SetSource(1, 2, 3, 4, 5, 6);
    a.setId(99);
    // Copy
    cImage b = a;
    EXPECT_EQ(b.spriteObject(), &g_spriteA);
    EXPECT_EQ(b.srcImageRect().left, 1);
    EXPECT_EQ(b.id(), 99);
    // Move
    cImage c = std::move(a);
    EXPECT_EQ(c.spriteObject(), &g_spriteA);
    EXPECT_EQ(c.id(), 99);
}

TEST(CImage, IsInheritsObject) {
    cImage img(7);
    EXPECT_EQ(img.id(), 7);
}
