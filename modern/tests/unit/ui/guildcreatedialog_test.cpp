// guildcreatedialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cGuildCreateDialog +
// cGuildUnionCreateDialog (guild create + guild union create
// dialogs).
//
// Covers modern/src/ui/guildcreatedialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\GuildCreateDialog.h (679 B).
//
// What's tested (cGuildCreateDialog):
//   - Default construction: 5 child pointers are null.
//   - Linking resolves the 5 children (cStatic + cEditBox
//     + cTextArea + cButton + cStatic) by id 280-284.
//   - SetActive override calls base SetActive (7-singleton
//     dispatch TODO).
//   - SetMunpaName sets the edit text + read-only mode
//     (1:1 quirk: legacy sets m_pGuildName->SetReadOnly(TRUE)).
//   - SetMunpaIntro sets the text area script text.
//   - SetMunpaName / SetMunpaIntro with null is safe.
//   - SetMunpaName / SetMunpaIntro without linked children
//     is safe.
//   - Accessors return the linked child pointers.
//
// What's tested (cGuildUnionCreateDialog):
//   - Default construction: 3 child pointers are null.
//   - Linking resolves the 3 children (cEditBox + cButton
//     + cTextArea) by id 290-292.
//   - Linking calls SetScriptText on the cTextArea with
//     placeholder text "GUILD_UNION_TEXT" (1:1 quirk:
//     legacy uses CHATMGR->GetChatMsg(1125)).
//   - SetActive override calls base SetActive (4-singleton
//     dispatch TODO).
//   - Accessors return the linked child pointers.
//
// 1:1 quirks preserved:
//   - Ctor drops m_type = WT_GUILDCREATEDLG /
//     WT_GUILDUNIONCREATEDLG (legacy cWindow type tag
//     removed in Phase 6).
//   - Linking's SetScriptText call uses placeholder
//     text "GUILD_UNION_TEXT" until CHATMGR is ported.
//   - SetMunpaName also sets read-only on the edit
//     box (1:1 with legacy m_pGuildName->SetReadOnly(TRUE)).
//   - SetActive matches base noexcept (R-12 polymorphic
//     virtual required).
//   - The 7-singleton dispatch in cGuildCreateDialog::SetActive
//     is documented as TODO (MAP / HERO / GUILDMGR /
//     GAMEIN / RESRCMGR / OBJECTSTATEMGR / OBJECTSTATE).
//   - The 4-singleton dispatch in
//     cGuildUnionCreateDialog::SetActive is documented as TODO.

#include "guildcreatedialog.hpp"
#include "cstatic.hpp"
#include "ceditbox.hpp"
#include "ctextarea.hpp"
#include "cbutton.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// cGuildCreateDialog — Construction
// ===========================================================================

TEST(CGuildCreateDialogTest, DefaultConstructionHasNullPointers) {
    cGuildCreateDialog dlg;
    EXPECT_EQ(dlg.GetLocation(),    nullptr);
    EXPECT_EQ(dlg.GetGuildName(),   nullptr);
    EXPECT_EQ(dlg.GetIntro(),       nullptr);
    EXPECT_EQ(dlg.GetOkButton(),    nullptr);
    EXPECT_EQ(dlg.GetCaptionName(), nullptr);
}

// ===========================================================================
// cGuildCreateDialog — Id constants
// ===========================================================================

TEST(CGuildCreateDialogTest, IdConstantsAreDistinct) {
    // 1:1 quirk: pick 280-284 to avoid collisions with
    // other Tier 2 dialog id ranges (cCharMakeDlg 200-203,
    // cGuildJoinDialog 210-212, cCharStateDialog 220-224,
    // cSOSDialog 230-231, cMiniFriendDialog 240-243,
    // cReviveDialog 250-252, cMPNoticeDialog 260-261,
    // cEventNotifyDialog 270-271).
    EXPECT_EQ(cGuildCreateDialog::kLocationId,    280);
    EXPECT_EQ(cGuildCreateDialog::kGuildNameId,   281);
    EXPECT_EQ(cGuildCreateDialog::kIntroId,       282);
    EXPECT_EQ(cGuildCreateDialog::kOkBtnId,       283);
    EXPECT_EQ(cGuildCreateDialog::kCaptionNameId, 284);

    // All distinct.
    EXPECT_NE(cGuildCreateDialog::kLocationId,    cGuildCreateDialog::kGuildNameId);
    EXPECT_NE(cGuildCreateDialog::kLocationId,    cGuildCreateDialog::kIntroId);
    EXPECT_NE(cGuildCreateDialog::kGuildNameId,   cGuildCreateDialog::kIntroId);
    EXPECT_NE(cGuildCreateDialog::kIntroId,       cGuildCreateDialog::kOkBtnId);
    EXPECT_NE(cGuildCreateDialog::kOkBtnId,       cGuildCreateDialog::kCaptionNameId);
}

// ===========================================================================
// cGuildCreateDialog — Linking
// ===========================================================================

namespace {

// Build a cGuildCreateDialog with 5 children wired in
// the modern id range (280-284). Returns the raw
// pointers via the out struct; ownership lives in
// the dlg (children are added via cWindow::Add).
struct GuildCreateChildren {
    cStatic*  location    = nullptr;
    cEditBox* guild_name  = nullptr;
    cTextArea* intro      = nullptr;
    cButton*  ok_btn     = nullptr;
    cStatic*  caption    = nullptr;
};

void BuildDlgWithChildren(cGuildCreateDialog& dlg, GuildCreateChildren& out) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto location = std::make_unique<cStatic>();
    location->Init(0, 0, 200, 14, nullptr,
                   cGuildCreateDialog::kLocationId);
    out.location = location.get();
    dlg.Add(std::unique_ptr<cWindow>(location.release()));

    auto name = std::make_unique<cEditBox>();
    name->Init(0, 0, 200, 14, nullptr, nullptr,
               cGuildCreateDialog::kGuildNameId);
    name->InitEditbox(50, 64);
    out.guild_name = name.get();
    dlg.Add(std::unique_ptr<cWindow>(name.release()));

    auto intro = std::make_unique<cTextArea>();
    intro->Init(0, 0, 200, 100, nullptr,
                cGuildCreateDialog::kIntroId);
    intro->InitTextArea({0, 0, 200, 100}, 256);
    out.intro = intro.get();
    dlg.Add(std::unique_ptr<cWindow>(intro.release()));

    auto ok = std::make_unique<cButton>();
    ok->Init(0, 0, 50, 14, nullptr, nullptr, nullptr,
             nullptr, nullptr, cGuildCreateDialog::kOkBtnId);
    out.ok_btn = ok.get();
    dlg.Add(std::unique_ptr<cWindow>(ok.release()));

    auto cap = std::make_unique<cStatic>();
    cap->Init(0, 0, 200, 14, nullptr,
              cGuildCreateDialog::kCaptionNameId);
    out.caption = cap.get();
    dlg.Add(std::unique_ptr<cWindow>(cap.release()));

    dlg.Linking();
}

}  // namespace

TEST(CGuildCreateDialogTest, LinkingResolvesAllFiveChildren) {
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);

    EXPECT_EQ(dlg.GetLocation(),    raws.location);
    EXPECT_EQ(dlg.GetGuildName(),   raws.guild_name);
    EXPECT_EQ(dlg.GetIntro(),       raws.intro);
    EXPECT_EQ(dlg.GetOkButton(),    raws.ok_btn);
    EXPECT_EQ(dlg.GetCaptionName(), raws.caption);
}

TEST(CGuildCreateDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cGuildCreateDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetLocation(),    nullptr);
    EXPECT_EQ(dlg.GetGuildName(),   nullptr);
    EXPECT_EQ(dlg.GetIntro(),       nullptr);
    EXPECT_EQ(dlg.GetOkButton(),    nullptr);
    EXPECT_EQ(dlg.GetCaptionName(), nullptr);
}

// ===========================================================================
// cGuildCreateDialog — SetActive (1:1 override, base + TODO)
// ===========================================================================

TEST(CGuildCreateDialogTest, SetActiveTrueUpdatesBaseState) {
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    EXPECT_FALSE(dlg.isActive());

    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildCreateDialogTest, SetActiveFalseUpdatesBaseState) {
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.SetActive(true);
    ASSERT_TRUE(dlg.isActive());

    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGuildCreateDialogTest, SetActiveWithoutLinksIsSafe) {
    cGuildCreateDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

// ===========================================================================
// cGuildCreateDialog — SetMunpaName / SetMunpaIntro
// ===========================================================================

TEST(CGuildCreateDialogTest, SetMunpaNameUpdatesEditTextAndReadOnly) {
    // 1:1 quirk: legacy SetMunpaName sets the edit
    // text + sets the edit box read-only.
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    ASSERT_NE(dlg.GetGuildName(), nullptr);

    EXPECT_FALSE(dlg.GetGuildName()->IsReadOnly());
    dlg.SetMunpaName("My Guild");
    EXPECT_STREQ(dlg.GetGuildName()->editText().c_str(), "My Guild");
    EXPECT_TRUE(dlg.GetGuildName()->IsReadOnly());
}

TEST(CGuildCreateDialogTest, SetMunpaNameWithEmptyStringIsSafe) {
    // 1:1 quirk: SetMunpaName with nullptr would be
    // UB in modern cEditBox::SetEditText (which
    // takes std::string). SetMunpaName with "" is
    // the safe equivalent.
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.SetMunpaName("");
    EXPECT_STREQ(dlg.GetGuildName()->editText().c_str(), "");
    EXPECT_TRUE(dlg.GetGuildName()->IsReadOnly());
}

TEST(CGuildCreateDialogTest, SetMunpaNameWithoutLinkIsSafe) {
    cGuildCreateDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetMunpaName("no edit attached");
    SUCCEED();
}

TEST(CGuildCreateDialogTest, SetMunpaIntroUpdatesScriptText) {
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    ASSERT_NE(dlg.GetIntro(), nullptr);

    dlg.SetMunpaIntro("Welcome to our guild!");
    EXPECT_STREQ(dlg.GetIntro()->GetScriptText().c_str(),
                 "Welcome to our guild!");
}

TEST(CGuildCreateDialogTest, SetMunpaIntroWithNullClearsText) {
    // 1:1 quirk: SetMunpaIntro with nullptr calls
    // m_pIntro->SetScriptText(nullptr) which clears
    // the text.
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.SetMunpaIntro("initial text");
    ASSERT_STREQ(dlg.GetIntro()->GetScriptText().c_str(), "initial text");

    dlg.SetMunpaIntro(nullptr);
    EXPECT_STREQ(dlg.GetIntro()->GetScriptText().c_str(), "");
}

TEST(CGuildCreateDialogTest, SetMunpaIntroWithoutLinkIsSafe) {
    cGuildCreateDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetMunpaIntro("no intro attached");
    SUCCEED();
}

// ===========================================================================
// cGuildUnionCreateDialog — Construction
// ===========================================================================

TEST(CGuildUnionCreateDialogTest, DefaultConstructionHasNullPointers) {
    cGuildUnionCreateDialog dlg;
    EXPECT_EQ(dlg.GetNameEdit(), nullptr);
    EXPECT_EQ(dlg.GetOkButton(), nullptr);
    EXPECT_EQ(dlg.GetText(),     nullptr);
}

// ===========================================================================
// cGuildUnionCreateDialog — Id constants
// ===========================================================================

TEST(CGuildUnionCreateDialogTest, IdConstantsAreDistinct) {
    // 1:1 quirk: pick 290-292 to avoid collisions with
    // cGuildCreateDialog (280-284) and other dialogs.
    EXPECT_EQ(cGuildUnionCreateDialog::kNameEditId, 290);
    EXPECT_EQ(cGuildUnionCreateDialog::kOkBtnId,   291);
    EXPECT_EQ(cGuildUnionCreateDialog::kTextId,     292);

    EXPECT_NE(cGuildUnionCreateDialog::kNameEditId,
              cGuildUnionCreateDialog::kOkBtnId);
    EXPECT_NE(cGuildUnionCreateDialog::kNameEditId,
              cGuildUnionCreateDialog::kTextId);
    EXPECT_NE(cGuildUnionCreateDialog::kOkBtnId,
              cGuildUnionCreateDialog::kTextId);
}

// ===========================================================================
// cGuildUnionCreateDialog — Linking
// ===========================================================================

namespace {

// Build a cGuildUnionCreateDialog with 3 children wired
// in the modern id range (290-292). Returns raw
// pointers via the out struct.
struct GuildUnionChildren {
    cEditBox* name_edit = nullptr;
    cButton*  ok_btn    = nullptr;
    cTextArea* text     = nullptr;
};

void BuildUnionDlgWithChildren(cGuildUnionCreateDialog& dlg,
                               GuildUnionChildren& out) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto name = std::make_unique<cEditBox>();
    name->Init(0, 0, 200, 14, nullptr, nullptr,
               cGuildUnionCreateDialog::kNameEditId);
    name->InitEditbox(50, 64);
    out.name_edit = name.get();
    dlg.Add(std::unique_ptr<cWindow>(name.release()));

    auto ok = std::make_unique<cButton>();
    ok->Init(0, 0, 50, 14, nullptr, nullptr, nullptr,
             nullptr, nullptr, cGuildUnionCreateDialog::kOkBtnId);
    out.ok_btn = ok.get();
    dlg.Add(std::unique_ptr<cWindow>(ok.release()));

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 200, 100, nullptr,
               cGuildUnionCreateDialog::kTextId);
    text->InitTextArea({0, 0, 200, 100}, 256);
    out.text = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));

    dlg.Linking();
}

}  // namespace

TEST(CGuildUnionCreateDialogTest, LinkingResolvesAllThreeChildren) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);

    EXPECT_EQ(dlg.GetNameEdit(), raws.name_edit);
    EXPECT_EQ(dlg.GetOkButton(), raws.ok_btn);
    EXPECT_EQ(dlg.GetText(),     raws.text);
}

TEST(CGuildUnionCreateDialogTest, LinkingCallsSetScriptText) {
    // 1:1 quirk: legacy calls
    //   m_pText->SetScriptText(CHATMGR->GetChatMsg(1125))
    // Modern port uses placeholder text
    // "GUILD_UNION_TEXT" until CHATMGR is ported.
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    ASSERT_NE(dlg.GetText(), nullptr);
    EXPECT_STREQ(dlg.GetText()->GetScriptText().c_str(),
                 "GUILD_UNION_TEXT");
}

TEST(CGuildUnionCreateDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cGuildUnionCreateDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetNameEdit(), nullptr);
    EXPECT_EQ(dlg.GetOkButton(), nullptr);
    EXPECT_EQ(dlg.GetText(),     nullptr);
}

// ===========================================================================
// cGuildUnionCreateDialog — SetActive
// ===========================================================================

TEST(CGuildUnionCreateDialogTest, SetActiveTrueUpdatesBaseState) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    EXPECT_FALSE(dlg.isActive());

    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildUnionCreateDialogTest, SetActiveFalseUpdatesBaseState) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    dlg.SetActive(true);
    ASSERT_TRUE(dlg.isActive());

    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGuildUnionCreateDialogTest, SetActiveWithoutLinksIsSafe) {
    cGuildUnionCreateDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

}  // namespace mxh::ui::test
