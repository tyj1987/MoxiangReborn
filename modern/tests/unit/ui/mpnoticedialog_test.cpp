// mpnoticedialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cMPNoticeDialog (MP notice dialog:
// 2 text areas showing caution + red caution messages).
//
// Covers modern/src/ui/mpnoticedialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\MPNoticeDialog.h (695 B) and
//   墨香【源码】\[Client]MH\MPNoticeDialog.cpp.
//
// What's tested:
//   - Default construction: 2 cTextArea pointers are null.
//   - Linking resolves the 2 cTextArea children
//     (kNCautionId=260, kNRedCautionId=261) by id.
//   - Linking calls SetScriptText on each (with placeholder
//     text "MP_NCAUTION" + "MP_NREDCAUTION" until CHATMGR
//     is ported).
//   - Linking without children leaves pointers null and is
//     safe.
//   - Accessors return the linked cTextArea pointers.
//
// 1:1 quirks preserved:
//   - Ctor drops m_type = WT_MPNOTICEDIALOG (legacy
//     cWindow type tag removed in Phase 6).
//   - Linking calls SetScriptText with the localized
//     chat msg. Modern port uses placeholder text
//     ("MP_NCAUTION" / "MP_NREDCAUTION") until CHATMGR
//     is ported. The SetScriptText signature is
//     exercised end-to-end either way.

#include "mpnoticedialog.hpp"
#include "ctextarea.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CMPNoticeDialogTest, DefaultConstructionHasNullPointers) {
    cMPNoticeDialog dlg;
    EXPECT_EQ(dlg.GetNCaution(),    nullptr);
    EXPECT_EQ(dlg.GetNRedCaution(), nullptr);
}

// ===========================================================================
// Id constants
// ===========================================================================

TEST(CMPNoticeDialogTest, IdConstantsAreDistinct) {
    EXPECT_NE(cMPNoticeDialog::kNCautionId,
              cMPNoticeDialog::kNRedCautionId);
}

TEST(CMPNoticeDialogTest, IdConstantsMatchExpectedLocalRange) {
    // 1:1 quirk: pick 260-261 to avoid collisions with
    // other Tier 2 dialog id ranges (cCharMakeDlg 200-203,
    // cGuildJoinDialog 210-212, cCharStateDialog 220-224,
    // cSOSDialog 230-231, cMiniFriendDialog 240-243,
    // cReviveDialog 250-252).
    EXPECT_EQ(cMPNoticeDialog::kNCautionId,    260);
    EXPECT_EQ(cMPNoticeDialog::kNRedCautionId, 261);
}

TEST(CMPNoticeDialogTest, ChatMsgIdsMatchLegacy) {
    // 1:1 with legacy CHATMGR->GetChatMsg(667) + (668).
    // Modern port keeps these constants for the
    // future CHATMGR port.
    EXPECT_EQ(cMPNoticeDialog::kNCautionChatMsgId,    667);
    EXPECT_EQ(cMPNoticeDialog::kNRedCautionChatMsgId, 668);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

// Build a cMPNoticeDialog with 2 cTextArea children wired
// in the modern id range (260-261). Returns raw pointers
// to the 2 text areas via the out vector; ownership
// lives in the dlg (children are added via cWindow::Add).
void BuildDlgWithTextAreas(cMPNoticeDialog& dlg,
                           std::vector<cTextArea*>& raws) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    raws.assign(2, nullptr);
    for (int i = 0; i < 2; ++i) {
        auto ta = std::make_unique<cTextArea>();
        // cDialog::Init signature: 6 params
        // (x, y, wid, hei, basicImage, id=0). Pass
        // id=260+i to match kNCautionId / kNRedCautionId.
        ta->Init(0, 0, 200, 100, nullptr, 260 + i);
        ta->InitTextArea(/*textRelRect=*/{0, 0, 200, 100},
                         /*bufSize=*/256);
        raws[i] = ta.get();
        dlg.Add(std::unique_ptr<cWindow>(ta.release()));
    }
    dlg.Linking();
}

}  // namespace

TEST(CMPNoticeDialogTest, LinkingResolvesBothTextAreas) {
    cMPNoticeDialog dlg;
    std::vector<cTextArea*> raws;
    BuildDlgWithTextAreas(dlg, raws);

    EXPECT_EQ(dlg.GetNCaution(),    raws[0]);
    EXPECT_EQ(dlg.GetNRedCaution(), raws[1]);
}

TEST(CMPNoticeDialogTest, LinkingCallsSetScriptText) {
    // 1:1 quirk: legacy calls
    // m_pNCaution->SetScriptText(CHATMGR->GetChatMsg(667))
    // m_pNRedCaution->SetScriptText(CHATMGR->GetChatMsg(668))
    // Modern port uses placeholder text "MP_NCAUTION" +
    // "MP_NREDCAUTION" until CHATMGR is ported.
    cMPNoticeDialog dlg;
    std::vector<cTextArea*> raws;
    BuildDlgWithTextAreas(dlg, raws);
    ASSERT_NE(dlg.GetNCaution(),    nullptr);
    ASSERT_NE(dlg.GetNRedCaution(), nullptr);

    EXPECT_EQ(dlg.GetNCaution()->GetScriptText(),    "MP_NCAUTION");
    EXPECT_EQ(dlg.GetNRedCaution()->GetScriptText(), "MP_NREDCAUTION");
}

TEST(CMPNoticeDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cMPNoticeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetNCaution(),    nullptr);
    EXPECT_EQ(dlg.GetNRedCaution(), nullptr);
}

TEST(CMPNoticeDialogTest, LinkingBeforeInitDoesNotCrash) {
    // Defensive: Linking before Init is a safe no-op.
    cMPNoticeDialog dlg;
    dlg.Linking();
    EXPECT_EQ(dlg.GetNCaution(),    nullptr);
    EXPECT_EQ(dlg.GetNRedCaution(), nullptr);
}

}  // namespace mxh::ui::test
