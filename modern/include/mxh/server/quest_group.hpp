#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mxh::server {

inline constexpr std::size_t MAX_QUEST = 1000u;
inline constexpr std::size_t MAX_QUESTEVENT_PLAYER = 100u;
inline constexpr std::size_t MAX_QUESTITEM = 100u;
inline constexpr std::uint32_t MAX_QUEST_PROBABILITY = 10000u;

struct QuestGroupQuest {
    std::uint32_t questIdx = 0;
    std::uint32_t subQuestFlag = 0;
    std::uint32_t data = 0;
    std::uint32_t time = 0;
    std::uint8_t checkType = 0;
    std::uint32_t checkTime = 0;
    bool complete = false;
    bool deleteRequested = false;
    std::unordered_set<std::uint32_t> activeSubquests;
    bool checkTimeActive = false;
    std::uint32_t checkDay = 0;
    std::uint32_t checkHour = 0;
    std::uint32_t checkMinute = 0;
    std::unordered_map<std::uint32_t, std::uint32_t> subQuestData;
    std::unordered_map<std::uint32_t, std::uint32_t> subQuestTime;
};

struct QuestGroupItem {
    std::uint32_t dwQuestIdx = 0;
    std::uint32_t dwItemIdx = 0;
    std::uint32_t dwItemNum = 0;
};

struct QuestGroupEvent {
    std::uint32_t questIdx = 0;
    std::uint32_t kind = 0;
    std::uint32_t param1 = 0;
    std::int32_t param2 = 0;
};

struct QuestGroupDispatch {
    std::uint32_t targetQuestIdx = 0;
    QuestGroupEvent event{};
};

struct QuestGroupState {
    std::uint32_t m_playerId = 0;
    bool m_hasPlayer = false;
    std::unordered_map<std::uint32_t, QuestGroupQuest> m_QuestTable;
    std::unordered_map<std::uint32_t, QuestGroupItem> m_QuestItemTable;
    std::vector<QuestGroupEvent> m_QuestEvent;
    std::uint8_t m_stage = 0;
    std::uint16_t m_savePoint = 0;
    std::uint16_t m_loginPoint = 0;
};

inline QuestGroupState make_quest_group() {
    QuestGroupState state;
    state.m_QuestTable.reserve(MAX_QUEST);
    state.m_QuestItemTable.reserve(MAX_QUESTITEM);
    state.m_QuestEvent.reserve(MAX_QUESTEVENT_PLAYER);
    return state;
}

inline void quest_group_initialize(QuestGroupState& state, std::uint32_t playerId) {
    state.m_playerId = playerId;
    state.m_hasPlayer = true;
    state.m_QuestEvent.clear();
}

inline void quest_group_release(QuestGroupState& state) {
    state.m_QuestTable.clear();
    state.m_QuestItemTable.clear();
    state.m_QuestEvent.clear();
    state.m_playerId = 0;
    state.m_hasPlayer = false;
}

inline bool quest_group_create_quest(QuestGroupState& state, std::uint32_t questIdx) {
    if (state.m_QuestTable.find(questIdx) != state.m_QuestTable.end()) return false;
    state.m_QuestTable.emplace(questIdx, QuestGroupQuest{questIdx});
    return true;
}


inline QuestGroupQuest* quest_group_get_quest(QuestGroupState& state,
                                              std::uint32_t questIdx) {
    const auto it = state.m_QuestTable.find(questIdx);
    return it == state.m_QuestTable.end() ? nullptr : &it->second;
}

inline bool quest_group_set_main_data(QuestGroupState& state, std::uint32_t questIdx,
                                      std::uint32_t subQuestFlag, std::uint32_t data,
                                      std::uint32_t time, std::uint8_t checkType,
                                      std::uint32_t checkTime) {
    auto* quest = quest_group_get_quest(state, questIdx);
    if (quest == nullptr) return true;
    quest->subQuestFlag = subQuestFlag;
    quest->data = data;
    quest->time = time;
    quest->checkType = checkType;
    quest->checkTime = checkTime;
    return true;
}

inline bool quest_group_set_subquest_data(QuestGroupState& state,
                                          std::uint32_t questIdx,
                                          std::uint32_t subQuestIdx,
                                          std::uint32_t data,
                                          std::uint32_t time) {
    auto* quest = quest_group_get_quest(state, questIdx);
    if (quest == nullptr) return false;
    quest->subQuestData[subQuestIdx] = data;
    quest->subQuestTime[subQuestIdx] = time;
    return true;
}

inline void quest_group_set_item(QuestGroupState& state, std::uint32_t questIdx,
                                 std::uint32_t itemIdx, std::uint32_t itemNum) {
    state.m_QuestItemTable.insert_or_assign(
        itemIdx, QuestGroupItem{questIdx, itemIdx, itemNum});
}

inline bool quest_group_add_event(QuestGroupState& state, QuestGroupEvent event) {
    if (state.m_QuestEvent.size() >= MAX_QUESTEVENT_PLAYER) return false;
    state.m_QuestEvent.push_back(event);
    return true;
}

inline bool quest_group_is_complete(const QuestGroupState& state,
                                    std::uint32_t questIdx) {
    const auto it = state.m_QuestTable.find(questIdx);
    return it != state.m_QuestTable.end() && it->second.complete;
}

inline std::size_t quest_group_delete_quest(QuestGroupState& state,
                                           std::uint32_t questIdx) {
    auto* quest = quest_group_get_quest(state, questIdx);
    if (quest == nullptr) return 0u;
    quest->deleteRequested = true;
    std::vector<std::uint32_t> itemIds;
    for (const auto& [itemIdx, item] : state.m_QuestItemTable)
        if (item.dwQuestIdx == questIdx) itemIds.push_back(itemIdx);
    for (const auto itemIdx : itemIds) state.m_QuestItemTable.erase(itemIdx);
    return itemIds.size();
}

inline int quest_group_process_quest_count(const QuestGroupState& state) {
    int count = 0;
    for (const auto& [questIdx, quest] : state.m_QuestTable) {
        static_cast<void>(questIdx);
        if (quest.subQuestFlag != 0u && !quest.complete) ++count;
    }
    return count;
}

inline std::vector<QuestGroupDispatch> quest_group_process(QuestGroupState& state) {
    std::vector<QuestGroupDispatch> dispatches;
    if (!state.m_hasPlayer || state.m_QuestEvent.empty()) return dispatches;
    for (const auto& [questIdx, quest] : state.m_QuestTable) {
        if (quest.complete) continue;
        for (const auto& event : state.m_QuestEvent)
            dispatches.push_back({questIdx, event});
    }
    state.m_QuestEvent.clear();
    return dispatches;
}

inline bool quest_group_add_count(QuestGroupState& state,
                                   std::uint32_t questIdx,
                                   std::uint32_t subQuestIdx,
                                   std::uint32_t maxCount) {
    auto* quest = quest_group_get_quest(state, questIdx);
    if (quest == nullptr) return false;
    auto& count = quest->subQuestData[subQuestIdx];
    if (count < maxCount) ++count;
    quest->subQuestTime[subQuestIdx] = 0u;
    return true;
}

inline bool quest_group_add_count_from_level_gap(QuestGroupState& state,
                                                  std::uint32_t questIdx,
                                                  std::uint32_t subQuestIdx,
                                                  std::uint32_t maxCount,
                                                  std::uint32_t minGap,
                                                  std::uint32_t maxGap,
                                                  std::int32_t playerLevel,
                                                  std::int32_t monsterLevel) {
    const auto gap = playerLevel - monsterLevel;
    const auto reverseGap = monsterLevel - playerLevel;
    if ((gap > 0 && static_cast<std::uint32_t>(gap) > minGap) ||
        (reverseGap > 0 && static_cast<std::uint32_t>(reverseGap) > maxGap)) return false;
    return quest_group_add_count(state, questIdx, subQuestIdx, maxCount);
}

inline bool quest_group_add_count_from_monster_level(QuestGroupState& state,
                                                      std::uint32_t questIdx,
                                                      std::uint32_t subQuestIdx,
                                                      std::uint32_t maxCount,
                                                      std::uint32_t minLevel,
                                                      std::uint32_t maxLevel,
                                                      std::uint32_t monsterLevel) {
    if (monsterLevel < minLevel || monsterLevel > maxLevel) return false;
    return quest_group_add_count(state, questIdx, subQuestIdx, maxCount);
}


inline bool quest_group_change_stage(std::uint8_t currentStage,
                                     std::uint8_t requestedStage,
                                     std::uint8_t& resultingStage) {
    if (currentStage == requestedStage) { resultingStage = currentStage; return true; }
    constexpr std::uint8_t normal = 0u;
    constexpr std::uint8_t hwa = 1u;
    constexpr std::uint8_t hyun = 2u;
    constexpr std::uint8_t geuk = 3u;
    constexpr std::uint8_t tal = 4u;
    bool allowed = (requestedStage == hwa && currentStage == normal) ||
                   (requestedStage == hyun && currentStage == hwa) ||
                   (requestedStage == geuk && currentStage == normal) ||
                   (requestedStage == tal && currentStage == geuk);
    if (!allowed) return false;
    resultingStage = requestedStage;
    return true;
}

inline std::uint32_t quest_group_take_money_per_count(QuestGroupState& state,
                                                       std::uint32_t itemIdx,
                                                       std::uint32_t moneyPerItem) {
    const auto it = state.m_QuestItemTable.find(itemIdx);
    if (it == state.m_QuestItemTable.end() || it->second.dwItemNum == 0u) return 0u;
    const auto total = it->second.dwItemNum * moneyPerItem;
    state.m_QuestItemTable.erase(it);
    return total;
}

inline bool check_quest_probability(std::uint32_t probability,
                                    std::uint32_t randomValue) {
    if (probability == 0u) return false;
    if (probability != MAX_QUEST_PROBABILITY &&
        randomValue % MAX_QUEST_PROBABILITY >= probability) return false;
    return true;
}

inline bool quest_group_start_subquest(QuestGroupState& state,
                                       std::uint32_t questIdx,
                                       std::uint32_t subQuestIdx,
                                       std::uint32_t nowMs) {
    auto* quest = quest_group_get_quest(state, questIdx);
    if (quest == nullptr || !quest->activeSubquests.insert(subQuestIdx).second)
        return false;
    quest->subQuestData[subQuestIdx] = 0;
    quest->subQuestTime[subQuestIdx] = nowMs;
    return true;
}

inline bool quest_group_end_subquest(QuestGroupState& state,
                                     std::uint32_t questIdx,
                                     std::uint32_t subQuestIdx,
                                     std::uint32_t nowMs) {
    auto* quest = quest_group_get_quest(state, questIdx);
    if (quest == nullptr || quest->activeSubquests.erase(subQuestIdx) == 0)
        return false;
    quest->subQuestData[subQuestIdx] = 0;
    quest->subQuestTime[subQuestIdx] = nowMs;
    if (subQuestIdx < 32u)
        quest->subQuestFlag |= (1u << (31u - subQuestIdx));
    if (subQuestIdx == 0u) {
        quest->data = 0;
        quest->time = nowMs;
    }
    return true;
}

inline bool quest_group_end_quest(QuestGroupState& state,
                                  std::uint32_t questIdx,
                                  std::uint32_t repeat,
                                  std::uint32_t nowMs) {
    auto* quest = quest_group_get_quest(state, questIdx);
    if (quest == nullptr) return false;
    quest->activeSubquests.clear();
    quest->subQuestData.clear();
    quest->subQuestTime.clear();
    quest->subQuestFlag = 0;
    quest->time = nowMs;
    quest->checkTimeActive = false;
    if (repeat != 0u) {
        quest->data = 0;
        quest->complete = false;
    } else {
        quest->data = 1;
        quest->complete = true;
    }
    return true;
}

inline bool quest_group_register_check_time(QuestGroupState& state,
                                            std::uint32_t questIdx,
                                            std::uint32_t subQuestIdx,
                                            std::uint32_t type,
                                            std::uint32_t day,
                                            std::uint32_t hour,
                                            std::uint32_t minute) {
    static_cast<void>(subQuestIdx);
    auto* quest = quest_group_get_quest(state, questIdx);
    if (quest == nullptr) return false;
    quest->checkTimeActive = true;
    quest->checkType = static_cast<std::uint8_t>(type);
    quest->checkDay = day;
    quest->checkHour = hour;
    quest->checkMinute = minute;
    return true;
}

inline bool quest_group_give_quest_item(QuestGroupState& state,
                                        std::uint32_t questIdx,
                                        std::uint32_t itemIdx) {
    const auto removed = state.m_QuestItemTable.erase(itemIdx);
    static_cast<void>(questIdx);
    return removed > 0u;
}

inline bool quest_group_take_quest_item(QuestGroupState& state,
                                        std::uint32_t questIdx,
                                        std::uint32_t subQuestIdx,
                                        std::uint32_t itemIdx,
                                        std::uint32_t itemNum,
                                        std::uint32_t probability) {
    if (!check_quest_probability(probability, itemNum)) return false;
    auto& item = state.m_QuestItemTable[itemIdx];
    item = QuestGroupItem{questIdx, itemIdx, itemNum};
    (void)quest_group_add_count(state, questIdx, subQuestIdx, itemNum);
    return true;
}

inline bool quest_group_save_login_point(QuestGroupState& state,
                                         std::uint32_t mapNum) {
    if (mapNum > 2000u) return false;
    state.m_savePoint = static_cast<std::uint16_t>(mapNum);
    state.m_loginPoint = static_cast<std::uint16_t>(mapNum + 2000u);
    return true;
}

inline bool quest_group_save_login_point(std::uint32_t mapNum,
                                          std::uint16_t& savePoint,
                                          std::uint16_t& loginPoint) {
    QuestGroupState state{};
    state.m_savePoint = savePoint;
    state.m_loginPoint = loginPoint;
    const auto ok = quest_group_save_login_point(state, mapNum);
    savePoint = state.m_savePoint;
    loginPoint = state.m_loginPoint;
    return ok;
}

}
