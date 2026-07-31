#pragma once

#include "ctabdialog.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace mxh::ui {

class cButton;
class cGuagen;
class cStatic;
class cWindow;

enum class PetStateAction : std::size_t {
    Seal = 0,
    UseRest,
    Inventory,
    Toggle,
    Count
};

class cPetStateDlg final : public cTabDialog {
public:
    using PetActionCallback = void (*)(void* userData);

    static constexpr std::size_t kSkillCount = 3;
    static constexpr std::int32_t kDialogId = 1507;
    static constexpr std::int32_t kSheet1Id = 1508;
    static constexpr std::int32_t kSheet2Id = 1509;
    static constexpr std::int32_t kLockBtnId = 1510;
    static constexpr std::int32_t kUseRestBtnId = 1511;
    static constexpr std::int32_t kInvenBtnId = 1512;
    static constexpr std::int32_t kToggleBtnId = 1513;
    static constexpr std::int32_t kNameId = 1514;
    static constexpr std::int32_t kGradeId = 1515;
    static constexpr std::int32_t kStateId = 1516;
    static constexpr std::int32_t kImageId = 1517;
    static constexpr std::int32_t kFriendGaugeId = 1518;
    static constexpr std::int32_t kStaminaGaugeId = 1519;
    static constexpr std::int32_t kFriendTextId = 1520;
    static constexpr std::int32_t kStaminaTextId = 1521;
    static constexpr std::int32_t kInvenNumId = 1522;
    static constexpr std::int32_t kSkill1Id = 1523;
    static constexpr std::int32_t kSkill2Id = 1524;
    static constexpr std::int32_t kSkill3Id = 1525;

    cPetStateDlg();
    ~cPetStateDlg() override;

    cPetStateDlg(const cPetStateDlg&) = delete;
    cPetStateDlg& operator=(const cPetStateDlg&) = delete;

    void Add(cWindow* window);
    void Linking();
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
    void SetBtnClick(std::int32_t btnKind) noexcept;

    cStatic* GetFriendShipTextWin() const noexcept { return m_pFriend; }
    cStatic* GetStaminaTextWin() const noexcept { return m_pStamina; }
    cGuagen* GetFriendShipGuage() const noexcept { return m_pFriendGuage; }
    cGuagen* GetStaminaGuage() const noexcept { return m_pStaminaGuage; }
    cStatic* GetNameTextWin() const noexcept { return m_pNameDlg; }
    cStatic* GetGradeTextWin() const noexcept { return m_pGradeDlg; }
    cStatic* Get2DImageWin() const noexcept { return m_pPetFaceImg; }
    cStatic* GetInvenNumTextWin() const noexcept { return m_pInvenNum; }
    cStatic* GetUseRestTextWin() const noexcept { return m_pStateDlg; }
    cStatic** GetPetBuffTextWin() noexcept { return m_pSkill.data(); }
    cStatic* const* GetPetBuffTextWin() const noexcept { return m_pSkill.data(); }

    cButton* petSealButton() const noexcept { return m_pPetSealBtn; }
    cButton* petUseRestButton() const noexcept { return m_pPetUseRestBtn; }
    cButton* petInventoryButton() const noexcept { return m_pPetInvenBtn; }
    cButton* dialogToggleButton() const noexcept { return m_pDlgToggleBtn; }

    void SetPetActionCallback(PetStateAction action,
                              PetActionCallback callback,
                              void* userData = nullptr) noexcept;

    void SetControlsForTest(
        cStatic* name, cStatic* grade, cStatic* state,
        cStatic* friendship, cStatic* stamina, cStatic* faceImage,
        cStatic* inventoryNumber, const std::array<cStatic*, kSkillCount>& skills,
        cGuagen* friendshipGauge, cGuagen* staminaGauge,
        cButton* sealButton, cButton* useRestButton,
        cButton* inventoryButton, cButton* toggleButton) noexcept;

private:
    struct CallbackSlot {
        PetActionCallback callback = nullptr;
        void* userData = nullptr;
    };

    void InvokeAction(PetStateAction action) const;

    cStatic* m_pNameDlg = nullptr;
    cStatic* m_pGradeDlg = nullptr;
    cStatic* m_pStateDlg = nullptr;
    cStatic* m_pFriend = nullptr;
    cStatic* m_pStamina = nullptr;
    cStatic* m_pPetFaceImg = nullptr;
    cStatic* m_pInvenNum = nullptr;
    std::array<cStatic*, kSkillCount> m_pSkill{};
    cGuagen* m_pFriendGuage = nullptr;
    cGuagen* m_pStaminaGuage = nullptr;
    cButton* m_pPetSealBtn = nullptr;
    cButton* m_pPetUseRestBtn = nullptr;
    cButton* m_pPetInvenBtn = nullptr;
    cButton* m_pDlgToggleBtn = nullptr;
    std::array<CallbackSlot, static_cast<std::size_t>(PetStateAction::Count)>
        m_petActionCallbacks{};
};

} // namespace mxh::ui
