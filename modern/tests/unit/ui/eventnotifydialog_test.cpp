// eventnotifydialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cEventNotifyDialog (GM event notification
// dialog: title + context text area).
//
// Covers modern/src/ui/eventnotifydialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\EventNotifyDialog.h (793 B) and
//   墨香【源码】\[Client]MH\EventNotifyDialog.cpp.
//
// What's tested:
//   - Default construction: 2 child pointers are null.
//   - Linking resolves the cStatic + cTextArea children
//     (kStcTitleId=270, kTAContextId=271) by id.
//   - Linking without children leaves pointers null and is
//     safe.
//   - SetActive override calls base SetActive + clears
//     context text on deactivation (1:1 quirk).
//   - SetActive true does NOT clear context text (1:1
//     quirk: clear only on val=false).
//   - ActionEvent override delegates to base cDialog
//     (click-to-close is documented as TODO).
//   - SetTitle calls m_pStcTitle->SetStaticText.
//   - SetTitle with null is safe (defensive null-check).
//   - SetTitle without linked cStatic is safe.
//   - SetContext calls m_pTAContext->SetScriptText.
//   - SetContext with null clears the text (1:1 quirk:
//     legacy SetScriptText is called with "" in SetActive).
//   - SetContext without linked cTextArea is safe.
//   - SetEventCount is a no-op (1:1 quirk: state machine
//     deferred).
//   - Accessors return the linked child pointers.
//
// 1:1 quirks preserved:
//   - Linking does not call SetToolTip (SCRIPTMGR
//     singleton deferred).
//   - SetActive clears the context text on deactivation
//     (legacy: m_pTAContext->SetScriptText("") when
//     !val).
//   - SetActive does NOT clear context text on activation
//     (legacy: only !val triggers clear).
//   - SetActive override matches base noexcept (R-12
//     polymorphic virtual required).
//   - ActionEvent override delegates to base (click-to-
//     close is commented out in legacy).
//   - SetEventCount body is no-op (state machine
//     deferred).

#include "eventnotifydialog.hpp"
#include "cstatic.hpp"
#include "ctextarea.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CEventNotifyDialogTest, DefaultConstructionHasNullPointers) {
    cEventNotifyDialog dlg;
    EXPECT_EQ(dlg.GetStcTitle(),  nullptr);
    EXPECT_EQ(dlg.GetTAContext(), nullptr);
}

// ===========================================================================
// Id constants
// ===========================================================================

TEST(CEventNotifyDialogTest, IdConstantsAreDistinct) {
    EXPECT_NE(cEventNotifyDialog::kStcTitleId,
              cEventNotifyDialog::kTAContextId);
}

TEST(CEventNotifyDialogTest, IdConstantsMatchExpectedLocalRange) {
    // 1:1 quirk: pick 270-271 to avoid collisions with
    // other Tier 2 dialog id ranges (cCharMakeDlg 200-203,
    // cGuildJoinDialog 210-212, cCharStateDialog 220-224,
    // cSOSDialog 230-231, cMiniFriendDialog 240-243,
    // cReviveDialog 250-252, cMPNoticeDialog 260-261).
    EXPECT_EQ(cEventNotifyDialog::kStcTitleId,  270);
    EXPECT_EQ(cEventNotifyDialog::kTAContextId, 271);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

// Build a cEventNotifyDialog with 2 children wired in
// the modern id range (270-271). Returns the raw
// pointers via the out vector; ownership lives in the
// dlg (children are added via cWindow::Add).
void BuildDlgWithChildren(cEventNotifyDialog& dlg,
                          cStatic*&   out_title,
                          cTextArea*& out_context) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto title = std::make_unique<cStatic>();
    title->Init(0, 0, 200, 14, nullptr,
                cEventNotifyDialog::kStcTitleId);
    out_title = title.get();
    dlg.Add(std::unique_ptr<cWindow>(title.release()));

    auto ctx = std::make_unique<cTextArea>();
    ctx->Init(0, 0, 200, 100, nullptr,
              cEventNotifyDialog::kTAContextId);
    ctx->InitTextArea(/*textRelRect=*/{0, 0, 200, 100},
                     /*bufSize=*/256);
    out_context = ctx.get();
    dlg.Add(std::unique_ptr<cWindow>(ctx.release()));

    dlg.Linking();
}

}  // namespace

TEST(CEventNotifyDialogTest, LinkingResolvesBothChildren) {
    cEventNotifyDialog dlg;
    cStatic*  raw_title  = nullptr;
    cTextArea* raw_context = nullptr;
    BuildDlgWithChildren(dlg, raw_title, raw_context);

    EXPECT_EQ(dlg.GetStcTitle(),  raw_title);
    EXPECT_EQ(dlg.GetTAContext(), raw_context);
}

TEST(CEventNotifyDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cEventNotifyDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetStcTitle(),  nullptr);
    EXPECT_EQ(dlg.GetTAContext(), nullptr);
}

TEST(CEventNotifyDialogTest, LinkingBeforeInitDoesNotCrash) {
    cEventNotifyDialog dlg;
    dlg.Linking();
    EXPECT_EQ(dlg.GetStcTitle(),  nullptr);
    EXPECT_EQ(dlg.GetTAContext(), nullptr);
}

// ===========================================================================
// SetActive (1:1 override, base + clear context on deactivation)
// ===========================================================================

TEST(CEventNotifyDialogTest, SetActiveTrueUpdatesBaseState) {
    cEventNotifyDialog dlg;
    cStatic*  raw_title  = nullptr;
    cTextArea* raw_context = nullptr;
    BuildDlgWithChildren(dlg, raw_title, raw_context);
    EXPECT_FALSE(dlg.isActive());

    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CEventNotifyDialogTest, SetActiveFalseUpdatesBaseState) {
    cEventNotifyDialog dlg;
    cStatic*  raw_title  = nullptr;
    cTextArea* raw_context = nullptr;
    BuildDlgWithChildren(dlg, raw_title, raw_context);
    dlg.SetActive(true);
    ASSERT_TRUE(dlg.isActive());

    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CEventNotifyDialogTest, SetActiveFalseClearsContextText) {
    // 1:1 quirk: legacy SetActive(false) calls
    // m_pTAContext->SetScriptText(""). Modern port
    // mirrors this.
    cEventNotifyDialog dlg;
    cStatic*  raw_title  = nullptr;
    cTextArea* raw_context = nullptr;
    BuildDlgWithChildren(dlg, raw_title, raw_context);
    ASSERT_NE(dlg.GetTAContext(), nullptr);

    // Pre-populate the context with some text.
    dlg.SetContext("Event notification text");
    ASSERT_EQ(dlg.GetTAContext()->GetScriptText(),
              "Event notification text");

    // Activate + deactivate.
    dlg.SetActive(true);
    dlg.SetActive(false);

    // The context text should be cleared on deactivation.
    EXPECT_EQ(dlg.GetTAContext()->GetScriptText(), "");
}

TEST(CEventNotifyDialogTest, SetActiveTrueDoesNotClearContextText) {
    // 1:1 quirk: clear only happens on val=false.
    cEventNotifyDialog dlg;
    cStatic*  raw_title  = nullptr;
    cTextArea* raw_context = nullptr;
    BuildDlgWithChildren(dlg, raw_title, raw_context);

    dlg.SetContext("Initial text");
    dlg.SetActive(true);
    // After SetActive(true), the context text should
    // NOT be cleared (1:1 quirk: clear only on !val).
    EXPECT_EQ(dlg.GetTAContext()->GetScriptText(), "Initial text");
}

TEST(CEventNotifyDialogTest, SetActiveWithoutContextIsSafe) {
    // Defensive: SetActive when m_pTAContext is null
    // must not crash.
    cEventNotifyDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

// ===========================================================================
// ActionEvent (1:1 override, base only)
// ===========================================================================

TEST(CEventNotifyDialogTest, ActionEventDelegatesToBase) {
    cEventNotifyDialog dlg;
    cStatic*  raw_title  = nullptr;
    cTextArea* raw_context = nullptr;
    BuildDlgWithChildren(dlg, raw_title, raw_context);
    dlg.SetActive(true);
    ASSERT_TRUE(dlg.isActive());

    // 1:1 quirk: legacy ActionEvent just calls
    // cDialog::ActionEvent (click-to-close is
    // commented out). Modern port: same. The call
    // must not crash.
    std::uint32_t we = dlg.ActionEvent(10, 10, 0x01);
    (void)we;  // exact value depends on cDialog's child
               // hit-test, but the call must succeed.
    SUCCEED();
}

// ===========================================================================
// SetTitle
// ===========================================================================

TEST(CEventNotifyDialogTest, SetTitleUpdatesStaticText) {
    cEventNotifyDialog dlg;
    cStatic*  raw_title  = nullptr;
    cTextArea* raw_context = nullptr;
    BuildDlgWithChildren(dlg, raw_title, raw_context);
    ASSERT_NE(dlg.GetStcTitle(), nullptr);

    dlg.SetTitle("Server Maintenance");
    EXPECT_STREQ(dlg.GetStcTitle()->GetStaticText().c_str(), "Server Maintenance");

    dlg.SetTitle("Different Title");
    EXPECT_STREQ(dlg.GetStcTitle()->GetStaticText().c_str(), "Different Title");
}

TEST(CEventNotifyDialogTest, SetTitleWithEmptyStringIsSafe) {
    // 1:1 quirk: SetTitle with nullptr is NOT
    // supported by cStatic::SetStaticText (which
    // takes std::string — std::string(nullptr) is
    // undefined behavior). SetTitle with an empty
    // string is the safe equivalent (matches the
    // legacy SetStaticText("") pattern).
    cEventNotifyDialog dlg;
    cStatic*  raw_title  = nullptr;
    cTextArea* raw_context = nullptr;
    BuildDlgWithChildren(dlg, raw_title, raw_context);
    dlg.SetTitle("");
    // The cStatic text should now be empty.
    EXPECT_STREQ(dlg.GetStcTitle()->GetStaticText().c_str(), "");
}

TEST(CEventNotifyDialogTest, SetTitleWithoutLinkIsSafe) {
    // Defensive: SetTitle when m_pStcTitle is null
    // must not crash.
    cEventNotifyDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetTitle("no title attached");
    SUCCEED();
}

// ===========================================================================
// SetContext
// ===========================================================================

TEST(CEventNotifyDialogTest, SetContextUpdatesScriptText) {
    cEventNotifyDialog dlg;
    cStatic*  raw_title  = nullptr;
    cTextArea* raw_context = nullptr;
    BuildDlgWithChildren(dlg, raw_title, raw_context);
    ASSERT_NE(dlg.GetTAContext(), nullptr);

    dlg.SetContext("Server is shutting down in 5 minutes");
    EXPECT_EQ(dlg.GetTAContext()->GetScriptText(),
              "Server is shutting down in 5 minutes");
}

TEST(CEventNotifyDialogTest, SetContextWithNullClearsText) {
    // 1:1 quirk: SetContext(nullptr) calls
    // m_pTAContext->SetScriptText(nullptr) which
    // clears the text. Modern port mirrors this.
    cEventNotifyDialog dlg;
    cStatic*  raw_title  = nullptr;
    cTextArea* raw_context = nullptr;
    BuildDlgWithChildren(dlg, raw_title, raw_context);

    dlg.SetContext("initial text");
    ASSERT_EQ(dlg.GetTAContext()->GetScriptText(), "initial text");

    dlg.SetContext(nullptr);
    EXPECT_EQ(dlg.GetTAContext()->GetScriptText(), "");
}

TEST(CEventNotifyDialogTest, SetContextWithoutLinkIsSafe) {
    cEventNotifyDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetContext("no context attached");
    SUCCEED();
}

TEST(CEventNotifyDialogTest, SetContextBeforeLinkingIsSafe) {
    // Defensive: SetContext before Linking must not
    // crash (m_pTAContext is null, the null-check
    // short-circuits).
    cEventNotifyDialog dlg;
    dlg.SetContext("test");
    SUCCEED();
}

// ===========================================================================
// SetEventCount (1:1 quirk: state machine deferred)
// ===========================================================================

TEST(CEventNotifyDialogTest, SetEventCountIsNoOp) {
    // 1:1 quirk: legacy SetEventCount(bool bAdd) was
    // a state-machine method. Modern port: no-op
    // until the event count tracker is ported.
    cEventNotifyDialog dlg;
    dlg.SetEventCount(true);
    dlg.SetEventCount(false);
    SUCCEED();
}

}  // namespace mxh::ui::test
