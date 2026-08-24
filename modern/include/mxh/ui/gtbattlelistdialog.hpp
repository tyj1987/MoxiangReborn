#pragma once

#include "legacy_window_event.hpp"

#include "mxh/ui/cDialog.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mxh::ui {

class cListCtrl;

enum class GTBattleGroup : std::uint8_t {
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    Unknown = 255,
};

struct GTBattleInfo {
    std::uint32_t battleId = 0;
    GTBattleGroup group = GTBattleGroup::Unknown;
    char guildName1[64]{};
    char guildName2[64]{};
};

class cGTBattleListDialog final : public cDialog {
public:
    using ChatTextFn = const char* (*)(std::int32_t messageId, void* userData);
    using InsertRowFn = void (*)(std::uint32_t battleId, void* userData);
    using ObserverJoinFn = bool (*)(std::uint32_t battleId, void* userData);

    static constexpr std::int32_t kBattleListId = 1390;
    static constexpr std::int32_t kNoSelection = -1;
    static constexpr std::int32_t kActionRowClick = legacy_window_event::kRowClick;
    static constexpr std::uint32_t kSelectedRgb = 0xFFFFEA00u;
    static constexpr std::uint32_t kDefaultRgb = 0xFFFFFFFFu;
    static constexpr std::int32_t kPlayOffMessageId = 953;
    static constexpr std::int32_t kGroupMessageId = 954;
    static constexpr std::int32_t kGuildPairMessageId = 955;
    static constexpr std::size_t kGroupCount = 4;
    static constexpr std::size_t kColumnCount = 2;

    cGTBattleListDialog();
    ~cGTBattleListDialog() override;

    cGTBattleListDialog(const cGTBattleListDialog&) = delete;
    cGTBattleListDialog& operator=(const cGTBattleListDialog&) = delete;

    void Linking();
    void SetActive(bool val) noexcept override;
    void HandleMouseAction(std::int32_t mouseX, std::int32_t mouseY,
                           std::uint32_t weFromDialog);
    bool EnterBattleOnObserver();

    void DeleteAddBattleInfo();
    void RefreshBattleList();
    void AddBattleInfo(const GTBattleInfo& info);
    bool DeleteBattleInfo(std::uint32_t battleId);
    void DeleteAllBattleInfo();

    void SetPlayOff(bool val) noexcept { m_bPlayOff = val; }
    bool playOff() const noexcept { return m_bPlayOff; }
    std::uint32_t battleCount() const noexcept { return m_BattleCount; }
    std::int32_t selectedIndex() const noexcept { return m_nPreSelectedIndex; }
    bool hasBattle(std::uint32_t battleId) const noexcept;

    void SetControlsForTest(cListCtrl* ctrl) noexcept;

    void SetCallbacks(ChatTextFn chatText,
                      InsertRowFn onInsertRow,
                      ObserverJoinFn onObserverJoin,
                      void* userData = nullptr) noexcept;

    void SetBattlesForTest(const GTBattleInfo* battles, std::size_t count) noexcept;
    void UseTestBattles(bool v) noexcept { m_useTestBattles = v; }

    cListCtrl* battleListCtrl() const noexcept { return m_pBattleListCtrl; }

    static char GroupInitial(GTBattleGroup group) noexcept;

private:
    void ApplySelectionColor(std::size_t rowIdx);
    void RestoreColor(std::size_t rowIdx);
    bool FormatBattleText(char* buf, std::size_t bufSize,
                          const GTBattleInfo& info) const;
    void InsertRowCtrl(const GTBattleInfo& info);
    const GTBattleInfo* FindBattle(std::uint32_t battleId) const noexcept;

    cListCtrl* m_pBattleListCtrl = nullptr;
    std::vector<GTBattleInfo> m_BattleList{};
    std::uint32_t m_BattleCount = 0;
    bool m_bPlayOff = false;
    std::int32_t m_nPreSelectedIndex = kNoSelection;

    const GTBattleInfo* m_testBattles = nullptr;
    std::size_t m_testCount = 0;
    bool m_useTestBattles = false;

    ChatTextFn m_chatTextFn = nullptr;
    InsertRowFn m_onInsertRowFn = nullptr;
    ObserverJoinFn m_observerJoinFn = nullptr;
    void* m_callbackUserData = nullptr;
};

} // namespace mxh::ui
