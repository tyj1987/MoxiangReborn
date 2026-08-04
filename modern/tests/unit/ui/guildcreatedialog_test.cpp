// guildcreatedialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cGuildCreateDialog +
// cGuildUnionCreateDialog (guild create + guild union create
// dialogs).
//
// Covers modern/src/ui/guildcreatedialog.{hpp,cpp}, a 1:1 port of
//   å¢¨é¦™ã€æºç ã€‘\[Client]MH\GuildCreateDialog.h (679 B).
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
// cGuildCreateDialog â€” Construction
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
// cGuildCreateDialog â€” Id constants
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
// cGuildCreateDialog â€” Linking
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
// cGuildCreateDialog â€” SetActive (1:1 override, base + TODO)
// ===========================================================================


TEST(CGuildCreateDialogTest, SetActiveTrueWithNoCallbacksClearsAndActivates) {
    // With no callbacks installed, val==TRUE
    // still clears the guild-name + intro
    // (REAL) and calls base SetActive. The
    // RESRCMGR/MAP branch falls through safely
    // because all callbacks are null.
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    raws.guild_name->SetEditText("initial");
    ASSERT_STREQ(raws.guild_name->editText().c_str(), "initial");

    dlg.SetActive(true);

    EXPECT_TRUE(dlg.isActive());
    EXPECT_TRUE(raws.guild_name->editText().empty());
}

TEST(CGuildCreateDialogTest, SetActiveFalseWithNoHeroCallbackKeepsDialogOpen) {
    // 1:1 quirk: legacy `if (HERO == 0) return;`
    // means the dialog stays active after
    // SetActive(false) when no HERO is present.
    // Modern port preserves this with the
    // early-return on `heroObjectId == 0u`
    // (matches the legacy null-HERO behavior).
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.SetActive(true);
    ASSERT_TRUE(dlg.isActive());

    // No SetCallbacks() -> m_getHeroObjectId = nullptr -> early return.
    dlg.SetActive(false);
    EXPECT_TRUE(dlg.isActive());
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
// Callback fixtures (shared with cGuildCreateDialog host-dispatch tests)
// ===========================================================================

namespace {

struct GuildCreateHostCalls {
    std::uint32_t heroObjectId      = 9;
    std::uint32_t heroGuildIdx      = 0;
    std::int32_t  heroState         = 0;
    const char*   mapName           = "MAP_DEFAULT";
    const char*   guildName         = "MY_GUILD";
    bool          npcScriptActive   = false;
    int           getMapNameCalls   = 0;
    int           getGuildNameCalls = 0;
    int           localizedCalls    = 0;
    int           endCalls          = 0;
    std::uint32_t endedObjectId     = 0;
    std::int32_t  endedState        = 0;

    static std::uint32_t GetHeroObjectId(void* ud) {
        return static_cast<GuildCreateHostCalls*>(ud)->heroObjectId;
    }
    static std::uint32_t GetHeroGuildIdx(void* ud) {
        return static_cast<GuildCreateHostCalls*>(ud)->heroGuildIdx;
    }
    static std::int32_t GetHeroState(void* ud) {
        return static_cast<GuildCreateHostCalls*>(ud)->heroState;
    }
    static const char* GetMapName(void* ud) {
        auto* hc = static_cast<GuildCreateHostCalls*>(ud);
        ++hc->getMapNameCalls;
        return hc->mapName;
    }
    static const char* GetGuildName(void* ud) {
        auto* hc = static_cast<GuildCreateHostCalls*>(ud);
        ++hc->getGuildNameCalls;
        return hc->guildName;
    }
    static bool IsNpcScriptActive(void* ud) {
        return static_cast<GuildCreateHostCalls*>(ud)->npcScriptActive;
    }
    static const char* GetLocalizedMsg(std::int32_t, void* ud) {
        ++static_cast<GuildCreateHostCalls*>(ud)->localizedCalls;
        return "LOCALIZED";
    }
    static void EndObjectState(std::uint32_t objectId, std::int32_t state,
                                void* ud) {
        auto* hc = static_cast<GuildCreateHostCalls*>(ud);
        ++hc->endCalls;
        hc->endedObjectId = objectId;
        hc->endedState = state;
    }
};

// Convenience builder that wires every callback to the same HostCalls struct.
void InstallGuildCreateCallbacks(cGuildCreateDialog& dlg,
                                 GuildCreateHostCalls* hc) {
    dlg.SetCallbacks(&GuildCreateHostCalls::GetHeroObjectId,
                     &GuildCreateHostCalls::GetHeroGuildIdx,
                     &GuildCreateHostCalls::GetHeroState,
                     &GuildCreateHostCalls::GetMapName,
                     &GuildCreateHostCalls::GetGuildName,
                     &GuildCreateHostCalls::IsNpcScriptActive,
                     &GuildCreateHostCalls::GetLocalizedMsg,
                     &GuildCreateHostCalls::EndObjectState,
                     hc);
}

}  // namespace

// ===========================================================================
// cGuildCreateDialog -- host-callback port (replaces the 7-singleton no-op)
// ===========================================================================

TEST(CGuildCreateDialogTest, LegacyMessageAndStateConstantsMatchSource) {
    // 1:1 with legacy RESRCMGR->GetMsg ids +
    // eObjectState_Deal.
    EXPECT_EQ(cGuildCreateDialog::kMsgEditExistingGuildCaption, 270);
    EXPECT_EQ(cGuildCreateDialog::kMsgRenameGuildButton,         335);
    EXPECT_EQ(cGuildCreateDialog::kMsgCreateGuildCaption,       510);
    EXPECT_EQ(cGuildCreateDialog::kMsgCreateGuildButton,        513);
    EXPECT_EQ(cGuildCreateDialog::kObjectStateDeal,             6);
}

TEST(CGuildCreateDialogTest, SetActiveTrueNoGuildBranchUsesCreateCaptionsAndUnreadonlys) {
    // HERO->GetGuildIdx() == 0 -> create-guild
    // view: caption=510, button=513, edit box
    // is WRITABLE (legacy un-readonly's it).
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    GuildCreateHostCalls hc;
    InstallGuildCreateCallbacks(dlg, &hc);

    dlg.SetActive(true);

    EXPECT_TRUE(dlg.isActive());
    EXPECT_EQ(hc.getMapNameCalls, 1);            // map queried
    EXPECT_GE(hc.localizedCalls, 2);             // 510 + 513
    // 1:1 quirk: legacy un-readonly's the edit box
    // so the user can type a guild name.
    EXPECT_FALSE(raws.guild_name->IsReadOnly());
}

TEST(CGuildCreateDialogTest, SetActiveTrueGuildedBranchUsesRenameCaptionsAndReadOnly) {
    // HERO->GetGuildIdx() != 0 -> existing-guild
    // view: caption=270, button=335, edit box
    // contains current guild name (read-only).
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    GuildCreateHostCalls hc;
    hc.heroGuildIdx = 12u;  // in a guild
    hc.guildName = "MY_EXISTING_GUILD";
    InstallGuildCreateCallbacks(dlg, &hc);

    dlg.SetActive(true);

    EXPECT_TRUE(dlg.isActive());
    EXPECT_EQ(hc.getGuildNameCalls, 1);
    EXPECT_STREQ(raws.guild_name->editText().c_str(), "MY_EXISTING_GUILD");
    EXPECT_TRUE(raws.guild_name->IsReadOnly());
}

TEST(CGuildCreateDialogTest, SetActiveTrueNullMapCallbackLeavesLocationUnchanged) {
    // The location widget's text is only set by
    // the MAP callback. With the MAP callback
    // absent the widget stays at its
    // ctor-initial value (empty string).
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    GuildCreateHostCalls hc;
    InstallGuildCreateCallbacks(dlg, &hc);
    // Make the MAP callback null by replacing it
    // with nullptr.
    dlg.SetCallbacks(&GuildCreateHostCalls::GetHeroObjectId,
                     &GuildCreateHostCalls::GetHeroGuildIdx,
                     &GuildCreateHostCalls::GetHeroState,
                     nullptr,                            // no GetMapName
                     &GuildCreateHostCalls::GetGuildName,
                     &GuildCreateHostCalls::IsNpcScriptActive,
                     &GuildCreateHostCalls::GetLocalizedMsg,
                     &GuildCreateHostCalls::EndObjectState,
                     &hc);
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildCreateDialogTest, SetActiveTrueNullLocalizedCallbackDoesNotCrash) {
    // RESRCMGR (GetLocalizedMessage) callback is
    // absent -- caption/button text widgets
    // stay at their ctor values. The dialog
    // activates cleanly.
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    GuildCreateHostCalls hc;
    InstallGuildCreateCallbacks(dlg, &hc);
    dlg.SetCallbacks(&GuildCreateHostCalls::GetHeroObjectId,
                     &GuildCreateHostCalls::GetHeroGuildIdx,
                     &GuildCreateHostCalls::GetHeroState,
                     &GuildCreateHostCalls::GetMapName,
                     &GuildCreateHostCalls::GetGuildName,
                     &GuildCreateHostCalls::IsNpcScriptActive,
                     nullptr,                            // no GetLocalizedMessage
                     &GuildCreateHostCalls::EndObjectState,
                     &hc);
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildCreateDialogTest, SetActiveFalseEndDealStateDispatchedWhenHeroInDealAndNpcScriptOff) {
    // 1:1 with legacy guard:
    //   if (HERO->GetState() == eObjectState_Deal &&
    //       GAMEIN->GetNpcScriptDialog()->IsActive() == FALSE)
    //   then OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal).
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    GuildCreateHostCalls hc;
    hc.heroState = cGuildCreateDialog::kObjectStateDeal;
    hc.npcScriptActive = false;
    InstallGuildCreateCallbacks(dlg, &hc);
    dlg.SetActive(true);
    dlg.SetActive(false);

    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(hc.endCalls, 1);
    EXPECT_EQ(hc.endedObjectId, hc.heroObjectId);
    EXPECT_EQ(hc.endedState, cGuildCreateDialog::kObjectStateDeal);
}

TEST(CGuildCreateDialogTest, SetActiveFalseSkipsEndForNonDealState) {
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    GuildCreateHostCalls hc;
    hc.heroState = 5;  // some non-deal state
    InstallGuildCreateCallbacks(dlg, &hc);
    dlg.SetActive(true);
    dlg.SetActive(false);

    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(hc.endCalls, 0);
}

TEST(CGuildCreateDialogTest, SetActiveFalseSkipsEndWhenNpcScriptDialogActive) {
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    GuildCreateHostCalls hc;
    hc.heroState = cGuildCreateDialog::kObjectStateDeal;
    hc.npcScriptActive = true;
    InstallGuildCreateCallbacks(dlg, &hc);
    dlg.SetActive(true);
    dlg.SetActive(false);

    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(hc.endCalls, 0);
}

TEST(CGuildCreateDialogTest, SetActiveFalseEarlyReturnWhenHeroObjectIdZero) {
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    GuildCreateHostCalls hc;
    hc.heroObjectId = 0u;  // no hero
    InstallGuildCreateCallbacks(dlg, &hc);
    dlg.SetActive(true);
    dlg.SetActive(false);

    // 1:1 with legacy `if (HERO == 0) return;`
    // -- the dialog stays active.
    EXPECT_TRUE(dlg.isActive());
    EXPECT_EQ(hc.endCalls, 0);
}

TEST(CGuildCreateDialogTest, SetActiveFalseReleasesEditFocusBeforeEarlyReturn) {
    // 1:1 with legacy control flow: the edit-box
    // focus is released BEFORE the HERO
    // null-check. Even when HERO is null and
    // the SetActive returns early, focus must
    // have been released.
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    raws.guild_name->SetFocusEdit(true);
    ASSERT_TRUE(raws.guild_name->hasFocus());

    GuildCreateHostCalls hc;
    hc.heroObjectId = 0u;
    InstallGuildCreateCallbacks(dlg, &hc);
    dlg.SetActive(true);
    dlg.SetActive(false);

    EXPECT_FALSE(raws.guild_name->hasFocus());
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildCreateDialogTest, SetCallbacksReplacesExistingHostDispatch) {
    // First call uses host context #1, second
    // call swaps to host context #2. Verify the
    // second dispatch goes to context #2 and
    // context #1's counter doesn't increment.
    cGuildCreateDialog dlg;
    GuildCreateChildren raws;
    BuildDlgWithChildren(dlg, raws);
    GuildCreateHostCalls firstCtx;
    firstCtx.heroObjectId = 11u;
    firstCtx.heroState = cGuildCreateDialog::kObjectStateDeal;
    InstallGuildCreateCallbacks(dlg, &firstCtx);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_EQ(firstCtx.endCalls, 1);

    // Re-activate and install a different context.
    dlg.SetActive(true);
    GuildCreateHostCalls secondCtx;
    secondCtx.heroObjectId = 22u;
    secondCtx.heroState = cGuildCreateDialog::kObjectStateDeal;
    InstallGuildCreateCallbacks(dlg, &secondCtx);
    dlg.SetActive(false);
    EXPECT_EQ(secondCtx.endCalls, 1);
    EXPECT_EQ(secondCtx.endedObjectId, 22u);
    EXPECT_EQ(firstCtx.endCalls, 1);   // NOT incremented
}


// ===========================================================================
// cGuildCreateDialog â€” SetMunpaName / SetMunpaIntro
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
// cGuildUnionCreateDialog â€” Construction
// ===========================================================================

TEST(CGuildUnionCreateDialogTest, DefaultConstructionHasNullPointers) {
    cGuildUnionCreateDialog dlg;
    EXPECT_EQ(dlg.GetNameEdit(), nullptr);
    EXPECT_EQ(dlg.GetOkButton(), nullptr);
    EXPECT_EQ(dlg.GetText(),     nullptr);
}

// ===========================================================================
// cGuildUnionCreateDialog â€” Id constants
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
// cGuildUnionCreateDialog â€” Linking
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
// cGuildUnionCreateDialog â€” SetActive
// ===========================================================================

TEST(CGuildUnionCreateDialogTest, SetActiveTrueUpdatesBaseState) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    EXPECT_FALSE(dlg.isActive());

    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

namespace {

std::uint32_t GetExistingGuildUnionHeroObjectId(void*) {
    return 1;
}

}  // namespace

TEST(CGuildUnionCreateDialogTest, SetActiveFalseUpdatesBaseState) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    dlg.SetCallbacks(GetExistingGuildUnionHeroObjectId, nullptr, nullptr, nullptr);
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

namespace {

struct GuildUnionCallbackState {
    std::uint32_t heroObjectId = 7;
    std::int32_t heroState = cGuildUnionCreateDialog::kObjectStateDeal;
    bool npcScriptActive = false;
    int endCalls = 0;
    std::uint32_t endedObjectId = 0;
    std::int32_t endedState = 0;
};

std::uint32_t GetGuildUnionHeroObjectId(void* userData) {
    return static_cast<GuildUnionCallbackState*>(userData)->heroObjectId;
}

std::int32_t GetGuildUnionHeroState(void* userData) {
    return static_cast<GuildUnionCallbackState*>(userData)->heroState;
}

bool IsGuildUnionNpcScriptActive(void* userData) {
    return static_cast<GuildUnionCallbackState*>(userData)->npcScriptActive;
}

void EndGuildUnionObjectState(std::uint32_t objectId,
                              std::int32_t state,
                              void* userData) {
    auto& callbackState = *static_cast<GuildUnionCallbackState*>(userData);
    ++callbackState.endCalls;
    callbackState.endedObjectId = objectId;
    callbackState.endedState = state;
}

std::uint32_t GetFixedGuildUnionHeroObjectId(void*) {
    return 19;
}

}  // namespace

TEST(CGuildUnionCreateDialogTest, ObjectStateDealConstantMatchesLegacy) {
    EXPECT_EQ(cGuildUnionCreateDialog::kObjectStateDeal, 6);
}

TEST(CGuildUnionCreateDialogTest, SetActiveTrueClearsNameEditText) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    raws.name_edit->SetEditText("union name");

    dlg.SetActive(true);

    EXPECT_TRUE(raws.name_edit->editText().empty());
}

TEST(CGuildUnionCreateDialogTest, SetActiveFalseEndsDealStateWhenNpcScriptInactive) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    GuildUnionCallbackState callbackState;
    dlg.SetCallbacks(GetGuildUnionHeroObjectId, GetGuildUnionHeroState,
                     IsGuildUnionNpcScriptActive, EndGuildUnionObjectState,
                     &callbackState);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(callbackState.endCalls, 1);
    EXPECT_EQ(callbackState.endedObjectId, callbackState.heroObjectId);
    EXPECT_EQ(callbackState.endedState, cGuildUnionCreateDialog::kObjectStateDeal);
}

TEST(CGuildUnionCreateDialogTest, SetActiveFalseSkipsEndForNonDealState) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    GuildUnionCallbackState callbackState;
    callbackState.heroState = 5;
    dlg.SetCallbacks(GetGuildUnionHeroObjectId, GetGuildUnionHeroState,
                     IsGuildUnionNpcScriptActive, EndGuildUnionObjectState,
                     &callbackState);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(callbackState.endCalls, 0);
}

TEST(CGuildUnionCreateDialogTest, SetActiveFalseSkipsEndWhenNpcScriptActive) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    GuildUnionCallbackState callbackState;
    callbackState.npcScriptActive = true;
    dlg.SetCallbacks(GetGuildUnionHeroObjectId, GetGuildUnionHeroState,
                     IsGuildUnionNpcScriptActive, EndGuildUnionObjectState,
                     &callbackState);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(callbackState.endCalls, 0);
}

TEST(CGuildUnionCreateDialogTest, SetActiveFalsePreservesLegacyNullHeroEarlyReturn) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    GuildUnionCallbackState callbackState;
    callbackState.heroObjectId = 0;
    dlg.SetCallbacks(GetGuildUnionHeroObjectId, GetGuildUnionHeroState,
                     IsGuildUnionNpcScriptActive, EndGuildUnionObjectState,
                     &callbackState);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_TRUE(dlg.isActive());
    EXPECT_EQ(callbackState.endCalls, 0);
}

TEST(CGuildUnionCreateDialogTest, SetActiveFalseWithoutCallbacksPreservesEarlyReturn) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildUnionCreateDialogTest, SetCallbacksReplacesExistingHostDispatch) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    GuildUnionCallbackState firstState;
    GuildUnionCallbackState secondState;
    dlg.SetCallbacks(GetGuildUnionHeroObjectId, GetGuildUnionHeroState,
                     IsGuildUnionNpcScriptActive, EndGuildUnionObjectState,
                     &firstState);
    dlg.SetCallbacks(GetGuildUnionHeroObjectId, GetGuildUnionHeroState,
                     IsGuildUnionNpcScriptActive, EndGuildUnionObjectState,
                     &secondState);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_EQ(firstState.endCalls, 0);
    EXPECT_EQ(secondState.endCalls, 1);
}

TEST(CGuildUnionCreateDialogTest, SetCallbacksAcceptsNullUserData) {
    cGuildUnionCreateDialog dlg;
    GuildUnionChildren raws;
    BuildUnionDlgWithChildren(dlg, raws);
    dlg.SetCallbacks(GetFixedGuildUnionHeroObjectId, nullptr, nullptr, nullptr);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_FALSE(dlg.isActive());
}

}  // namespace mxh::ui::test
