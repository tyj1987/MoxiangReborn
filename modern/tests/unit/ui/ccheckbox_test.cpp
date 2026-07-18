// ccheckbox_test.cpp — 1:1 port verification tests for cCheckBox.

#include "ccheckbox.hpp"
#include "cwindow.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

using mxh::ui::cCheckBox;
using mxh::ui::cWindow;
using mxh::ui::cDialog;
using mxh::ui::CMouse;
using mxh::ui::CheckboxCallback;
using mxh::ui::kWeChecked;
using mxh::ui::kWeNotChecked;

namespace {

// Test fixture: a check box with no callback, ready for
// direct SetChecked / ToggleForTesting calls.
class CCheckBoxTest : public ::testing::Test {
protected:
    void SetUp() override {
        cCheckBox::ClearTestInjections();
        dlg_ = std::make_unique<cCheckBox>();
        dlg_->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, nullptr, 0);
    }
    void TearDown() override {
        cCheckBox::ClearTestInjections();
    }
    std::unique_ptr<cCheckBox> dlg_;
};

}  // namespace

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

TEST(CCheckBox, WeConstantsMatchLegacyBitField) {
    // 1:1 with legacy WE_CHECKED=128 / WE_NOTCHECKED=256
    // (per legacy cWindowDef.h enum WINDOW_EVENT).
    EXPECT_EQ(kWeChecked,    128u);
    EXPECT_EQ(kWeNotChecked, 256u);
}

TEST(CCheckBox, DefaultColorIsWhite) {
    // 1:1 with legacy `m_dwCheckBoxTextColor=RGB_HALF(255,255,255)`.
    cCheckBox dlg;
    EXPECT_EQ(dlg.checkBoxTextColor(), 0xFFFFFFFFu);
}

TEST(CCheckBox, DefaultCheckedIsFalse) {
    // 1:1 with legacy `m_fChecked = FALSE` in ctor.
    cCheckBox dlg;
    EXPECT_FALSE(dlg.IsChecked());
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

TEST_F(CCheckBoxTest, InitStoresPositionAndId) {
    auto d = std::make_unique<cCheckBox>();
    d->Init(50, 30, 25, 25, nullptr, nullptr, nullptr, nullptr, 999);
    EXPECT_EQ(d->absX(), 50);
    EXPECT_EQ(d->absY(), 30);
    EXPECT_EQ(d->id(), 999);
}

TEST_F(CCheckBoxTest, InitWithCallbackStoresIt) {
    bool fired = false;
    CheckboxCallback cb = [&fired](std::int32_t, void*, std::uint32_t) {
        fired = true;
    };
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, cb, 0);
    d->ToggleForTesting();
    EXPECT_TRUE(fired);
}

TEST_F(CCheckBoxTest, InitWithNullCallbackIsTolerated) {
    // 1:1 with legacy: `if (Func != NULL) cbWindowFunc = Func;`
    // — null callback is OK, dialog can still be created.
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, nullptr, 0);
    SUCCEED();
}

// ---------------------------------------------------------------------------
// SetChecked / IsChecked
// ---------------------------------------------------------------------------

TEST_F(CCheckBoxTest, SetCheckedTrueChangesState) {
    dlg_->SetChecked(true);
    EXPECT_TRUE(dlg_->IsChecked());
}

TEST_F(CCheckBoxTest, SetCheckedFalseChangesState) {
    dlg_->SetChecked(true);
    dlg_->SetChecked(false);
    EXPECT_FALSE(dlg_->IsChecked());
}

// ---------------------------------------------------------------------------
// SetCheckBoxMsg
// ---------------------------------------------------------------------------

TEST_F(CCheckBoxTest, SetCheckBoxMsgStoresTextAndColor) {
    // 1:1 with legacy: strcpy + m_dwCheckBoxTextColor = color.
    dlg_->SetCheckBoxMsg("Enable", 0xFF00FF00u);
    EXPECT_EQ(dlg_->checkBoxText(), "Enable");
    EXPECT_EQ(dlg_->checkBoxTextColor(), 0xFF00FF00u);
}

TEST_F(CCheckBoxTest, SetCheckBoxMsgNullTextClearsText) {
    dlg_->SetCheckBoxMsg("Hello", 0xFF808080u);
    EXPECT_EQ(dlg_->checkBoxText(), "Hello");
    dlg_->SetCheckBoxMsg(nullptr, 0xFF808080u);
    EXPECT_EQ(dlg_->checkBoxText(), "");
}

// ---------------------------------------------------------------------------
// ToggleForTesting
// ---------------------------------------------------------------------------

TEST_F(CCheckBoxTest, ToggleFlipsChecked) {
    // 1:1 with legacy: m_fChecked ^= TRUE (XOR).
    EXPECT_FALSE(dlg_->IsChecked());
    dlg_->ToggleForTesting();
    EXPECT_TRUE(dlg_->IsChecked());
    dlg_->ToggleForTesting();
    EXPECT_FALSE(dlg_->IsChecked());
}

TEST_F(CCheckBoxTest, ToggleFiresCallbackWithChecked) {
    bool fired = false;
    std::uint32_t lastWe = 0;
    CheckboxCallback cb = [&fired, &lastWe](std::int32_t, void*,
                                              std::uint32_t we) {
        fired = true;
        lastWe = we;
    };
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, cb, 100);
    d->ToggleForTesting();
    EXPECT_TRUE(fired);
    EXPECT_EQ(lastWe, kWeChecked);
    EXPECT_EQ(d->callbackFiredCount(), 1u);
    EXPECT_EQ(d->lastCallbackWe(), kWeChecked);
    EXPECT_EQ(d->lastCallbackId(), 100);
}

TEST_F(CCheckBoxTest, ToggleSecondTimeFiresCallbackWithNotChecked) {
    // 1:1 with legacy: cbWindowFunc fires with WE_NOTCHECKED
    // when the toggle goes from true to false.
    std::uint32_t lastWe = 0;
    CheckboxCallback cb = [&lastWe](std::int32_t, void*,
                                     std::uint32_t we) {
        lastWe = we;
    };
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, cb, 0);
    d->ToggleForTesting();
    EXPECT_EQ(lastWe, kWeChecked);
    d->ToggleForTesting();
    EXPECT_EQ(lastWe, kWeNotChecked);
}

TEST_F(CCheckBoxTest, ToggleWithoutCallbackIsNoOp) {
    // 1:1 with legacy: `if (Func != NULL) cbWindowFunc = Func;`
    // — null callback means no fire. Modern port: no-op.
    dlg_->ToggleForTesting();
    EXPECT_EQ(dlg_->callbackFiredCount(), 0u);
}

TEST_F(CCheckBoxTest, TogglePassesParentDialogToCallback) {
    // 1:1 with legacy: cbWindowFunc(m_ID, m_pParent, ...).
    // Modern port: parent is the test-injectable
    // m_parentDialog (cWindow*).
    auto parent = std::make_unique<cDialog>();
    parent->Init(0, 0, 100, 100, nullptr, 0);
    void* receivedParent = nullptr;
    CheckboxCallback cb = [&receivedParent](std::int32_t, void* p,
                                             std::uint32_t) {
        receivedParent = p;
    };
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, cb, 0);
    d->SetParentDialogForTesting(parent.get());
    d->ToggleForTesting();
    EXPECT_EQ(receivedParent, parent.get());
    EXPECT_EQ(d->lastCallbackParent(), parent.get());
}

TEST_F(CCheckBoxTest, ToggleIncrementsCallbackCount) {
    std::uint32_t callCount = 0;
    CheckboxCallback cb = [&callCount](std::int32_t, void*,
                                        std::uint32_t) {
        ++callCount;
    };
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, cb, 0);
    d->ToggleForTesting();
    d->ToggleForTesting();
    d->ToggleForTesting();
    EXPECT_EQ(d->callbackFiredCount(), 3u);
    EXPECT_EQ(callCount, 3u);
}

TEST_F(CCheckBoxTest, ToggleOnDisabledCheckBoxIsNoOp) {
    // 1:1 with legacy: `if (m_bDisable) return we;` — disabled
    // checkbox doesn't toggle. Modern port: same guard.
    bool fired = false;
    CheckboxCallback cb = [&fired](std::int32_t, void*, std::uint32_t) {
        fired = true;
    };
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, cb, 0);
    d->SetDisable(true);
    d->ToggleForTesting();
    EXPECT_FALSE(fired);
    EXPECT_FALSE(d->IsChecked());
}

TEST_F(CCheckBoxTest, ToggleOnDisabledCheckBoxDoesNotChangeState) {
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, nullptr, 0);
    d->SetDisable(true);
    d->SetChecked(false);
    d->ToggleForTesting();
    EXPECT_FALSE(d->IsChecked());  // No change.
}

// ---------------------------------------------------------------------------
// ActionEvent (no-op shell — modern CMouse is stubbed)
// ---------------------------------------------------------------------------

TEST_F(CCheckBoxTest, ActionEventOnDisabledReturnsZero) {
    // 1:1 with legacy: `if (m_bDisable) return we;` returns
    // WE_NULL=0. Modern port: `if (!isEnabled()) return 0`.
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, nullptr, 0);
    d->SetDisable(true);
    EXPECT_EQ(d->ActionEvent(nullptr), 0u);
}

TEST_F(CCheckBoxTest, ActionEventOnEnabledReturnsZero) {
    // 1:1 with legacy: ActionEvent returns we (which is 0
    // when no click is detected). Modern port: same.
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, nullptr, 0);
    EXPECT_EQ(d->ActionEvent(nullptr), 0u);
}

// ---------------------------------------------------------------------------
// Render (no-op stub)
// ---------------------------------------------------------------------------

TEST_F(CCheckBoxTest, RenderIsNoOp) {
    // 1:1 quirk: legacy Render draws checkBoxImage + checkImage
    // + text. Modern port: Render is a no-op stub (Phase 6.x
    // render wiring deferred).
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, nullptr, 0);
    d->SetChecked(true);
    d->SetCheckBoxMsg("Hello", 0xFF00FF00u);
    d->Render();
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Test-injection cleanup
// ---------------------------------------------------------------------------

TEST_F(CCheckBoxTest, ClearTestInjectionsResetsState) {
    cCheckBox::ClearTestInjections();
    auto d = std::make_unique<cCheckBox>();
    d->Init(0, 0, 20, 20, nullptr, nullptr, nullptr,
            [](std::int32_t, void*, std::uint32_t) {}, 0);
    d->ToggleForTesting();
    EXPECT_EQ(d->callbackFiredCount(), 1u);
    cCheckBox::ClearTestInjections();
    EXPECT_EQ(d->callbackFiredCount(), 0u);
}
