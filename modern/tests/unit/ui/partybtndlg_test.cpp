#include "mxh/ui/partybtndlg.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cStatic.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cPartyBtnDlg;
using mxh::ui::cStatic;
using mxh::ui::PartyState;

namespace {

struct Controls {
    cStatic background;
    cButton secede;
    cButton transfer;
    cButton forcedSecede;
    cButton addMember;
    cButton breakUp;
    cButton warSuggest;
    cButton option;
    cButton member;

    Controls() {
        background.Init(0, 0, 100, 20, nullptr, cPartyBtnDlg::kBackgroundId);
        secede.Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                     cPartyBtnDlg::kSecedeButtonId);
        transfer.Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                       cPartyBtnDlg::kTransferButtonId);
        forcedSecede.Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                           cPartyBtnDlg::kForcedSecedeButtonId);
        addMember.Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                        cPartyBtnDlg::kAddMemberButtonId);
        breakUp.Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                     cPartyBtnDlg::kBreakUpButtonId);
        warSuggest.Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                         cPartyBtnDlg::kWarSuggestButtonId);
        option.Init(10, 30, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                    cPartyBtnDlg::kOptionButtonId);
        member.Init(10, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                    cPartyBtnDlg::kMemberButtonId);
    }

    void Attach(cPartyBtnDlg& dialog) {
        dialog.SetControlsForTest(&background, &secede, &transfer,
                                  &forcedSecede, &addMember, &breakUp,
                                  &warSuggest, &option, &member);
    }

    void SetAllActive(bool active) {
        background.SetActive(active);
        secede.SetActive(active);
        transfer.SetActive(active);
        forcedSecede.SetActive(active);
        addMember.SetActive(active);
        breakUp.SetActive(active);
        warSuggest.SetActive(active);
        option.SetActive(active);
        member.SetActive(active);
    }
};

void ExpectActionButtonsActive(const Controls& controls, bool active) {
    EXPECT_EQ(controls.secede.isActive(), active);
    EXPECT_EQ(controls.transfer.isActive(), active);
    EXPECT_EQ(controls.forcedSecede.isActive(), active);
    EXPECT_EQ(controls.addMember.isActive(), active);
    EXPECT_EQ(controls.breakUp.isActive(), active);
    EXPECT_EQ(controls.warSuggest.isActive(), active);
}

}  // namespace

TEST(PartyBtnDlgTest, InheritsFromDialog) {
    static_assert(std::is_base_of_v<cDialog, cPartyBtnDlg>);
    SUCCEED();
}

TEST(PartyBtnDlgTest, IsNonCopyable) {
    static_assert(!std::is_copy_constructible_v<cPartyBtnDlg>);
    static_assert(!std::is_copy_assignable_v<cPartyBtnDlg>);
    SUCCEED();
}

TEST(PartyBtnDlgTest, ConstantsMatchLegacyWindowIdsAndColors) {
    EXPECT_EQ(cPartyBtnDlg::kBackgroundId, 502);
    EXPECT_EQ(cPartyBtnDlg::kSecedeButtonId, 503);
    EXPECT_EQ(cPartyBtnDlg::kTransferButtonId, 504);
    EXPECT_EQ(cPartyBtnDlg::kForcedSecedeButtonId, 505);
    EXPECT_EQ(cPartyBtnDlg::kBreakUpButtonId, 506);
    EXPECT_EQ(cPartyBtnDlg::kAddMemberButtonId, 507);
    EXPECT_EQ(cPartyBtnDlg::kWarSuggestButtonId, 508);
    EXPECT_EQ(cPartyBtnDlg::kOptionButtonId, 509);
    EXPECT_EQ(cPartyBtnDlg::kMemberButtonId, 510);
    EXPECT_EQ(cPartyBtnDlg::kUsableImageColor, 0x00FFFFFFu);
    EXPECT_EQ(cPartyBtnDlg::kDisabledImageColor, 0x00FF6464u);
}

TEST(PartyBtnDlgTest, ConstructorDefaultsOptionOnAndControlsNull) {
    cPartyBtnDlg dialog;
    EXPECT_TRUE(dialog.optionVisible());
    EXPECT_EQ(dialog.partyState().partyIndex, 0);
    EXPECT_EQ(dialog.partyState().masterId, 0);
    EXPECT_EQ(dialog.partyState().heroId, 0);
    EXPECT_EQ(dialog.background(), nullptr);
    EXPECT_EQ(dialog.secedeButton(), nullptr);
    EXPECT_EQ(dialog.memberButton(), nullptr);
}

TEST(PartyBtnDlgTest, LinkingFindsAllControlsByExactIds) {
    cPartyBtnDlg dialog;
    dialog.Init(0, 0, 300, 200, nullptr);

    auto background = std::make_unique<cStatic>();
    background->Init(0, 0, 100, 20, nullptr, cPartyBtnDlg::kBackgroundId);
    auto* backgroundRaw = background.get();
    dialog.Add(std::move(background));

    auto secede = std::make_unique<cButton>();
    secede->Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                 cPartyBtnDlg::kSecedeButtonId);
    auto* secedeRaw = secede.get();
    dialog.Add(std::move(secede));

    auto transfer = std::make_unique<cButton>();
    transfer->Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                   cPartyBtnDlg::kTransferButtonId);
    auto* transferRaw = transfer.get();
    dialog.Add(std::move(transfer));

    auto forced = std::make_unique<cButton>();
    forced->Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                 cPartyBtnDlg::kForcedSecedeButtonId);
    auto* forcedRaw = forced.get();
    dialog.Add(std::move(forced));

    auto add = std::make_unique<cButton>();
    add->Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
              cPartyBtnDlg::kAddMemberButtonId);
    auto* addRaw = add.get();
    dialog.Add(std::move(add));

    auto breakup = std::make_unique<cButton>();
    breakup->Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                  cPartyBtnDlg::kBreakUpButtonId);
    auto* breakupRaw = breakup.get();
    dialog.Add(std::move(breakup));

    auto war = std::make_unique<cButton>();
    war->Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
              cPartyBtnDlg::kWarSuggestButtonId);
    auto* warRaw = war.get();
    dialog.Add(std::move(war));

    auto option = std::make_unique<cButton>();
    option->Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                 cPartyBtnDlg::kOptionButtonId);
    auto* optionRaw = option.get();
    dialog.Add(std::move(option));

    auto member = std::make_unique<cButton>();
    member->Init(0, 0, 40, 20, nullptr, nullptr, nullptr, {}, nullptr,
                 cPartyBtnDlg::kMemberButtonId);
    auto* memberRaw = member.get();
    dialog.Add(std::move(member));

    dialog.Linking();
    EXPECT_EQ(dialog.background(), backgroundRaw);
    EXPECT_EQ(dialog.secedeButton(), secedeRaw);
    EXPECT_EQ(dialog.transferButton(), transferRaw);
    EXPECT_EQ(dialog.forcedSecedeButton(), forcedRaw);
    EXPECT_EQ(dialog.addMemberButton(), addRaw);
    EXPECT_EQ(dialog.breakUpButton(), breakupRaw);
    EXPECT_EQ(dialog.warSuggestButton(), warRaw);
    EXPECT_EQ(dialog.optionButton(), optionRaw);
    EXPECT_EQ(dialog.memberButton(), memberRaw);
}

TEST(PartyBtnDlgTest, ShowNonPartyHidesAllPartyControls) {
    cPartyBtnDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.SetAllActive(true);

    dialog.ShowNonPartyDlg();
    EXPECT_TRUE(controls.background.isActive());
    ExpectActionButtonsActive(controls, false);
    EXPECT_FALSE(controls.option.isActive());
    EXPECT_FALSE(controls.member.isActive());
}

TEST(PartyBtnDlgTest, ShowPartyMasterEnablesActionsAndColorsSecedeDisabled) {
    cPartyBtnDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.ShowPartyMasterDlg();
    EXPECT_TRUE(controls.background.isActive());
    ExpectActionButtonsActive(controls, true);
    EXPECT_TRUE(controls.option.isActive());
    EXPECT_TRUE(controls.member.isActive());
    EXPECT_EQ(controls.secede.imageRGB(), cPartyBtnDlg::kDisabledImageColor);
    EXPECT_EQ(controls.transfer.imageRGB(), cPartyBtnDlg::kUsableImageColor);
    EXPECT_EQ(controls.warSuggest.imageRGB(), cPartyBtnDlg::kUsableImageColor);
}

TEST(PartyBtnDlgTest, ShowPartyMasterWithOptionOffHidesActionButtons) {
    cPartyBtnDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.ShowOption(false);
    dialog.ShowPartyMasterDlg();
    EXPECT_FALSE(controls.background.isActive());
    ExpectActionButtonsActive(controls, false);
    EXPECT_TRUE(controls.option.isActive());
    EXPECT_TRUE(controls.member.isActive());
}

TEST(PartyBtnDlgTest, ShowPartyMemberEnablesOnlySecedeImage) {
    cPartyBtnDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.ShowPartyMemberDlg();
    EXPECT_TRUE(controls.background.isActive());
    ExpectActionButtonsActive(controls, true);
    EXPECT_EQ(controls.secede.imageRGB(), cPartyBtnDlg::kUsableImageColor);
    EXPECT_EQ(controls.transfer.imageRGB(), cPartyBtnDlg::kDisabledImageColor);
    EXPECT_EQ(controls.addMember.imageRGB(), cPartyBtnDlg::kDisabledImageColor);
}

TEST(PartyBtnDlgTest, ShowPartyMemberWithOptionOffHidesActionButtons) {
    cPartyBtnDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.ShowOption(false);
    dialog.ShowPartyMemberDlg();
    EXPECT_FALSE(controls.background.isActive());
    ExpectActionButtonsActive(controls, false);
    EXPECT_TRUE(controls.option.isActive());
    EXPECT_TRUE(controls.member.isActive());
}

TEST(PartyBtnDlgTest, ShowOptionTogglesBackgroundAndActionsButKeepsTogglesActive) {
    cPartyBtnDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.ShowOption(false);
    EXPECT_FALSE(controls.background.isActive());
    ExpectActionButtonsActive(controls, false);
    EXPECT_TRUE(controls.option.isActive());
    EXPECT_TRUE(controls.member.isActive());

    dialog.ShowOption(true);
    EXPECT_TRUE(controls.background.isActive());
    ExpectActionButtonsActive(controls, true);
}

TEST(PartyBtnDlgTest, RefreshSelectsNonPartyBranch) {
    cPartyBtnDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.SetAllActive(true);
    dialog.SetPartyState(PartyState{0, 100, 100});

    dialog.RefreshDlg();
    ExpectActionButtonsActive(controls, false);
    EXPECT_FALSE(controls.option.isActive());
    EXPECT_FALSE(controls.member.isActive());
}

TEST(PartyBtnDlgTest, RefreshSelectsMasterBranch) {
    cPartyBtnDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.SetPartyState(PartyState{7, 100, 100});

    dialog.RefreshDlg();
    EXPECT_EQ(controls.secede.imageRGB(), cPartyBtnDlg::kDisabledImageColor);
    EXPECT_EQ(controls.transfer.imageRGB(), cPartyBtnDlg::kUsableImageColor);
}

TEST(PartyBtnDlgTest, RefreshSelectsMemberBranch) {
    cPartyBtnDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.SetPartyState(PartyState{7, 100, 200});

    dialog.RefreshDlg();
    EXPECT_EQ(controls.secede.imageRGB(), cPartyBtnDlg::kUsableImageColor);
    EXPECT_EQ(controls.transfer.imageRGB(), cPartyBtnDlg::kDisabledImageColor);
}

TEST(PartyBtnDlgTest, RenderMovesMemberButtonByOptionState) {
    cPartyBtnDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.ShowOption(true);
    dialog.Render();
    EXPECT_EQ(controls.member.absY(), controls.option.absY() + 100);

    dialog.ShowOption(false);
    dialog.Render();
    EXPECT_EQ(controls.member.absY(), controls.option.absY() + 20);
}

TEST(PartyBtnDlgTest, MissingControlsAreGuarded) {
    cPartyBtnDlg dialog;
    dialog.RefreshDlg();
    dialog.ShowNonPartyDlg();
    dialog.ShowPartyMasterDlg();
    dialog.ShowPartyMemberDlg();
    dialog.ShowOption(false);
    dialog.Render();
    SUCCEED();
}
