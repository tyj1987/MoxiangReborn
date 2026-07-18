// guagedialog_test.cpp — 1:1 port verification tests for cGuageDialog.

#include "guagedialog.hpp"
#include "cbutton.hpp"
#include "cstatic.hpp"
#include "cobjectguagen.hpp"
#include "cWindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

using mxh::ui::cGuageDialog;
using mxh::ui::cButton;
using mxh::ui::cStatic;
using mxh::ui::cObjectGuagen;
using mxh::ui::cWindow;
using WE = cWindow::WindowEvent;

namespace {

std::unique_ptr<cGuageDialog> MakeDialog() {
    auto d = std::make_unique<cGuageDialog>();
    d->Init(0, 0, 200, 100, nullptr, 909);
    return d;
}

}  // namespace

// ---------------------------------------------------------------------------
// Constants + construction
// ---------------------------------------------------------------------------

TEST(CGuageDialog, IdConstantsMatchLocalRange) {
    EXPECT_EQ(cGuageDialog::kIdMussangBtn,   900);
    EXPECT_EQ(cGuageDialog::kIdFlicker01,    901);
    EXPECT_EQ(cGuageDialog::kIdGuageMussang, 902);
    EXPECT_EQ(cGuageDialog::kFlickerTimeMs,  100u);
}

TEST(CGuageDialog, DefaultConstructionHasNullPointersAndFalseFlags) {
    auto d = MakeDialog();
    EXPECT_EQ(d->GetMussangButton(), nullptr);
    EXPECT_EQ(d->GetFlicker01(),     nullptr);
    EXPECT_FALSE(d->isFlickerActive());
    EXPECT_FALSE(d->isFlickerOn());
    EXPECT_EQ(d->flickSwapTime(), 0u);
    EXPECT_EQ(d->imageRGB(), 0xFFFFFFFFu);  // default full color
    EXPECT_EQ(d->nowMillis(), 0u);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CGuageDialog, LinkingMaterializesMussangButtonAndFlicker) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_NE(d->GetMussangButton(), nullptr);
    EXPECT_NE(d->GetFlicker01(),     nullptr);
    EXPECT_EQ(d->GetMussangButton()->id(), cGuageDialog::kIdMussangBtn);
    EXPECT_EQ(d->GetFlicker01()->id(),     cGuageDialog::kIdFlicker01);
}

TEST(CGuageDialog, LinkingDisablesMussangButtonByDefault) {
    // 1:1 quirk: legacy `if( m_pMussangBtn ) DisableMussangBtn(TRUE);`
    // — unconditional disable on Link.
    auto d = MakeDialog();
    d->Linking();
    EXPECT_FALSE(d->GetMussangButton()->isEnabled());
    EXPECT_EQ(d->imageRGB(), 0xFFC8C8C8u);  // HalfColor (1:1 with legacy RGBA_MAKE)
}

TEST(CGuageDialog, LinkingFlickerStartsInactive) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_FALSE(d->isFlickerActive());
    EXPECT_FALSE(d->isFlickerOn());
}

TEST(CGuageDialog, LinkingIdempotent) {
    auto d = MakeDialog();
    d->Linking();
    const cButton* btnFirst = d->GetMussangButton();
    const cStatic* flFirst  = d->GetFlicker01();
    d->Linking();
    EXPECT_EQ(d->GetMussangButton(), btnFirst);
    EXPECT_EQ(d->GetFlicker01(),     flFirst);
}

// ---------------------------------------------------------------------------
// DisableMussangBtn
// ---------------------------------------------------------------------------

TEST(CGuageDialog, DisableMussangBtnTrueSetsHalfColor) {
    auto d = MakeDialog();
    d->Linking();
    d->DisableMussangBtn(true);
    EXPECT_FALSE(d->GetMussangButton()->isEnabled());
    EXPECT_EQ(d->imageRGB(), 0xFFC8C8C8u);
}

TEST(CGuageDialog, DisableMussangBtnFalseSetsFullColor) {
    auto d = MakeDialog();
    d->Linking();
    d->DisableMussangBtn(false);
    EXPECT_TRUE(d->GetMussangButton()->isEnabled());
    EXPECT_EQ(d->imageRGB(), 0xFFFFFFFFu);
}

TEST(CGuageDialog, DisableMussangBtnToggleRoundTrip) {
    auto d = MakeDialog();
    d->Linking();
    d->DisableMussangBtn(false);
    EXPECT_EQ(d->imageRGB(), 0xFFFFFFFFu);
    d->DisableMussangBtn(true);
    EXPECT_EQ(d->imageRGB(), 0xFFC8C8C8u);
    d->DisableMussangBtn(false);
    EXPECT_EQ(d->imageRGB(), 0xFFFFFFFFu);
}

// ---------------------------------------------------------------------------
// OnActionEvent
// ---------------------------------------------------------------------------

TEST(CGuageDialog, OnActionEventNonBtnClickIsNoOp) {
    auto d = MakeDialog();
    d->Linking();
    const std::uint32_t kMouseMove = static_cast<std::uint32_t>(WE::MouseMove);
    d->OnActionEvent(cGuageDialog::kIdMussangBtn, nullptr, kMouseMove);
    // Stub conservative: never allows, so this just no-ops.
    SUCCEED();
}

TEST(CGuageDialog, OnActionEventMussangBtnIsNoOpWhenHeroInvalid) {
    // 1:1 quirk: legacy `if(!HERO->IsDied() && !HERO->InTitan()) ...`
    // Stub returns true for both → branch never executes.
    auto d = MakeDialog();
    d->Linking();
    const std::uint32_t kLButtonClick = static_cast<std::uint32_t>(WE::LButtonClick);
    d->OnActionEvent(cGuageDialog::kIdMussangBtn, nullptr, kLButtonClick);
    SUCCEED();
}

TEST(CGuageDialog, OnActionEventUnknownIdIsNoOp) {
    auto d = MakeDialog();
    d->Linking();
    const std::uint32_t kLButtonClick = static_cast<std::uint32_t>(WE::LButtonClick);
    d->OnActionEvent(99999, nullptr, kLButtonClick);
    SUCCEED();
}

// ---------------------------------------------------------------------------
// SetFlicker / FlickerMussangGuage
// ---------------------------------------------------------------------------

TEST(CGuageDialog, SetFlickerTrueActivatesFlicker) {
    auto d = MakeDialog();
    d->Linking();
    d->SetFlicker(true);
    EXPECT_TRUE(d->isFlickerActive());
    // 1:1 quirk: legacy `m_pFlicker01->SetActive(bFlicker);` — but
    // cStatic has no SetActive (per R-12 fix). Modern port uses
    // SetVisible, which is the 1:1 behavioral equivalent.
    EXPECT_TRUE(d->GetFlicker01()->isVisible());
}

TEST(CGuageDialog, SetFlickerFalseDeactivatesFlicker) {
    auto d = MakeDialog();
    d->Linking();
    d->SetFlicker(true);
    d->SetFlicker(false);
    EXPECT_FALSE(d->isFlickerActive());
    EXPECT_FALSE(d->GetFlicker01()->isVisible());
}

TEST(CGuageDialog, FlickerNoSwapWhenBelowThreshold) {
    auto d = MakeDialog();
    d->Linking();
    d->SetMillisForTesting(0);
    d->SetFlicker(true);
    // Advance < kFlickerTimeMs → no swap.
    d->AdvanceMillisForTesting(50);
    d->FlickerMussangGuage();
    // m_bFlActive was initialised to false; no swap means still false.
    EXPECT_FALSE(d->isFlickerOn());
}

TEST(CGuageDialog, FlickerSwapsAfterThreshold) {
    auto d = MakeDialog();
    d->Linking();
    d->SetMillisForTesting(0);
    d->SetFlicker(true);
    // Advance > kFlickerTimeMs → swap to true.
    d->AdvanceMillisForTesting(150);
    d->FlickerMussangGuage();
    EXPECT_TRUE(d->isFlickerOn());
    EXPECT_TRUE(d->GetFlicker01()->isVisible());
}

TEST(CGuageDialog, FlickerDoubleSwapTogglesBackToFalse) {
    auto d = MakeDialog();
    d->Linking();
    d->SetMillisForTesting(0);
    d->SetFlicker(true);
    // First swap (0 → true)
    d->AdvanceMillisForTesting(150);
    d->FlickerMussangGuage();
    EXPECT_TRUE(d->isFlickerOn());
    // Second swap (true → false)
    d->AdvanceMillisForTesting(150);
    d->FlickerMussangGuage();
    EXPECT_FALSE(d->isFlickerOn());
    EXPECT_FALSE(d->GetFlicker01()->isVisible());
}

TEST(CGuageDialog, FlickerNoOpWhenInactive) {
    auto d = MakeDialog();
    d->Linking();
    d->SetMillisForTesting(0);
    // m_bFlicker is false (default); FlickerMussangGuage is no-op.
    d->AdvanceMillisForTesting(500);
    d->FlickerMussangGuage();
    EXPECT_FALSE(d->isFlickerOn());
    // flickSwapTime stays at 0 (no reset because branch never ran).
    EXPECT_EQ(d->flickSwapTime(), 0u);
}
