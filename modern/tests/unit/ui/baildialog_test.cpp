// baildialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cBailDialog (bail dialog: enter amount
// of bad fame to pay off + show the bail cost + minimum required).
//
// Covers modern/src/ui/baildialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\BailDialog.h (497 B) and
//   `墨香【源码】\[Client]MH\BailDialog.cpp`.
//
// What's tested:
//   - Default construction: 2 child pointers null, m_BadFame = 0.
//   - Linking resolves the cEditBox + cTextArea children
//     (kBailEditBoxId=320, kBailTextId=321) by id.
//   - Linking calls SetValidCheck + SetAlign on the
//     cEditBox.
//   - Linking calls SetScriptText on the cTextArea with
//     placeholder text "BAIL_TEXT_PLACEHOLDER".
//   - Open / Close / SetFame / SetBadFrameSync are
//     no-op stubs (4-singleton dispatches TODO).
//   - Accessors return the linked child pointers + m_BadFame.
//   - Defensive null-checks: Linking + Open + Close +
//     SetFame + SetBadFrameSync without link are safe.
//
// 1:1 quirks preserved:
//   - Ctor initializes 2 child pointers to null + m_BadFame
//     to 0 (modern port uses default member init).
//   - Linking calls SetValidCheck(1) (VCM_NUMBER = digits
//     only) + SetAlign(TextAlign::Right) on the cEditBox.
//   - Linking's SetScriptText call uses placeholder
//     text "BAIL_TEXT_PLACEHOLDER" (legacy uses
//     CHATMGR->GetChatMsg(644) + AddComma-formatted
//     bail cost).
//   - Open / Close / SetFame / SetBadFrameSync are
//     documented as TODO (4-singleton dispatches
//     deferred).

#include "baildialog.hpp"
#include "ceditbox.hpp"
#include "ctextarea.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CBailDialogTest, DefaultConstructionIsValid) {
    cBailDialog dlg;
    // Default: 2 child pointers null, m_BadFame = 0.
    EXPECT_EQ(dlg.GetBailEditBox(), nullptr);
    EXPECT_EQ(dlg.GetBailText(),    nullptr);
    EXPECT_EQ(dlg.GetBadFame(),     0u);
}

// ===========================================================================
// Id constants
// ===========================================================================

TEST(CBailDialogTest, IdConstantsMatchExpectedLocalRange) {
    // 1:1 quirk: pick 320-321 to avoid collisions with
    // other Tier 2 dialog id ranges (cCharMakeDlg 200-203,
    // cGuildJoinDialog 210-212, cCharStateDialog 220-224,
    // cSOSDialog 230-231, cMiniFriendDialog 240-243,
    // cReviveDialog 250-252, cMPNoticeDialog 260-261,
    // cEventNotifyDialog 270-271, cGuildCreateDialog
    // 280-284, cGuildUnionCreateDialog 290-292,
    // cChaseInputDialog 300, cChaseDialog 310-311).
    EXPECT_EQ(cBailDialog::kBailEditBoxId, 320);
    EXPECT_EQ(cBailDialog::kBailTextId,    321);
    EXPECT_NE(cBailDialog::kBailEditBoxId, cBailDialog::kBailTextId);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

// Build a cBailDialog with 2 children wired in the
// modern id range (320-321). Returns the raw pointers
// via the out struct; ownership lives in the dlg
// (children are added via cWindow::Add).
struct BailChildren {
    cEditBox* edit = nullptr;
    cTextArea* text = nullptr;
};

void BuildDlgWithChildren(cBailDialog& dlg, BailChildren& out) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr,
               cBailDialog::kBailEditBoxId);
    edit->InitEditbox(50, 64);
    out.edit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 200, 100, nullptr, cBailDialog::kBailTextId);
    text->InitTextArea({0, 0, 200, 100}, 256);
    out.text = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));

    dlg.Linking();
}

}  // namespace

TEST(CBailDialogTest, LinkingResolvesBothChildren) {
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);

    EXPECT_EQ(dlg.GetBailEditBox(), raws.edit);
    EXPECT_EQ(dlg.GetBailText(),    raws.text);
}

TEST(CBailDialogTest, LinkingConfiguresValidCheckAndAlign) {
    // 1:1 quirk: legacy calls
    //   m_pBailEdtBox->SetValidCheck(VCM_NUMBER)
    //   m_pBailEdtBox->SetAlign(TXT_RIGHT)
    // Modern port uses SetValidCheck(1) (VCM_NUMBER = 1)
    // + SetAlign(TextAlign::Right = 2).
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    ASSERT_NE(dlg.GetBailEditBox(), nullptr);
    EXPECT_EQ(dlg.GetBailEditBox()->GetValidCheckMethod(), 1);
    EXPECT_EQ(dlg.GetBailEditBox()->textAlign(),
              cEditBox::TextAlign::Right);
}

TEST(CBailDialogTest, LinkingCallsSetScriptText) {
    // 1:1 quirk: legacy calls
    //   wsprintf(buf, CHATMGR->GetChatMsg(644), ...);
    //   m_pBailText->SetScriptText(buf);
    // Modern port uses placeholder text
    // "BAIL_TEXT_PLACEHOLDER" until CHATMGR is ported.
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    ASSERT_NE(dlg.GetBailText(), nullptr);
    EXPECT_STREQ(dlg.GetBailText()->GetScriptText().c_str(),
                 "BAIL_TEXT_PLACEHOLDER");
}

TEST(CBailDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cBailDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetBailEditBox(), nullptr);
    EXPECT_EQ(dlg.GetBailText(),    nullptr);
}

// ===========================================================================
// Open / Close / SetFame / SetBadFrameSync (TODO 4-singleton dispatches)
// ===========================================================================

TEST(CBailDialogTest, OpenIsNoOp) {
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.Open();
    // 1:1 quirk: legacy Open conditionally activates
    // the dialog based on HERO->GetBadFame() vs
    // MIN_BADFAME_FOR_BAIL. Modern port: TODO.
    SUCCEED();
}

TEST(CBailDialogTest, OpenWithoutLinkIsSafe) {
    cBailDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.Open();
    SUCCEED();
}

TEST(CBailDialogTest, CloseIsNoOp) {
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.Close();
    // 1:1 quirk: legacy Close calls SetDisable(FALSE) +
    // SetActive(FALSE) + OBJECTSTATEMGR->EndObjectState.
    // Modern port: TODO. No observable state change
    // until those singletons are ported.
    SUCCEED();
}

TEST(CBailDialogTest, CloseWithoutLinkIsSafe) {
    cBailDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.Close();
    SUCCEED();
}

TEST(CBailDialogTest, SetFameIsNoOp) {
    // 1:1 quirk: legacy SetFame reads the edit text
    // as a number + checks the hero's bad fame + money
    // + shows WINDOWMGR msg boxes. Modern port: TODO.
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.SetFame();
    // The body is a no-op until the 4-singleton
    // dispatch is ported.
    EXPECT_EQ(dlg.GetBadFame(), 0u);
}

TEST(CBailDialogTest, SetFameWithoutLinkIsSafe) {
    cBailDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetFame();
    SUCCEED();
}

TEST(CBailDialogTest, SetBadFrameSyncIsNoOp) {
    // 1:1 quirk: legacy SetBadFrameSync sends MSG_FAME
    // network message + closes the dialog. Modern port:
    // TODO. m_BadFame stays at 0 (the body is a
    // no-op until the 3-singleton dispatch is ported).
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.SetBadFrameSync();
    EXPECT_EQ(dlg.GetBadFame(), 0u);
}

TEST(CBailDialogTest, SetBadFrameSyncWithoutLinkIsSafe) {
    cBailDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetBadFrameSync();
    SUCCEED();
}

}  // namespace mxh::ui::test
