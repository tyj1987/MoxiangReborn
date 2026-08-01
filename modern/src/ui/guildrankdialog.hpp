#pragma once

#include "mxh/ui/cDialog.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace mxh::ui {

class cButton;
class cComboBox;
class cTextArea;

enum class GuildRankMode : std::uint8_t {
    Dan = 0,
    Guild = 1,
    Max = 2
};

struct GuildRankSelection {
    std::uint32_t selectedMemberId = 0;
    std::uint32_t heroId = 0;
    std::string selectedMemberName;
};

class cGuildRankDialog final : public cDialog {
public:
    using ChatTextCallback = const char* (*)(std::int32_t messageId, void* userData);
    using SystemMessageCallback = void (*)(const char* message, void* userData);

    static constexpr std::uint8_t kUnsetMode = 255;
    static constexpr std::uint8_t kMaxGuildLevel = 5;
    static constexpr std::int32_t kDialogId = 546;
    static constexpr std::int32_t kDanRankComboId = 547;
    static constexpr std::int32_t kGuildRankComboId = 548;
    static constexpr std::int32_t kDanOkButtonId = 549;
    static constexpr std::int32_t kGuildOkButtonId = 550;
    static constexpr std::int32_t kMemberNameId = 551;
    static constexpr std::int32_t kInvalidSelectionMessageId = 714;
    static constexpr std::int32_t kMemberNameFormatMessageId = 718;

    cGuildRankDialog();
    ~cGuildRankDialog() override;

    cGuildRankDialog(const cGuildRankDialog&) = delete;
    cGuildRankDialog& operator=(const cGuildRankDialog&) = delete;

    void SetActive(bool val) noexcept override;
    void Linking();
    void ShowGuildRankMode(std::uint8_t guildLevel);
    void SetActiveGuildRankMode(std::int32_t showMode, bool active) noexcept;
    void SetName(const char* name);

    void SetSelection(const GuildRankSelection& selection) { m_selection = selection; }
    void SetChatCallbacks(ChatTextCallback chatText,
                          SystemMessageCallback systemMessage,
                          void* userData = nullptr) noexcept;
    void SetControlsForTest(cTextArea* memberName, cComboBox* guildRank,
                            cComboBox* danRank, cButton* guildOk,
                            cButton* danOk) noexcept;

    std::uint8_t currentMode() const noexcept { return m_CurGuildRankMode; }
    cTextArea* memberNameControl() const noexcept { return m_pRankMemberName; }
    cComboBox* guildRankCombo() const noexcept { return m_pRankComboBox; }
    cComboBox* danRankCombo() const noexcept { return m_pDRankComboBox; }
    cButton* guildOkButton() const noexcept { return m_pOkBtn; }
    cButton* danOkButton() const noexcept { return m_pDOkBtn; }

private:
    static constexpr std::size_t kModeCount = 2;
    using ModeControls = std::array<cWindow*, 2>;

    cTextArea* m_pRankMemberName = nullptr;
    cComboBox* m_pRankComboBox = nullptr;
    cComboBox* m_pDRankComboBox = nullptr;
    cButton* m_pOkBtn = nullptr;
    cButton* m_pDOkBtn = nullptr;
    std::array<ModeControls, kModeCount> m_GuildRankCtrlList{};
    std::uint8_t m_CurGuildRankMode = kUnsetMode;
    GuildRankSelection m_selection{};
    ChatTextCallback m_chatTextCallback = nullptr;
    SystemMessageCallback m_systemMessageCallback = nullptr;
    void* m_callbackUserData = nullptr;
};

} // namespace mxh::ui
