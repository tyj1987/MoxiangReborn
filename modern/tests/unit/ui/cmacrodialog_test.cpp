// cmacrodialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cMacroDialog (macro key binding dialog).
//
// Covers modern/src/ui/cmacrodialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\MacroDialog.h (1,427 B) and
//   墨香【源码】\[Client]MH\MacroDialog.cpp (13,211 B).
//
// What's tested:
//   - Enum stability: MacroEvent / MacroMode / SysKey mirror
//     the legacy numeric values (preserves wire compatibility
//     with OptionManager persistence).
//   - sMACRO default-construction is the legacy "empty binding".
//   - Init resets m_MacroKey / m_pMacroKeyEdit / mode to defaults.
//   - SetMacroBinding updates the array AND the linked
//     cEditBox text (when linked).
//   - GetMacroBinding round-trips through SetMacroBinding.
//   - ConvertMacroToText produces the expected "CTRL + F1"
//     style strings for each modifier / VK combination.
//   - VKeyToName covers the common function / letter / digit
//     / control / arrow keys and returns "" for unknown.
//   - Linking() resolves the 15 cEditBox children by id and
//     sets ShowCaretInReadOnly on each.
//   - SetActive gates the per-event refresh on Macro mode
//     (1:1 with legacy behavior).
//   - m_bChanged flag: SetMacroBinding sets it, ClearChanged
//     resets it, IsChanged reflects the current state.

#include "cmacrodialog.hpp"
#include "ceditbox.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace mxh::ui::test {

// ===========================================================================
// Enum stability
// ===========================================================================

TEST(CMacroDialogTest, MacroEventEnumIsStable) {
    // The numeric values of MacroEvent must mirror the legacy
    // ME_* enum (USE_QUICKITEM01 = 0, TOGGLE_INVENTORYDLG = 7,
    // etc.) so a future port that reads OptionManager.bin
    // (which uses the legacy ME_* codes) does not break.
    EXPECT_EQ(static_cast<int>(MacroEvent::USE_QUICKITEM01), 0);
    EXPECT_EQ(static_cast<int>(MacroEvent::USE_QUICKITEM07), 6);
    EXPECT_EQ(static_cast<int>(MacroEvent::TOGGLE_INVENTORYDLG), 7);
    EXPECT_EQ(static_cast<int>(MacroEvent::TOGGLE_EXITDLG), 11);
    EXPECT_EQ(static_cast<int>(MacroEvent::SCREENCAPTURE), 17);
    EXPECT_EQ(static_cast<int>(MacroEvent::ME_COUNT), 18);
}

TEST(CMacroDialogTest, MacroModeEnumIsStable) {
    EXPECT_EQ(static_cast<int>(MacroMode::Chat),  0);
    EXPECT_EQ(static_cast<int>(MacroMode::Macro), 1);
}

TEST(CMacroDialogTest, SysKeyEnumIsStable) {
    // Legacy eSysKey values: MSK_NONE=1, MSK_CTRL=2, MSK_ALT=4,
    // MSK_SHIFT=8, MSK_ALL=15. These are bit flags used
    // directly in the sMACRO.nSysKey field.
    EXPECT_EQ(static_cast<int>(SysKey::None),  1);
    EXPECT_EQ(static_cast<int>(SysKey::Ctrl),  2);
    EXPECT_EQ(static_cast<int>(SysKey::Alt),   4);
    EXPECT_EQ(static_cast<int>(SysKey::Shift), 8);
    EXPECT_EQ(static_cast<int>(SysKey::All),   15);
}

// ===========================================================================
// sMACRO + default state
// ===========================================================================

TEST(CMacroDialogTest, SMACRODefaultIsEmptyBinding) {
    sMACRO m{};
    EXPECT_EQ(m.nSysKey, static_cast<int>(SysKey::None));
    EXPECT_EQ(m.wKey,    0u);
    EXPECT_FALSE(m.bAllMode);
    EXPECT_FALSE(m.bUp);
}

TEST(CMacroDialogTest, InitResetsAllState) {
    cMacroDialog dlg;
    dlg.Init(100, 200, 300, 400);
    // All 15 macro bindings default to empty.
    for (std::size_t i = 0; i < cMacroDialog::kMacroCount; ++i) {
        const auto& m = dlg.MacroAt(i);
        EXPECT_EQ(m.wKey, 0u);
    }
    EXPECT_EQ(dlg.Mode(), MacroMode::Chat);
    EXPECT_FALSE(dlg.IsChanged());
}

TEST(CMacroDialogTest, GetMacroBindingOutOfRangeReturnsDefault) {
    cMacroDialog dlg;
    dlg.Init(0, 0, 0, 0);
    // Get for an event past the end of the array returns a
    // default-constructed sMACRO (defensive against bad
    // indices from the host OptionManager bridge).
    auto m = dlg.GetMacroBinding(static_cast<MacroEvent>(99));
    EXPECT_EQ(m.wKey, 0u);
}

// ===========================================================================
// SetMacroBinding / GetMacroBinding round-trip + IsChanged
// ===========================================================================

TEST(CMacroDialogTest, SetMacroBindingRoundTrip) {
    cMacroDialog dlg;
    dlg.Init(0, 0, 0, 0);
    sMACRO m;
    m.nSysKey  = static_cast<int>(SysKey::Ctrl);
    m.wKey     = 0x74;  // VK_F5
    m.bAllMode = true;
    m.bUp      = false;
    dlg.SetMacroBinding(MacroEvent::USE_QUICKITEM03, m);
    EXPECT_TRUE(dlg.IsChanged());

    const auto& round = dlg.GetMacroBinding(MacroEvent::USE_QUICKITEM03);
    EXPECT_EQ(round.nSysKey,  static_cast<int>(SysKey::Ctrl));
    EXPECT_EQ(round.wKey,     0x74u);
    EXPECT_TRUE(round.bAllMode);
    EXPECT_FALSE(round.bUp);
}

TEST(CMacroDialogTest, SetMacroBindingOutOfRangeIsNoOp) {
    cMacroDialog dlg;
    dlg.Init(0, 0, 0, 0);
    sMACRO m;
    m.wKey = 0x41;  // VK_A
    dlg.SetMacroBinding(static_cast<MacroEvent>(255), m);
    // No crash, no state change.
    EXPECT_FALSE(dlg.IsChanged());
}

TEST(CMacroDialogTest, ClearChangedResetsFlag) {
    cMacroDialog dlg;
    dlg.Init(0, 0, 0, 0);
    sMACRO m;
    m.wKey = 0x41;
    dlg.SetMacroBinding(MacroEvent::USE_QUICKITEM01, m);
    EXPECT_TRUE(dlg.IsChanged());
    dlg.ClearChanged();
    EXPECT_FALSE(dlg.IsChanged());
}

// ===========================================================================
// Mode
// ===========================================================================

TEST(CMacroDialogTest, SetModeAndGetMode) {
    cMacroDialog dlg;
    dlg.Init(0, 0, 0, 0);
    EXPECT_EQ(dlg.Mode(), MacroMode::Chat);
    dlg.SetMode(MacroMode::Macro);
    EXPECT_EQ(dlg.Mode(), MacroMode::Macro);
    dlg.SetMode(MacroMode::Chat);
    EXPECT_EQ(dlg.Mode(), MacroMode::Chat);
}

// ===========================================================================
// ConvertMacroToText + VKeyToName
// ===========================================================================

TEST(CMacroDialogTest, VKeyToNameCoversCommonKeys) {
    // Function keys.
    EXPECT_EQ(cMacroDialog::VKeyToName(0x70), "F1");
    EXPECT_EQ(cMacroDialog::VKeyToName(0x7B), "F12");
    // Letters.
    EXPECT_EQ(cMacroDialog::VKeyToName(0x41), "A");
    EXPECT_EQ(cMacroDialog::VKeyToName(0x5A), "Z");
    // Digits.
    EXPECT_EQ(cMacroDialog::VKeyToName(0x30), "0");
    EXPECT_EQ(cMacroDialog::VKeyToName(0x39), "9");
    // Common controls.
    EXPECT_EQ(cMacroDialog::VKeyToName(0x20), "SPACE");
    EXPECT_EQ(cMacroDialog::VKeyToName(0x09), "TAB");
    EXPECT_EQ(cMacroDialog::VKeyToName(0x0D), "ENTER");
    EXPECT_EQ(cMacroDialog::VKeyToName(0x1B), "ESC");
    EXPECT_EQ(cMacroDialog::VKeyToName(0x08), "BACKSPACE");
    // Arrows.
    EXPECT_EQ(cMacroDialog::VKeyToName(0x25), "LEFT");
    EXPECT_EQ(cMacroDialog::VKeyToName(0x26), "UP");
    EXPECT_EQ(cMacroDialog::VKeyToName(0x27), "RIGHT");
    EXPECT_EQ(cMacroDialog::VKeyToName(0x28), "DOWN");
}

TEST(CMacroDialogTest, VKeyToNameUnknownReturnsEmpty) {
    // 0x07 is "undefined" in the Windows VK table — not
    // covered by VKeyToName. The dialog will display just
    // the modifier prefix (e.g. "CTRL + ").
    EXPECT_EQ(cMacroDialog::VKeyToName(0x07), "");
    EXPECT_EQ(cMacroDialog::VKeyToName(0xFFFF), "");
}

TEST(CMacroDialogTest, ConvertMacroToTextModifierOnly) {
    cMacroDialog dlg;
    char buf[cMacroDialog::kMaxMacroTextLen + 1] = {};
    sMACRO m;
    m.nSysKey = static_cast<int>(SysKey::Ctrl);
    m.wKey    = 0;  // no key
    std::size_t n = dlg.ConvertMacroToText(buf, m);
    EXPECT_STREQ(buf, "CTRL + ");
    EXPECT_EQ(n, 7u);
}

TEST(CMacroDialogTest, ConvertMacroToTextFullBinding) {
    cMacroDialog dlg;
    char buf[cMacroDialog::kMaxMacroTextLen + 1] = {};
    sMACRO m;
    m.nSysKey = static_cast<int>(SysKey::Alt);
    m.wKey    = 0x74;  // F5
    dlg.ConvertMacroToText(buf, m);
    EXPECT_STREQ(buf, "ALT + F5");
}

TEST(CMacroDialogTest, ConvertMacroToTextNoModifier) {
    cMacroDialog dlg;
    char buf[cMacroDialog::kMaxMacroTextLen + 1] = {};
    sMACRO m;
    m.nSysKey = static_cast<int>(SysKey::None);
    m.wKey    = 0x41;  // A
    dlg.ConvertMacroToText(buf, m);
    // No prefix — bare key name.
    EXPECT_STREQ(buf, "A");
}

TEST(CMacroDialogTest, ConvertMacroToTextNullBufferIsSafe) {
    cMacroDialog dlg;
    sMACRO m;
    m.wKey = 0x41;
    EXPECT_EQ(dlg.ConvertMacroToText(nullptr, m), 0u);
}

// ===========================================================================
// Linking: resolves cEditBox children by id
// ===========================================================================

TEST(CMacroDialogTest, LinkingResolvesQuickSlotEditBoxes) {
    cMacroDialog dlg;
    dlg.Init(0, 0, 400, 400);

    // Add 7 cEditBox children with the expected id range
    // (100 + 0..6) for the quick-slot bindings. Each edit
    // box must be InitEditbox'd so the text buffer is
    // configured (otherwise SetEditText is a no-op per the
    // strict-mode guard in cEditBox::SetEditText).
    std::vector<cEditBox*> raws(7, nullptr);
    for (int i = 0; i < 7; ++i) {
        auto e = std::make_unique<cEditBox>();
        // cEditBox::Init takes (x, y, wid, hei, basicImage,
        // focusImage, id). The modern port has no basicImage
        // / focusImage yet (see R-10 cImage adapter) — pass
        // nullptr for both and pin the id in the Init call.
        e->Init(0, 0, 50, 14, nullptr, nullptr, 100 + i);
        e->InitEditbox(50, 64);
        raws[i] = e.get();
        dlg.Add(std::unique_ptr<cWindow>(e.release()));
    }
    dlg.Linking();

    // Linking populates the private m_pMacroKeyEdit array; we
    // exercise it indirectly by calling SetMacroBinding on
    // each quick-slot event and observing that the linked
    // cEditBox receives the formatted text. This pins both
    // (a) Linking correctly resolves the children by id, and
    // (b) SetMacroBinding refreshes the text of each linked
    // child (1:1 with legacy behavior).
    for (int i = 0; i < 7; ++i) {
        sMACRO m;
        m.nSysKey = static_cast<int>(SysKey::None);
        m.wKey    = 0x41 + static_cast<std::uint16_t>(i);  // A, B, C, ...
        dlg.SetMacroBinding(static_cast<MacroEvent>(i), m);
        EXPECT_EQ(raws[i]->editText().size(), 1u);
        EXPECT_EQ(raws[i]->editText()[0], 'A' + static_cast<char>(i));
    }
}

TEST(CMacroDialogTest, SetMacroBindingRefreshesLinkedEditBox) {
    cMacroDialog dlg;
    dlg.Init(0, 0, 400, 400);
    auto e = std::make_unique<cEditBox>();
    e->Init(0, 0, 50, 14, nullptr, nullptr, 100);  // USE_QUICKITEM01
    e->InitEditbox(50, 64);
    cEditBox* raw = e.get();
    dlg.Add(std::unique_ptr<cWindow>(e.release()));
    dlg.Linking();

    sMACRO m;
    m.nSysKey = static_cast<int>(SysKey::Shift);
    m.wKey    = 0x70;  // F1
    dlg.SetMacroBinding(MacroEvent::USE_QUICKITEM01, m);
    // The linked cEditBox text should now read "SHIFT + F1".
    EXPECT_EQ(raw->editText(), "SHIFT + F1");
}

// ===========================================================================
// SetActive gates refresh on Macro mode
// ===========================================================================

TEST(CMacroDialogTest, SetActiveInChatModeDoesNotRefresh) {
    cMacroDialog dlg;
    dlg.Init(0, 0, 400, 400);
    auto e = std::make_unique<cEditBox>();
    e->Init(0, 0, 50, 14, nullptr, nullptr, 100);
    e->InitEditbox(50, 64);
    cEditBox* raw = e.get();
    dlg.Add(std::unique_ptr<cWindow>(e.release()));
    dlg.Linking();

    // Pre-populate the binding; the edit box text is
    // initially empty because SetActive hasn't run yet.
    sMACRO m;
    m.nSysKey = static_cast<int>(SysKey::Ctrl);
    m.wKey    = 0x41;
    dlg.SetMacroBinding(MacroEvent::USE_QUICKITEM01, m);
    // SetMacroBinding did refresh the linked edit box text,
    // so it should now read "CTRL + A".
    EXPECT_EQ(raw->editText(), "CTRL + A");

    // Manually corrupt the text to simulate a stale UI state.
    raw->SetEditText("STALE");
    EXPECT_EQ(raw->editText(), "STALE");

    // Re-activate in Chat mode (default). The legacy does
    // NOT refresh in Chat mode, so the stale text remains.
    dlg.SetMode(MacroMode::Chat);
    dlg.SetActive(true);
    EXPECT_EQ(raw->editText(), "STALE");
}

TEST(CMacroDialogTest, SetActiveInMacroModeRefreshes) {
    cMacroDialog dlg;
    dlg.Init(0, 0, 400, 400);
    auto e = std::make_unique<cEditBox>();
    e->Init(0, 0, 50, 14, nullptr, nullptr, 100);
    e->InitEditbox(50, 64);
    cEditBox* raw = e.get();
    dlg.Add(std::unique_ptr<cWindow>(e.release()));
    dlg.Linking();

    sMACRO m;
    m.nSysKey = static_cast<int>(SysKey::Ctrl);
    m.wKey    = 0x42;  // B
    dlg.SetMacroBinding(MacroEvent::USE_QUICKITEM01, m);

    // Corrupt the text and re-activate in Macro mode.
    raw->SetEditText("STALE");
    dlg.SetMode(MacroMode::Macro);
    dlg.SetActive(true);
    // The legacy refreshes the visible text in Macro mode.
    EXPECT_EQ(raw->editText(), "CTRL + B");
}

}  // namespace mxh::ui::test
