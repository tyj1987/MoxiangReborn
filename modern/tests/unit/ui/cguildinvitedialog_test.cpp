//
// Unit tests for mxh::ui::cGuildInviteDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//  * Constants: kIdInviteText=420, kFlgMember=0,
//    kFlgStudent=1, kChatMsgMember=45, kChatMsgStudent=1370
//  * Default construction: invite text is null
//  * SetInviteTextForTest stores the pointer
//  * Linking is a no-op (host injects text first)
//  * SetInfo with kFlgMember calls SetScriptText
//    with chatmsg 45 format applied to
//    (GuildName, MasterName)
//  * SetInfo with kFlgStudent calls SetScriptText
//    with chatmsg 1370 format applied to
//    (MasterName, GuildName) -- arg order is swapped
//  * SetInfo with default chatmsg format "%s %s"
//    produces "Guild Master" (member) or
//    "Master Guild" (student)
//  * Custom chatmsg format (e.g. "[%s]-[%s]") is
//    honoured by both branches
//  * SetInfo with kFlgMember + null guildName is
//    a no-op (script text unchanged)
//  * SetInfo with kFlgMember + null masterName is
//    a no-op
//  * SetInfo without Linking is a no-op
//  * NonCopyable
//

#include "mxh/ui/cguildinvitedialog.hpp"
#include "mxh/ui/ctextarea.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using mxh::ui::cGuildInviteDialog;
using mxh::ui::cTextArea;

namespace {

struct Harness {
    cGuildInviteDialog dlg;
    cTextArea inviteText;

    Harness() {
        dlg.SetInviteTextForTest(&inviteText);
    }
};

const char* bracketFormat(int /*id*/, void* /*user*/) { return "[%s]-[%s]"; }

}  // namespace


TEST(CGuildInviteDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(cGuildInviteDialog::kIdInviteText, 420);
    EXPECT_EQ(cGuildInviteDialog::kFlgMember,  0);
    EXPECT_EQ(cGuildInviteDialog::kFlgStudent, 1);
    EXPECT_EQ(cGuildInviteDialog::kChatMsgMember,  45);
    EXPECT_EQ(cGuildInviteDialog::kChatMsgStudent, 1370);
}

TEST(CGuildInviteDialog, DefaultConstructionHasNullInviteText) {
    cGuildInviteDialog d;
    EXPECT_EQ(d.GetInviteTextForTest(), nullptr);
}

TEST(CGuildInviteDialog, SetInviteTextStoresPointer) {
    cGuildInviteDialog d;
    cTextArea ta;
    d.SetInviteTextForTest(&ta);
    EXPECT_EQ(d.GetInviteTextForTest(), &ta);
}


TEST(CGuildInviteDialog, LinkingIsNoOpWithInjectedText) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetInviteTextForTest(), &h.inviteText);
}


TEST(CGuildInviteDialog, SetInfoMemberBranch) {
    Harness h;
    h.dlg.SetInfo("Dragons", "Alice", cGuildInviteDialog::kFlgMember);
    // 1:1 with legacy sprintf(text, "%s %s", "Dragons", "Alice")
    EXPECT_EQ(h.inviteText.GetScriptText(), "Dragons Alice");
}

TEST(CGuildInviteDialog, SetInfoStudentBranchSwapsArgs) {
    Harness h;
    h.dlg.SetInfo("Dragons", "Alice", cGuildInviteDialog::kFlgStudent);
    // 1:1 with legacy sprintf(text, "%s %s", "Alice", "Dragons")
    // (MasterName first, GuildName second in the
    // student branch).
    EXPECT_EQ(h.inviteText.GetScriptText(), "Alice Dragons");
}

TEST(CGuildInviteDialog, SetInfoCustomFormatMember) {
    Harness h;
    h.dlg.SetChatMsgCallbackForTest(&bracketFormat, nullptr);
    h.dlg.SetInfo("Dragons", "Alice", cGuildInviteDialog::kFlgMember);
    EXPECT_EQ(h.inviteText.GetScriptText(), "[Dragons]-[Alice]");
}

TEST(CGuildInviteDialog, SetInfoCustomFormatStudent) {
    Harness h;
    h.dlg.SetChatMsgCallbackForTest(&bracketFormat, nullptr);
    h.dlg.SetInfo("Dragons", "Alice", cGuildInviteDialog::kFlgStudent);
    EXPECT_EQ(h.inviteText.GetScriptText(), "[Alice]-[Dragons]");
}

TEST(CGuildInviteDialog, SetInfoUnknownFlgKindFallsToStudent) {
    // 1:1 with legacy `else` fallthrough: any
    // non-AsMember value takes the student branch.
    Harness h;
    h.dlg.SetInfo("G", "M", 99);
    EXPECT_EQ(h.inviteText.GetScriptText(), "M G");
}


TEST(CGuildInviteDialog, SetInfoNullGuildNameIsNoOp) {
    Harness h;
    h.inviteText.SetScriptText("untouched");
    h.dlg.SetInfo(nullptr, "Alice", cGuildInviteDialog::kFlgMember);
    EXPECT_EQ(h.inviteText.GetScriptText(), "untouched");
}

TEST(CGuildInviteDialog, SetInfoNullMasterNameIsNoOp) {
    Harness h;
    h.inviteText.SetScriptText("untouched");
    h.dlg.SetInfo("Dragons", nullptr, cGuildInviteDialog::kFlgMember);
    EXPECT_EQ(h.inviteText.GetScriptText(), "untouched");
}

TEST(CGuildInviteDialog, SetInfoNullBothIsNoOp) {
    Harness h;
    h.inviteText.SetScriptText("untouched");
    h.dlg.SetInfo(nullptr, nullptr, cGuildInviteDialog::kFlgMember);
    EXPECT_EQ(h.inviteText.GetScriptText(), "untouched");
}

TEST(CGuildInviteDialog, SetInfoWithoutLinkingIsNoOp) {
    cGuildInviteDialog d;
    d.SetInfo("Dragons", "Alice", cGuildInviteDialog::kFlgMember);
    SUCCEED();
}


TEST(CGuildInviteDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible<cGuildInviteDialog>::value,
                  "cGuildInviteDialog must not be copyable");
    static_assert(!std::is_copy_assignable<cGuildInviteDialog>::value,
                  "cGuildInviteDialog must not be copy-assignable");
    SUCCEED();
}
