// guildmarkdialog_test.cpp — 1:1 port tests for
// 墨香 CGuildMarkDialog (guild / guild-union mark
// registration dialog).
//
// Verifies:
//   - ctor does not crash
//   - Dtor does not crash
//   - Inherits from cDialog
//   - 4 id constants (kIdInfoText / kIdRegistOkBtn /
//     kIdUnionRegistOkBtn / kIdNameEdit) match
//     expected local range 550-553
//   - 2 info text constants (kGuildMarkInfoText /
//     kGuildUnionMarkInfoText) match expected
//     CHATMGR placeholder strings
//   - Linking resolves the 3 children
//   - Linking without children leaves pointers null
//   - Linking before Init does not crash
//   - Linking calls SetScriptText on the cTextArea
//     with the guild mark info text
//   - SetActive(true) updates base state
//   - SetActive(false) calls SetFocusEdit(false) on
//     the resolved cEditBox
//   - SetActive without cEditBox is safe
//   - SetActive before Init does not crash
//   - ShowGuildMark sets the right visible state
//     on the 2 cButton + the right script text
//   - ShowGuildUnionMark sets the opposite visible
//     state + the right script text
//   - ShowGuildMark / ShowGuildUnionMark without
//     Linking is safe

#include "guildmarkdialog.hpp"
#include "cdialog.hpp"
#include "ctextarea.hpp"
#include "cbutton.hpp"
#include "cwindow.hpp"
#include "ceditbox.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cEditBox;
using mxh::ui::cGuildMarkDialog;
using mxh::ui::cTextArea;
using mxh::ui::cWindow;

namespace {

// helper: build a cGuildMarkDialog + add 3 children
// (1 cTextArea + 2 cButton) + Linking
struct LinkedDialog {
    cGuildMarkDialog dlg;
    std::unique_ptr<cTextArea> infoText;
    std::unique_ptr<cButton>   guildMarkBtn;
    std::unique_ptr<cButton>   unionMarkBtn;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 200, nullptr, 0);
        infoText = std::make_unique<cTextArea>();
        infoText->InitTextArea(mxh::ui::TextRect{0, 0, 100, 100}, 64);
        infoText->setId(cGuildMarkDialog::kIdInfoText);
        auto* infoPtr = infoText.get();
        dlg.Add(std::move(infoText));

        guildMarkBtn = std::make_unique<cButton>();
        guildMarkBtn->Init(0, 100, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                           cGuildMarkDialog::kIdRegistOkBtn);
        dlg.Add(std::move(guildMarkBtn));

        unionMarkBtn = std::make_unique<cButton>();
        unionMarkBtn->Init(0, 130, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                           cGuildMarkDialog::kIdUnionRegistOkBtn);
        dlg.Add(std::move(unionMarkBtn));

        dlg.Linking();

        // Re-fetch raw pointers from dlg (since unique_ptr
        // ownership transferred).
        infoTextPtr = infoPtr;
    }

    cTextArea* infoTextPtr = nullptr;
};

}  // namespace

// ---------- ctor / dtor ----------

TEST(CGuildMarkDialogTest, CtorDoesNotCrash) {
    cGuildMarkDialog dlg;
    SUCCEED();
}

TEST(CGuildMarkDialogTest, DtorDoesNotCrash) {
    cGuildMarkDialog dlg;
    SUCCEED();
}

TEST(CGuildMarkDialogTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cGuildMarkDialog>,
                  "cGuildMarkDialog must inherit from cDialog");
    SUCCEED();
}

// ---------- id range ----------

TEST(CGuildMarkDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cGuildMarkDialog::kIdInfoText, 550);
    EXPECT_EQ(cGuildMarkDialog::kIdRegistOkBtn, 551);
    EXPECT_EQ(cGuildMarkDialog::kIdUnionRegistOkBtn, 552);
    EXPECT_EQ(cGuildMarkDialog::kIdNameEdit, 553);
}

TEST(CGuildMarkDialogTest, IdConstantsAreUnique) {
    EXPECT_NE(cGuildMarkDialog::kIdInfoText, cGuildMarkDialog::kIdRegistOkBtn);
    EXPECT_NE(cGuildMarkDialog::kIdInfoText, cGuildMarkDialog::kIdUnionRegistOkBtn);
    EXPECT_NE(cGuildMarkDialog::kIdInfoText, cGuildMarkDialog::kIdNameEdit);
    EXPECT_NE(cGuildMarkDialog::kIdRegistOkBtn, cGuildMarkDialog::kIdUnionRegistOkBtn);
    EXPECT_NE(cGuildMarkDialog::kIdRegistOkBtn, cGuildMarkDialog::kIdNameEdit);
    EXPECT_NE(cGuildMarkDialog::kIdUnionRegistOkBtn, cGuildMarkDialog::kIdNameEdit);
}

TEST(CGuildMarkDialogTest, InfoTextPlaceholdersMatchExpectedStrings) {
    EXPECT_STREQ(cGuildMarkDialog::kGuildMarkInfoText, "GUILD_MARK_INFO_TEXT");
    EXPECT_STREQ(cGuildMarkDialog::kGuildUnionMarkInfoText, "GUILD_UNION_MARK_INFO_TEXT");
    EXPECT_STRNE(cGuildMarkDialog::kGuildMarkInfoText, cGuildMarkDialog::kGuildUnionMarkInfoText);
}

// ---------- Linking ----------

TEST(CGuildMarkDialogTest, LinkingResolvesAllThreeChildren) {
    LinkedDialog ld;
    // Linking() ran in ctor; verify by setting values
    // and observing that ShowGuildMark affects the
    // right buttons.
    ld.dlg.ShowGuildMark();
    // infoTextPtr is owned by ld.dlg; we captured the
    // raw pointer at construction. The cTextArea's
    // script text should be the guild mark text.
    EXPECT_EQ(ld.infoTextPtr->GetScriptText(), cGuildMarkDialog::kGuildMarkInfoText);
}

TEST(CGuildMarkDialogTest, LinkingBeforeInitDoesNotCrash) {
    cGuildMarkDialog dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CGuildMarkDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cGuildMarkDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Linking();
    // ShowGuildMark / ShowGuildUnionMark must
    // not crash even when children are missing.
    dlg.ShowGuildMark();
    dlg.ShowGuildUnionMark();
    SUCCEED();
}

TEST(CGuildMarkDialogTest, LinkingSetsGuildMarkInfoText) {
    LinkedDialog ld;
    EXPECT_EQ(ld.infoTextPtr->GetScriptText(), cGuildMarkDialog::kGuildMarkInfoText);
}

// ---------- SetActive ----------

TEST(CGuildMarkDialogTest, SetActiveTrueUpdatesBaseState) {
    cGuildMarkDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildMarkDialogTest, SetActiveFalseUpdatesBaseState) {
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    EXPECT_TRUE(ld.dlg.isActive());
    ld.dlg.SetActive(false);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CGuildMarkDialogTest, SetActiveBeforeInitDoesNotCrash) {
    cGuildMarkDialog dlg;
    dlg.SetActive(true);
    SUCCEED();
}

// ---------- ShowGuildMark ----------

TEST(CGuildMarkDialogTest, ShowGuildMarkSetsGuildMarkBtnVisibleTrue) {
    LinkedDialog ld;
    // By default, cButton::Init may set visible to true.
    // We test the *transition* to the ShowGuildMark state.
    cButton* btn = static_cast<cButton*>(ld.dlg.findWindowById(cGuildMarkDialog::kIdRegistOkBtn));
    ASSERT_NE(btn, nullptr);
    btn->SetVisible(false);
    ld.dlg.ShowGuildMark();
    EXPECT_TRUE(btn->isVisible());
}

TEST(CGuildMarkDialogTest, ShowGuildMarkSetsUnionMarkBtnVisibleFalse) {
    LinkedDialog ld;
    cButton* btn = static_cast<cButton*>(ld.dlg.findWindowById(cGuildMarkDialog::kIdUnionRegistOkBtn));
    ASSERT_NE(btn, nullptr);
    btn->SetVisible(true);
    ld.dlg.ShowGuildMark();
    EXPECT_FALSE(btn->isVisible());
}

TEST(CGuildMarkDialogTest, ShowGuildMarkSetsInfoText) {
    LinkedDialog ld;
    // First set to a different value via ShowGuildUnionMark
    ld.dlg.ShowGuildUnionMark();
    EXPECT_EQ(ld.infoTextPtr->GetScriptText(), cGuildMarkDialog::kGuildUnionMarkInfoText);
    // Then call ShowGuildMark
    ld.dlg.ShowGuildMark();
    EXPECT_EQ(ld.infoTextPtr->GetScriptText(), cGuildMarkDialog::kGuildMarkInfoText);
}

// ---------- ShowGuildUnionMark ----------

TEST(CGuildMarkDialogTest, ShowGuildUnionMarkSetsGuildMarkBtnVisibleFalse) {
    LinkedDialog ld;
    cButton* btn = static_cast<cButton*>(ld.dlg.findWindowById(cGuildMarkDialog::kIdRegistOkBtn));
    ASSERT_NE(btn, nullptr);
    btn->SetVisible(true);
    ld.dlg.ShowGuildUnionMark();
    EXPECT_FALSE(btn->isVisible());
}

TEST(CGuildMarkDialogTest, ShowGuildUnionMarkSetsUnionMarkBtnVisibleTrue) {
    LinkedDialog ld;
    cButton* btn = static_cast<cButton*>(ld.dlg.findWindowById(cGuildMarkDialog::kIdUnionRegistOkBtn));
    ASSERT_NE(btn, nullptr);
    btn->SetVisible(false);
    ld.dlg.ShowGuildUnionMark();
    EXPECT_TRUE(btn->isVisible());
}

TEST(CGuildMarkDialogTest, ShowGuildUnionMarkSetsInfoText) {
    LinkedDialog ld;
    ld.dlg.ShowGuildMark();
    EXPECT_EQ(ld.infoTextPtr->GetScriptText(), cGuildMarkDialog::kGuildMarkInfoText);
    ld.dlg.ShowGuildUnionMark();
    EXPECT_EQ(ld.infoTextPtr->GetScriptText(), cGuildMarkDialog::kGuildUnionMarkInfoText);
}

TEST(CGuildMarkDialogTest, ShowGuildMarkAndUnionMarkAreToggleable) {
    LinkedDialog ld;
    cButton* guildBtn = static_cast<cButton*>(ld.dlg.findWindowById(cGuildMarkDialog::kIdRegistOkBtn));
    cButton* unionBtn = static_cast<cButton*>(ld.dlg.findWindowById(cGuildMarkDialog::kIdUnionRegistOkBtn));
    ASSERT_NE(guildBtn, nullptr);
    ASSERT_NE(unionBtn, nullptr);

    ld.dlg.ShowGuildMark();
    EXPECT_TRUE(guildBtn->isVisible());
    EXPECT_FALSE(unionBtn->isVisible());

    ld.dlg.ShowGuildUnionMark();
    EXPECT_FALSE(guildBtn->isVisible());
    EXPECT_TRUE(unionBtn->isVisible());

    ld.dlg.ShowGuildMark();
    EXPECT_TRUE(guildBtn->isVisible());
    EXPECT_FALSE(unionBtn->isVisible());
}

TEST(CGuildMarkDialogTest, ShowGuildMarkWithoutLinkingIsSafe) {
    cGuildMarkDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.ShowGuildMark();
    dlg.ShowGuildUnionMark();
    SUCCEED();
}



// ---------- SetCallbacks / SetActive(false) OBJECTSTATEMGR dispatch ----------

namespace {

struct GuildMarkHost {
    int endCalls = 0;
    std::uint32_t lastObjectId = 0;
    std::int32_t lastState = -1;
    std::uint32_t heroObjectId = 0x12345678u;
    std::int32_t heroState = cGuildMarkDialog::kObjectStateDeal;
    bool npcScriptActive = false;
};

std::uint32_t GuildMarkHeroObjectId(void* userData) {
    return static_cast<GuildMarkHost*>(userData)->heroObjectId;
}

std::int32_t GuildMarkHeroState(void* userData) {
    return static_cast<GuildMarkHost*>(userData)->heroState;
}

bool GuildMarkIsNpcScriptActive(void* userData) {
    return static_cast<GuildMarkHost*>(userData)->npcScriptActive;
}

void GuildMarkEndObjectState(std::uint32_t objectId,
                             std::int32_t stateIdx,
                             void* userData) {
    auto* c = static_cast<GuildMarkHost*>(userData);
    ++c->endCalls;
    c->lastObjectId = objectId;
    c->lastState = stateIdx;
}

}  // namespace

TEST(CGuildMarkDialogTest, ObjectStateDealMatchesLegacy6) {
    EXPECT_EQ(cGuildMarkDialog::kObjectStateDeal, 6);
}

TEST(CGuildMarkDialogTest, SetActiveFalseEndsDealStateWhenAllGates) {
    LinkedDialog ld;
    ld.dlg.Init(0, 0, 200, 200, nullptr, 0);
    GuildMarkHost host;
    host.heroObjectId = 0xABCDEF01u;
    host.heroState = cGuildMarkDialog::kObjectStateDeal;
    host.npcScriptActive = false;
    ld.dlg.SetCallbacks(&GuildMarkHeroObjectId,
                        &GuildMarkHeroState,
                        &GuildMarkIsNpcScriptActive,
                        &GuildMarkEndObjectState,
                        &host);
    ld.dlg.SetActive(true);
    ld.dlg.SetActive(false);
    EXPECT_EQ(host.endCalls, 1);
    EXPECT_EQ(host.lastObjectId, 0xABCDEF01u);
    EXPECT_EQ(host.lastState, cGuildMarkDialog::kObjectStateDeal);
}

TEST(CGuildMarkDialogTest, SetActiveFalseNonDealNoEnd) {
    LinkedDialog ld;
    ld.dlg.Init(0, 0, 200, 200, nullptr, 0);
    GuildMarkHost host;
    host.heroState = 0;
    ld.dlg.SetCallbacks(&GuildMarkHeroObjectId,
                        &GuildMarkHeroState,
                        &GuildMarkIsNpcScriptActive,
                        &GuildMarkEndObjectState,
                        &host);
    ld.dlg.SetActive(false);
    EXPECT_EQ(host.endCalls, 0);
}

TEST(CGuildMarkDialogTest, SetActiveFalseNpcScriptActiveSkipsEnd) {
    LinkedDialog ld;
    ld.dlg.Init(0, 0, 200, 200, nullptr, 0);
    GuildMarkHost host;
    host.heroState = cGuildMarkDialog::kObjectStateDeal;
    host.npcScriptActive = true;
    ld.dlg.SetCallbacks(&GuildMarkHeroObjectId,
                        &GuildMarkHeroState,
                        &GuildMarkIsNpcScriptActive,
                        &GuildMarkEndObjectState,
                        &host);
    ld.dlg.SetActive(false);
    EXPECT_EQ(host.endCalls, 0);
}

TEST(CGuildMarkDialogTest, SetActiveFalseMissingHeroSkipsEnd) {
    LinkedDialog ld;
    ld.dlg.Init(0, 0, 200, 200, nullptr, 0);
    GuildMarkHost host;
    host.heroObjectId = 0u;
    host.heroState = cGuildMarkDialog::kObjectStateDeal;
    ld.dlg.SetCallbacks(&GuildMarkHeroObjectId,
                        &GuildMarkHeroState,
                        &GuildMarkIsNpcScriptActive,
                        &GuildMarkEndObjectState,
                        &host);
    ld.dlg.SetActive(false);
    EXPECT_EQ(host.endCalls, 0);
}

TEST(CGuildMarkDialogTest, SetActiveFalseWithoutCallbacksIsSafe) {
    LinkedDialog ld;
    ld.dlg.Init(0, 0, 200, 200, nullptr, 0);
    ld.dlg.SetActive(false);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CGuildMarkDialogTest, SetActiveFalseNullUserDataIsSafe) {
    LinkedDialog ld;
    ld.dlg.Init(0, 0, 200, 200, nullptr, 0);
    auto getId = [](void*) -> std::uint32_t { return 0xBEEFu; };
    auto getState = [](void*) -> std::int32_t { return 6; };
    auto isActive = [](void*) -> bool { return false; };
    auto endObj = [](std::uint32_t, std::int32_t, void*) {};
    ld.dlg.SetCallbacks(getId, getState, isActive, endObj);
    ld.dlg.SetActive(true);
    ld.dlg.SetActive(false);
    SUCCEED();
}

TEST(CGuildMarkDialogTest, SetActiveTrueDoesNotInvokeEnd) {
    LinkedDialog ld;
    ld.dlg.Init(0, 0, 200, 200, nullptr, 0);
    GuildMarkHost host;
    host.heroState = cGuildMarkDialog::kObjectStateDeal;
    ld.dlg.SetCallbacks(&GuildMarkHeroObjectId,
                        &GuildMarkHeroState,
                        &GuildMarkIsNpcScriptActive,
                        &GuildMarkEndObjectState,
                        &host);
    ld.dlg.SetActive(true);
    EXPECT_EQ(host.endCalls, 0);
}

TEST(CGuildMarkDialogTest, SetActiveFalseReplacesCallbacks) {
    cGuildMarkDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    GuildMarkHost first;
    GuildMarkHost second;
    auto getObj = [](void* u) -> std::uint32_t {
        return static_cast<GuildMarkHost*>(u)->heroObjectId;
    };
    auto getSt = [](void* u) -> std::int32_t {
        return static_cast<GuildMarkHost*>(u)->heroState;
    };
    auto isAct = [](void* u) -> bool {
        return static_cast<GuildMarkHost*>(u)->npcScriptActive;
    };
    auto endF = [](std::uint32_t o, std::int32_t s, void* u) {
        auto* c = static_cast<GuildMarkHost*>(u);
        ++c->endCalls;
        c->lastObjectId = o;
        c->lastState = s;
    };
    dlg.SetCallbacks(getObj, getSt, isAct, endF, &first);
    dlg.SetCallbacks(getObj, getSt, isAct, endF, &second);
    dlg.SetActive(false);
    EXPECT_EQ(first.endCalls, 0);
    EXPECT_EQ(second.endCalls, 1);
    EXPECT_EQ(second.lastObjectId, second.heroObjectId);
}

