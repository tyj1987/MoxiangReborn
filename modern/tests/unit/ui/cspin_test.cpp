// cspin_test.cpp — 1:1 port verification tests for cSpin.

#include "cspin.hpp"
#include "cbutton.hpp"
#include "ceditbox.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

using mxh::ui::cSpin;
using mxh::ui::cButton;
using mxh::ui::cEditBox;
using mxh::ui::SPINUNIT;

namespace {

std::unique_ptr<cSpin> MakeSpin() {
    auto s = std::make_unique<cSpin>();
    s->Init(0, 0, 100, 20, nullptr, {}, 1001);
    s->InitSpin(20, 20);
    return s;
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction defaults
// ---------------------------------------------------------------------------

TEST(CSpin, DefaultConstructionHasZeroChildren) {
    cSpin s;
    EXPECT_EQ(s.GetUpButton(), nullptr);
    EXPECT_EQ(s.GetDownButton(), nullptr);
    EXPECT_EQ(s.GetUnit(), 10u);
    EXPECT_EQ(s.GetMin(), 0u);
    EXPECT_EQ(s.GetMax(), 100u);
}

TEST(CSpin, DefaultValueIsZero) {
    auto s = MakeSpin();
    EXPECT_EQ(s->GetValue(), 0);
    EXPECT_EQ(s->editText(), "0");
}

TEST(CSpin, InitStoresConfig) {
    cSpin s;
    s.Init(10, 20, 100, 20, nullptr, {}, 2002);
    EXPECT_EQ(s.id(), 2002);
    EXPECT_EQ(s.GetUnit(), 10u);  // default preserved
    EXPECT_EQ(s.GetMin(),  0u);
    EXPECT_EQ(s.GetMax(),  100u);
}

TEST(CSpin, InitSpinSetsBuffer) {
    auto s = MakeSpin();
    EXPECT_EQ(s->maxBytes(), 20u);  // InitEditbox(20, 20) stores bufBytes
    EXPECT_EQ(s->GetValue(), 0);
}

// ---------------------------------------------------------------------------
// Setters / getters
// ---------------------------------------------------------------------------

TEST(CSpin, SetUnitGetUnitRoundTrip) {
    auto s = MakeSpin();
    s->SetUnit(7);
    EXPECT_EQ(s->GetUnit(), 7u);
}

TEST(CSpin, SetMinGetMinRoundTrip) {
    auto s = MakeSpin();
    s->SetMin(5);
    EXPECT_EQ(s->GetMin(), 5u);
}

TEST(CSpin, SetMaxGetMaxRoundTrip) {
    auto s = MakeSpin();
    s->SetMax(500);
    EXPECT_EQ(s->GetMax(), 500u);
}

TEST(CSpin, SetMinMaxRoundTrip) {
    auto s = MakeSpin();
    s->SetMinMax(20, 80);
    EXPECT_EQ(s->GetMin(), 20u);
    EXPECT_EQ(s->GetMax(), 80u);
}

// ---------------------------------------------------------------------------
// SetValue / GetValue
// ---------------------------------------------------------------------------

TEST(CSpin, SetValueStoresInRange) {
    auto s = MakeSpin();
    s->SetValue(42);
    EXPECT_EQ(s->GetValue(), 42);
    EXPECT_EQ(s->editText(), "42");
}

TEST(CSpin, SetValueBelowMinClampsToMin) {
    auto s = MakeSpin();
    s->SetMinMax(0, 100);
    s->SetValue(0);  // edge: 0 == min
    EXPECT_EQ(s->GetValue(), 0);
    // Try to set below min via parseCurrentValue path:
    s->SetEditText("-5");  // editText not clamped; GetValue parses
    // Negative doesn't parse to SPINUNIT; GetValue should clamp via parse.
    EXPECT_EQ(s->GetValue(), 0);  // parseCurrentValue → 0 (fallback)
}

TEST(CSpin, SetValueAboveMaxClampsToMax) {
    auto s = MakeSpin();
    s->SetMinMax(0, 100);
    s->SetValue(500);  // above max
    EXPECT_EQ(s->GetValue(), 100);
    EXPECT_EQ(s->editText(), "100");
}

TEST(CSpin, SetValueFormatsWithCommas) {
    auto s = MakeSpin();
    s->SetMinMax(0, 1000000);
    s->SetValue(12345);
    EXPECT_EQ(s->GetValue(), 12345);
    EXPECT_EQ(s->editText(), "12,345");
}

TEST(CSpin, GetValueParsesCommasInBuffer) {
    auto s = MakeSpin();
    s->SetMinMax(0, 1000000);
    s->SetEditText("1,234,567");
    EXPECT_EQ(s->GetValue(), 1234567);
}

TEST(CSpin, GetValueEmptyBufferReturnsMin) {
    auto s = MakeSpin();
    s->SetMin(5);
    s->SetEditText("");
    EXPECT_EQ(s->GetValue(), 5);  // min fallback
}

// ---------------------------------------------------------------------------
// IncUnit / DecUnit
// ---------------------------------------------------------------------------

TEST(CSpin, IncUnitBumpsByDefault) {
    auto s = MakeSpin();
    s->SetValue(0);
    s->IncUnit();
    EXPECT_EQ(s->GetValue(), 10);  // default m_Unit=10
}

TEST(CSpin, DecUnitBumpsByDefault) {
    auto s = MakeSpin();
    s->SetValue(50);
    s->DecUnit();
    EXPECT_EQ(s->GetValue(), 40);
}

TEST(CSpin, IncUnitSaturatesAtMax) {
    auto s = MakeSpin();
    s->SetMinMax(0, 50);
    s->SetValue(45);
    s->IncUnit();
    EXPECT_EQ(s->GetValue(), 50);
    s->IncUnit();
    EXPECT_EQ(s->GetValue(), 50);  // saturated
}

TEST(CSpin, DecUnitSaturatesAtMin) {
    auto s = MakeSpin();
    s->SetMinMax(0, 100);
    s->SetValue(5);
    s->DecUnit();
    EXPECT_EQ(s->GetValue(), 0);
    s->DecUnit();
    EXPECT_EQ(s->GetValue(), 0);  // saturated
}

TEST(CSpin, IncUnitRespectsCustomUnit) {
    auto s = MakeSpin();
    s->SetUnit(3);
    s->SetValue(10);
    s->IncUnit();
    EXPECT_EQ(s->GetValue(), 13);
}

TEST(CSpin, DecUnitRespectsCustomUnit) {
    auto s = MakeSpin();
    s->SetUnit(3);
    s->SetValue(10);
    s->DecUnit();
    EXPECT_EQ(s->GetValue(), 7);
}

// ---------------------------------------------------------------------------
// AddSpinButton
// ---------------------------------------------------------------------------

TEST(CSpin, AddUpButtonStoresPointer) {
    auto s = MakeSpin();
    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr, 100);
    s->AddSpinButton(std::move(btn), cSpin::SpinButtonKind::Up);
    EXPECT_NE(s->GetUpButton(), nullptr);
    EXPECT_EQ(s->GetDownButton(), nullptr);
}

TEST(CSpin, AddDownButtonStoresPointer) {
    auto s = MakeSpin();
    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr, 101);
    s->AddSpinButton(std::move(btn), cSpin::SpinButtonKind::Down);
    EXPECT_EQ(s->GetUpButton(), nullptr);
    EXPECT_NE(s->GetDownButton(), nullptr);
}

TEST(CSpin, AddBothButtonsInOrder) {
    auto s = MakeSpin();
    auto up = std::make_unique<cButton>();
    up->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr, 200);
    auto down = std::make_unique<cButton>();
    down->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr, 201);
    s->AddSpinButton(std::move(up),   cSpin::SpinButtonKind::Up);
    s->AddSpinButton(std::move(down), cSpin::SpinButtonKind::Down);
    EXPECT_NE(s->GetUpButton(), nullptr);
    EXPECT_NE(s->GetDownButton(), nullptr);
    EXPECT_NE(s->GetUpButton(),   s->GetDownButton());
}

TEST(CSpin, AddUpButtonTwiceIsIdempotent) {
    auto s = MakeSpin();
    auto btn1 = std::make_unique<cButton>();
    btn1->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr, 300);
    auto btn2 = std::make_unique<cButton>();
    btn2->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr, 301);
    s->AddSpinButton(std::move(btn1), cSpin::SpinButtonKind::Up);
    s->AddSpinButton(std::move(btn2), cSpin::SpinButtonKind::Up);  // ignored
    EXPECT_NE(s->GetUpButton(), nullptr);
    EXPECT_EQ(s->GetUpButton()->id(), 300);  // first one wins
}

TEST(CSpin, AddNullButtonIsSafe) {
    auto s = MakeSpin();
    s->AddSpinButton(nullptr, cSpin::SpinButtonKind::Up);
    EXPECT_EQ(s->GetUpButton(), nullptr);
}
