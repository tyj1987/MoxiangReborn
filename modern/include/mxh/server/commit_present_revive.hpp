#pragma once
#include "mxh/server/revive_candidate.hpp"
#include "mxh/db/revive_state.hpp"
#include "mxh/server/revive_vitality_messages.hpp"

namespace mxh::server {
// Caller owns the dispatch/session fence, but not an open DB transaction.
// No runtime publication or notifications happen here. Uncertain requires drain.
inline mxh::db::ReviveCommit commit_present_revive(
    mxh::db::IDbAdapter& db,const mxh::db::ModernShopState& previous,
    const Player& original,const PresentReviveCandidate& candidate) {
    const auto& before=original.state();
    const auto& after=candidate.actor.state();
    // A durable revive must be representable by the client protocol. Validate
    // before opening a transaction, including map identity and signed deltas.
    if(!prepare_revive_vitality_messages(original,candidate.actor))
        return mxh::db::ReviveCommit::Rejected;
    std::optional<std::vector<mxh::db::PersistedPet>> pets;
    if(previous.pets || !candidate.pets.m_PetInfoList.empty()) {
        pets.emplace();
        for(const auto& pet:candidate.pets.m_PetInfoList)
            pets->push_back({pet.PetSummonItemDBIdx,pet.PetKind,pet.PetGrade,pet.PetStamina,
                pet.PetFriendly,pet.bAlive,pet.bRest,pet.bSummonning});
    }
    return mxh::db::commit_revive_state(db,before.player_id,before.user_id,previous,
        {before.progress.level,before.progress.money,before.progress.level_exp},
        {after.progress.level,after.progress.money,after.progress.level_exp},
        candidate.shop.rows,
        {after.vitals.current_hp,after.vitals.current_shield,after.vitals.current_mp},pets);
}
} // namespace mxh::server
