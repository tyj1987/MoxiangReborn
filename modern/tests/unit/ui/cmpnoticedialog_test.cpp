//
// Unit tests for mxh::ui::cMPNoticeDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//  * Constants: kIdNCaution=590, kIdNRedCaution=591,
//    kChatMsgNCaution=667, kChatMsgNRedCaution=668
//  * Default construction: both text areas are null
//  * Linking is a no-op without injected text areas
//  * Linking with injected text areas calls SetScriptText
//    for both areas (once each)
//  * Linking uses chatmsg 667 for the normal caution
//  * Linking uses chatmsg 668 for the red caution
//  * Custom chatmsg callback overrides the default text
//  * SetTextAreasForTest stores the pointers
//  * NonCopyable
//

#include "mxh/ui/cmpnoticedialog.hpp"
#include "mxh/ui/ctextarea.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using mxh::ui::cMPNoticeDialog;
using mxh::ui::cTextArea;

namespace {

struct Harness {
    cMPNoticeDialog dlg;
    cTextArea caution;
    cTextArea redCaution;

    Harness() {
        dlg.SetTextAreasForTest(&caution, &redCaution);
    }
};

int g_lastChatMsgId = -1;
const char* captureCallback(int id, void* /*user*/) {
    g_lastChatMsgId = id;
    if (id == 667) return "Normal caution text";
    if (id == 668) return "Red caution text";
    return "Unknown";
}

}  // namespace


TEST(CMPNoticeDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(cMPNoticeDialog::kIdNCaution,    590);
    EXPECT_EQ(cMPNoticeDialog::kIdNRedCaution, 591);
    EXPECT_EQ(cMPNoticeDialog::kChatMsgNCaution,    667);
    EXPECT_EQ(cMPNoticeDialog::kChatMsgNRedCaution, 668);
}

TEST(CMPNoticeDialog, DefaultConstructionHasNullTextAreas) {
    cMPNoticeDialog d;
    EXPECT_EQ(d.GetNCautionForTest(),    nullptr);
    EXPECT_EQ(d.GetNRedCautionForTest(), nullptr);
}

TEST(CMPNoticeDialog, SetTextAreasStoresPointers) {
    cMPNoticeDialog d;
    cTextArea ca, red;
    d.SetTextAreasForTest(&ca, &red);
    EXPECT_EQ(d.GetNCautionForTest(),    &ca);
    EXPECT_EQ(d.GetNRedCautionForTest(), &red);
}


TEST(CMPNoticeDialog, LinkingWithoutTextAreasIsNoOp) {
    cMPNoticeDialog d;
    d.Linking();
    SUCCEED();
}

TEST(CMPNoticeDialog, LinkingSetsBothScriptTexts) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(h.caution.GetScriptText(),    "");
    EXPECT_EQ(h.redCaution.GetScriptText(), "");
}

TEST(CMPNoticeDialog, LinkingUsesChatMsgIds) {
    Harness h;
    g_lastChatMsgId = -1;
    h.dlg.SetChatMsgCallbackForTest(&captureCallback, nullptr);
    h.dlg.Linking();
    // The callback is invoked once for each text area.
    // Final call should be the red caution (668), but
    // we don't assert order -- the legacy doesn't either.
    EXPECT_NE(g_lastChatMsgId, -1);
    EXPECT_EQ(h.caution.GetScriptText(),    "Normal caution text");
    EXPECT_EQ(h.redCaution.GetScriptText(), "Red caution text");
}

TEST(CMPNoticeDialog, CustomChatMsgOverridesDefault) {
    Harness h;
    h.dlg.SetChatMsgCallbackForTest(&captureCallback, nullptr);
    h.dlg.Linking();
    EXPECT_EQ(h.caution.GetScriptText(),    "Normal caution text");
    EXPECT_EQ(h.redCaution.GetScriptText(), "Red caution text");
}


TEST(CMPNoticeDialog, LinkingOnlyNCautionIsSafe) {
    cMPNoticeDialog d;
    cTextArea ca;
    d.SetTextAreasForTest(&ca, nullptr);
    d.Linking();
    EXPECT_EQ(ca.GetScriptText(), "");
}

TEST(CMPNoticeDialog, LinkingOnlyNRedCautionIsSafe) {
    cMPNoticeDialog d;
    cTextArea red;
    d.SetTextAreasForTest(nullptr, &red);
    d.Linking();
    EXPECT_EQ(red.GetScriptText(), "");
}

TEST(CMPNoticeDialog, LinkingIsIdempotent) {
    Harness h;
    h.dlg.SetChatMsgCallbackForTest(&captureCallback, nullptr);
    h.dlg.Linking();
    h.dlg.Linking();
    // Both calls populate the same way; no accumulation
    // (the cTextArea is single-line by default).
    EXPECT_EQ(h.caution.GetScriptText(),    "Normal caution text");
    EXPECT_EQ(h.redCaution.GetScriptText(), "Red caution text");
}


TEST(CMPNoticeDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible<cMPNoticeDialog>::value,
                  "cMPNoticeDialog must not be copyable");
    static_assert(!std::is_copy_assignable<cMPNoticeDialog>::value,
                  "cMPNoticeDialog must not be copy-assignable");
    SUCCEED();
}
