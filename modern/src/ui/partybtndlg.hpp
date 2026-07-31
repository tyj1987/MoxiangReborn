// Modern 1:1 port of legacy CPartyBtnDlg.

#pragma once

#include "mxh/ui/cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;
class cStatic;

struct PartyState {
    std::int32_t partyIndex = 0;
    std::int32_t masterId = 0;
    std::int32_t heroId = 0;
};

class cPartyBtnDlg final : public cDialog {
public:
    static constexpr std::int32_t kBackgroundId = 502;
    static constexpr std::int32_t kSecedeButtonId = 503;
    static constexpr std::int32_t kTransferButtonId = 504;
    static constexpr std::int32_t kForcedSecedeButtonId = 505;
    static constexpr std::int32_t kBreakUpButtonId = 506;
    static constexpr std::int32_t kAddMemberButtonId = 507;
    static constexpr std::int32_t kWarSuggestButtonId = 508;
    static constexpr std::int32_t kOptionButtonId = 509;
    static constexpr std::int32_t kMemberButtonId = 510;

    static constexpr std::uint32_t kUsableImageColor = 0x00FFFFFFu;
    static constexpr std::uint32_t kDisabledImageColor = 0x00FF6464u;

    cPartyBtnDlg();
    ~cPartyBtnDlg() override;

    cPartyBtnDlg(const cPartyBtnDlg&) = delete;
    cPartyBtnDlg& operator=(const cPartyBtnDlg&) = delete;

    void Linking();
    void RefreshDlg();
    void ShowNonPartyDlg();
    void ShowPartyMasterDlg();
    void ShowPartyMemberDlg();
    void ShowOption(bool option);
    void Render() override;

    void SetPartyState(const PartyState& state) noexcept { m_partyState = state; }
    const PartyState& partyState() const noexcept { return m_partyState; }
    bool optionVisible() const noexcept { return m_bOption; }

    void SetControlsForTest(cStatic* background, cButton* secede,
                            cButton* transfer, cButton* forcedSecede,
                            cButton* addMember, cButton* breakUp,
                            cButton* warSuggest, cButton* option,
                            cButton* member) noexcept;

    cStatic* background() const noexcept { return m_pBackGround; }
    cButton* secedeButton() const noexcept { return m_pSecedeBtn; }
    cButton* transferButton() const noexcept { return m_pTransferBtn; }
    cButton* forcedSecedeButton() const noexcept { return m_pForcedSecedeBtn; }
    cButton* addMemberButton() const noexcept { return m_pAddMemberBtn; }
    cButton* breakUpButton() const noexcept { return m_pBreakUpBtn; }
    cButton* warSuggestButton() const noexcept { return m_pWarSuggestBtn; }
    cButton* optionButton() const noexcept { return m_pOptionBtn; }
    cButton* memberButton() const noexcept { return m_pMemberBtn; }

private:
    void SetActionButtonsActive(bool active);
    void SetActionImageColors(std::uint32_t secedeColor,
                              std::uint32_t otherColor);

    cStatic* m_pBackGround = nullptr;
    cButton* m_pSecedeBtn = nullptr;
    cButton* m_pTransferBtn = nullptr;
    cButton* m_pForcedSecedeBtn = nullptr;
    cButton* m_pAddMemberBtn = nullptr;
    cButton* m_pBreakUpBtn = nullptr;
    cButton* m_pWarSuggestBtn = nullptr;
    cButton* m_pOptionBtn = nullptr;
    cButton* m_pMemberBtn = nullptr;

    bool m_bOption = true;
    PartyState m_partyState{};
};

}  // namespace mxh::ui
