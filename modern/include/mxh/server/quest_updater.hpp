#pragma once

#include <cstdint>
#include <optional>

namespace mxh::server {

enum class QuestDbCommandKind : std::uint8_t {
    MainQuestInsert,
    EndQuestNew,
    DeleteQuestNew,
    MainQuestUpdate,
    SubQuestInsert,
    EndSubQuestNew,
    SubQuestUpdate,
    QuestItemDelete,
    QuestItemInsert,
    QuestItemUpdate,
    MainQuestUpdateCheckTime,
};

struct QuestUpdaterQuestView {
    std::uint32_t questIdx = 0;
    std::uint32_t subQuestFlag = 0;
    std::uint32_t questTime = 0;
};

struct QuestUpdaterSubQuestView {
    std::uint32_t data = 0;
    std::uint32_t time = 0;
};

struct QuestDbCommand {
    QuestDbCommandKind kind = QuestDbCommandKind::MainQuestInsert;
    std::uint32_t playerId = 0;
    std::uint32_t questIdx = 0;
    std::uint32_t subQuestIdx = 0;
    std::uint32_t flag = 0;
    std::uint32_t param = 0;
    std::uint32_t time = 0;
    std::uint32_t itemIdx = 0;
    std::uint32_t itemNum = 0;
    std::uint32_t checkType = 0;
    std::uint32_t checkTime = 0;
    std::uint32_t completion = 0;
};

inline std::optional<QuestDbCommand> quest_updater_start_quest(
    std::uint32_t playerId, const QuestUpdaterQuestView* quest) {
    if (quest == nullptr) return std::nullopt;
    QuestDbCommand command;
    command.kind = QuestDbCommandKind::MainQuestInsert;
    command.playerId = playerId;
    command.questIdx = quest->questIdx;
    command.flag = quest->subQuestFlag;
    command.time = quest->questTime;
    return command;
}

inline std::optional<QuestDbCommand> quest_updater_end_quest(
    std::uint32_t playerId, const QuestUpdaterQuestView* quest) {
    if (quest == nullptr) return std::nullopt;
    QuestDbCommand command;
    command.kind = QuestDbCommandKind::EndQuestNew;
    command.playerId = playerId;
    command.questIdx = quest->questIdx;
    command.flag = quest->subQuestFlag;
    command.completion = 1u;
    command.time = quest->questTime;
    return command;
}

inline std::optional<QuestDbCommand> quest_updater_delete_quest(
    std::uint32_t playerId, const QuestUpdaterQuestView* quest) {
    if (quest == nullptr) return std::nullopt;
    QuestDbCommand command;
    command.kind = QuestDbCommandKind::DeleteQuestNew;
    command.playerId = playerId;
    command.questIdx = quest->questIdx;
    return command;
}

inline QuestDbCommand quest_updater_update_quest(std::uint32_t playerId,
                                                 std::uint32_t flag,
                                                 std::uint32_t param,
                                                 std::uint32_t time) {
    QuestDbCommand command;
    command.kind = QuestDbCommandKind::MainQuestUpdate;
    command.playerId = playerId;
    command.questIdx = 0u;
    command.flag = flag;
    command.param = param;
    command.time = time;
    return command;
}

inline std::optional<QuestDbCommand> quest_updater_start_subquest(
    std::uint32_t playerId, const QuestUpdaterQuestView* quest,
    std::uint32_t subQuestIdx, QuestUpdaterSubQuestView subQuest) {
    if (quest == nullptr) return std::nullopt;
    QuestDbCommand command;
    command.kind = QuestDbCommandKind::SubQuestInsert;
    command.playerId = playerId;
    command.questIdx = quest->questIdx;
    command.subQuestIdx = subQuestIdx;
    command.param = subQuest.data;
    command.time = subQuest.time;
    return command;
}

inline std::optional<QuestDbCommand> quest_updater_end_subquest(
    std::uint32_t playerId, const QuestUpdaterQuestView* quest,
    std::uint32_t subQuestIdx) {
    if (quest == nullptr) return std::nullopt;
    QuestDbCommand command;
    command.kind = QuestDbCommandKind::EndSubQuestNew;
    command.playerId = playerId;
    command.questIdx = quest->questIdx;
    command.subQuestIdx = subQuestIdx;
    command.flag = quest->subQuestFlag;
    command.time = quest->questTime;
    return command;
}

inline std::optional<QuestDbCommand> quest_updater_update_subquest(
    std::uint32_t playerId, const QuestUpdaterQuestView* quest,
    std::uint32_t subQuestIdx, QuestUpdaterSubQuestView subQuest) {
    if (quest == nullptr) return std::nullopt;
    QuestDbCommand command;
    command.kind = QuestDbCommandKind::SubQuestUpdate;
    command.playerId = playerId;
    command.questIdx = quest->questIdx;
    command.subQuestIdx = subQuestIdx;
    command.param = subQuest.data;
    command.time = subQuest.time;
    return command;
}

inline QuestDbCommand quest_updater_give_quest_item(std::uint32_t playerId,
                                                    std::uint32_t itemIdx,
                                                    std::uint32_t itemNum) {
    QuestDbCommand command;
    command.kind = QuestDbCommandKind::QuestItemDelete;
    command.playerId = playerId;
    command.itemIdx = itemIdx;
    static_cast<void>(itemNum);
    return command;
}

inline QuestDbCommand quest_updater_take_quest_item(std::uint32_t playerId,
                                                    std::uint32_t questIdx,
                                                    std::uint32_t itemIdx,
                                                    std::uint32_t itemNum) {
    QuestDbCommand command;
    command.kind = QuestDbCommandKind::QuestItemInsert;
    command.playerId = playerId;
    command.questIdx = questIdx;
    command.itemIdx = itemIdx;
    command.itemNum = itemNum;
    return command;
}

inline QuestDbCommand quest_updater_update_quest_item(std::uint32_t playerId,
                                                      std::uint32_t questIdx,
                                                      std::uint32_t itemIdx,
                                                      std::uint32_t itemNum) {
    auto command = quest_updater_take_quest_item(playerId, questIdx, itemIdx, itemNum);
    command.kind = QuestDbCommandKind::QuestItemUpdate;
    return command;
}

inline QuestDbCommand quest_updater_update_check_time(std::uint32_t playerId,
                                                      std::uint32_t questIdx,
                                                      std::uint32_t checkType,
                                                      std::uint32_t checkTime) {
    QuestDbCommand command;
    command.kind = QuestDbCommandKind::MainQuestUpdateCheckTime;
    command.playerId = playerId;
    command.questIdx = questIdx;
    command.checkType = checkType;
    command.checkTime = checkTime;
    return command;
}

}
