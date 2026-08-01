#include "partymemberdlg.hpp"

#include "mxh/ui/cStatic.hpp"
#include "mxh/ui/cobjectguagen.hpp"
#include "mxh/ui/cpushupbutton.hpp"
#include "mxh/ui/partybtndlg.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cObjectGuagen;
using mxh::ui::cPartyBtnDlg;
using mxh::ui::cPartyMemberDlg;
using mxh::ui::cPushupButton;
using mxh::ui::cStatic;
using mxh::ui::cWindow;
using mxh::ui::PartyMemberData;

namespace {

struct Controls {
    cPushupButton name;
    cObjectGuagen life;
    cObjectGuagen naeryuk;
    cStatic level;

    Controls() {
        name.Init(0, 0, 80, 20, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
        life.Init(0, 0, 80, 6, nullptr, 0);
        naeryuk.Init(0, 0, 80, 6, nullptr, 0);
        level.Init(0, 0, 50, 20, nullptr, 0);
    }

    void Attach(cPartyMemberDlg& dialog) {
        dialog.SetControlsForTest(&name, &life, &naeryuk, &level);
    }
};

PartyMemberData MakeMember(bool logged = true) {
    PartyMemberData info;
    info.memberId = 77;
    info.logged = logged;
    info.lifePercent = 75;
    info.shieldPercent = 25;
    info.naeryukPercent = 40;
    info.name = "Mavis";
    info.level = 123;
    info.posX = 10;
    info.posZ = 20;
    return info;
}

struct ClickCapture {
    int calls = 0;
    std::uint32_t memberId = 0;
};

void CaptureClick(std::uint32_t memberId, void* userData) {
    auto* capture = static_cast<ClickCapture*>(userData);
    ++capture->calls;
    capture->memberId = memberId;
}

constexpr std::uint32_t ClickEvent() {
    return static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick);
}

} // namespace

TEST(PartyMemberDlgTest, InheritsDialogAndIsNonCopyable) {
    static_assert(std::is_base_of_v<cDialog, cPartyMemberDlg>);
    static_assert(!std::is_copy_constructible_v<cPartyMemberDlg>);
    static_assert(!std::is_copy_assignable_v<cPartyMemberDlg>);
    SUCCEED();
}

TEST(PartyMemberDlgTest, ConstantsMatchPreprocessedLegacyIdsAndColors) {
    EXPECT_EQ(cPartyMemberDlg::kMemberNameBaseId, 423);
    EXPECT_EQ(cPartyMemberDlg::kMemberLifeBaseId, 429);
    EXPECT_EQ(cPartyMemberDlg::kMemberNaeryukBaseId, 435);
    EXPECT_EQ(cPartyMemberDlg::kMemberLevelBaseId, 441);
    EXPECT_EQ(cPartyMemberDlg::kMemberSlotCount, 6);
    EXPECT_EQ(cPartyMemberDlg::kLoginBasicColor, 0xFFFFFFFFu);
    EXPECT_EQ(cPartyMemberDlg::kLoginOverColor, 0xFFFFFFFFu);
    EXPECT_EQ(cPartyMemberDlg::kLoginPressColor, 0xFFFFEA00u);
    EXPECT_EQ(cPartyMemberDlg::kLogoutBasicColor, 0xFFACB6C7u);
    EXPECT_EQ(cPartyMemberDlg::kLogoutOverColor, 0xFFACB6C7u);
    EXPECT_EQ(cPartyMemberDlg::kLogoutPressColor, 0xFFFFEA00u);
}

TEST(PartyMemberDlgTest, ConstructorMatchesLegacyDefaults) {
    cPartyMemberDlg dialog;
    EXPECT_EQ(dialog.memberId(), 0u);
    EXPECT_FALSE(dialog.realActive());
    EXPECT_FALSE(dialog.setTopOnActive());
    EXPECT_EQ(dialog.memberIndex(), -1);
    EXPECT_TRUE(dialog.optionVisible());
    EXPECT_TRUE(dialog.memberVisible());
    EXPECT_EQ(dialog.nameControl(), nullptr);
    EXPECT_EQ(dialog.lifeGauge(), nullptr);
    EXPECT_EQ(dialog.naeryukGauge(), nullptr);
    EXPECT_EQ(dialog.levelControl(), nullptr);
    EXPECT_EQ(dialog.partyButtonDialog(), nullptr);
}

TEST(PartyMemberDlgTest, LinkingUsesIndexedLegacyIdRanges) {
    cPartyMemberDlg dialog;
    auto name = std::make_unique<cPushupButton>();
    name->Init(0, 0, 20, 10, nullptr, nullptr, nullptr, nullptr, nullptr, 425);
    auto life = std::make_unique<cObjectGuagen>();
    life->Init(0, 0, 20, 10, nullptr, 431);
    auto naeryuk = std::make_unique<cObjectGuagen>();
    naeryuk->Init(0, 0, 20, 10, nullptr, 437);
    auto level = std::make_unique<cStatic>();
    level->Init(0, 0, 20, 10, nullptr, 443);
    cPushupButton* nameRaw = name.get();
    cObjectGuagen* lifeRaw = life.get();
    cObjectGuagen* naeryukRaw = naeryuk.get();
    cStatic* levelRaw = level.get();
    dialog.Add(std::move(name));
    dialog.Add(std::move(life));
    dialog.Add(std::move(naeryuk));
    dialog.Add(std::move(level));
    dialog.Linking(2);
    EXPECT_EQ(dialog.memberIndex(), 2);
    EXPECT_EQ(dialog.nameControl(), nameRaw);
    EXPECT_EQ(dialog.lifeGauge(), lifeRaw);
    EXPECT_EQ(dialog.naeryukGauge(), naeryukRaw);
    EXPECT_EQ(dialog.levelControl(), levelRaw);
}

TEST(PartyMemberDlgTest, LinkingRejectsWrongControlTypes) {
    cPartyMemberDlg dialog;
    auto wrong = std::make_unique<cStatic>();
    wrong->Init(0, 0, 20, 10, nullptr, cPartyMemberDlg::kMemberNameBaseId);
    dialog.Add(std::move(wrong));
    dialog.Linking(0);
    EXPECT_EQ(dialog.nameControl(), nullptr);
    EXPECT_EQ(dialog.memberIndex(), 0);
}

TEST(PartyMemberDlgTest, TestInjectionPublishesControls) {
    cPartyMemberDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    EXPECT_EQ(dialog.nameControl(), &controls.name);
    EXPECT_EQ(dialog.lifeGauge(), &controls.life);
    EXPECT_EQ(dialog.naeryukGauge(), &controls.naeryuk);
    EXPECT_EQ(dialog.levelControl(), &controls.level);
}

TEST(PartyMemberDlgTest, SetActiveRemembersRequestButHidesEmptySlot) {
    cPartyMemberDlg dialog;
    dialog.SetActive(true);
    EXPECT_TRUE(dialog.realActive());
    EXPECT_FALSE(dialog.isActive());
}

TEST(PartyMemberDlgTest, MemberDataRestoresRememberedActiveState) {
    cPartyMemberDlg dialog;
    dialog.SetActive(true);
    PartyMemberData info = MakeMember();
    dialog.SetMemberData(&info);
    EXPECT_EQ(dialog.memberId(), info.memberId);
    EXPECT_TRUE(dialog.realActive());
    EXPECT_TRUE(dialog.isActive());
}

TEST(PartyMemberDlgTest, HiddenMemberForcesRealAndActualInactive) {
    cPartyMemberDlg dialog;
    PartyMemberData info = MakeMember();
    dialog.SetMemberData(&info);
    dialog.ShowMember(false);
    dialog.SetActive(true);
    EXPECT_FALSE(dialog.realActive());
    EXPECT_FALSE(dialog.isActive());
}

TEST(PartyMemberDlgTest, DisabledDialogIgnoresSetActive) {
    cPartyMemberDlg dialog;
    dialog.SetDisable(true);
    dialog.SetActive(true);
    EXPECT_FALSE(dialog.realActive());
    EXPECT_FALSE(dialog.isActive());
}

TEST(PartyMemberDlgTest, NullMemberClearsIdAndHidesButKeepsRequest) {
    cPartyMemberDlg dialog;
    PartyMemberData info = MakeMember();
    dialog.SetMemberData(&info);
    dialog.SetActive(true);
    dialog.SetMemberData(nullptr);
    EXPECT_EQ(dialog.memberId(), 0u);
    EXPECT_TRUE(dialog.realActive());
    EXPECT_FALSE(dialog.isActive());
}

TEST(PartyMemberDlgTest, LoggedMemberUpdatesTextGaugesAndLevel) {
    cPartyMemberDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    PartyMemberData info = MakeMember(true);
    dialog.SetMemberData(&info);
    EXPECT_EQ(controls.name.text(), "Mavis");
    EXPECT_EQ(controls.name.textBasicColor(), cPartyMemberDlg::kLoginBasicColor);
    EXPECT_EQ(controls.name.textOverColor(), cPartyMemberDlg::kLoginOverColor);
    EXPECT_EQ(controls.name.textPressColor(), cPartyMemberDlg::kLoginPressColor);
    EXPECT_FLOAT_EQ(controls.life.GetCurPercentRate(), 0.75f);
    EXPECT_FLOAT_EQ(controls.naeryuk.GetCurPercentRate(), 0.40f);
    EXPECT_EQ(controls.level.GetStaticText(), "Lv.123");
}

TEST(PartyMemberDlgTest, LoggedGaugeValuesClampThroughObjectGauge) {
    cPartyMemberDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    PartyMemberData info = MakeMember(true);
    info.lifePercent = 255;
    info.naeryukPercent = 200;
    dialog.SetMemberData(&info);
    EXPECT_FLOAT_EQ(controls.life.GetCurPercentRate(), 1.0f);
    EXPECT_FLOAT_EQ(controls.naeryuk.GetCurPercentRate(), 1.0f);
}

TEST(PartyMemberDlgTest, LoggedOutMemberUsesGrayAndClearsStatus) {
    cPartyMemberDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.life.SetValue(0.8f, 0);
    controls.naeryuk.SetValue(0.6f, 0);
    controls.level.SetStaticText("old");
    PartyMemberData info = MakeMember(false);
    dialog.SetMemberData(&info);
    EXPECT_EQ(controls.name.text(), "Mavis");
    EXPECT_EQ(controls.name.textBasicColor(), cPartyMemberDlg::kLogoutBasicColor);
    EXPECT_EQ(controls.name.textOverColor(), cPartyMemberDlg::kLogoutOverColor);
    EXPECT_EQ(controls.name.textPressColor(), cPartyMemberDlg::kLogoutPressColor);
    EXPECT_FLOAT_EQ(controls.life.GetCurPercentRate(), 0.0f);
    EXPECT_FLOAT_EQ(controls.naeryuk.GetCurPercentRate(), 0.0f);
    EXPECT_EQ(controls.level.GetStaticText(), "");
}

TEST(PartyMemberDlgTest, NullMemberPreservesLegacyVisualContents) {
    cPartyMemberDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    PartyMemberData info = MakeMember(true);
    dialog.SetMemberData(&info);
    dialog.SetMemberData(nullptr);
    EXPECT_EQ(controls.name.text(), "Mavis");
    EXPECT_FLOAT_EQ(controls.life.GetCurPercentRate(), 0.75f);
    EXPECT_EQ(controls.level.GetStaticText(), "Lv.123");
}

TEST(PartyMemberDlgTest, SetMemberDataWithoutLinkedControlsIsSafe) {
    cPartyMemberDlg dialog;
    PartyMemberData info = MakeMember(true);
    dialog.SetMemberData(&info);
    EXPECT_EQ(dialog.memberId(), 77u);
}

TEST(PartyMemberDlgTest, SetNameButtonPushUpForwardsState) {
    cPartyMemberDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.SetNameBtnPushUp(true);
    EXPECT_TRUE(controls.name.IsPushed());
    dialog.SetNameBtnPushUp(false);
    EXPECT_FALSE(controls.name.IsPushed());
}

TEST(PartyMemberDlgTest, SetNameButtonPushUpWithoutControlIsSafe) {
    cPartyMemberDlg dialog;
    dialog.SetNameBtnPushUp(true);
    SUCCEED();
}

TEST(PartyMemberDlgTest, ClickEventPublishesCurrentMemberId) {
    cPartyMemberDlg dialog;
    ClickCapture capture;
    PartyMemberData info = MakeMember();
    dialog.SetMemberData(&info);
    dialog.SetActive(true);
    dialog.SetClickedMemberCallback(&CaptureClick, &capture);
    dialog.SetActionEventResultForTest(ClickEvent());
    EXPECT_EQ(dialog.ActionEvent(0, 0, 0), ClickEvent());
    EXPECT_EQ(capture.calls, 1);
    EXPECT_EQ(capture.memberId, info.memberId);
}

TEST(PartyMemberDlgTest, InactiveDialogReturnsNullWithoutPublishing) {
    cPartyMemberDlg dialog;
    ClickCapture capture;
    dialog.SetClickedMemberCallback(&CaptureClick, &capture);
    dialog.SetActionEventResultForTest(ClickEvent());
    EXPECT_EQ(dialog.ActionEvent(0, 0, 0), 0u);
    EXPECT_EQ(capture.calls, 0);
}

TEST(PartyMemberDlgTest, NonClickEventIsReturnedWithoutPublishing) {
    cPartyMemberDlg dialog;
    ClickCapture capture;
    PartyMemberData info = MakeMember();
    dialog.SetMemberData(&info);
    dialog.SetActive(true);
    dialog.SetClickedMemberCallback(&CaptureClick, &capture);
    const auto move = static_cast<std::uint32_t>(cWindow::WindowEvent::MouseMove);
    dialog.SetActionEventResultForTest(move);
    EXPECT_EQ(dialog.ActionEvent(0, 0, 0), move);
    EXPECT_EQ(capture.calls, 0);
}

TEST(PartyMemberDlgTest, ClickWithoutCallbackIsSafe) {
    cPartyMemberDlg dialog;
    PartyMemberData info = MakeMember();
    dialog.SetMemberData(&info);
    dialog.SetActive(true);
    dialog.SetActionEventResultForTest(ClickEvent());
    EXPECT_EQ(dialog.ActionEvent(0, 0, 0), ClickEvent());
}

TEST(PartyMemberDlgTest, ClearingTestEventRestoresBaseDispatch) {
    cPartyMemberDlg dialog;
    PartyMemberData info = MakeMember();
    dialog.SetMemberData(&info);
    dialog.SetActive(true);
    dialog.SetActionEventResultForTest(ClickEvent());
    dialog.ClearActionEventResultForTest();
    EXPECT_EQ(dialog.ActionEvent(-100, -100, 0), 0u);
}

TEST(PartyMemberDlgTest, PartyButtonDialogPointerIsStored) {
    cPartyMemberDlg dialog;
    cPartyBtnDlg partyButtons;
    dialog.SetPartyBtnDlg(&partyButtons);
    EXPECT_EQ(dialog.partyButtonDialog(), &partyButtons);
}

TEST(PartyMemberDlgTest, RenderWithOptionUsesLegacyCoordinates) {
    cPartyMemberDlg dialog;
    Controls controls;
    cPartyBtnDlg partyButtons;
    controls.Attach(dialog);
    dialog.SetPartyBtnDlg(&partyButtons);
    dialog.Linking(2);
    dialog.SetControlsForTest(&controls.name, &controls.life,
                              &controls.naeryuk, &controls.level);
    partyButtons.SetAbsXY(100, 200);
    dialog.ShowOption(true);
    dialog.Render();
    EXPECT_EQ(dialog.absX(), 100);
    EXPECT_EQ(dialog.absY(), 430);
    EXPECT_EQ(controls.name.absX(), 109);
    EXPECT_EQ(controls.name.absY(), 444);
    EXPECT_EQ(controls.life.absX(), 106);
    EXPECT_EQ(controls.life.absY(), 464);
    EXPECT_EQ(controls.naeryuk.absX(), 106);
    EXPECT_EQ(controls.naeryuk.absY(), 470);
    EXPECT_EQ(controls.level.absX(), 109);
    EXPECT_EQ(controls.level.absY(), 431);
}

TEST(PartyMemberDlgTest, RenderWithoutOptionUsesLegacyCoordinates) {
    cPartyMemberDlg dialog;
    Controls controls;
    cPartyBtnDlg partyButtons;
    controls.Attach(dialog);
    dialog.SetPartyBtnDlg(&partyButtons);
    dialog.Linking(2);
    dialog.SetControlsForTest(&controls.name, &controls.life,
                              &controls.naeryuk, &controls.level);
    partyButtons.SetAbsXY(100, 200);
    dialog.ShowOption(false);
    dialog.Render();
    EXPECT_EQ(dialog.absX(), 100);
    EXPECT_EQ(dialog.absY(), 350);
    EXPECT_EQ(controls.name.absY(), 364);
    EXPECT_EQ(controls.life.absY(), 384);
    EXPECT_EQ(controls.naeryuk.absY(), 390);
    EXPECT_EQ(controls.level.absY(), 351);
}

TEST(PartyMemberDlgTest, RenderWithoutPartyButtonDialogDoesNotMove) {
    cPartyMemberDlg dialog;
    dialog.SetAbsXY(7, 9);
    dialog.Linking(1);
    dialog.Render();
    EXPECT_EQ(dialog.absX(), 7);
    EXPECT_EQ(dialog.absY(), 9);
}

TEST(PartyMemberDlgTest, RenderBeforeLinkingDoesNotMove) {
    cPartyMemberDlg dialog;
    cPartyBtnDlg partyButtons;
    partyButtons.SetAbsXY(100, 200);
    dialog.SetPartyBtnDlg(&partyButtons);
    dialog.SetAbsXY(7, 9);
    dialog.Render();
    EXPECT_EQ(dialog.absX(), 7);
    EXPECT_EQ(dialog.absY(), 9);
}

TEST(PartyMemberDlgTest, ShowFlagsStoreIndependentState) {
    cPartyMemberDlg dialog;
    dialog.ShowOption(false);
    dialog.ShowMember(false);
    EXPECT_FALSE(dialog.optionVisible());
    EXPECT_FALSE(dialog.memberVisible());
    dialog.ShowOption(true);
    EXPECT_TRUE(dialog.optionVisible());
    EXPECT_FALSE(dialog.memberVisible());
}
