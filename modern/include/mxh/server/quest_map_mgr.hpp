#pragma once

#include <cstdint>

namespace mxh::server {

inline constexpr std::uint16_t QUESTMAPNUM1 = 73u;
inline constexpr std::uint16_t QUESTMAPNUM2 = 37u;
inline constexpr std::uint16_t QUESTMAPNUM3 = 95u;

struct QuestMapMgrState {
    bool m_bQuestMap = false;
    bool m_bQuestChannelInitialized = false;
};

struct QuestMapRemoveResult {
    bool deleteQuestRecallMonster = false;
    bool destroyQuestMapChannel = false;
    std::uint32_t channelId = 0;
};

struct QuestMapDeathResult {
    bool handled = false;
    bool readyToRevive = true;
};

inline QuestMapMgrState make_quest_map_mgr() {
    return QuestMapMgrState{};
}

inline bool quest_map_mgr_init(QuestMapMgrState& state,
                               std::uint16_t mapNum,
                               bool mapIsQuestRoom) {
    static_cast<void>(mapNum);
    state.m_bQuestMap = mapIsQuestRoom;
    state.m_bQuestChannelInitialized = mapIsQuestRoom;
    return state.m_bQuestMap;
}

inline bool is_quest_map(const QuestMapMgrState& state) {
    return state.m_bQuestMap;
}

inline QuestMapRemoveResult quest_map_remove_player(const QuestMapMgrState& state,
                                                    std::uint32_t channelId) {
    if (!state.m_bQuestMap) return {};
    return {true, true, channelId};
}

inline QuestMapDeathResult quest_map_die_player(const QuestMapMgrState& state,
                                                bool currentReadyToRevive) {
    if (!state.m_bQuestMap) return {false, currentReadyToRevive};
    return {true, false};
}

}
