// titanguagedlg_test.cpp — 1:1 port verification tests for cTitanGuageDlg.

#include "titanguagedlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>

using mxh::ui::cTitanGuageDlg;
using mxh::ui::cObjectGuagen;
using mxh::ui::cStatic;
using mxh::ui::cDialog;
using mxh::ui::TitanCalcStats;

namespace {

std::unique_ptr<cTitanGuageDlg> MakeDialog() {
    auto d = std::make_unique<cTitanGuageDlg>();
    d->Init(0, 0, 200, 100, nullptr, 1003);
    return d;
}

}  // namespace

// ---------------------------------------------------------------------------
// Constants + construction
// ---------------------------------------------------------------------------

TEST(CTitanGuageDlg, IdConstantsMatchLocalRange) {
    EXPECT_EQ(cTitanGuageDlg::kIdTitanGuage, 1000);
    EXPECT_EQ(cTitanGuageDlg::kIdHpText,     1001);
    EXPECT_EQ(cTitanGuageDlg::kIdLookBtn,    1002);
}

TEST(CTitanGuageDlg, DefaultConstructionHasNullChildren) {
    auto d = MakeDialog();
    EXPECT_EQ(d->guage(), nullptr);
    EXPECT_EQ(d->hpPercentText(), nullptr);
}

TEST(CTitanGuageDlg, DefaultStatsHasZeroMaxFuel) {
    auto d = MakeDialog();
    // Default-constructed TitanCalcStats: MaxFuel=0, MaxSpell=0.
    EXPECT_EQ(d->statsForTesting().MaxFuel, 0u);
    EXPECT_EQ(d->statsForTesting().MaxSpell, 0u);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CTitanGuageDlg, LinkingMaterializesBothChildren) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_NE(d->guage(), nullptr);
    EXPECT_NE(d->hpPercentText(), nullptr);
}

TEST(CTitanGuageDlg, LinkingSetsChildIds) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_EQ(d->guage()->id(), cTitanGuageDlg::kIdTitanGuage);
    EXPECT_EQ(d->hpPercentText()->id(), cTitanGuageDlg::kIdHpText);
}

TEST(CTitanGuageDlg, LinkingIdempotent) {
    auto d = MakeDialog();
    d->Linking();
    const cObjectGuagen* guageFirst   = d->guage();
    const cStatic*       textFirst    = d->hpPercentText();
    d->Linking();
    EXPECT_EQ(d->guage(), guageFirst);
    EXPECT_EQ(d->hpPercentText(), textFirst);
}

// ---------------------------------------------------------------------------
// SetLife
// ---------------------------------------------------------------------------

TEST(CTitanGuageDlg, SetLifeWithZeroMaxFuelIsSafe) {
    auto d = MakeDialog();
    d->Linking();
    cTitanGuageDlg::ClearTitanStatsForTesting();
    // MaxFuel=0 → SetValue(0,0) + percent " : X/0".
    d->SetLife(50u);
    EXPECT_EQ(d->guage()->GetValue(), 0.0f);
    EXPECT_EQ(d->hpPercentText()->GetStaticText(), " : 50/0");
}

TEST(CTitanGuageDlg, SetLifeHalfMax) {
    auto d = MakeDialog();
    d->Linking();
    cTitanGuageDlg::SetTitanStatsForTesting(TitanCalcStats{100u, 0u});
    d->SetLife(50u);
    EXPECT_FLOAT_EQ(d->guage()->GetValue(), 0.5f);
    EXPECT_EQ(d->hpPercentText()->GetStaticText(), " : 50/100");
}

TEST(CTitanGuageDlg, SetLifeFullMax) {
    auto d = MakeDialog();
    d->Linking();
    cTitanGuageDlg::SetTitanStatsForTesting(TitanCalcStats{200u, 0u});
    d->SetLife(200u);
    EXPECT_FLOAT_EQ(d->guage()->GetValue(), 1.0f);
    EXPECT_EQ(d->hpPercentText()->GetStaticText(), " : 200/200");
}

TEST(CTitanGuageDlg, SetLifeOverMax) {
    auto d = MakeDialog();
    d->Linking();
    cTitanGuageDlg::SetTitanStatsForTesting(TitanCalcStats{100u, 0u});
    d->SetLife(150u);  // 150 / 100 = 1.5
    // 1:1 quirk: modern cObjectGuagen::SetValue clamps val > 1.0
    // to 1.0 (per its own 1:1 port quirk — see cObjectGuagen.cpp
    // line 59-60). Legacy CObjectGuagen::SetValue did not clamp
    // and would have stored 1.5. The clamp is a modern port quirk
    // that affects SetLife's guage value. The percent text is
    // unclamped (legacy sprintf).
    EXPECT_FLOAT_EQ(d->guage()->GetValue(), 1.0f);
    EXPECT_EQ(d->hpPercentText()->GetStaticText(), " : 150/100");
}

TEST(CTitanGuageDlg, SetLifeUpdatesStatsForTesting) {
    auto d = MakeDialog();
    d->Linking();
    cTitanGuageDlg::SetTitanStatsForTesting(TitanCalcStats{300u, 0u});
    d->SetLife(100u);
    EXPECT_EQ(d->statsForTesting().MaxFuel, 300u);
    EXPECT_FLOAT_EQ(d->guage()->GetValue(), 100.0f / 300.0f);
}

// ---------------------------------------------------------------------------
// SetNaeRyuk
// ---------------------------------------------------------------------------

TEST(CTitanGuageDlg, SetNaeRyukIsNoOp) {
    // 1:1 quirk: legacy SetNaeRyuk body is fully commented out.
    // Modern port: empty body, no observable side effect.
    auto d = MakeDialog();
    d->Linking();
    cTitanGuageDlg::SetTitanStatsForTesting(TitanCalcStats{100u, 200u});
    d->SetNaeRyuk(50u);
    // Guage should still be the default (Linking initialised to 0).
    EXPECT_FLOAT_EQ(d->guage()->GetValue(), 0.0f);
    // Text should still be empty.
    EXPECT_EQ(d->hpPercentText()->GetStaticText(), "");
}

// ---------------------------------------------------------------------------
// SetActive override
// ---------------------------------------------------------------------------

TEST(CTitanGuageDlg, SetActiveTrueDelegatesToBase) {
    auto d = MakeDialog();
    d->Linking();
    d->SetActive(true);
    EXPECT_TRUE(d->isActive());
}

TEST(CTitanGuageDlg, SetActiveFalseDelegatesToBase) {
    auto d = MakeDialog();
    d->Linking();
    d->SetActive(true);
    d->SetActive(false);
    EXPECT_FALSE(d->isActive());
}

TEST(CTitanGuageDlg, SetActiveFalseDoesNotCrashWithoutGAMEIN) {
    // 1:1 quirk: legacy SetActive(false) cascades to
    // GAMEIN->GetTitanInventoryDlg()->SetActive(FALSE) — modern
    // port: GAMEIN is stubbed, so the cascade is a no-op. The
    // dialog close is preserved.
    auto d = MakeDialog();
    d->Linking();
    d->SetActive(false);
    EXPECT_FALSE(d->isActive());
}

// ---------------------------------------------------------------------------
// OnActionEventStatic
// ---------------------------------------------------------------------------

TEST(CTitanGuageDlg, OnActionEventStaticLookBtnIsNoOp) {
    // 1:1 quirk: legacy OnActionEvent branches on TITAN_GUAGE_LOOKBTN
    // and toggles TitanInventoryDlg via GAMEIN. Stubbed no-op.
    EXPECT_TRUE(cTitanGuageDlg::OnActionEventStatic(
        cTitanGuageDlg::kIdLookBtn, nullptr, 0u));
}

TEST(CTitanGuageDlg, OnActionEventStaticUnknownIdIsNoOp) {
    EXPECT_TRUE(cTitanGuageDlg::OnActionEventStatic(99999, nullptr, 0u));
}
