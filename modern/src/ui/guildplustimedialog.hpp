#pragma once

#include "mxh/ui/cDialog.hpp"

#include <cstddef>
#include <cstdint>

namespace mxh::ui {

class cStatic;
class cListDialog;

enum class GuildPlusTimeKind : std::uint8_t {
    Suryun = 0,
    Mugong = 1,
    Exp = 2,
    DamageUp = 3,
    Unknown = 255,
};

struct GuildPlusTimeEntry {
    GuildPlusTimeKind kind = GuildPlusTimeKind::Unknown;
    std::int32_t addData = 0;
    std::uint32_t needPoint = 0;
};

class cGuildPlusTimeDialog final : public cDialog {
public:
    using ChatTextFn = const char* (*)(std::int32_t messageId, void* userData);
    using UseGuildPointFn = void (*)(std::int32_t forKind, std::int32_t slot, void* userData);
    using PlustimeCountFn = std::size_t (*)(void* userData);
    using PlustimeEntryFn = GuildPlusTimeKind (*)(std::size_t index,
                                                  std::int32_t* addData,
                                                  std::uint32_t* needPoint,
                                                  void* userData);

    static constexpr std::int32_t kCloseButtonId = 364;
    static constexpr std::int32_t kPlustimeStartButtonId = 513;
    static constexpr std::int32_t kPointStaticId = 516;
    static constexpr std::int32_t kPlustimeListId = 517;

    static constexpr std::int32_t kNoSelection = -1;
    static constexpr std::int32_t kGuildPlusTimeForKind = 0;
    static constexpr std::uint32_t kDefaultInitialPoints = 0;
    static constexpr std::uint32_t kDefaultTextColor = 0xFFFFFFFFu;
    static constexpr std::int32_t kSuryunMessageId = 1377;
    static constexpr std::int32_t kMugongMessageId = 1378;
    static constexpr std::int32_t kExpMessageId = 1379;
    static constexpr std::int32_t kDamageUpMessageId = 1380;
    static constexpr std::int32_t kActionBtnClick = 0x0001;

    cGuildPlusTimeDialog();
    ~cGuildPlusTimeDialog() override;

    cGuildPlusTimeDialog(const cGuildPlusTimeDialog&) = delete;
    cGuildPlusTimeDialog& operator=(const cGuildPlusTimeDialog&) = delete;

    void Linking();
    void SetActive(bool val) noexcept override;
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
    void HandleMouseAction(std::int32_t mouseX, std::int32_t mouseY,
                           std::uint32_t weFromDialog);

    void SetGuildPointText(std::uint32_t guildPoint);
    void LoadPlustimeList();

    int currentSelectedItem() const noexcept { return m_CurrentSelectedItem; }

    void SetControlsForTest(cStatic* point, cListDialog* list) noexcept;

    void SetCallbacks(ChatTextFn chatText,
                      UseGuildPointFn useGuildPoint,
                      PlustimeCountFn getCount,
                      PlustimeEntryFn getEntry,
                      void* userData = nullptr) noexcept;

    void SetPlustimeEntries(const GuildPlusTimeEntry* entries,
                            std::size_t count) noexcept;

    cStatic* pointStatic() const noexcept { return m_pCurrentHavePoint; }
    cListDialog* plusItemList() const noexcept { return m_pPlusItemList; }

private:
    void DispatchStartButton();
    void FormatThousands(char* buffer, std::int32_t value) const;
    const char* ResolveChatFormat(std::int32_t messageId) const;

    cStatic* m_pCurrentHavePoint = nullptr;
    cListDialog* m_pPlusItemList = nullptr;
    std::int32_t m_CurrentSelectedItem = kNoSelection;

    ChatTextFn m_chatTextFn = nullptr;
    UseGuildPointFn m_useGuildPointFn = nullptr;
    PlustimeCountFn m_getCountFn = nullptr;
    PlustimeEntryFn m_getEntryFn = nullptr;
    void* m_callbackUserData = nullptr;

    const GuildPlusTimeEntry* m_testEntries = nullptr;
    std::size_t m_testCount = 0;
};

} // namespace mxh::ui
