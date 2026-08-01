#include "gtscoreinfodialog.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cStatic.hpp"

#include <cstdio>
#include <cstring>

namespace mxh::ui {

cGTScoreInfoDialog::cGTScoreInfoDialog() = default;
cGTScoreInfoDialog::~cGTScoreInfoDialog() = default;

std::uint32_t cGTScoreInfoDialog::DefaultTick() noexcept {
    return 0;
}

void cGTScoreInfoDialog::DefaultTextWriter(cStatic* ctrl,
                                           const char* text,
                                           void* /*userData*/) {
    if (ctrl && text) {
        ctrl->SetStaticText(text);
    }
}

void cGTScoreInfoDialog::SetCallbacks(TickProvider tickProvider,
                                      TextWriter textWriter,
                                      void* userData) noexcept {
    m_tickProvider = tickProvider;
    m_textWriter = textWriter;
    m_callbackUserData = userData;
}

std::uint32_t cGTScoreInfoDialog::ConsumeTick() const noexcept {
    return m_tickProvider ? m_tickProvider() : 0;
}

void cGTScoreInfoDialog::FormatTime(char* buffer,
                                    std::size_t bufferSize,
                                    std::uint32_t timeMs) const {
    if (!buffer || bufferSize == 0) {
        return;
    }
    const std::uint32_t minutes = timeMs / kMinuteMs;
    const std::uint32_t seconds = (timeMs % kMinuteMs) / kSecondMs;
    std::snprintf(buffer, bufferSize, "%02u:%02u",
                  static_cast<unsigned>(minutes),
                  static_cast<unsigned>(seconds));
}

void cGTScoreInfoDialog::UpdateTimeText() {
    if (!m_pTimeStatic) {
        return;
    }
    char buffer[32]{};
    FormatTime(buffer, sizeof(buffer), m_FightTime);
    TextWriter writer = m_textWriter ? m_textWriter : DefaultTextWriter;
    writer(m_pTimeStatic, buffer, m_callbackUserData);
}

cStatic* cGTScoreInfoDialog::ResolveGuildName(std::int32_t team) const noexcept {
    if (team < 0 || static_cast<std::size_t>(team) >= kTeamCount) {
        return nullptr;
    }
    return m_pGuildName[team];
}

cStatic* cGTScoreInfoDialog::ResolveGuildScore(std::int32_t team) const noexcept {
    if (team < 0 || static_cast<std::size_t>(team) >= kTeamCount) {
        return nullptr;
    }
    return m_pGuildScore[team];
}

std::int32_t cGTScoreInfoDialog::teamScore(std::int32_t team) const noexcept {
    if (team < 0 || static_cast<std::size_t>(team) >= kTeamCount) {
        return 0;
    }
    return m_Score[team];
}

cStatic* cGTScoreInfoDialog::guildNameControl(std::int32_t team) const noexcept {
    return ResolveGuildName(team);
}

cStatic* cGTScoreInfoDialog::guildScoreControl(std::int32_t team) const noexcept {
    return ResolveGuildScore(team);
}

void cGTScoreInfoDialog::SetControlsForTest(cStatic* gName1,
                                            cStatic* gName2,
                                            cStatic* gScore1,
                                            cStatic* gScore2,
                                            cStatic* timeStatic,
                                            cButton* outBtn) noexcept {
    m_pGuildName[0] = gName1;
    m_pGuildName[1] = gName2;
    m_pGuildScore[0] = gScore1;
    m_pGuildScore[1] = gScore2;
    m_pTimeStatic = timeStatic;
    m_pOutBtn = outBtn;
}

void cGTScoreInfoDialog::Linking() {
    cStatic* guildNames[kTeamCount] = {
        dynamic_cast<cStatic*>(findWindowById(kGuildName1Id)),
        dynamic_cast<cStatic*>(findWindowById(kGuildName2Id)),
    };
    cStatic* guildScores[kTeamCount] = {
        dynamic_cast<cStatic*>(findWindowById(kGuildMember1Id)),
        dynamic_cast<cStatic*>(findWindowById(kGuildMember2Id)),
    };
    for (std::size_t i = 0; i < kTeamCount; ++i) {
        m_pGuildName[i] = guildNames[i];
        m_pGuildScore[i] = guildScores[i];
    }
    m_pTimeStatic = dynamic_cast<cStatic*>(findWindowById(kGTimeId));
    m_pOutBtn = dynamic_cast<cButton*>(findWindowById(kOutBtnId));

    m_Score[0] = 0;
    m_Score[1] = 0;
    m_FightTime = 0;
    m_EntranceTime = kInitialEntranceMs;
    m_bStart = false;
}

void cGTScoreInfoDialog::Process() {
    const std::uint32_t tick = ConsumeTick();
    if (!m_bStart) {
        if (tick < m_EntranceTime) {
            m_EntranceTime -= tick;
        } else {
            m_EntranceTime = 0;
        }
        return;
    }

    if (tick < m_FightTime) {
        m_FightTime -= tick;
    } else {
        m_FightTime = 0;
    }
    UpdateTimeText();
}

void cGTScoreInfoDialog::SetBattleInfo(const GTScoreBattleInfo& info) {
    TextWriter writer = m_textWriter ? m_textWriter : DefaultTextWriter;
    for (std::size_t i = 0; i < kTeamCount; ++i) {
        if (m_pGuildName[i]) {
            writer(m_pGuildName[i], info.guildName[i], m_callbackUserData);
        }
        m_Score[i] = info.score[i];
    }
    m_EntranceTime = info.entranceTime;
    m_FightTime = info.fightTime;
    UpdateTimeText();
}

void cGTScoreInfoDialog::StartBattle() noexcept {
    m_bStart = true;
}

void cGTScoreInfoDialog::EndBattle() noexcept {
    m_bStart = false;
    m_FightTime = 0;
}

void cGTScoreInfoDialog::SetTeamScore(std::int32_t team, std::int32_t count) {
    if (team < 0 || static_cast<std::size_t>(team) >= kTeamCount) {
        return;
    }
    m_Score[team] = count;
}

void cGTScoreInfoDialog::ShowOutBtn(bool show) {
    if (m_pOutBtn) {
        m_pOutBtn->SetActive(show);
    }
}

} // namespace mxh::ui
