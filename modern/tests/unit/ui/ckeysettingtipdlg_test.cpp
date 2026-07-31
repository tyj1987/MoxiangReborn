//
// Unit tests for mxh::ui::cKeySettingTipDlg (Phase C dialog port).
//
// Locks down the 1:1 surface of legacy CKeySettingTipDlg
// (keyboard shortcut tip dialog: 2 cImageSelf + Render
// with 2 guards + 3-step rendering):
//   * Constants: kModeHidden=2, kNumImages=2,
//                kSrcRect=0,0,1024,768,
//                kPathImage0="Image/2D/KeySetting1.tga",
//                kPathImage1="Image/2D/KeySetting2.tga"
//   * Default construction: m_wMode = 2 (hidden)
//   * Inherits from cDialog
//   * NonCopyable
//   * SetMode / GetMode round-trip
//   * GetMode default is kModeHidden
//   * Linking stores 2 image paths
//   * Linking calls LoadSprite twice with both paths
//   * Linking calls SetImageSrcRect twice with
//     (0,0,1024,768)
//   * Linking without callbacks is safe
//   * Linking is idempotent (paths stay the same)
//   * Render guards: !m_bActive -> no callbacks
//   * Render guards: m_wMode > 1 -> no callbacks
//   * Render fires 3 callbacks in order
//     (RenderWindow, RenderSprite, RenderComponent)
//   * Render passes m_wMode to the render callback
//   * Render mode 0 uses mode 0
//   * Render mode 1 uses mode 1
//   * Render without callback is safe (no crash)
//   * Render before Linking is safe
//   * Render before SetMode uses default (kModeHidden)
//   * GetImagePathForTest returns the right path
//   * HasRenderCallbackForTest reports state
//

#include "mxh/ui/ckeysettingtipdlg.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/cwindow.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

using mxh::ui::cDialog;
using mxh::ui::cKeySettingTipDlg;
using mxh::ui::cWindow;

namespace {

// LoadSprite callback state.
struct LoadSpriteCall {
    int slot = -1;
    std::string path;
};
LoadSpriteCall g_lastLoadSprite;
int            g_loadSpriteCount = 0;

void ResetLoadSpriteState() {
    g_lastLoadSprite = LoadSpriteCall{};
    g_loadSpriteCount = 0;
}

void TestLoadSpriteCallback(int slot, const char* path, void* /*user*/) {
    g_lastLoadSprite.slot = slot;
    g_lastLoadSprite.path = (path ? path : "");
    ++g_loadSpriteCount;
}

// SetImageSrcRect callback state.
struct SrcRectCall {
    int slot = -1;
    int left = 0, top = 0, right = 0, bottom = 0;
};
SrcRectCall g_lastSrcRect;
int         g_setSrcRectCount = 0;

void ResetSrcRectState() {
    g_lastSrcRect = SrcRectCall{};
    g_setSrcRectCount = 0;
}

void TestSetSrcRectCallback(int slot, int left, int top, int right,
                            int bottom, void* /*user*/) {
    g_lastSrcRect.slot   = slot;
    g_lastSrcRect.left   = left;
    g_lastSrcRect.top    = top;
    g_lastSrcRect.right  = right;
    g_lastSrcRect.bottom = bottom;
    ++g_setSrcRectCount;
}

// Render callback state.
int g_renderMode = -1;
cKeySettingTipDlg::RenderStep g_lastRenderStep =
    cKeySettingTipDlg::RenderStep::RenderWindow;
int g_renderCallCount = 0;
std::vector<cKeySettingTipDlg::RenderStep> g_renderStepOrder;

void ResetRenderState() {
    g_renderMode = -1;
    g_lastRenderStep = cKeySettingTipDlg::RenderStep::RenderWindow;
    g_renderCallCount = 0;
    g_renderStepOrder.clear();
}

void TestRenderCallback(int mode, cKeySettingTipDlg::RenderStep step,
                        void* /*user*/) {
    g_renderMode = mode;
    g_lastRenderStep = step;
    ++g_renderCallCount;
    g_renderStepOrder.push_back(step);
}

struct Harness {
    cKeySettingTipDlg dlg;

    Harness() {
        dlg.Init(0, 0, 1024, 768, nullptr, 0);
        dlg.SetActive(true);
        dlg.SetLoadSpriteCallbackForTest(TestLoadSpriteCallback, nullptr);
        dlg.SetImageSrcRectCallbackForTest(TestSetSrcRectCallback, nullptr);
        dlg.SetRenderCallbacksForTest(TestRenderCallback, nullptr);
        ResetLoadSpriteState();
        ResetSrcRectState();
        ResetRenderState();
    }
};

}  // namespace

// ---------- Construction / destruction ----------

TEST(CKeySettingTipDlgTest, CtorDoesNotCrash) {
    cKeySettingTipDlg dlg;
    SUCCEED();
}

TEST(CKeySettingTipDlgTest, DtorDoesNotCrash) {
    cKeySettingTipDlg dlg;
    SUCCEED();
}

TEST(CKeySettingTipDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cKeySettingTipDlg>,
                  "cKeySettingTipDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CKeySettingTipDlgTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cKeySettingTipDlg>,
                  "cKeySettingTipDlg must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cKeySettingTipDlg>,
                  "cKeySettingTipDlg must be non-copy-assignable");
    SUCCEED();
}

// ---------- Constants ----------

TEST(CKeySettingTipDlgTest, ModeHiddenIs2) {
    EXPECT_EQ(cKeySettingTipDlg::kModeHidden, 2u);
}

TEST(CKeySettingTipDlgTest, NumImagesIs2) {
    EXPECT_EQ(cKeySettingTipDlg::kNumImages, 2u);
}

TEST(CKeySettingTipDlgTest, SrcRectIs0_0_1024_768) {
    EXPECT_EQ(cKeySettingTipDlg::kSrcRectLeft, 0);
    EXPECT_EQ(cKeySettingTipDlg::kSrcRectTop, 0);
    EXPECT_EQ(cKeySettingTipDlg::kSrcRectRight, 1024);
    EXPECT_EQ(cKeySettingTipDlg::kSrcRectBottom, 768);
}

TEST(CKeySettingTipDlgTest, ImagePathsMatchLegacy) {
    EXPECT_STREQ(cKeySettingTipDlg::kPathImage0, "Image/2D/KeySetting1.tga");
    EXPECT_STREQ(cKeySettingTipDlg::kPathImage1, "Image/2D/KeySetting2.tga");
}

TEST(CKeySettingTipDlgTest, RenderStepEnumValues) {
    EXPECT_EQ(static_cast<int>(cKeySettingTipDlg::RenderStep::RenderWindow), 0);
    EXPECT_EQ(static_cast<int>(cKeySettingTipDlg::RenderStep::RenderSprite), 1);
    EXPECT_EQ(static_cast<int>(cKeySettingTipDlg::RenderStep::RenderComponent), 2);
}

// ---------- Default state ----------

TEST(CKeySettingTipDlgTest, DefaultModeIsHidden) {
    cKeySettingTipDlg dlg;
    EXPECT_EQ(dlg.GetMode(), cKeySettingTipDlg::kModeHidden);
}

TEST(CKeySettingTipDlgTest, DefaultPathsAreEmpty) {
    cKeySettingTipDlg dlg;
    EXPECT_EQ(dlg.GetImagePathForTest(0), "");
    EXPECT_EQ(dlg.GetImagePathForTest(1), "");
}

// ---------- SetMode / GetMode ----------

TEST(CKeySettingTipDlgTest, SetModeStoresValue) {
    Harness h;
    h.dlg.SetMode(0);
    EXPECT_EQ(h.dlg.GetMode(), 0u);
    h.dlg.SetMode(1);
    EXPECT_EQ(h.dlg.GetMode(), 1u);
    h.dlg.SetMode(5);
    EXPECT_EQ(h.dlg.GetMode(), 5u);
}

TEST(CKeySettingTipDlgTest, SetModeToHiddenStoresHidden) {
    Harness h;
    h.dlg.SetMode(0);
    h.dlg.SetMode(cKeySettingTipDlg::kModeHidden);
    EXPECT_EQ(h.dlg.GetMode(), cKeySettingTipDlg::kModeHidden);
}

// ---------- Linking ----------

TEST(CKeySettingTipDlgTest, LinkingStoresImagePaths) {
    Harness h;
    h.dlg.Linking();
    EXPECT_STREQ(h.dlg.GetImagePathForTest(0).c_str(),
                 cKeySettingTipDlg::kPathImage0);
    EXPECT_STREQ(h.dlg.GetImagePathForTest(1).c_str(),
                 cKeySettingTipDlg::kPathImage1);
}

TEST(CKeySettingTipDlgTest, LinkingCallsLoadSpriteTwice) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(g_loadSpriteCount, 2);
}

TEST(CKeySettingTipDlgTest, LinkingPassesCorrectPaths) {
    Harness h;
    h.dlg.Linking();
    EXPECT_STREQ(g_lastLoadSprite.path.c_str(),
                 cKeySettingTipDlg::kPathImage1);
    EXPECT_EQ(g_lastLoadSprite.slot, 1);
}

TEST(CKeySettingTipDlgTest, LinkingCallsSetSrcRectTwice) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(g_setSrcRectCount, 2);
}

TEST(CKeySettingTipDlgTest, LinkingPassesCorrectSrcRect) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(g_lastSrcRect.slot, 1);
    EXPECT_EQ(g_lastSrcRect.left, 0);
    EXPECT_EQ(g_lastSrcRect.top, 0);
    EXPECT_EQ(g_lastSrcRect.right, 1024);
    EXPECT_EQ(g_lastSrcRect.bottom, 768);
}

TEST(CKeySettingTipDlgTest, LinkingWithoutCallbacksIsSafe) {
    cKeySettingTipDlg dlg;
    dlg.Init(0, 0, 1024, 768, nullptr, 0);
    dlg.Linking();
    EXPECT_STREQ(dlg.GetImagePathForTest(0).c_str(),
                 cKeySettingTipDlg::kPathImage0);
}

TEST(CKeySettingTipDlgTest, LinkingIsIdempotent) {
    Harness h;
    h.dlg.Linking();
    int firstLoadCount = g_loadSpriteCount;
    int firstRectCount = g_setSrcRectCount;
    h.dlg.Linking();
    EXPECT_EQ(g_loadSpriteCount, firstLoadCount + 2);
    EXPECT_EQ(g_setSrcRectCount, firstRectCount + 2);
}

TEST(CKeySettingTipDlgTest, LinkingBeforeInitDoesNotCrash) {
    cKeySettingTipDlg dlg;
    dlg.Linking();
    SUCCEED();
}

// ---------- Render: guards ----------

TEST(CKeySettingTipDlgTest, RenderInactiveDoesNotFireCallbacks) {
    Harness h;
    h.dlg.SetActive(false);
    h.dlg.SetMode(0);
    h.dlg.Render();
    EXPECT_EQ(g_renderCallCount, 0);
}

TEST(CKeySettingTipDlgTest, RenderWithModeGreaterThan1DoesNotFire) {
    Harness h;
    h.dlg.SetMode(2);
    h.dlg.Render();
    EXPECT_EQ(g_renderCallCount, 0);
}

TEST(CKeySettingTipDlgTest, RenderWithMode3DoesNotFire) {
    Harness h;
    h.dlg.SetMode(3);
    h.dlg.Render();
    EXPECT_EQ(g_renderCallCount, 0);
}

TEST(CKeySettingTipDlgTest, RenderWithMode255DoesNotFire) {
    Harness h;
    h.dlg.SetMode(255);
    h.dlg.Render();
    EXPECT_EQ(g_renderCallCount, 0);
}

// ---------- Render: actual rendering ----------

TEST(CKeySettingTipDlgTest, RenderMode0FiresAll3Callbacks) {
    Harness h;
    h.dlg.SetMode(0);
    h.dlg.Render();
    EXPECT_EQ(g_renderCallCount, 3);
}

TEST(CKeySettingTipDlgTest, RenderMode1FiresAll3Callbacks) {
    Harness h;
    h.dlg.SetMode(1);
    h.dlg.Render();
    EXPECT_EQ(g_renderCallCount, 3);
}

TEST(CKeySettingTipDlgTest, RenderFiresInCorrectOrder) {
    Harness h;
    h.dlg.SetMode(0);
    h.dlg.Render();
    ASSERT_EQ(g_renderStepOrder.size(), 3u);
    EXPECT_EQ(g_renderStepOrder[0],
              cKeySettingTipDlg::RenderStep::RenderWindow);
    EXPECT_EQ(g_renderStepOrder[1],
              cKeySettingTipDlg::RenderStep::RenderSprite);
    EXPECT_EQ(g_renderStepOrder[2],
              cKeySettingTipDlg::RenderStep::RenderComponent);
}

TEST(CKeySettingTipDlgTest, RenderPassesModeToCallback) {
    Harness h;
    h.dlg.SetMode(0);
    h.dlg.Render();
    EXPECT_EQ(g_renderMode, 0);
    g_renderMode = -1;
    h.dlg.SetMode(1);
    h.dlg.Render();
    EXPECT_EQ(g_renderMode, 1);
}

TEST(CKeySettingTipDlgTest, RenderWithoutCallbackIsSafe) {
    cKeySettingTipDlg dlg;
    dlg.Init(0, 0, 1024, 768, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetMode(0);
    dlg.Render();
    SUCCEED();
}

TEST(CKeySettingTipDlgTest, RenderBeforeLinkingIsSafe) {
    cKeySettingTipDlg dlg;
    dlg.Init(0, 0, 1024, 768, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetMode(0);
    dlg.Render();
    SUCCEED();
}

TEST(CKeySettingTipDlgTest, RenderBeforeSetModeUsesDefault) {
    Harness h;
    // Default mode is kModeHidden (2), so Render is a no-op.
    h.dlg.Render();
    EXPECT_EQ(g_renderCallCount, 0);
}

// ---------- HasRenderCallbackForTest ----------

TEST(CKeySettingTipDlgTest, HasRenderCallbackDefaultsFalse) {
    cKeySettingTipDlg dlg;
    EXPECT_FALSE(dlg.HasRenderCallbackForTest());
}

TEST(CKeySettingTipDlgTest, HasRenderCallbackAfterSetReturnsTrue) {
    Harness h;
    EXPECT_TRUE(h.dlg.HasRenderCallbackForTest());
}