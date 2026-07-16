// cguagen_test.cpp — Phase 12.x 1:1 port tests for cGuagen.
//
// 1:1 port of legacy `cGuagen` from
//   `墨香【源码】\[Client]MH\interface\cGuagen.h`.
//
// Verifies:
//   1. Default construction zeroes all geometry and 0% value
//   2. SetValue clamps to [0, 1] (legacy semantics: only upper bound)
//   3. SetValue preserves in-range values
//   4. SetPieceImage stores the cImage
//   5. SetGuageWidth / SetGuagePieceWidth / SetGuagePieceHeightScale
//      store their respective float values
//   6. SetGuageImagePos overloads (int32 + float) both populate m_imgRelPos
//   7. Render is a no-op (does not crash, returns)
//   8. cGuagen inherits from cWindow

#include "cGuagen.hpp"
#include "cWindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CGuagen, DefaultConstructionZeroesAll) {
    cGuagen g;
    EXPECT_EQ(g.GetValue(), 0.0f);
    EXPECT_EQ(g.GetGuageWidth(), 0.0f);
    EXPECT_EQ(g.GetGuagePieceWidth(), 0.0f);
    EXPECT_EQ(g.GetGuagePieceHeightScale(), 1.0f);
    EXPECT_EQ(g.GetImageRelX(), 0.0f);
    EXPECT_EQ(g.GetImageRelY(), 0.0f);
}

TEST(CGuagen, InheritsCWindow) {
    cGuagen g;
    cWindow* base = &g;
    EXPECT_NE(base, nullptr);
    // cWindow::Render is virtual; cGuagen overrides it.
    base->Render();  // should not crash
}

// ===========================================================================
// SetValue (clamps to [0, 1])
// ===========================================================================

TEST(CGuagen, SetValueStoresInRange) {
    cGuagen g;
    g.SetValue(0.5f);
    EXPECT_FLOAT_EQ(g.GetValue(), 0.5f);
}

TEST(CGuagen, SetValueClampsAboveOne) {
    cGuagen g;
    g.SetValue(2.5f);
    EXPECT_FLOAT_EQ(g.GetValue(), 1.0f);
    EXPECT_LE(g.GetValue(), 1.0f);
}

TEST(CGuagen, SetValuePreservesZero) {
    cGuagen g;
    g.SetValue(0.0f);
    EXPECT_FLOAT_EQ(g.GetValue(), 0.0f);
}

TEST(CGuagen, SetValuePreservesOne) {
    cGuagen g;
    g.SetValue(1.0f);
    EXPECT_FLOAT_EQ(g.GetValue(), 1.0f);
}

TEST(CGuagen, SetValueIsIdempotent) {
    cGuagen g;
    g.SetValue(0.7f);
    g.SetValue(0.7f);
    EXPECT_FLOAT_EQ(g.GetValue(), 0.7f);
}

TEST(CGuagen, SetValueClampFromNegativeNotClampedInLegacy) {
    // Legacy only clamps `1.f < m_fPercentRate`. Negative values pass
    // through. Modern port mirrors this — the caller is responsible
    // for non-negative inputs. Document the behavior here.
    cGuagen g;
    g.SetValue(-0.5f);
    EXPECT_FLOAT_EQ(g.GetValue(), -0.5f);
}

// ===========================================================================
// SetGuageImagePos (int32 + float overloads)
// ===========================================================================

TEST(CGuagen, SetGuageImagePosInt32) {
    cGuagen g;
    g.SetGuageImagePos(10, 20);
    EXPECT_FLOAT_EQ(g.GetImageRelX(), 10.0f);
    EXPECT_FLOAT_EQ(g.GetImageRelY(), 20.0f);
}

TEST(CGuagen, SetGuageImagePosFloat) {
    cGuagen g;
    g.SetGuageImagePos(1.5f, 2.5f);
    EXPECT_FLOAT_EQ(g.GetImageRelX(), 1.5f);
    EXPECT_FLOAT_EQ(g.GetImageRelY(), 2.5f);
}

TEST(CGuagen, SetGuageImagePosNegative) {
    cGuagen g;
    g.SetGuageImagePos(-100, -200);
    EXPECT_FLOAT_EQ(g.GetImageRelX(), -100.0f);
    EXPECT_FLOAT_EQ(g.GetImageRelY(), -200.0f);
}

// ===========================================================================
// SetGuageWidth / SetGuagePieceWidth / SetGuagePieceHeightScale
// ===========================================================================

TEST(CGuagen, SetGuageWidth) {
    cGuagen g;
    g.SetGuageWidth(123.5f);
    EXPECT_FLOAT_EQ(g.GetGuageWidth(), 123.5f);
}

TEST(CGuagen, SetGuagePieceWidth) {
    cGuagen g;
    g.SetGuagePieceWidth(7.0f);
    EXPECT_FLOAT_EQ(g.GetGuagePieceWidth(), 7.0f);
}

TEST(CGuagen, SetGuagePieceHeightScale) {
    cGuagen g;
    g.SetGuagePieceHeightScale(0.5f);
    EXPECT_FLOAT_EQ(g.GetGuagePieceHeightScale(), 0.5f);
}

TEST(CGuagen, PieceWidthAndBarWidthAreIndependent) {
    cGuagen g;
    g.SetGuageWidth(100.0f);
    g.SetGuagePieceWidth(5.0f);
    EXPECT_FLOAT_EQ(g.GetGuageWidth(), 100.0f);
    EXPECT_FLOAT_EQ(g.GetGuagePieceWidth(), 5.0f);
}

// ===========================================================================
// SetPieceImage (cImage — value semantics)
// ===========================================================================

TEST(CGuagen, SetPieceImageDefaultIsNullSprite) {
    cGuagen g;
    // Default cImage has no sprite bound; get returns a reference to a
    // null cImage. Just verify it doesn't crash and IsNull() returns true.
    const cImage& before = g.GetPieceImage();
    EXPECT_TRUE(before.IsNull());
}

TEST(CGuagen, SetPieceImageStoresCopy) {
    cGuagen g;
    cImage img;
    img.SetSpriteObject(reinterpret_cast<void*>(0xDEADBEEF));
    g.SetPieceImage(img);
    const cImage& stored = g.GetPieceImage();
    EXPECT_FALSE(stored.IsNull());
    EXPECT_EQ(stored.spriteObject(), reinterpret_cast<void*>(0xDEADBEEF));
}

// ===========================================================================
// Render is a no-op (smoke test)
// ===========================================================================

TEST(CGuagen, RenderIsNoop) {
    cGuagen g;
    g.SetValue(0.42f);
    g.SetGuageWidth(100.0f);
    g.SetGuagePieceWidth(5.0f);
    g.SetGuageImagePos(1, 2);
    // Should not crash even with all data set.
    g.Render();
    EXPECT_NO_THROW(g.Render());
    EXPECT_FLOAT_EQ(g.GetValue(), 0.42f);  // Render doesn't mutate state
}

}  // namespace mxh::ui::test
