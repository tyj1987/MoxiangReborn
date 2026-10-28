#pragma once

#include "mxh/ui/cDialog.hpp"

#include <cstdint>
#include <string>

namespace mxh::ui {

class cObjectGuagen;
class cPartyBtnDlg;
class cPushupButton;
class cStatic;

struct PartyMemberData {
    std::uint32_t memberId = 0;
    bool logged = false;
    std::uint8_t lifePercent = 0;
    std::uint8_t shieldPercent = 0;
    std::uint8_t naeryukPercent = 0;
    std::string name;
    std::uint16_t level = 0;
    std::uint16_t posX = 0;
    std::uint16_t posZ = 0;
};

class cPartyMemberDlg final : public cDialog {
public:
    using ClickedMemberCallback = void (*)(std::uint32_t memberId, void* userData);

    static constexpr std::int32_t kMemberNameBaseId = 423;
    static constexpr std::int32_t kMemberLifeBaseId = 429;
    static constexpr std::int32_t kMemberNaeryukBaseId = 435;
    static constexpr std::int32_t kMemberLevelBaseId = 441;
    static constexpr std::int32_t kMemberSlotCount = 6;

    static constexpr std::uint32_t kLoginBasicColor = 0xFFFFFFFFu;
    static constexpr std::uint32_t kLoginOverColor = 0xFFFFFFFFu;
    static constexpr std::uint32_t kLoginPressColor = 0xFFFFEA00u;
    static constexpr std::uint32_t kLogoutBasicColor = 0xFFACB6C7u;
    static constexpr std::uint32_t kLogoutOverColor = 0xFFACB6C7u;
    static constexpr std::uint32_t kLogoutPressColor = 0xFFFFEA00u;

    cPartyMemberDlg();
    ~cPartyMemberDlg() override;

    cPartyMemberDlg(const cPartyMemberDlg&) = delete;
    cPartyMemberDlg& operator=(const cPartyMemberDlg&) = delete;

    void SetActive(bool val) noexcept override;
    void Linking(std::int32_t index);
    void SetMemberData(const PartyMemberData* info);
    void SetNameBtnPushUp(bool val);
    void SetPartyBtnDlg(cPartyBtnDlg* dialog) noexcept { m_pPartyBtnDlg = dialog; }
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;
    void Render() override;

    void ShowOption(bool option) noexcept { m_bOption = option; }
    void ShowMember(bool member) noexcept { m_bMember = member; }

    void SetClickedMemberCallback(ClickedMemberCallback callback,
                                  void* userData = nullptr) noexcept;
    void SetActionEventResultForTest(std::uint32_t result) noexcept;
    void ClearActionEventResultForTest() noexcept;
    void SetControlsForTest(cPushupButton* name, cObjectGuagen* life,
                            cObjectGuagen* naeryuk, cStatic* level) noexcept;

    std::uint32_t memberId() const noexcept { return m_MemberID; }
    bool realActive() const noexcept { return m_bRealActive; }
    std::int32_t memberIndex() const noexcept { return m_nIndex; }
    bool optionVisible() const noexcept { return m_bOption; }
    bool memberVisible() const noexcept { return m_bMember; }
    bool setTopOnActive() const noexcept { return m_bSetTopOnActive; }
    cPushupButton* nameControl() const noexcept { return m_pName; }
    cObjectGuagen* lifeGauge() const noexcept { return m_pLife; }
    cObjectGuagen* naeryukGauge() const noexcept { return m_pNaeryuk; }
    cStatic* levelControl() const noexcept { return m_pLevel; }
    cPartyBtnDlg* partyButtonDialog() const noexcept { return m_pPartyBtnDlg; }

private:
    cPushupButton* m_pName = nullptr;
    cObjectGuagen* m_pLife = nullptr;
    cObjectGuagen* m_pNaeryuk = nullptr;
    cStatic* m_pLevel = nullptr;
    cPartyBtnDlg* m_pPartyBtnDlg = nullptr;

    std::uint32_t m_MemberID = 0;
    bool m_bRealActive = false;
    bool m_bSetTopOnActive = false;
    std::int32_t m_nIndex = -1;
    bool m_bOption = true;
    bool m_bMember = true;

    ClickedMemberCallback m_clickedMemberCallback = nullptr;
    void* m_clickedMemberUserData = nullptr;
    bool m_hasActionEventResultForTest = false;
    std::uint32_t m_actionEventResultForTest = 0;
};

} // namespace mxh::ui
