#include "petstatedlg.hpp"

#include "cButton.hpp"
#include "cDialog.hpp"
#include "cGuagen.hpp"
#include "cStatic.hpp"
#include "cpushupbutton.hpp"
#include "ctabdialog.hpp"
#include "cWindow.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cGuagen;
using mxh::ui::cPetStateDlg;
using mxh::ui::cPushupButton;
using mxh::ui::cStatic;
using mxh::ui::cTabDialog;
using mxh::ui::cWindow;
using mxh::ui::PetStateAction;

namespace {

constexpr std::uint32_t ClickEvent() {
    return static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick);
}

std::unique_ptr<cStatic> MakeStatic(std::int32_t id) {
    auto control = std::make_unique<cStatic>();
    control->Init(0, 0, 20, 10, nullptr, id);
    return control;
}

std::unique_ptr<cGuagen> MakeGauge(std::int32_t id) {
    auto control = std::make_unique<cGuagen>();
    control->Init(0, 0, 20, 10, nullptr, id);
    return control;
}

std::unique_ptr<cButton> MakeButton(std::int32_t id) {
    auto control = std::make_unique<cButton>();
    control->Init(0, 0, 20, 10, nullptr, nullptr, nullptr, {}, nullptr, id);
    return control;
}

std::unique_ptr<cPushupButton> MakeTabButton(std::int32_t id) {
    auto control = std::make_unique<cPushupButton>();
    control->Init(0, 0, 20, 10, nullptr, nullptr, nullptr, nullptr, nullptr, id);
    return control;
}

struct LinkedControls {
    cStatic* name = nullptr;
    cStatic* grade = nullptr;
    cStatic* state = nullptr;
    cStatic* friendship = nullptr;
    cStatic* stamina = nullptr;
    cStatic* face = nullptr;
    cStatic* inventoryNumber = nullptr;
    std::array<cStatic*, cPetStateDlg::kSkillCount> skills{};
    cGuagen* friendshipGauge = nullptr;
    cGuagen* staminaGauge = nullptr;
    cButton* seal = nullptr;
    cButton* useRest = nullptr;
    cButton* inventory = nullptr;
    cButton* toggle = nullptr;
};

template <typename Control>
Control* AddToSheet(cDialog& sheet, std::unique_ptr<Control> control) {
    Control* raw = control.get();
    sheet.Add(std::move(control));
    return raw;
}

template <typename Control>
Control* AddToTop(cPetStateDlg& dialog, std::unique_ptr<Control> control) {
    Control* raw = control.get();
    dialog.Add(control.release());
    return raw;
}

LinkedControls BuildLinkedTree(cPetStateDlg& dialog) {
    LinkedControls controls;
    dialog.InitTab(2);
    controls.name = AddToTop(dialog, MakeStatic(cPetStateDlg::kNameId));
    controls.seal = AddToTop(dialog, MakeButton(cPetStateDlg::kLockBtnId));
    controls.useRest = AddToTop(dialog, MakeButton(cPetStateDlg::kUseRestBtnId));
    controls.inventory = AddToTop(dialog, MakeButton(cPetStateDlg::kInvenBtnId));
    controls.toggle = AddToTop(dialog, MakeButton(cPetStateDlg::kToggleBtnId));

    auto statusSheet = std::make_unique<cDialog>();
    statusSheet->Init(0, 0, 100, 80, nullptr, cPetStateDlg::kSheet1Id);
    controls.grade = AddToSheet(*statusSheet, MakeStatic(cPetStateDlg::kGradeId));
    controls.state = AddToSheet(*statusSheet, MakeStatic(cPetStateDlg::kStateId));
    controls.friendship = AddToSheet(*statusSheet, MakeStatic(cPetStateDlg::kFriendTextId));
    controls.stamina = AddToSheet(*statusSheet, MakeStatic(cPetStateDlg::kStaminaTextId));
    controls.face = AddToSheet(*statusSheet, MakeStatic(cPetStateDlg::kImageId));
    controls.friendshipGauge = AddToSheet(*statusSheet, MakeGauge(cPetStateDlg::kFriendGaugeId));
    controls.staminaGauge = AddToSheet(*statusSheet, MakeGauge(cPetStateDlg::kStaminaGaugeId));
    dialog.Add(statusSheet.release());

    auto inventorySheet = std::make_unique<cDialog>();
    inventorySheet->Init(0, 0, 100, 80, nullptr, cPetStateDlg::kSheet2Id);
    controls.inventoryNumber = AddToSheet(*inventorySheet, MakeStatic(cPetStateDlg::kInvenNumId));
    controls.skills[0] = AddToSheet(*inventorySheet, MakeStatic(cPetStateDlg::kSkill1Id));
    controls.skills[1] = AddToSheet(*inventorySheet, MakeStatic(cPetStateDlg::kSkill2Id));
    controls.skills[2] = AddToSheet(*inventorySheet, MakeStatic(cPetStateDlg::kSkill3Id));
    dialog.Add(inventorySheet.release());
    return controls;
}

void Increment(void* userData) {
    ++*static_cast<int*>(userData);
}

} // namespace

TEST(PetStateDlgTest, InheritsTabDialogAndIsNonCopyable) {
    static_assert(std::is_base_of_v<cTabDialog, cPetStateDlg>);
    static_assert(!std::is_copy_constructible_v<cPetStateDlg>);
    static_assert(!std::is_copy_assignable_v<cPetStateDlg>);
    SUCCEED();
}

TEST(PetStateDlgTest, ConstantsMatchLegacyWindowIds) {
    EXPECT_EQ(cPetStateDlg::kDialogId, 1466);
    EXPECT_EQ(cPetStateDlg::kSheet1Id, 1467);
    EXPECT_EQ(cPetStateDlg::kSheet2Id, 1468);
    EXPECT_EQ(cPetStateDlg::kLockBtnId, 1469);
    EXPECT_EQ(cPetStateDlg::kUseRestBtnId, 1470);
    EXPECT_EQ(cPetStateDlg::kInvenBtnId, 1471);
    EXPECT_EQ(cPetStateDlg::kToggleBtnId, 1472);
    EXPECT_EQ(cPetStateDlg::kNameId, 1473);
    EXPECT_EQ(cPetStateDlg::kGradeId, 1474);
    EXPECT_EQ(cPetStateDlg::kStateId, 1475);
    EXPECT_EQ(cPetStateDlg::kImageId, 1476);
    EXPECT_EQ(cPetStateDlg::kFriendGaugeId, 1477);
    EXPECT_EQ(cPetStateDlg::kStaminaGaugeId, 1478);
    EXPECT_EQ(cPetStateDlg::kFriendTextId, 1479);
    EXPECT_EQ(cPetStateDlg::kStaminaTextId, 1480);
    EXPECT_EQ(cPetStateDlg::kInvenNumId, 1481);
    EXPECT_EQ(cPetStateDlg::kSkill1Id, 1482);
    EXPECT_EQ(cPetStateDlg::kSkill2Id, 1483);
    EXPECT_EQ(cPetStateDlg::kSkill3Id, 1484);
}

TEST(PetStateDlgTest, ConstructorInitializesAllLegacyPointersToNull) {
    cPetStateDlg dialog;
    EXPECT_EQ(dialog.GetNameTextWin(), nullptr);
    EXPECT_EQ(dialog.GetGradeTextWin(), nullptr);
    EXPECT_EQ(dialog.GetUseRestTextWin(), nullptr);
    EXPECT_EQ(dialog.GetFriendShipTextWin(), nullptr);
    EXPECT_EQ(dialog.GetStaminaTextWin(), nullptr);
    EXPECT_EQ(dialog.Get2DImageWin(), nullptr);
    EXPECT_EQ(dialog.GetInvenNumTextWin(), nullptr);
    EXPECT_EQ(dialog.GetFriendShipGuage(), nullptr);
    EXPECT_EQ(dialog.GetStaminaGuage(), nullptr);
    EXPECT_EQ(dialog.petSealButton(), nullptr);
    EXPECT_EQ(dialog.petUseRestButton(), nullptr);
    EXPECT_EQ(dialog.petInventoryButton(), nullptr);
    EXPECT_EQ(dialog.dialogToggleButton(), nullptr);
    for (std::size_t index = 0; index < cPetStateDlg::kSkillCount; ++index) {
        EXPECT_EQ(dialog.GetPetBuffTextWin()[index], nullptr);
    }
}

TEST(PetStateDlgTest, AddPushupButtonRoutesToTabButton) {
    cPetStateDlg dialog;
    dialog.InitTab(2);
    auto button = MakeTabButton(40);
    cPushupButton* raw = button.get();
    dialog.Add(button.release());
    EXPECT_EQ(dialog.GetTabBtn(0), raw);
    EXPECT_EQ(dialog.curIdx1(), 1);
    EXPECT_EQ(dialog.componentCount(), 0u);
}

TEST(PetStateDlgTest, AddDialogRoutesToTabSheet) {
    cPetStateDlg dialog;
    dialog.InitTab(2);
    auto sheet = std::make_unique<cDialog>();
    cDialog* raw = sheet.get();
    dialog.Add(sheet.release());
    EXPECT_EQ(dialog.GetTabSheet(0), raw);
    EXPECT_EQ(dialog.curIdx2(), 1);
    EXPECT_EQ(dialog.componentCount(), 0u);
}

TEST(PetStateDlgTest, AddOrdinaryControlRoutesToDialogChildren) {
    cPetStateDlg dialog;
    dialog.InitTab(2);
    auto control = MakeStatic(88);
    cStatic* raw = control.get();
    dialog.Add(control.release());
    ASSERT_EQ(dialog.componentCount(), 1u);
    EXPECT_EQ(dialog.componentAt(0), raw);
    EXPECT_EQ(dialog.curIdx1(), 0);
    EXPECT_EQ(dialog.curIdx2(), 0);
}

TEST(PetStateDlgTest, AddNullIsDefensivelyIgnored) {
    cPetStateDlg dialog;
    dialog.InitTab(2);
    dialog.Add(nullptr);
    EXPECT_EQ(dialog.componentCount(), 0u);
    EXPECT_EQ(dialog.curIdx1(), 0);
    EXPECT_EQ(dialog.curIdx2(), 0);
}

TEST(PetStateDlgTest, AddUsesIndependentLegacyTabIndices) {
    cPetStateDlg dialog;
    dialog.InitTab(2);
    dialog.Add(MakeTabButton(1).release());
    dialog.Add(std::make_unique<cDialog>().release());
    dialog.Add(MakeTabButton(2).release());
    dialog.Add(std::make_unique<cDialog>().release());
    EXPECT_EQ(dialog.curIdx1(), 2);
    EXPECT_EQ(dialog.curIdx2(), 2);
    EXPECT_NE(dialog.GetTabBtn(1), nullptr);
    EXPECT_NE(dialog.GetTabSheet(1), nullptr);
}

TEST(PetStateDlgTest, LinkingResolvesControlsAcrossTopAndBothSheets) {
    cPetStateDlg dialog;
    const LinkedControls controls = BuildLinkedTree(dialog);
    dialog.Linking();
    EXPECT_EQ(dialog.GetNameTextWin(), controls.name);
    EXPECT_EQ(dialog.GetGradeTextWin(), controls.grade);
    EXPECT_EQ(dialog.GetUseRestTextWin(), controls.state);
    EXPECT_EQ(dialog.GetFriendShipTextWin(), controls.friendship);
    EXPECT_EQ(dialog.GetStaminaTextWin(), controls.stamina);
    EXPECT_EQ(dialog.Get2DImageWin(), controls.face);
    EXPECT_EQ(dialog.GetInvenNumTextWin(), controls.inventoryNumber);
    EXPECT_EQ(dialog.GetFriendShipGuage(), controls.friendshipGauge);
    EXPECT_EQ(dialog.GetStaminaGuage(), controls.staminaGauge);
    EXPECT_EQ(dialog.petSealButton(), controls.seal);
    EXPECT_EQ(dialog.petUseRestButton(), controls.useRest);
    EXPECT_EQ(dialog.petInventoryButton(), controls.inventory);
    EXPECT_EQ(dialog.dialogToggleButton(), controls.toggle);
    EXPECT_EQ(dialog.GetPetBuffTextWin()[0], controls.skills[0]);
    EXPECT_EQ(dialog.GetPetBuffTextWin()[1], controls.skills[1]);
    EXPECT_EQ(dialog.GetPetBuffTextWin()[2], controls.skills[2]);
}

TEST(PetStateDlgTest, LinkingWithoutTabSheetsLeavesSheetControlsNull) {
    cPetStateDlg dialog;
    dialog.InitTab(2);
    dialog.Linking();
    EXPECT_EQ(dialog.GetGradeTextWin(), nullptr);
    EXPECT_EQ(dialog.GetFriendShipGuage(), nullptr);
    EXPECT_EQ(dialog.GetInvenNumTextWin(), nullptr);
    EXPECT_EQ(dialog.GetPetBuffTextWin()[2], nullptr);
}

TEST(PetStateDlgTest, LinkingRejectsControlsWithWrongRuntimeType) {
    cPetStateDlg dialog;
    dialog.InitTab(2);
    AddToTop(dialog, MakeButton(cPetStateDlg::kNameId));
    auto statusSheet = std::make_unique<cDialog>();
    AddToSheet(*statusSheet, MakeButton(cPetStateDlg::kGradeId));
    dialog.Add(statusSheet.release());
    dialog.Linking();
    EXPECT_EQ(dialog.GetNameTextWin(), nullptr);
    EXPECT_EQ(dialog.GetGradeTextWin(), nullptr);
}

TEST(PetStateDlgTest, TestInjectionPublishesEveryLegacyControl) {
    cPetStateDlg dialog;
    cStatic name, grade, state, friendship, stamina, face, inventoryNumber;
    cStatic skill1, skill2, skill3;
    cGuagen friendshipGauge, staminaGauge;
    cButton seal, useRest, inventory, toggle;
    const std::array<cStatic*, cPetStateDlg::kSkillCount> skills{
        &skill1, &skill2, &skill3
    };
    dialog.SetControlsForTest(
        &name, &grade, &state, &friendship, &stamina, &face,
        &inventoryNumber, skills, &friendshipGauge, &staminaGauge,
        &seal, &useRest, &inventory, &toggle);
    EXPECT_EQ(dialog.GetNameTextWin(), &name);
    EXPECT_EQ(dialog.GetGradeTextWin(), &grade);
    EXPECT_EQ(dialog.GetUseRestTextWin(), &state);
    EXPECT_EQ(dialog.GetFriendShipTextWin(), &friendship);
    EXPECT_EQ(dialog.GetStaminaTextWin(), &stamina);
    EXPECT_EQ(dialog.Get2DImageWin(), &face);
    EXPECT_EQ(dialog.GetInvenNumTextWin(), &inventoryNumber);
    EXPECT_EQ(dialog.GetFriendShipGuage(), &friendshipGauge);
    EXPECT_EQ(dialog.GetStaminaGuage(), &staminaGauge);
    EXPECT_EQ(dialog.petSealButton(), &seal);
    EXPECT_EQ(dialog.petUseRestButton(), &useRest);
    EXPECT_EQ(dialog.petInventoryButton(), &inventory);
    EXPECT_EQ(dialog.dialogToggleButton(), &toggle);
    EXPECT_EQ(dialog.GetPetBuffTextWin()[0], &skill1);
    EXPECT_EQ(dialog.GetPetBuffTextWin()[1], &skill2);
    EXPECT_EQ(dialog.GetPetBuffTextWin()[2], &skill3);
}

TEST(PetStateDlgTest, SealClickDispatchesSealCallback) {
    cPetStateDlg dialog;
    int calls = 0;
    dialog.SetPetActionCallback(PetStateAction::Seal, &Increment, &calls);
    dialog.OnActionEvent(cPetStateDlg::kLockBtnId, nullptr, ClickEvent());
    EXPECT_EQ(calls, 1);
}

TEST(PetStateDlgTest, UseRestClickDispatchesUseRestCallback) {
    cPetStateDlg dialog;
    int calls = 0;
    dialog.SetPetActionCallback(PetStateAction::UseRest, &Increment, &calls);
    dialog.OnActionEvent(cPetStateDlg::kUseRestBtnId, nullptr, ClickEvent());
    EXPECT_EQ(calls, 1);
}

TEST(PetStateDlgTest, InventoryClickDispatchesInventoryCallback) {
    cPetStateDlg dialog;
    int calls = 0;
    dialog.SetPetActionCallback(PetStateAction::Inventory, &Increment, &calls);
    dialog.OnActionEvent(cPetStateDlg::kInvenBtnId, nullptr, ClickEvent());
    EXPECT_EQ(calls, 1);
}

TEST(PetStateDlgTest, ToggleClickDispatchesToggleCallback) {
    cPetStateDlg dialog;
    int calls = 0;
    dialog.SetPetActionCallback(PetStateAction::Toggle, &Increment, &calls);
    dialog.OnActionEvent(cPetStateDlg::kToggleBtnId, nullptr, ClickEvent());
    EXPECT_EQ(calls, 1);
}

TEST(PetStateDlgTest, UnknownButtonIdDoesNotDispatch) {
    cPetStateDlg dialog;
    int calls = 0;
    dialog.SetPetActionCallback(PetStateAction::Seal, &Increment, &calls);
    dialog.OnActionEvent(99999, nullptr, ClickEvent());
    EXPECT_EQ(calls, 0);
}

TEST(PetStateDlgTest, NonClickEventDoesNotDispatch) {
    cPetStateDlg dialog;
    int calls = 0;
    dialog.SetPetActionCallback(PetStateAction::Seal, &Increment, &calls);
    dialog.OnActionEvent(
        cPetStateDlg::kLockBtnId, nullptr,
        static_cast<std::uint32_t>(cWindow::WindowEvent::MouseMove));
    EXPECT_EQ(calls, 0);
}

TEST(PetStateDlgTest, UnboundActionIsSafeNoOp) {
    cPetStateDlg dialog;
    dialog.OnActionEvent(cPetStateDlg::kLockBtnId, nullptr, ClickEvent());
    SUCCEED();
}

TEST(PetStateDlgTest, ActionCallbacksKeepIndependentUserData) {
    cPetStateDlg dialog;
    int sealCalls = 0;
    int toggleCalls = 0;
    dialog.SetPetActionCallback(PetStateAction::Seal, &Increment, &sealCalls);
    dialog.SetPetActionCallback(PetStateAction::Toggle, &Increment, &toggleCalls);
    dialog.OnActionEvent(cPetStateDlg::kLockBtnId, nullptr, ClickEvent());
    dialog.OnActionEvent(cPetStateDlg::kToggleBtnId, nullptr, ClickEvent());
    dialog.OnActionEvent(cPetStateDlg::kToggleBtnId, nullptr, ClickEvent());
    EXPECT_EQ(sealCalls, 1);
    EXPECT_EQ(toggleCalls, 2);
}

TEST(PetStateDlgTest, NullCallbackClearsExistingBinding) {
    cPetStateDlg dialog;
    int calls = 0;
    dialog.SetPetActionCallback(PetStateAction::Seal, &Increment, &calls);
    dialog.SetPetActionCallback(PetStateAction::Seal, nullptr);
    dialog.OnActionEvent(cPetStateDlg::kLockBtnId, nullptr, ClickEvent());
    EXPECT_EQ(calls, 0);
}

TEST(PetStateDlgTest, InvalidActionBindingIsIgnored) {
    cPetStateDlg dialog;
    int calls = 0;
    dialog.SetPetActionCallback(static_cast<PetStateAction>(99), &Increment, &calls);
    dialog.OnActionEvent(cPetStateDlg::kLockBtnId, nullptr, ClickEvent());
    EXPECT_EQ(calls, 0);
}

TEST(PetStateDlgTest, SetBtnClickPreservesLegacyNoOpBody) {
    cPetStateDlg dialog;
    int calls = 0;
    dialog.SetPetActionCallback(PetStateAction::UseRest, &Increment, &calls);
    dialog.SetBtnClick(0);
    dialog.SetBtnClick(1);
    dialog.SetBtnClick(-1);
    EXPECT_EQ(calls, 0);
}

TEST(PetStateDlgTest, PetBuffGetterReturnsStableThreeSlotArray) {
    cPetStateDlg dialog;
    cStatic skill1, skill2, skill3;
    const std::array<cStatic*, cPetStateDlg::kSkillCount> skills{
        &skill1, &skill2, &skill3
    };
    dialog.SetControlsForTest(
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, skills,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    cStatic** first = dialog.GetPetBuffTextWin();
    cStatic** second = dialog.GetPetBuffTextWin();
    EXPECT_EQ(first, second);
    EXPECT_EQ(first[0], &skill1);
    EXPECT_EQ(first[2], &skill3);
}
