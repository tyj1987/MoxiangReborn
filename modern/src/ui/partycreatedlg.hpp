#pragma once

#include "legacy_window_event.hpp"

#include "mxh/ui/cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;
class cComboBox;
class cCheckBox;
class cEditBox;

enum class PartyDivisionOption : std::uint8_t {
    Random = 0,
    Damage = 1,
    Unknown = 255,
};

struct PartyCreateOptions {
    char theme[32]{};
    std::uint16_t minLevel = 0;
    std::uint16_t maxLevel = 0;
    bool isPublic = true;
    PartyDivisionOption division = PartyDivisionOption::Random;
    std::uint16_t limitCount = 0;
};

class cPartyCreateDlg final : public cDialog {
public:
    using CreateSynFn = bool (*)(const PartyCreateOptions& opts,
                                 void* userData);
    using ChatMessageFn = const char* (*)(std::int32_t messageId,
                                          void* userData);
    using ResourceMsgFn = const char* (*)(std::int32_t messageId,
                                          void* userData);
    using HasPartyFn = bool (*)(void* userData);

    static constexpr std::int32_t kThemeEditId = 391;
    static constexpr std::int32_t kMinLevelEditId = 392;
    static constexpr std::int32_t kMaxLevelEditId = 393;
    static constexpr std::int32_t kPublicCheckId = 394;
    static constexpr std::int32_t kPrivateCheckId = 395;
    static constexpr std::int32_t kDistributeComboId = 396;
    static constexpr std::int32_t kMemberNumComboId = 397;
    static constexpr std::int32_t kOkButtonId = 398;
    static constexpr std::int32_t kCancelButtonId = 399;

    static constexpr std::int32_t kActionBtnClick = legacy_window_event::kButtonClick;
    static constexpr std::int32_t kMaxPartyNameLength = 15;
    static constexpr std::int32_t kChatPartyNameTooLong = 1742;
    static constexpr std::int32_t kResourceRandomOption = 483;
    static constexpr std::int32_t kResourceDamageOption = 484;
    static constexpr std::uint16_t kDefaultMinLevel = 1;
    static constexpr std::uint16_t kDefaultMaxLevel = 99;
    static constexpr std::size_t kDistributeBufferSize = 16;

    cPartyCreateDlg();
    ~cPartyCreateDlg() override;

    cPartyCreateDlg(const cPartyCreateDlg&) = delete;
    cPartyCreateDlg& operator=(const cPartyCreateDlg&) = delete;

    void Linking();
    void SetActive(bool val) noexcept override;
    bool OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
    bool CreatePartySyn();

    void InitOption();

    void SetControlsForTest(cEditBox* theme, cEditBox* minLevel,
                            cEditBox* maxLevel, cCheckBox* publicCheck,
                            cCheckBox* privateCheck, cComboBox* distribute,
                            cComboBox* memberNum, cButton* okBtn,
                            cButton* cancelBtn) noexcept;

    void SetCallbacks(CreateSynFn createSyn, ChatMessageFn chatMsg,
                      ResourceMsgFn resourceMsg, HasPartyFn hasParty,
                      void* userData = nullptr) noexcept;

    bool isProcessing() const noexcept { return m_bProcessing; }

    cEditBox* themeEdit() const noexcept { return m_pThemeEdit; }
    cEditBox* minLevelEdit() const noexcept { return m_pMinLevelEdit; }
    cEditBox* maxLevelEdit() const noexcept { return m_pMaxLevelEdit; }
    cCheckBox* publicCheck() const noexcept { return m_pPublicCheck; }
    cCheckBox* privateCheck() const noexcept { return m_pPrivateCheck; }
    cComboBox* distributeCombo() const noexcept { return m_pDistribute; }
    cComboBox* memberNumCombo() const noexcept { return m_pMemberNumCombo; }
    cButton* okButton() const noexcept { return m_pOKBtn; }
    cButton* cancelButton() const noexcept { return m_pCancelBtn; }

private:
    PartyDivisionOption ResolveDivisionOption(const char* text) const;
    void DispatchChatMessage(std::int32_t messageId);
    void ApplyOptionDefaults();

    cEditBox* m_pThemeEdit = nullptr;
    cEditBox* m_pMinLevelEdit = nullptr;
    cEditBox* m_pMaxLevelEdit = nullptr;
    cCheckBox* m_pPublicCheck = nullptr;
    cCheckBox* m_pPrivateCheck = nullptr;
    cComboBox* m_pDistribute = nullptr;
    cComboBox* m_pMemberNumCombo = nullptr;
    cButton* m_pOKBtn = nullptr;
    cButton* m_pCancelBtn = nullptr;

    bool m_bProcessing = false;

    CreateSynFn m_createSynFn = nullptr;
    ChatMessageFn m_chatMsgFn = nullptr;
    ResourceMsgFn m_resourceMsgFn = nullptr;
    HasPartyFn m_hasPartyFn = nullptr;
    void* m_callbackUserData = nullptr;
};

} // namespace mxh::ui
