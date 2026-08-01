#pragma once

#include "mxh/ui/cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;
class cButton;

struct GTScoreBattleInfo {
    char guildName[2][64]{};
    std::int32_t score[2]{};
    std::uint32_t entranceTime = 0;
    std::uint32_t fightTime = 0;
};

class cGTScoreInfoDialog final : public cDialog {
public:
    using TickProvider = std::uint32_t (*)();
    using TextWriter = void (*)(cStatic* ctrl, const char* text, void* userData);

    static constexpr std::int32_t kDialogId = 1326;
    static constexpr std::int32_t kGTimeId = 1394;
    static constexpr std::int32_t kOutBtnId = 1395;
    static constexpr std::int32_t kGuildName1Id = 1396;
    static constexpr std::int32_t kGuildMember1Id = 1397;
    static constexpr std::int32_t kGuildName2Id = 1398;
    static constexpr std::int32_t kGuildMember2Id = 1399;

    static constexpr std::uint32_t kInitialEntranceMs = 120000;
    static constexpr std::uint32_t kMinuteMs = 60000;
    static constexpr std::uint32_t kSecondMs = 1000;
    static constexpr std::int32_t kInvalidTeam = -1;
    static constexpr std::size_t kTeamCount = 2;

    cGTScoreInfoDialog();
    ~cGTScoreInfoDialog() override;

    cGTScoreInfoDialog(const cGTScoreInfoDialog&) = delete;
    cGTScoreInfoDialog& operator=(const cGTScoreInfoDialog&) = delete;

    void Linking();
    void Process();

    void SetBattleInfo(const GTScoreBattleInfo& info);
    void StartBattle() noexcept;
    void EndBattle() noexcept;
    void SetTeamScore(std::int32_t team, std::int32_t count);
    void ShowOutBtn(bool show);

    void SetControlsForTest(cStatic* gName1, cStatic* gName2,
                            cStatic* gScore1, cStatic* gScore2,
                            cStatic* timeStatic, cButton* outBtn) noexcept;

    void SetCallbacks(TickProvider tickProvider,
                      TextWriter textWriter,
                      void* userData = nullptr) noexcept;

    std::int32_t teamScore(std::int32_t team) const noexcept;
    bool battleStarted() const noexcept { return m_bStart; }
    std::uint32_t fightTime() const noexcept { return m_FightTime; }
    std::uint32_t entranceTime() const noexcept { return m_EntranceTime; }

    cStatic* guildNameControl(std::int32_t team) const noexcept;
    cStatic* guildScoreControl(std::int32_t team) const noexcept;
    cStatic* timeStaticControl() const noexcept { return m_pTimeStatic; }
    cButton* outButtonControl() const noexcept { return m_pOutBtn; }

private:
    static std::uint32_t DefaultTick() noexcept;
    static void DefaultTextWriter(cStatic* ctrl, const char* text, void* userData);

    std::uint32_t ConsumeTick() const noexcept;
    void FormatTime(char* buffer, std::size_t bufferSize,
                    std::uint32_t timeMs) const;
    void UpdateTimeText();
    cStatic* ResolveGuildName(std::int32_t team) const noexcept;
    cStatic* ResolveGuildScore(std::int32_t team) const noexcept;

    cStatic* m_pGuildName[kTeamCount] = {nullptr, nullptr};
    cStatic* m_pGuildScore[kTeamCount] = {nullptr, nullptr};
    cStatic* m_pTimeStatic = nullptr;
    cButton* m_pOutBtn = nullptr;

    std::int32_t m_Score[kTeamCount] = {0, 0};
    std::uint32_t m_FightTime = 0;
    std::uint32_t m_EntranceTime = kInitialEntranceMs;
    bool m_bStart = false;

    TickProvider m_tickProvider = nullptr;
    TextWriter m_textWriter = nullptr;
    void* m_callbackUserData = nullptr;
};

} // namespace mxh::ui
