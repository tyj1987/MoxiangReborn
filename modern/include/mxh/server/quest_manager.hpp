// quest_manager.hpp - 1:1 port of legacy [Server]Map/QuestManager.h
// (CQuestManager + CQuestBase + CQuestGroup). Modern port models the
// state machine + tracker interfaces as POD structs + free functions.

#pragma once

#include "mxh/server/player.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mxh::server {

// ---- Quest states (legacy eQuestState in QuestDefines.h) ----
enum class QuestState : std::uint8_t {
    None     = 0,
    Accepted = 1,  // accepted, not yet complete
    Complete = 2,  // all sub-conditions met, ready to turn in
    Rewarded = 3,  // reward taken, quest finished
    Failed   = 4,  // timed out or failed sub-condition
};

// ---- Sub-condition types (legacy eQuestSubKind) ----
enum class QuestSubKind : std::uint8_t {
    None      = 0,
    Kill      = 1,  // kill N of monster_kind
    Collect   = 2,  // collect N of item_idx
    ReachMap  = 3,  // visit map_num
    TalkNpc   = 4,  // talk to npc_idx
    Survive   = 5,  // survive timer
};

// ---- Sub-condition (legacy QUEST_SUB) ----
struct QuestSub final {
    QuestSubKind kind        = QuestSubKind::None;
    std::uint32_t target_id  = 0;     // monster_kind / item_idx / npc_idx / map_num
    std::uint32_t count      = 0;     // current progress
    std::uint32_t target     = 0;     // required count
};

// ---- Quest definition (loaded from QuestScript.bin / QuestInfo.bin) ----
struct QuestDefinition final {
    std::uint32_t quest_id        = 0;
    std::string   title;
    std::uint16_t min_level       = 1;
    std::uint16_t max_level       = 99;
    std::uint32_t reward_exp      = 0;
    std::uint32_t reward_money    = 0;
    std::uint32_t reward_item_idx = 0;     // 0 = no item
    std::uint32_t reward_item_qty = 0;
    std::uint32_t timer_seconds   = 0;     // 0 = no timer
    std::vector<QuestSub> subs;
};

// ---- Quest runtime progress (one player x one quest) ----
struct QuestProgress final {
    std::uint32_t player_id = 0;
    std::uint32_t quest_id  = 0;
    QuestState    state     = QuestState::None;
    std::uint32_t accepted_time_ms = 0;  // legacy GetStartTime()
    std::vector<QuestSub> subs;          // copied from definition on accept
};

// ---- Quest factory ----
QuestProgress start_quest(std::uint32_t player_id,
                          const QuestDefinition& def,
                          std::uint32_t now_ms) noexcept;

// ---- Sub-progress mutation ----
// Increment kill / collect counter by delta. Returns true if the sub
// is complete (count reaches target).
bool increment_sub(QuestProgress& progress,
                    QuestSubKind kind,
                    std::uint32_t target_id,
                    std::uint32_t delta) noexcept;

// ---- Quest state transitions ----
QuestState evaluate_quest_state(QuestProgress& progress) noexcept;

QuestState complete_quest(QuestProgress& progress) noexcept;
QuestState fail_quest(QuestProgress& progress) noexcept;
QuestState reward_quest(QuestProgress& progress) noexcept;

enum class QuestRewardStatus : std::uint8_t {
    Granted = 0,
    NotComplete = 1,
    PlayerInactive = 2,
};

struct QuestRewardResult final {
    QuestRewardStatus status = QuestRewardStatus::NotComplete;
    std::uint32_t experience = 0;
    std::uint32_t money = 0;
    std::uint32_t item_idx = 0;
    std::uint32_t item_qty = 0;
};

QuestRewardResult claim_quest_reward(QuestProgress& progress,
                                      const QuestDefinition& def,
                                      Player& player,
                                      std::uint32_t next_level_exp) noexcept;


// ---- Quest log (per-player quest list) ----
struct QuestLog final {
    std::uint32_t player_id = 0;
    std::vector<QuestProgress> quests;
};

std::optional<QuestProgress*> find_quest(QuestLog& log, std::uint32_t quest_id) noexcept;
bool accept_quest(QuestLog& log, const QuestDefinition& def, std::uint32_t now_ms) noexcept;


bool accept_quest(QuestLog& log,
                  const QuestDefinition& def,
                  std::uint32_t now_ms,
                  std::uint16_t player_level) noexcept;
// drop_quest: player gives up an active quest. Returns true if removed.
bool drop_quest(QuestLog& log, std::uint32_t quest_id) noexcept;

// QuestSummary: per-state counts for a QuestLog.
struct QuestSummary {
    std::uint32_t accepted = 0;
    std::uint32_t complete = 0;
    std::uint32_t rewarded = 0;
    std::uint32_t failed   = 0;
};
QuestSummary summarize(const QuestLog& log) noexcept;

inline constexpr std::size_t LIMIT_PROCESS_QUEST = 20u;

struct QuestTickResult final {
    QuestState state = QuestState::None;
    bool expired = false;
    bool changed = false;
};

QuestTickResult tick_quest(QuestProgress& progress,
                           const QuestDefinition& def,
                           std::uint32_t now_ms) noexcept;

std::size_t active_quest_count(const QuestLog& log) noexcept;
bool can_accept_quest(const QuestDefinition& def,
                      std::uint16_t player_level) noexcept;

}  // namespace mxh::server
