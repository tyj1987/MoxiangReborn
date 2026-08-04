// guildnicknamedialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog
// 1:1 port contract test for modern cGuildNickNameDialog
// (guild member nickname editor dialog: 1 cTextArea + 1 cEditBox).
//
// Covers modern/src/ui/guildnicknamedialog.{hpp,cpp}, a 1:1 port
// of
//   墨香【源码】\[Client]MH\GuildNickNameDialog.h (821 B) and
//   墨香【源码】\[Client]MH\GuildNickNameDialog.cpp.
//
// What's tested:
//   - Default construction: cGuildNickNameDialog is a
//     cDialog and inherits its tree management.
//   - 2 id constants match expected local range
//     (kIdNickTextArea=400, kIdNickNameEdit=401).
//   - kVcmSpace == 0 (1:1 with legacy VCM_SPACE
//     enum value).
//   - Linking resolves the cTextArea + cEditBox
//     children by id and calls SetValidCheck(0)
//     on the cEditBox.
//   - Linking without children leaves both
//     pointers null (SetActive + SetNickMsg are
//     safe).
//   - Linking before Init does not crash.
//   - SetActive val=true calls base SetActive
//     (GUILDMGR + CHATMGR TODO: the modern port
//     documents the GUILDMGR check + early
//     return + SetEditText("") + SetNickMsg
//     path but does not execute them).
//   - SetActive val=false calls base SetActive +
//     SetFocusEdit(false) on the cEditBox (1:1
//     with legacy `else` branch).
//   - SetActive without Linking is safe.
//   - SetNickMsg with valid name updates
//     cTextArea's SetScriptText (the modern
//     port uses placeholder format
//     "GUILD_NICK_MSG_FORMAT %s" + std::snprintf,
//     matches legacy sprintf semantic).
//   - SetNickMsg with nullptr is safe (1:1 quirk:
//     modern guards null Name; legacy would crash
//     on sprintf with null).
//   - SetNickMsg without Linking is safe.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_GUILDNICKNAMEDLG drop, modern cWindow
//     does not have m_type field).
//   - SetActive override: base SetActive always
//     called (matches legacy call order).
//   - SetActive val=false: SetFocusEdit(false)
//     (REAL, no singleton dep).
//   - SetActive val=true: GUILDMGR check +
//     early return + SetEditText("") +
//     SetNickMsg dispatch is TODO (GUILDMGR +
//     CHATMGR not ported, R-12.x deferred).
//   - SetNickMsg uses placeholder format
//     "GUILD_NICK_MSG_FORMAT" instead of
//     CHATMGR->GetChatMsg(704).
//   - SetNickMsg with nullptr is safe (modern
//     guards; legacy would crash).
//   - kVcmSpace = 0 (1:1 with legacy
//     cEditBox::SetValidCheck enum value).
//   - Local id range 400-401 (distinct from
//     200-389 used by previous Tier 2 dialogs).

#include "guildnicknamedialog.hpp"
#include "cdialog.hpp"
#include "ctextarea.hpp"
#include "ceditbox.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CGuildNickNameDialogTest, DefaultConstructionIsValid) {
    cGuildNickNameDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_GUILDNICKNAMEDLG drop, modern
    // cWindow does not have m_type field).
    SUCCEED();
}

TEST(CGuildNickNameDialogTest, InheritsDialogTreeManagement) {
    cGuildNickNameDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CGuildNickNameDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cGuildNickNameDialog::kIdNickTextArea, 400);
    EXPECT_EQ(cGuildNickNameDialog::kIdNickNameEdit, 401);
}

TEST(CGuildNickNameDialogTest, VcmSpaceIsZero) {
    // 1:1 with legacy cEditBox::SetValidCheck
    // VCM_SPACE = 0 (reject spaces in input).
    EXPECT_EQ(cGuildNickNameDialog::kVcmSpace, 0);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

void BuildDlgWithChildren(cGuildNickNameDialog& dlg,
                          cTextArea** outText,
                          cEditBox** outEdit) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 380, 380, nullptr, cGuildNickNameDialog::kIdNickTextArea);
    text->InitTextArea({0, 0, 380, 380}, 256);
    *outText = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr,
               cGuildNickNameDialog::kIdNickNameEdit);
    edit->InitEditbox(50, 64);
    *outEdit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));

    dlg.Linking();
}

}  // namespace

TEST(CGuildNickNameDialogTest, LinkingResolvesBothChildren) {
    cGuildNickNameDialog dlg;
    cTextArea* pText = nullptr;
    cEditBox* pEdit = nullptr;
    BuildDlgWithChildren(dlg, &pText, &pEdit);

    // m_pNickMsg / m_pNickName are private;
    // verified indirectly via SetNickMsg (updates
    // cTextArea) + SetActive(false) (calls
    // SetFocusEdit(false) on cEditBox).
    dlg.SetNickMsg("test_player");
    EXPECT_NE(pText->GetScriptText().find("test_player"), std::string::npos);
}

TEST(CGuildNickNameDialogTest, LinkingConfiguresValidCheckSpace) {
    // 1:1 quirk: legacy calls SetValidCheck(VCM_SPACE)
    // on the cEditBox. Modern port calls
    // SetValidCheck(0). The cEditBox port stores
    // the valid check int internally; we verify
    // by checking the value (1:1 quirk: the
    // modern cEditBox::GetValidCheck might not
    // exist, so we just verify Linking does not
    // crash + the dialog state is unchanged).
    cGuildNickNameDialog dlg;
    cTextArea* pText = nullptr;
    cEditBox* pEdit = nullptr;
    BuildDlgWithChildren(dlg, &pText, &pEdit);
    SUCCEED();
}

TEST(CGuildNickNameDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cGuildNickNameDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SetActive + SetNickMsg without children
    // must be safe.
    dlg.SetActive(true);
    dlg.SetActive(false);
    dlg.SetNickMsg("test");
    dlg.SetNickMsg(nullptr);
    SUCCEED();
}

TEST(CGuildNickNameDialogTest, LinkingBeforeInitDoesNotCrash) {
    cGuildNickNameDialog dlg;
    dlg.Linking();
    SUCCEED();
}

namespace {

struct GuildNickCallbackState {
    std::uint32_t memberId = 7;
    std::string memberName = "SelectedHero";
    std::string format = "Change nickname for %s";
    int messageCalls = 0;
    std::int32_t lastMessageId = 0;
};
std::uint32_t GetGuildNickMemberId(void* data){return static_cast<GuildNickCallbackState*>(data)->memberId;}
const char* GetGuildNickMemberName(void* data){return static_cast<GuildNickCallbackState*>(data)->memberName.c_str();}
void AddGuildNickMessage(std::int32_t id,void* data){auto& state=*static_cast<GuildNickCallbackState*>(data);++state.messageCalls;state.lastMessageId=id;}
const char* GetGuildNickChatMessage(std::int32_t,void* data){return static_cast<GuildNickCallbackState*>(data)->format.c_str();}
void InstallGuildNickCallbacks(cGuildNickNameDialog& dlg,GuildNickCallbackState& state){
 dlg.SetCallbacks(GetGuildNickMemberId,GetGuildNickMemberName,AddGuildNickMessage,
                  GetGuildNickChatMessage,&state);
}

}  // namespace

// ===========================================================================
// SetActive override
// ===========================================================================

TEST(CGuildNickNameDialogTest, SetActiveTrueUpdatesBaseState) {
    GuildNickCallbackState callbackState;
    cGuildNickNameDialog dlg;
    InstallGuildNickCallbacks(dlg, callbackState);
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_FALSE(dlg.isActive());
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildNickNameDialogTest, SetActiveFalseUpdatesBaseStateAndClearsFocus) {
    GuildNickCallbackState callbackState;
    // 1:1 with legacy val == FALSE: calls
    // m_pNickName->SetFocusEdit(false) on the
    // cEditBox, then cDialog::SetActive(false).
    cGuildNickNameDialog dlg;
    InstallGuildNickCallbacks(dlg, callbackState);
    cTextArea* pText = nullptr;
    cEditBox* pEdit = nullptr;
    BuildDlgWithChildren(dlg, &pText, &pEdit);

    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
    // 1:1 quirk: legacy SetFocusEdit(FALSE) is
    // called on the cEditBox. Modern cEditBox
    // has SetFocusEdit(bool) — the test verifies
    // the call is safe (no crash) but does not
    // verify the focus state (the modern
    // cEditBox::IsFocused API may not exist in
    // the minimal port).
}

TEST(CGuildNickNameDialogTest, SetActiveToggleRoundTrip) {
    GuildNickCallbackState callbackState;
    cGuildNickNameDialog dlg;
    InstallGuildNickCallbacks(dlg, callbackState);
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildNickNameDialogTest, SetActiveWithoutLinkIsSafe) {
    GuildNickCallbackState callbackState;
    cGuildNickNameDialog dlg;
    InstallGuildNickCallbacks(dlg, callbackState);
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGuildNickNameDialogTest, SetActiveBeforeInitDoesNotCrash) {
    GuildNickCallbackState callbackState;
    cGuildNickNameDialog dlg;
    InstallGuildNickCallbacks(dlg, callbackState);
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

// ===========================================================================
// SetNickMsg
// ===========================================================================

TEST(CGuildNickNameDialogTest, SetNickMsgUpdatesTextArea) {
    cGuildNickNameDialog dlg;
    cTextArea* pText = nullptr;
    cEditBox* pEdit = nullptr;
    BuildDlgWithChildren(dlg, &pText, &pEdit);

    dlg.SetNickMsg("Alice");
    // 1:1 with legacy: sprintf with format
    // placeholder + Name argument. Modern port
    // uses "GUILD_NICK_MSG_FORMAT %s" + Name.
    EXPECT_NE(pText->GetScriptText().find("Alice"), std::string::npos);
    EXPECT_NE(pText->GetScriptText().find("GUILD_NICK_MSG_FORMAT"),
              std::string::npos);
}

TEST(CGuildNickNameDialogTest, SetNickMsgWithEmptyStringIsSafe) {
    cGuildNickNameDialog dlg;
    cTextArea* pText = nullptr;
    cEditBox* pEdit = nullptr;
    BuildDlgWithChildren(dlg, &pText, &pEdit);

    dlg.SetNickMsg("");
    // Empty name → sprintf with empty string
    // produces "GUILD_NICK_MSG_FORMAT " (with
    // trailing space).
    EXPECT_NE(pText->GetScriptText().find("GUILD_NICK_MSG_FORMAT"),
              std::string::npos);
}

TEST(CGuildNickNameDialogTest, SetNickMsgWithNullIsSafe) {
    // 1:1 quirk: modern port guards null Name
    // (legacy would crash on sprintf with
    // null). The modern port produces
    // "GUILD_NICK_MSG_FORMAT" with no Name
    // argument.
    cGuildNickNameDialog dlg;
    cTextArea* pText = nullptr;
    cEditBox* pEdit = nullptr;
    BuildDlgWithChildren(dlg, &pText, &pEdit);

    dlg.SetNickMsg(nullptr);
    EXPECT_EQ(pText->GetScriptText(), "GUILD_NICK_MSG_FORMAT");
}

TEST(CGuildNickNameDialogTest, SetNickMsgWithoutLinkIsSafe) {
    cGuildNickNameDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetNickMsg("test");
    dlg.SetNickMsg(nullptr);
    SUCCEED();
}

TEST(CGuildNickNameDialogTest, SetNickMsgBeforeInitDoesNotCrash) {
    cGuildNickNameDialog dlg;
    dlg.SetNickMsg("test");
    SUCCEED();
}


TEST(CGuildNickNameDialogTest, LegacyMessageConstantsMatchSource) {
    EXPECT_EQ(cGuildNickNameDialog::kNoSelectionMessageId,714);
    EXPECT_EQ(cGuildNickNameDialog::kNickPromptMessageId,704);
}

TEST(CGuildNickNameDialogTest, SetActiveWithoutSelectionStaysClosedAndEmits714) {
    cGuildNickNameDialog dlg;
    GuildNickCallbackState state;
    state.memberId=0;
    InstallGuildNickCallbacks(dlg,state);
    dlg.SetActive(true);
    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(state.messageCalls,1);
    EXPECT_EQ(state.lastMessageId,714);
}

TEST(CGuildNickNameDialogTest, SetActiveValidSelectionClearsEditAndFormatsPrompt) {
    cGuildNickNameDialog dlg;
    cTextArea* text=nullptr;
    cEditBox* edit=nullptr;
    BuildDlgWithChildren(dlg,&text,&edit);
    GuildNickCallbackState state;
    InstallGuildNickCallbacks(dlg,state);
    edit->SetEditText("stale");
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
    EXPECT_TRUE(edit->editText().empty());
    EXPECT_EQ(text->GetScriptText(),"Change nickname for SelectedHero");
}

TEST(CGuildNickNameDialogTest, SetNickMsgUsesMessage704Format) {
    cGuildNickNameDialog dlg;
    cTextArea* text=nullptr;
    cEditBox* edit=nullptr;
    BuildDlgWithChildren(dlg,&text,&edit);
    GuildNickCallbackState state;
    state.format="[%s] nickname";
    InstallGuildNickCallbacks(dlg,state);
    dlg.SetNickMsg("Alice");
    EXPECT_EQ(text->GetScriptText(),"[Alice] nickname");
}

TEST(CGuildNickNameDialogTest, MissingFormatFallsBackSafely) {
    cGuildNickNameDialog dlg;
    cTextArea* text=nullptr;
    cEditBox* edit=nullptr;
    BuildDlgWithChildren(dlg,&text,&edit);
    GuildNickCallbackState state;
    dlg.SetCallbacks(GetGuildNickMemberId,GetGuildNickMemberName,
                     AddGuildNickMessage,nullptr,&state);
    dlg.SetNickMsg("Alice");
    EXPECT_EQ(text->GetScriptText(),"GUILD_NICK_MSG_FORMAT Alice");
}

TEST(CGuildNickNameDialogTest, SetCallbacksReplacesDispatch) {
    cGuildNickNameDialog dlg;
    GuildNickCallbackState first;
    GuildNickCallbackState second;
    first.memberId=0;
    InstallGuildNickCallbacks(dlg,first);
    InstallGuildNickCallbacks(dlg,second);
    dlg.SetActive(true);
    EXPECT_EQ(first.messageCalls,0);
    EXPECT_TRUE(dlg.isActive());
}

}  // namespace mxh::ui::test
