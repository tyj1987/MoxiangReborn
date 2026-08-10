// mxh/src/services/QuestServiceImpl.hpp
// Phase 13.2: Real IQuestService implementation backed by the
// mxh::server quest_manager module (1:1 port of legacy
// [Server]Map/QuestManager.{h,cpp}).
//
// The service is per-player: the dialog holds the service, the
// MapHandler owns the Player + QuestLog via PlayerInfo. claimQuest()
// drives the quest state machine (Accepted -> Complete -> Rewarded)
// through quest_manager::reward_quest, which is the same state
// transition the legacy CQuestGroup::RewardQuest performs.
//
// 1:1 quirk: legacy CQuestGroup::RewardQuest *also* applies the
// reward (money/exp/item) to the player; the modern port splits
// this into two layers:
//   - Service (this file): state transition only.
//   - MapHandler (orchestrator): reward application + DB persistence.
// This split mirrors Phase 13's InventoryServiceImpl pattern: the
// service exposes the read/transition path the dialog needs; the
// MapHandler (which already owns PlayerInfo + QuestLog) drives the
// side effects under its critical section. When the quest_manager
// reward path lands as a single free function (claim_quest_reward),
// the orchestrator will call it directly and the service remains a
// thin transition-only shim.
//
// Threading: claimQuest() is called from the dialog's tick thread;
// it mutates QuestLog.quests[i].state, which lives inside the
// MapHandler's player_mu_ critical section. The dialog holds the
// service for the lifetime of the dialog, and the MapHandler holds
// the underlying state for the lifetime of the player's session.

#pragma once

#include "mxh/services/IQuestService.hpp"

#include "mxh/server/quest_manager.hpp"
#include "mxh/server/player.hpp"

#include <cstdint>

namespace mxh::services {

class QuestServiceImpl final : public IQuestService {
public:
    // Bind the service to a specific player + quest log.  Both
    // references must remain valid for the lifetime of the service.
    QuestServiceImpl(mxh::server::Player& player,
                     mxh::server::QuestLog& log) noexcept
        : m_player(player), m_log(log) {}

    // claimQuest: 1:1 with legacy CQuestGroup::RewardQuest.  Validates
    // the quest exists + is in Complete state, transitions it to
    // Rewarded, returns true.  Rejects unknown / non-Complete quests
    // (the legacy guard returns false so the dialog can show the
    // "quest not complete" error string).
    //
    // The reward application (money / exp / item) is the orchestrator's
    // job; the service intentionally does NOT mutate player.money /
    // player.experience.  This matches the Phase 13 service-model
    // separation: the dialog asks "is this claim acceptable?" and the
    // orchestrator asks "what reward should I apply?".
    bool claimQuest(std::uint32_t quest_id) override {
        auto prog_opt = mxh::server::find_quest(m_log, quest_id);
        if (!prog_opt.has_value()) return false;
        mxh::server::QuestProgress* progress = prog_opt.value();
        if (progress == nullptr) return false;
        if (progress->state != mxh::server::QuestState::Complete) return false;
        mxh::server::reward_quest(*progress);
        return true;
    }

    // Read-only inspectors (useful for tests + orchestrator diagnostics).
    const mxh::server::Player& player() const noexcept { return m_player; }
    const mxh::server::QuestLog& log() const noexcept { return m_log; }

private:
    mxh::server::Player& m_player;
    mxh::server::QuestLog& m_log;
};

}  // namespace mxh::services
