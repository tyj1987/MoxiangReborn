// guildinvitedialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cGuildInviteDialog (guild invitation
// display dialog: 1 cTextArea).
//
// Covers modern/src/ui/guildinvitedialog.{hpp,cpp}, a 1:1 port
// of
//   墨香【源码】\[Client]MH\GuildInviteDialog.h (837 B) and
//   墨香【源码】\[Client]MH\GuildInviteDialog.cpp.
//
// What's tested:
//   - Default construction: cGuildInviteDialog is a
//     cDialog and inherits its tree management.
//   - Id constant matches expected local range
//     (kIdInviteText=420).
//   - kFlgMember=0 + kFlgStudent=1 (1:1 with legacy
//     AsMember/AsStudent enum).
//   - Linking resolves the cTextArea child by id.
//   - Linking without children leaves m_pInviteMsg
//     null (SetInfo is safe).
//   - Linking before Init does not crash.
//   - SetInfo with kFlgMember + valid names updates
//     cTextArea's SetScriptText with the member
//     format (1:1 with legacy sprintf).
//   - SetInfo with kFlgStudent + valid names updates
//     cTextArea's SetScriptText with the student
//     format (1:1 with legacy else branch).
//   - SetInfo with kFlgMember + null names is safe
//     (1:1 quirk: modern port guards null names;
//     legacy would crash on sprintf with null).
//   - SetInfo without Linking is safe.
//   - SetInfo before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_GUILDINVITEDLG drop, modern cWindow
//     does not have m_type field).
//   - SetInfo uses placeholder format strings
//     "GUILD_INVITE_MSG_MEMBER" /
//     "GUILD_INVITE_MSG_STUDENT" instead of
//     CHATMGR->GetChatMsg(45) / GetChatMsg(1370).
//   - SetInfo with null names is safe (modern
//     port guards; legacy would crash).
//   - kFlgMember=0 / kFlgStudent=1 (1:1 with
//     legacy AsMember/AsStudent enum).
//   - Local id range 420 (distinct from 200-410
//     used by previous Tier 2 dialogs).

#include "guildinvitedialog.hpp"
#include "cdialog.hpp"
#include "ctextarea.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CGuildInviteDialogTest, DefaultConstructionIsValid) {
    cGuildInviteDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_GUILDINVITEDLG drop, modern
    // cWindow does not have m_type field).
    SUCCEED();
}

TEST(CGuildInviteDialogTest, InheritsDialogTreeManagement) {
    cGuildInviteDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CGuildInviteDialogTest, IdConstantMatchesExpectedLocalRange) {
    EXPECT_EQ(cGuildInviteDialog::kIdInviteText, 420);
}

TEST(CGuildInviteDialogTest, FlgKindConstantsMatchLegacy) {
    // 1:1 with legacy AsMember = 0 / AsStudent = 1.
    EXPECT_EQ(cGuildInviteDialog::kFlgMember, 0);
    EXPECT_EQ(cGuildInviteDialog::kFlgStudent, 1);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

void BuildDlgWithChildren(cGuildInviteDialog& dlg, cTextArea** outText) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 380, 380, nullptr, cGuildInviteDialog::kIdInviteText);
    text->InitTextArea({0, 0, 380, 380}, 256);
    *outText = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));

    dlg.Linking();
}

}  // namespace

TEST(CGuildInviteDialogTest, LinkingResolvesTextArea) {
    cGuildInviteDialog dlg;
    cTextArea* pText = nullptr;
    BuildDlgWithChildren(dlg, &pText);
    // m_pInviteMsg is private; verified indirectly
    // via SetInfo updating the cTextArea.
    dlg.SetInfo("Knights", "Arthur", cGuildInviteDialog::kFlgMember);
    EXPECT_NE(pText->GetScriptText().find("Knights"), std::string::npos);
    EXPECT_NE(pText->GetScriptText().find("Arthur"), std::string::npos);
}

TEST(CGuildInviteDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cGuildInviteDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SetInfo without m_pInviteMsg is safe.
    dlg.SetInfo("Knights", "Arthur", cGuildInviteDialog::kFlgMember);
    SUCCEED();
}

TEST(CGuildInviteDialogTest, LinkingBeforeInitDoesNotCrash) {
    cGuildInviteDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// SetInfo
// ===========================================================================

TEST(CGuildInviteDialogTest, SetInfoAsMemberUpdatesTextArea) {
    cGuildInviteDialog dlg;
    cTextArea* pText = nullptr;
    BuildDlgWithChildren(dlg, &pText);

    dlg.SetInfo("Knights", "Arthur", cGuildInviteDialog::kFlgMember);
    EXPECT_NE(pText->GetScriptText().find("Knights"), std::string::npos);
    EXPECT_NE(pText->GetScriptText().find("Arthur"), std::string::npos);
    EXPECT_NE(pText->GetScriptText().find("GUILD_INVITE_MSG_MEMBER"),
              std::string::npos);
    // AsStudent format should NOT appear.
    EXPECT_EQ(pText->GetScriptText().find("GUILD_INVITE_MSG_STUDENT"),
              std::string::npos);
}

TEST(CGuildInviteDialogTest, SetInfoAsStudentUpdatesTextArea) {
    cGuildInviteDialog dlg;
    cTextArea* pText = nullptr;
    BuildDlgWithChildren(dlg, &pText);

    dlg.SetInfo("Knights", "Arthur", cGuildInviteDialog::kFlgStudent);
    EXPECT_NE(pText->GetScriptText().find("Knights"), std::string::npos);
    EXPECT_NE(pText->GetScriptText().find("Arthur"), std::string::npos);
    EXPECT_NE(pText->GetScriptText().find("GUILD_INVITE_MSG_STUDENT"),
              std::string::npos);
    // AsMember format should NOT appear.
    EXPECT_EQ(pText->GetScriptText().find("GUILD_INVITE_MSG_MEMBER"),
              std::string::npos);
}

TEST(CGuildInviteDialogTest, SetInfoAsStudentWithUnknownFlgKind) {
    // 1:1 with legacy `else` fallthrough: any
    // value != AsMember uses the student branch.
    cGuildInviteDialog dlg;
    cTextArea* pText = nullptr;
    BuildDlgWithChildren(dlg, &pText);

    dlg.SetInfo("Knights", "Arthur", /*flgKind=*/999);
    EXPECT_NE(pText->GetScriptText().find("GUILD_INVITE_MSG_STUDENT"),
              std::string::npos);
}

TEST(CGuildInviteDialogTest, SetInfoAsMemberWithNullNamesIsSafe) {
    // 1:1 quirk: modern port guards null names
    // (legacy would crash on sprintf with null).
    cGuildInviteDialog dlg;
    cTextArea* pText = nullptr;
    BuildDlgWithChildren(dlg, &pText);

    dlg.SetInfo(nullptr, nullptr, cGuildInviteDialog::kFlgMember);
    // The text is empty (guarded null names).
    EXPECT_EQ(pText->GetScriptText(), "");
}

TEST(CGuildInviteDialogTest, SetInfoAsStudentWithNullNamesIsSafe) {
    cGuildInviteDialog dlg;
    cTextArea* pText = nullptr;
    BuildDlgWithChildren(dlg, &pText);

    dlg.SetInfo(nullptr, nullptr, cGuildInviteDialog::kFlgStudent);
    EXPECT_EQ(pText->GetScriptText(), "");
}

TEST(CGuildInviteDialogTest, SetInfoOverwritesPreviousText) {
    cGuildInviteDialog dlg;
    cTextArea* pText = nullptr;
    BuildDlgWithChildren(dlg, &pText);

    dlg.SetInfo("First", "Guild1", cGuildInviteDialog::kFlgMember);
    EXPECT_NE(pText->GetScriptText().find("First"), std::string::npos);

    dlg.SetInfo("Second", "Guild2", cGuildInviteDialog::kFlgMember);
    // Old text overwritten.
    EXPECT_EQ(pText->GetScriptText().find("First"), std::string::npos);
    EXPECT_NE(pText->GetScriptText().find("Second"), std::string::npos);
}

TEST(CGuildInviteDialogTest, SetInfoWithoutLinkIsSafe) {
    cGuildInviteDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetInfo("Knights", "Arthur", cGuildInviteDialog::kFlgMember);
    dlg.SetInfo("Knights", "Arthur", cGuildInviteDialog::kFlgStudent);
    dlg.SetInfo(nullptr, nullptr, cGuildInviteDialog::kFlgMember);
    SUCCEED();
}

TEST(CGuildInviteDialogTest, SetInfoBeforeInitDoesNotCrash) {
    cGuildInviteDialog dlg;
    dlg.SetInfo("Knights", "Arthur", cGuildInviteDialog::kFlgMember);
    SUCCEED();
}

}  // namespace mxh::ui::test
