#pragma once

#include <cstdint>
#include <optional>

namespace mxh::server {

using QuestStateType = std::uint32_t;
inline constexpr std::uint8_t QUEST_STATE_MAX_BIT = 32u;

struct QuestFlag {
    QuestStateType value = 0;
};

struct QuestStateNotification {
    std::uint32_t objectId = 0;
    std::uint32_t questIdx = 0;
    QuestStateType state = 0;
};

struct CQuestBaseState {
    std::uint32_t m_dwQuestIdx = 0;
    QuestFlag m_State{};
    int m_nValidNum = 0;
};

inline bool quest_flag_is_set(const QuestFlag& flag, std::uint8_t bit) {
    if (bit < 1u || bit > QUEST_STATE_MAX_BIT) return true;
    const QuestStateType mask = QuestStateType{1u} << (QUEST_STATE_MAX_BIT - bit);
    return (flag.value & mask) != 0u;
}

inline void quest_flag_set_field(QuestFlag& flag, std::uint8_t bit,
                                 bool bSetZero = false) {
    if (bit < 1u || bit > QUEST_STATE_MAX_BIT) return;
    const QuestStateType ch = bSetZero ? 1u : 0u;
    flag.value |= ch << (QUEST_STATE_MAX_BIT - bit);
}

inline CQuestBaseState make_cquest_base() {
    return CQuestBaseState{};
}

inline void cquest_base_init(CQuestBaseState& state, std::uint32_t questIdx,
                             QuestStateType questState) {
    state.m_dwQuestIdx = questIdx;
    state.m_State.value = questState;
}

inline void cquest_base_set_valid_bit_num(CQuestBaseState& state, int count) {
    state.m_nValidNum = count;
}

inline std::optional<QuestStateNotification> cquest_base_set_state(
    CQuestBaseState& state, std::uint8_t field,
    bool mapServerBuild = true, std::uint32_t heroId = 0) {
    quest_flag_set_field(state.m_State, field);
    if (mapServerBuild) return std::nullopt;
    return QuestStateNotification{heroId, state.m_dwQuestIdx, state.m_State.value};
}

inline std::optional<QuestStateNotification> cquest_base_set_value(
    CQuestBaseState& state, QuestStateType value,
    bool mapServerBuild = true, std::uint32_t heroId = 0) {
    state.m_State.value = value;
    if (mapServerBuild) return std::nullopt;
    return QuestStateNotification{heroId, state.m_dwQuestIdx, state.m_State.value};
}

inline bool cquest_base_is_complete(const CQuestBaseState& state) {
    return quest_flag_is_set(state.m_State, 1u);
}

inline std::uint32_t cquest_base_get_quest_idx(const CQuestBaseState& state) {
    return state.m_dwQuestIdx;
}

}
