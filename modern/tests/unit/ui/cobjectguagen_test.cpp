// cobjectguagen_test.cpp — 1:1 port tests for
// 墨香 CObjectGuagen (object gauge with effect-time
// interpolation).
//
// Verifies:
//   - ctor does not crash
//   - Dtor does not crash
//   - Inherits from cGuagen
//   - SetValue clamps val > 1 to 1
//   - SetValue updates m_fCurPercentRate (via cGuagen)
//   - SetValue updates m_fOldPercentRate + m_fIncAmount
//     when estTime != 0
//   - SetValue with estTime = 0 sets m_fCurPercentRate
//     directly
//   - SetValue before Init does not crash
//   - ActionEvent returns 0 (WE_NULL)
//   - ActionEvent before Init does not crash
//   - Render is no-op
//   - Render before Init does not crash
//   - GetOldPercentRate / GetCurPercentRate /
//     GetIncAmount / GetEffectTime / GetStartTime
//     getters work
//   - SetGuageEffectPieceWidth /
//     SetGuageEffectPieceHeightScale setters
//     work
//   - IsBlink / SetBlink work

#include "cobjectguagen.hpp"
#include "cguagen.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using mxh::ui::cGuagen;
using mxh::ui::cObjectGuagen;
using mxh::ui::cWindow;
using mxh::ui::GUAGEVAL;

// ---------- ctor / dtor ----------

TEST(CObjectGuagenTest, CtorDoesNotCrash) {
    cObjectGuagen g;
    SUCCEED();
}

TEST(CObjectGuagenTest, DtorDoesNotCrash) {
    cObjectGuagen g;
    SUCCEED();
}

TEST(CObjectGuagenTest, InheritsFromCGuagen) {
    static_assert(std::is_base_of_v<cGuagen, cObjectGuagen>,
                  "cObjectGuagen must inherit from cGuagen");
    SUCCEED();
}

TEST(CObjectGuagenTest, IsACGuagen) {
    cObjectGuagen g;
    cGuagen* base = &g;
    EXPECT_NE(base, nullptr);
    cWindow* winBase = static_cast<cWindow*>(base);
    EXPECT_NE(winBase, nullptr);
}

// ---------- state defaults ----------

TEST(CObjectGuagenTest, DefaultStateIsZero) {
    cObjectGuagen g;
    EXPECT_FLOAT_EQ(g.GetOldPercentRate(), 0.0f);
    EXPECT_FLOAT_EQ(g.GetCurPercentRate(), 0.0f);
    EXPECT_FLOAT_EQ(g.GetIncAmount(), 0.0f);
    EXPECT_EQ(g.GetEffectTime(), 0u);
    EXPECT_EQ(g.GetStartTime(), 0u);
    EXPECT_FALSE(g.IsBlink());
}

TEST(CObjectGuagenTest, DefaultEffectPieceHeightScaleIsOne) {
    cObjectGuagen g;
    EXPECT_FLOAT_EQ(g.GetGuageEffectPieceHeightScaleY(), 1.0f);
}

TEST(CObjectGuagenTest, DefaultEffectPieceWidthIsZero) {
    cObjectGuagen g;
    EXPECT_FLOAT_EQ(g.GetGuageEffectPieceWidth(), 0.0f);
}

// ---------- SetValue ----------

TEST(CObjectGuagenTest, SetValueClampsToOne) {
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    g.SetValue(1.5f, 0);
    // The cGuagen::SetValue stores the clamped value.
    EXPECT_FLOAT_EQ(g.GetValue(), 1.0f);
}

TEST(CObjectGuagenTest, SetValueUpdatesCurPercentRate) {
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    g.SetValue(0.5f, 0);
    // cGuagen::SetValue stores the value.
    EXPECT_FLOAT_EQ(g.GetValue(), 0.5f);
    EXPECT_FLOAT_EQ(g.GetCurPercentRate(), 0.5f);
}

TEST(CObjectGuagenTest, SetValueWithEstTimeUpdatesIncAmount) {
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    g.SetValue(0.0f, 0);  // first call: set initial
    g.SetValue(1.0f, 1000);  // 1000 ms effect time
    // 1:1 with legacy (val - m_fOldPercentRate) / estTime
    // = (1.0 - 0.0) / 1000 = 0.001
    EXPECT_FLOAT_EQ(g.GetIncAmount(), 0.001f);
    EXPECT_EQ(g.GetEffectTime(), 1000u);
}

TEST(CObjectGuagenTest, SetValueWithZeroEstTimeSetsCurPercent) {
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    g.SetValue(0.5f, 0);
    // estTime = 0 → m_fCurPercentRate = val
    EXPECT_FLOAT_EQ(g.GetCurPercentRate(), 0.5f);
    EXPECT_EQ(g.GetEffectTime(), 0u);
}

TEST(CObjectGuagenTest, SetValuePreservesOldPercentRate) {
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    g.SetValue(0.3f, 0);  // first call: m_fCurPercentRate = 0.3
    g.SetValue(0.7f, 1000);  // second call: m_fOldPercentRate = 0.3
    EXPECT_FLOAT_EQ(g.GetOldPercentRate(), 0.3f);
}

TEST(CObjectGuagenTest, SetValueWithMultipleCalls) {
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    g.SetValue(0.0f, 0);
    g.SetValue(0.25f, 1000);
    g.SetValue(0.5f, 1000);
    g.SetValue(0.75f, 1000);
    g.SetValue(1.0f, 1000);
    EXPECT_FLOAT_EQ(g.GetValue(), 1.0f);
}

TEST(CObjectGuagenTest, SetValueBeforeInitDoesNotCrash) {
    cObjectGuagen g;
    g.SetValue(0.5f, 0);
    SUCCEED();
}

TEST(CObjectGuagenTest, SetValueWithNegativeValIsSafe) {
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    g.SetValue(-0.5f, 0);
    // cGuagen::SetValue may or may not clamp negatives;
    // the test just ensures no crash.
    SUCCEED();
}

// ---------- ActionEvent ----------

TEST(CObjectGuagenTest, ActionEventReturnsZero) {
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    EXPECT_EQ(g.ActionEvent(), 0u);
}

TEST(CObjectGuagenTest, ActionEventBeforeInitDoesNotCrash) {
    cObjectGuagen g;
    g.ActionEvent();
    SUCCEED();
}

// ---------- Render ----------

TEST(CObjectGuagenTest, RenderIsNoOp) {
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    g.Render();
    SUCCEED();
}

TEST(CObjectGuagenTest, RenderBeforeInitDoesNotCrash) {
    cObjectGuagen g;
    g.Render();
    SUCCEED();
}

TEST(CObjectGuagenTest, RenderAfterSetValueDoesNotCrash) {
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    g.SetValue(0.5f, 1000);
    g.Render();
    SUCCEED();
}

// ---------- setters / getters ----------

TEST(CObjectGuagenTest, SetGuageEffectPieceWidthUpdatesGetter) {
    cObjectGuagen g;
    g.SetGuageEffectPieceWidth(32.0f);
    EXPECT_FLOAT_EQ(g.GetGuageEffectPieceWidth(), 32.0f);
}

TEST(CObjectGuagenTest, SetGuageEffectPieceHeightScaleUpdatesGetter) {
    cObjectGuagen g;
    g.SetGuageEffectPieceHeightScale(1.5f);
    EXPECT_FLOAT_EQ(g.GetGuageEffectPieceHeightScaleY(), 1.5f);
}

TEST(CObjectGuagenTest, SetBlinkTogglesState) {
    cObjectGuagen g;
    EXPECT_FALSE(g.IsBlink());
    g.SetBlink(true);
    EXPECT_TRUE(g.IsBlink());
    g.SetBlink(false);
    EXPECT_FALSE(g.IsBlink());
}

TEST(CObjectGuagenTest, SetGuageEffectPieceWidthAndHeightScaleCoexist) {
    cObjectGuagen g;
    g.SetGuageEffectPieceWidth(64.0f);
    g.SetGuageEffectPieceHeightScale(2.0f);
    EXPECT_FLOAT_EQ(g.GetGuageEffectPieceWidth(), 64.0f);
    EXPECT_FLOAT_EQ(g.GetGuageEffectPieceHeightScaleY(), 2.0f);
}
