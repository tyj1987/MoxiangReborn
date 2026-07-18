// skillpointnotify_test.cpp — 1:1 port verification tests for cSkillPointNotify.

#include "skillpointnotify.hpp"
#include "cbutton.hpp"
#include "ctextarea.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <memory>

using mxh::ui::cSkillPointNotify;
using mxh::ui::cButton;
using mxh::ui::cTextArea;

namespace {

std::unique_ptr<cSkillPointNotify> MakeDialog() {
    auto d = std::make_unique<cSkillPointNotify>();
    d->Init(0, 0, 240, 120, nullptr, 799);
    return d;
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction + constants
// ---------------------------------------------------------------------------

TEST(CSkillPointNotify, IdConstantsAreLegacy) {
    EXPECT_EQ(cSkillPointNotify::kIdNotifyText1, 800);
    EXPECT_EQ(cSkillPointNotify::kIdNotifyText2, 801);
    EXPECT_EQ(cSkillPointNotify::kIdStartBtn,    802);
}

TEST(CSkillPointNotify, ChildrenNullBeforeLinking) {
    auto d = MakeDialog();
    EXPECT_EQ(d->GetNotifyText1(), nullptr);
    EXPECT_EQ(d->GetNotifyText2(), nullptr);
    EXPECT_EQ(d->GetStartButton(), nullptr);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CSkillPointNotify, LinkingMaterializesAllThreeChildren) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_NE(d->GetNotifyText1(), nullptr);
    EXPECT_NE(d->GetNotifyText2(), nullptr);
    EXPECT_NE(d->GetStartButton(), nullptr);
}

TEST(CSkillPointNotify, LinkingSetsChildIds) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_EQ(d->GetNotifyText1()->id(), 800);
    EXPECT_EQ(d->GetNotifyText2()->id(), 801);
    EXPECT_EQ(d->GetStartButton()->id(), 802);
}

TEST(CSkillPointNotify, LinkingIdempotent) {
    auto d = MakeDialog();
    d->Linking();
    d->Linking();
    EXPECT_NE(d->GetNotifyText1(), nullptr);
    EXPECT_NE(d->GetNotifyText2(), nullptr);
    EXPECT_NE(d->GetStartButton(), nullptr);
}

// ---------------------------------------------------------------------------
// InitTextArea
// ---------------------------------------------------------------------------

TEST(CSkillPointNotify, InitTextAreaPopulatesBothTexts) {
    auto d = MakeDialog();
    d->Linking();
    d->InitTextArea();
    EXPECT_FALSE(d->GetNotifyText1()->GetScriptText().empty());
    EXPECT_FALSE(d->GetNotifyText2()->GetScriptText().empty());
    // 1:1 quirk: legacy CHATMGR->GetChatMsg(735/736) returns distinct
    // strings for the two info lines; modern placeholders are likewise
    // distinct.
    EXPECT_NE(d->GetNotifyText1()->GetScriptText(),
              d->GetNotifyText2()->GetScriptText());
}

TEST(CSkillPointNotify, InitTextAreaDefensiveBeforeLinking) {
    auto d = MakeDialog();
    d->InitTextArea();  // all 3 cTextArea/cButton are nullptr, must not crash
    EXPECT_EQ(d->GetNotifyText1(), nullptr);
    EXPECT_EQ(d->GetNotifyText2(), nullptr);
}

TEST(CSkillPointNotify, InitTextAreaCanBeCalledMultipleTimes) {
    auto d = MakeDialog();
    d->Linking();
    d->InitTextArea();
    const auto first1 = d->GetNotifyText1()->GetScriptText();
    d->InitTextArea();
    EXPECT_EQ(d->GetNotifyText1()->GetScriptText(), first1);  // idempotent
}

// ---------------------------------------------------------------------------
// Defaults: no children means dialog is still "fresh" / not yet initialized.
// ---------------------------------------------------------------------------

TEST(CSkillPointNotify, NotActiveByDefault) {
    auto d = MakeDialog();
    EXPECT_FALSE(d->isActive());
}
