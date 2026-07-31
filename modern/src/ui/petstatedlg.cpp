#include "petstatedlg.hpp"

#include "cButton.hpp"
#include "cDialog.hpp"
#include "cGuagen.hpp"
#include "cStatic.hpp"
#include "cpushupbutton.hpp"
#include "cWindow.hpp"

#include <memory>

namespace mxh::ui {
namespace {

template <typename Control>
Control* FindControl(cDialog* dialog, std::int32_t id) {
    if (!dialog) {
        return nullptr;
    }
    return dynamic_cast<Control*>(dialog->findWindowById(id));
}

} // namespace

cPetStateDlg::cPetStateDlg() = default;
cPetStateDlg::~cPetStateDlg() = default;

void cPetStateDlg::Add(cWindow* window) {
    if (!window) {
        return;
    }

    if (auto* button = dynamic_cast<cPushupButton*>(window)) {
        AddTabBtn(curIdx1_++, std::unique_ptr<cPushupButton>(button));
    } else if (dynamic_cast<cDialog*>(window) != nullptr) {
        AddTabSheet(curIdx2_++, std::unique_ptr<cWindow>(window));
    } else {
        cDialog::Add(std::unique_ptr<cWindow>(window));
    }
}

void cPetStateDlg::Linking() {
    m_pNameDlg = dynamic_cast<cStatic*>(FindAnyWindowForID(kNameId));
    m_pPetSealBtn = dynamic_cast<cButton*>(FindAnyWindowForID(kLockBtnId));
    m_pPetUseRestBtn = dynamic_cast<cButton*>(FindAnyWindowForID(kUseRestBtnId));
    m_pPetInvenBtn = dynamic_cast<cButton*>(FindAnyWindowForID(kInvenBtnId));
    m_pDlgToggleBtn = dynamic_cast<cButton*>(FindAnyWindowForID(kToggleBtnId));

    auto* statusSheet = dynamic_cast<cDialog*>(GetTabSheet(0));
    m_pGradeDlg = FindControl<cStatic>(statusSheet, kGradeId);
    m_pStateDlg = FindControl<cStatic>(statusSheet, kStateId);
    m_pFriend = FindControl<cStatic>(statusSheet, kFriendTextId);
    m_pStamina = FindControl<cStatic>(statusSheet, kStaminaTextId);
    m_pPetFaceImg = FindControl<cStatic>(statusSheet, kImageId);
    m_pFriendGuage = FindControl<cGuagen>(statusSheet, kFriendGaugeId);
    m_pStaminaGuage = FindControl<cGuagen>(statusSheet, kStaminaGaugeId);

    auto* inventorySheet = dynamic_cast<cDialog*>(GetTabSheet(1));
    m_pInvenNum = FindControl<cStatic>(inventorySheet, kInvenNumId);
    m_pSkill[0] = FindControl<cStatic>(inventorySheet, kSkill1Id);
    m_pSkill[1] = FindControl<cStatic>(inventorySheet, kSkill2Id);
    m_pSkill[2] = FindControl<cStatic>(inventorySheet, kSkill3Id);
}

void cPetStateDlg::OnActionEvent(std::int32_t lId, void* /*p*/,
                                 std::uint32_t we) {
    if (we != static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick)) {
        return;
    }

    switch (lId) {
    case kLockBtnId:
        InvokeAction(PetStateAction::Seal);
        break;
    case kUseRestBtnId:
        InvokeAction(PetStateAction::UseRest);
        break;
    case kInvenBtnId:
        InvokeAction(PetStateAction::Inventory);
        break;
    case kToggleBtnId:
        InvokeAction(PetStateAction::Toggle);
        break;
    default:
        break;
    }
}

void cPetStateDlg::SetBtnClick(std::int32_t btnKind) noexcept {
    (void)btnKind;
}

void cPetStateDlg::SetPetActionCallback(PetStateAction action,
                                         PetActionCallback callback,
                                         void* userData) noexcept {
    const auto index = static_cast<std::size_t>(action);
    if (index >= m_petActionCallbacks.size()) {
        return;
    }
    m_petActionCallbacks[index] = {callback, userData};
}

void cPetStateDlg::SetControlsForTest(
    cStatic* name, cStatic* grade, cStatic* state,
    cStatic* friendship, cStatic* stamina, cStatic* faceImage,
    cStatic* inventoryNumber, const std::array<cStatic*, kSkillCount>& skills,
    cGuagen* friendshipGauge, cGuagen* staminaGauge,
    cButton* sealButton, cButton* useRestButton,
    cButton* inventoryButton, cButton* toggleButton) noexcept {
    m_pNameDlg = name;
    m_pGradeDlg = grade;
    m_pStateDlg = state;
    m_pFriend = friendship;
    m_pStamina = stamina;
    m_pPetFaceImg = faceImage;
    m_pInvenNum = inventoryNumber;
    m_pSkill = skills;
    m_pFriendGuage = friendshipGauge;
    m_pStaminaGuage = staminaGauge;
    m_pPetSealBtn = sealButton;
    m_pPetUseRestBtn = useRestButton;
    m_pPetInvenBtn = inventoryButton;
    m_pDlgToggleBtn = toggleButton;
}

void cPetStateDlg::InvokeAction(PetStateAction action) const {
    const auto index = static_cast<std::size_t>(action);
    if (index >= m_petActionCallbacks.size()) {
        return;
    }
    const CallbackSlot& slot = m_petActionCallbacks[index];
    if (slot.callback) {
        slot.callback(slot.userData);
    }
}

} // namespace mxh::ui
