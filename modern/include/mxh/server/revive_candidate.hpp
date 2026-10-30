#pragma once
#include "mxh/server/player.hpp"
#include "mxh/server/revive_shop_plan.hpp"
#include "mxh/game/experience_curve.hpp"
#include "mxh/server/present_revive_policy.hpp"
#include "mxh/server/present_revive_context.hpp"
#include "mxh/server/pet_manager.hpp"
#include "mxh/server/revive_shop_manager.hpp"
#include <limits>

namespace mxh::server {
struct ReviveCandidate {
    Player actor;
    ReviveShopPlan shop;
};

// The caller selects an applicable ordinary penalty branch and owns the DB
// transaction. No online state, item manager, or network message is mutated.
inline std::optional<ReviveCandidate> prepare_ordinary_revive_candidate(
    const Player& original, const mxh::game::ExperienceCurve& curve,
    const mxh::game::ExpPenaltyTable& table, mxh::game::ReviveLocation location,
    const mxh::db::LegacyShopAppearanceRows& rows, std::uint32_t combined_item) {
    if (original.lifecycle() != PlayerLifecycle::Dead) return std::nullopt;
    try {
        const auto& state=original.state();
        const auto raw_loss=mxh::game::unprotected_revive_loss(table,location,state.progress.level,
            state.progress.money,curve.max_exp_point(state.progress.level));
        if (!raw_loss) return std::nullopt;
        const auto shop=prepare_revive_shop(rows,*raw_loss,state.shop_options.ProtectCount,combined_item);
        if (!shop || shop->protection.loss.money>state.progress.money) return std::nullopt;
        const auto experience=curve.reduce_exp({state.progress.level,state.progress.level_exp},
            shop->protection.loss.experience);
        const auto next_max=curve.max_exp_point(experience.level);
        if (experience.exp_point>std::numeric_limits<std::uint32_t>::max() ||
            next_max>std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
        ReviveCandidate candidate{original,*shop};
        auto& next=candidate.actor.state();
        next.progress.money-=shop->protection.loss.money;
        next.progress.level=experience.level;
        next.progress.level_exp=static_cast<std::uint32_t>(experience.exp_point);
        // Current admission stores within-level experience in both fields.
        next.progress.total_exp=next.progress.level_exp;
        next.progress.max_exp=static_cast<std::uint32_t>(next_max);
        if (shop->protection.consume_combined)
            next.shop_options.ProtectCount=shop->protection.remaining_combined;
        next.recompute_max_stats();
        if (!candidate.actor.revive()) return std::nullopt;
        return candidate;
    } catch (const std::exception&) { return std::nullopt; }
}
// Context must be resolved from authoritative map/death state before calling.
// Exempt branches must not consume protection records or recompute experience.
struct PresentReviveCandidate {
    Player actor;
    ReviveShopPlan shop;
    PetManagerState pets;
    MasterRevivePetEffect pet_effect;
    ShopItemManager manager;
};
inline std::optional<PresentReviveCandidate> prepare_present_revive_candidate(
    const Player& original, const mxh::game::ExperienceCurve& curve,
    const mxh::game::ExpPenaltyTable& table,
    const mxh::db::LegacyShopAppearanceRows& rows, const ShopItemManager& manager,
    const PresentReviveContext& context, const PetManagerState& pets,
    mxh::game::ReviveLocation location = mxh::game::ReviveLocation::Present) {
    if (context.dead != (original.lifecycle() == PlayerLifecycle::Dead) ||
        context.level != original.state().progress.level ||
        context.died_for_battle_channel != original.state().death_flags.battle_channel ||
        context.died_for_guild_field_war != original.state().death_flags.guild_field_war)
        return std::nullopt;
    std::optional<ReviveCandidate> player;
    switch (decide_present_revive(context)) {
    case PresentReviveDecision::RecoverWithLoss:
        player=prepare_ordinary_revive_candidate(original,curve,table,
            location,rows,manager.protect_item_idx());
        break;
    case PresentReviveDecision::RecoverWithoutLoss: {
        ReviveCandidate result{original,{}};
        result.shop.rows=rows;
        if (!result.actor.revive()) return std::nullopt;
        player=std::move(result);
        break;
    }
    default: return std::nullopt;
    }
    if (!player) return std::nullopt;
    auto next_manager=prepare_revive_shop_manager(manager,rows,player->shop);
    if(!next_manager) return std::nullopt;
    PresentReviveCandidate result{std::move(player->actor),std::move(player->shop),pets,{},std::move(*next_manager)};
    if (result.shop.reduce_pet_friendship) {
        // A dangling or duplicate summoned identity cannot be silently ignored.
        if (pets.m_curSummonItemDBIdx && std::count_if(pets.m_PetInfoList.begin(),
            pets.m_PetInfoList.end(),[&](const auto& pet) {
                return pet.PetSummonItemDBIdx == *pets.m_curSummonItemDBIdx;
            }) != 1) return std::nullopt;
        result.pet_effect=apply_master_revive_pet_loss(result.pets);
    }
    return result;
}
// Runtime entry: derive per-player facts from the authoritative objects rather
// than asking the network handler to manufacture a second copy of those facts.
inline std::optional<PresentReviveCandidate> prepare_resolved_present_revive(
    const Player& original,const mxh::game::ExperienceCurve& curve,
    const mxh::game::ExpPenaltyTable& table,const mxh::db::LegacyShopAppearanceRows& rows,
    const ShopItemManager& manager,const PetManagerState& pets,
    const LootingManagerState& looting,const mxh::game::MapKindTable& maps,
    bool battle_channel,bool exit_started,bool korean_first_action_penalty) {
    const auto context=resolve_present_revive_context(original,looting,maps,
        battle_channel,exit_started,korean_first_action_penalty);
    if(!context) return std::nullopt;
    return prepare_present_revive_candidate(original,curve,table,rows,manager,*context,pets);
}
} // namespace mxh::server
