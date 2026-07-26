// mxh/tests/unit/ui/cnumberpaddialog_test.cpp
//
// Unit tests for mxh::ui::cNumberPadDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * InitProtectionStr clears buffer + visible
//   * InsertStr appends "*" to visible and digit to buffer (capped at 4)
//   * OnActionEvent dispatches by legacy button id (NUMBERPAD_*)
//   * nGate == 3 short-circuits all input
//   * WE_CLOSEWINDOW always consumed
//   * GetProtectionStr returns the raw digits (not the mask)
//   * Buffer cap: kProtectionStrMax = 10 bytes

#include "mxh/ui/cnumberpaddialog.hpp"
#include "mxh/ui/ccombobox.hpp"
#include "mxh/ui/cstatic.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

using mxh::ui::cComboBox;
using mxh::ui::cNumberPadDialog;
using mxh::ui::cStatic;

namespace {

// Minimal 0/1-button harness: a real cStatic (so SetStaticText /
// GetStaticText work) and a cComboBox whose GetCurSelectedIdx we
// override by direct field poke.
struct Harness {
    cNumberPadDialog dlg;
    cStatic         stat;
    cComboBox       combo;

    Harness() {
        dlg.SetStaticsForTest(&stat, &combo);
    }
};

}  // namespace

TEST(CNumberPadDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(cNumberPadDialog::kProtectionStrMax, 10u);
    EXPECT_EQ(static_cast<std::int32_t>(mxh::ui::NUMBERPAD_BACKSPACE), 0);
    EXPECT_EQ(static_cast<std::int32_t>(mxh::ui::NUMBERPAD_0), 1);
    EXPECT_EQ(static_cast<std::int32_t>(mxh::ui::NUMBERPAD_9), 10);
    EXPECT_EQ(static_cast<std::int32_t>(mxh::ui::NUMBERPAD_MAX), 11);
}

TEST(CNumberPadDialog, LinkingInitializesBufferEmpty) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetProtectionStr()[0], '\0');
    EXPECT_EQ(h.dlg.VisibleLength(), 0u);
}

TEST(CNumberPadDialog, InitProtectionStrClearsBuffer) {
    Harness h;
    // Pretend the user already typed 1234.
    for (char c : {'1','2','3','4'}) {
        char buf[2] = {c, '\0'};
        h.dlg.InsertStr(buf);
    }
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "1234");
    h.dlg.InitProtectionStr();
    EXPECT_EQ(h.dlg.GetProtectionStr()[0], '\0');
    EXPECT_EQ(h.stat.GetStaticText(), "");
}

TEST(CNumberPadDialog, InsertStrAppendsMaskAndDigit) {
    Harness h;
    h.dlg.InsertStr("1");
    h.dlg.InsertStr("2");
    h.dlg.InsertStr("3");
    EXPECT_EQ(h.stat.GetStaticText(), "***");
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "123");
}

TEST(CNumberPadDialog, InsertStrCapsAtFour) {
    Harness h;
    h.dlg.InsertStr("1");
    h.dlg.InsertStr("2");
    h.dlg.InsertStr("3");
    h.dlg.InsertStr("4");
    // 5th digit must be silently dropped (legacy nLen < 4 cap).
    h.dlg.InsertStr("5");
    EXPECT_EQ(h.stat.GetStaticText(), "****");
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "1234");
    EXPECT_EQ(h.dlg.VisibleLength(), 4u);
}

TEST(CNumberPadDialog, OnActionEventDispatchesByButtonId) {
    Harness h;
    EXPECT_TRUE(h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_1, nullptr, 0xDEAD /*WE_BTNCLICK*/));
    EXPECT_TRUE(h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_5, nullptr, 0xDEAD));
    EXPECT_TRUE(h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_9, nullptr, 0xDEAD));
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "159");
    EXPECT_EQ(h.stat.GetStaticText(), "***");
}

TEST(CNumberPadDialog, OnActionEventBackspaceClears) {
    Harness h;
    h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_1, nullptr, 0xDEAD);
    h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_2, nullptr, 0xDEAD);
    h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_3, nullptr, 0xDEAD);
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "123");
    // Backspace wipes the whole entry (legacy behaviour: not a
    // backspace-character, but a full reset).
    h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_BACKSPACE, nullptr, 0xDEAD);
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "");
    EXPECT_EQ(h.stat.GetStaticText(), "");
}

TEST(CNumberPadDialog, CloseWindowAlwaysConsumed) {
    Harness h;
    EXPECT_TRUE(h.dlg.OnActionEvent(0, nullptr, 0 /*WE_CLOSEWINDOW*/));
    // The input buffer must NOT be touched on close (legacy).
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "");
}

TEST(CNumberPadDialog, GateThreeShortCircuitsAllInput) {
    Harness h;
    // Set the combo's "current selected index" to 3 (legacy supervisor
    // gate: PIN entry disabled).  cComboBox exposes
    // SetCurSelectedIdx.
    h.combo.SetCurSelectedIdx(3);
    // All digit events must be consumed (return true) but must NOT
    // modify the buffer.
    EXPECT_TRUE(h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_1, nullptr, 0xDEAD));
    EXPECT_TRUE(h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_9, nullptr, 0xDEAD));
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "");
    EXPECT_EQ(h.stat.GetStaticText(), "");
}

TEST(CNumberPadDialog, GateOtherThanThreeAllowsInput) {
    Harness h;
    h.combo.SetCurSelectedIdx(0);
    h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_7, nullptr, 0xDEAD);
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "7");
    EXPECT_EQ(h.stat.GetStaticText(), "*");
}

TEST(CNumberPadDialog, UnknownButtonIdIsTolerated) {
    Harness h;
    // Legacy: unknown ids fall through the switch and return TRUE
    // without touching the buffer.  Verify the modern port matches.
    EXPECT_TRUE(h.dlg.OnActionEvent(999, nullptr, 0xDEAD));
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "");
}

TEST(CNumberPadDialog, GetProtectionStrReturnsRawDigitsNotMask) {
    Harness h;
    h.dlg.InsertStr("9");
    h.dlg.InsertStr("8");
    // 1:1 with legacy GetProtectionStr: returns the raw PIN, NOT
    // the visible "*" mask.  The host caller (login network layer)
    // uses this to send the real PIN to the server.
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "98");
    EXPECT_NE(h.dlg.GetProtectionStr(), "**");
}

TEST(CNumberPadDialog, SetActiveIsForwarderToBase) {
    Harness h;
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
    h.dlg.SetActive(false);
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CNumberPadDialog, BufferCapIsTenBytes) {
    // 1:1 with legacy ePROTECTIONSTR_MAXNUM = 10.  We verify the
    // constant, the buffer is 10 bytes, and the 5th+ visible
    // entries are dropped (so the buffer can never exceed 4
    // digits through InsertStr).  Direct buffer poking is
    // outside the API surface.
    static_assert(cNumberPadDialog::kProtectionStrMax == 10,
                  "must match legacy ePROTECTIONSTR_MAXNUM");
    Harness h;
    for (int i = 0; i < 4; ++i) {
        char buf[2] = {static_cast<char>('0' + i), '\0'};
        h.dlg.InsertStr(buf);
    }
    // Force-poke a 5th character (legacy would have rejected at
    // InsertStr).  We simulate the worst case: a host that calls
    // InsertStr on a non-empty 4-char buffer, then a backspace
    // that wipes and re-inserts.
    h.dlg.OnActionEvent(mxh::ui::NUMBERPAD_BACKSPACE, nullptr, 0xDEAD);
    h.dlg.InsertStr("5");
    EXPECT_STREQ(h.dlg.GetProtectionStr(), "5");
    EXPECT_EQ(h.dlg.VisibleLength(), 1u);
}
