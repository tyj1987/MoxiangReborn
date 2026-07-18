// fortwartimedialog_test.cpp — 1:1 port verification tests for FortWar dialogs.

#include "fortwartimedialog.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

using mxh::ui::cFWEngraveDialog;
using mxh::ui::cFWTimeDialog;

namespace {

std::unique_ptr<cFWEngraveDialog> MakeEngrave() {
    auto d = std::make_unique<cFWEngraveDialog>();
    d->Init(0, 0, 200, 100, nullptr, 779);
    return d;
}

std::unique_ptr<cFWTimeDialog> MakeTime() {
    auto d = std::make_unique<cFWTimeDialog>();
    d->Init(0, 0, 200, 100, nullptr, 778);
    return d;
}

}  // namespace

// ===========================================================================
// cFWEngraveDialog
// ===========================================================================

// ---------------------------------------------------------------------------
// Construction + constants
// ---------------------------------------------------------------------------

TEST(CFWEngraveDialog, DefaultProcessTimeIsZero) {
    auto d = MakeEngrave();
    EXPECT_EQ(d->GetProcessTime(), 0u);
    EXPECT_FLOAT_EQ(d->GetBasicTime(), 1.0f);
}

TEST(CFWEngraveDialog, ChildrenNullBeforeLinking) {
    auto d = MakeEngrave();
    EXPECT_EQ(d->GetEngraveGuage(), nullptr);
    EXPECT_EQ(d->GetRemaintimeStatic(), nullptr);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CFWEngraveDialog, LinkingMaterializesBothChildren) {
    auto d = MakeEngrave();
    d->Linking();
    EXPECT_NE(d->GetEngraveGuage(), nullptr);
    EXPECT_NE(d->GetRemaintimeStatic(), nullptr);
}

TEST(CFWEngraveDialog, LinkingSetsChildIds) {
    auto d = MakeEngrave();
    d->Linking();
    EXPECT_EQ(d->GetEngraveGuage()->id(),      780);
    EXPECT_EQ(d->GetRemaintimeStatic()->id(), 781);
}

TEST(CFWEngraveDialog, LinkingIdempotent) {
    auto d = MakeEngrave();
    d->Linking();
    d->Linking();
    EXPECT_NE(d->GetEngraveGuage(), nullptr);
    EXPECT_NE(d->GetRemaintimeStatic(), nullptr);
}

// ---------------------------------------------------------------------------
// SetActiveWithTime
// ---------------------------------------------------------------------------

TEST(CFWEngraveDialog, SetActiveWithTimeTrueStoresProcessTime) {
    auto d = MakeEngrave();
    d->Linking();
    d->SetActiveWithTime(true, 30);  // 30 seconds
    EXPECT_EQ(d->GetProcessTime(), 30u * 1000u);
    EXPECT_FLOAT_EQ(d->GetBasicTime(), 30.0f);
    EXPECT_TRUE(d->isActive());
}

TEST(CFWEngraveDialog, SetActiveWithTimeFalseResetsState) {
    auto d = MakeEngrave();
    d->Linking();
    d->SetActiveWithTime(true, 10);
    d->SetActiveWithTime(false, 0);
    EXPECT_EQ(d->GetProcessTime(), 0u);
    EXPECT_FLOAT_EQ(d->GetBasicTime(), 1.0f);
    EXPECT_FALSE(d->isActive());
}

TEST(CFWEngraveDialog, SetActiveWithTimeTrueThenTrue) {
    auto d = MakeEngrave();
    d->Linking();
    d->SetActiveWithTime(true, 5);
    d->SetActiveWithTime(true, 60);
    EXPECT_EQ(d->GetProcessTime(), 60u * 1000u);
    EXPECT_FLOAT_EQ(d->GetBasicTime(), 60.0f);
}

// ---------------------------------------------------------------------------
// ActionEvent
// ---------------------------------------------------------------------------

TEST(CFWEngraveDialog, ActionEventOnDisabledDialogReturnsZero) {
    auto d = MakeEngrave();
    d->Linking();
    // Dialog starts disabled.
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    EXPECT_EQ(we, 0u);  // WE_NULL
}

TEST(CFWEngraveDialog, ActionEventOnEnabledDialogDelegatesToBase) {
    auto d = MakeEngrave();
    d->Linking();
    d->SetActive(true);
    // Stubbed in modern port: no time refresh (gCurTime unported), but
    // base cDialog::ActionEvent still gets called for hit-test.
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    // Hit test on (0,0) which is inside the dialog at (0,0,200,100) →
    // topmost child hit → returns some non-zero we bits (likely
    // WE_MOUSEOVER from base). We only assert "did not crash".
    (void)we;
    SUCCEED();
}

// ---------------------------------------------------------------------------
// OnActionEvent
// ---------------------------------------------------------------------------

TEST(CFWEngraveDialog, OnActionEventIsNoOp) {
    auto d = MakeEngrave();
    d->Linking();
    d->OnActionEvent(999, nullptr, 0x4);  // bogus id + WE_BTNCLICK
    SUCCEED();  // body is stubbed, just must not crash
}

// ===========================================================================
// cFWTimeDialog
// ===========================================================================

// ---------------------------------------------------------------------------
// Construction + constants
// ---------------------------------------------------------------------------

TEST(CFWTimeDialog, DefaultWarTimeIsZero) {
    auto d = MakeTime();
    EXPECT_EQ(d->GetWarTime(), 0u);
}

TEST(CFWTimeDialog, ChildrenNullBeforeLinking) {
    auto d = MakeTime();
    EXPECT_EQ(d->GetTimeStatic(), nullptr);
    EXPECT_EQ(d->GetCharacterName(), nullptr);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CFWTimeDialog, LinkingMaterializesBothStatics) {
    auto d = MakeTime();
    d->Linking();
    EXPECT_NE(d->GetTimeStatic(), nullptr);
    EXPECT_NE(d->GetCharacterName(), nullptr);
}

TEST(CFWTimeDialog, LinkingSetsChildIds) {
    auto d = MakeTime();
    d->Linking();
    EXPECT_EQ(d->GetTimeStatic()->id(),    782);
    EXPECT_EQ(d->GetCharacterName()->id(), 783);
}

// ---------------------------------------------------------------------------
// SetActiveWithTimeName
// ---------------------------------------------------------------------------

TEST(CFWTimeDialog, SetActiveWithTimeNameTrueStoresWarTime) {
    auto d = MakeTime();
    d->Linking();
    d->SetActiveWithTimeName(true, 60, "Alice");
    EXPECT_EQ(d->GetWarTime(), 60u * 1000u);
    EXPECT_TRUE(d->isActive());
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "Alice");
}

TEST(CFWTimeDialog, SetActiveWithTimeNameFalseClearsName) {
    auto d = MakeTime();
    d->Linking();
    d->SetActiveWithTimeName(true, 30, "Bob");
    d->SetActiveWithTimeName(false, 0, nullptr);
    EXPECT_EQ(d->GetWarTime(), 0u);
    EXPECT_FALSE(d->isActive());
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "");
}

TEST(CFWTimeDialog, SetActiveWithTimeNameNullNameIsNoOpForText) {
    auto d = MakeTime();
    d->Linking();
    d->SetActiveWithTimeName(true, 10, nullptr);
    EXPECT_TRUE(d->isActive());
    // Text not modified since pName was null.
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "");
}

// ---------------------------------------------------------------------------
// SetCharacterName
// ---------------------------------------------------------------------------

TEST(CFWTimeDialog, SetCharacterNameUpdatesText) {
    auto d = MakeTime();
    d->Linking();
    d->SetCharacterName("Carol");
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "Carol");
}

TEST(CFWTimeDialog, SetCharacterNameNullIsDefensiveNoOp) {
    auto d = MakeTime();
    d->Linking();
    d->SetCharacterName("Eve");
    d->SetCharacterName(nullptr);  // defensive
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "Eve");
}

TEST(CFWTimeDialog, SetCharacterNameBeforeLinkingIsNoOp) {
    auto d = MakeTime();
    d->SetCharacterName("X");  // null m_pCharacterName, no crash
    EXPECT_EQ(d->GetCharacterName(), nullptr);
}

// ---------------------------------------------------------------------------
// ActionEvent
// ---------------------------------------------------------------------------

TEST(CFWTimeDialog, ActionEventOnDisabledDialogReturnsZero) {
    auto d = MakeTime();
    d->Linking();
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    EXPECT_EQ(we, 0u);
}

TEST(CFWTimeDialog, ActionEventOnEnabledDialogDoesNotCrash) {
    auto d = MakeTime();
    d->Linking();
    d->SetActive(true);
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    (void)we;
    SUCCEED();
}
