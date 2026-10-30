#include "mxh/server/player.hpp"
#include "mxh/server/money_manager.hpp"
#include "mxh/server/revive_shop_manager.hpp"
#include "mxh/server/revive_candidate.hpp"
#include "mxh/server/present_revive_context.hpp"

#include <gtest/gtest.h>

namespace {
mxh::server::PlayerSpawnInfo spawn() {
    mxh::server::PlayerSpawnInfo info;
    info.player_id = 1001;
    info.user_id = 77;
    info.level = 10;
    info.map_num = 73;
    info.name = "Moxian";
    info.base.level = 10;
    info.base.cheryuk = 20;
    info.base.simmek = 10;
    return info;
}

mxh::server::Player active_player() {
    mxh::server::Player player;
    EXPECT_TRUE(player.initialize(spawn()));
    EXPECT_TRUE(player.activate());
    return player;
}
}

TEST(PlayerLifecycle, InitializeThenActivateMatchesMapEntryFlow) {
    mxh::server::Player player;
    EXPECT_EQ(player.lifecycle(), mxh::server::PlayerLifecycle::Disconnected);
    EXPECT_TRUE(player.initialize(spawn()));
    EXPECT_EQ(player.lifecycle(), mxh::server::PlayerLifecycle::Loading);
    EXPECT_TRUE(player.activate());
    EXPECT_TRUE(player.is_active());
    EXPECT_EQ(player.state().map_num, 73u);
    EXPECT_EQ(player.state().name, "Moxian");
}

TEST(PlayerLifecycle, ReinitializeWhileActiveIsRejected) {
    auto player = active_player();
    EXPECT_FALSE(player.initialize(spawn()));
    EXPECT_TRUE(player.begin_logout());
    player.release();
    EXPECT_TRUE(player.initialize(spawn()));
}

TEST(PlayerMoney, AddAndSpendRespectLegacyCap) {
    auto player = active_player();
    EXPECT_TRUE(player.set_money(mxh::server::MXH_PLAYER_MAX_MONEY - 5));
    EXPECT_TRUE(player.add_money(10));
    EXPECT_EQ(player.state().progress.money, mxh::server::MXH_PLAYER_MAX_MONEY);
    EXPECT_TRUE(player.spend_money(100));
    EXPECT_EQ(player.state().progress.money, mxh::server::MXH_PLAYER_MAX_MONEY - 100);
    EXPECT_TRUE(player.add_money(1));
    EXPECT_FALSE(player.spend_money(mxh::server::MXH_PLAYER_MAX_MONEY));
}

TEST(PlayerExperience, CarriesRemainderAcrossLevels) {
    auto player = active_player();
    player.state().progress.level_exp = 90;
    EXPECT_EQ(player.add_experience(25, 100), 1u);
    EXPECT_EQ(player.state().progress.level, 11u);
    EXPECT_EQ(player.state().progress.level_exp, 15u);
    EXPECT_EQ(player.state().progress.total_exp, 25u);
}

TEST(PlayerProgress, ExperienceAdvancesOnceAndStopsAtOriginalMaximum) {
    auto player=active_player();
    player.state().progress.level=99;
    player.state().progress.level_exp=0;
    EXPECT_EQ(player.add_experience(1000,100),1u);
    EXPECT_EQ(player.state().progress.level,100u);
    EXPECT_EQ(player.state().progress.level_exp,900u);
    EXPECT_EQ(player.add_experience(1,200),1u);
    EXPECT_EQ(player.state().progress.level,101u);
    EXPECT_EQ(player.state().progress.level_exp,701u);
    player.state().progress.level=120;
    EXPECT_EQ(player.add_experience(1,200),0u);
    EXPECT_EQ(player.state().progress.level,120u);
    EXPECT_EQ(player.state().progress.level_exp,502u);
    // Already-loaded sentinel level is ignored by SetPlayerExpPoint itself.
    player.state().progress.level=121;
    const auto before=player.state().progress;
    EXPECT_EQ(player.add_experience(1000,200),0u);
    EXPECT_EQ(player.state().progress.level_exp,before.level_exp);
    EXPECT_EQ(player.state().progress.total_exp,before.total_exp);
}

TEST(PlayerInventory, RequestedEmptyPositionWins) {
    auto player = active_player();
    auto item = mxh::game::make_item(500, 42, 5);
    auto slot = player.insert_inventory_item(item);
    ASSERT_TRUE(slot.has_value());
    EXPECT_EQ(*slot, 5u);
    EXPECT_EQ(player.state().inventory.items[5].Position, 5u);
}

TEST(PlayerInventory, OccupiedRequestedPositionFallsBackToFirstEmpty) {
    auto player = active_player();
    player.state().inventory.items[5] = mxh::game::make_item(1, 1, 5);
    auto slot = player.insert_inventory_item(mxh::game::make_item(2, 2, 5));
    ASSERT_TRUE(slot.has_value());
    EXPECT_EQ(*slot, 0u);
}

TEST(PlayerInventory, DuplicateDbItemIsRejected) {
    auto player = active_player();
    EXPECT_TRUE(player.insert_inventory_item(mxh::game::make_item(500, 42, 0)));
    EXPECT_FALSE(player.insert_inventory_item(mxh::game::make_item(500, 42, 1)));
}

TEST(PlayerEquipment, EquipAndUnequipMoveWirePositions) {
    auto player = active_player();
    ASSERT_TRUE(player.insert_inventory_item(mxh::game::make_item(500, 42, 0)));
    EXPECT_TRUE(player.equip_inventory_item(0, mxh::game::WEARED_WEAPON));
    EXPECT_EQ(player.state().equipment.items[mxh::game::WEARED_WEAPON].Position, 81u);
    EXPECT_TRUE(player.unequip_item(mxh::game::WEARED_WEAPON, 3));
    EXPECT_EQ(player.state().inventory.items[3].Position, 3u);
    EXPECT_EQ(player.state().equipment.items[mxh::game::WEARED_WEAPON].dwDBIdx, 0u);
}

TEST(PlayerShopInventory, ExpiryRemovalRequiresExactIdentityAndOpenAbsoluteSlot) {
    auto player = active_player();
    const auto original = mxh::game::make_item(8001,55001,409);
    player.state().shop_inventory.items[19] = original;
    auto wrong = original; wrong.dwDBIdx=8002;
    EXPECT_FALSE(player.remove_shop_inventory_item(wrong));
    wrong=original; wrong.wIconIdx=55002;
    EXPECT_FALSE(player.remove_shop_inventory_item(wrong));
    for (auto position : {0u,389u,410u,429u,65535u}) {
        wrong=original; wrong.Position=static_cast<std::uint16_t>(position);
        EXPECT_FALSE(player.remove_shop_inventory_item(wrong));
    }
    EXPECT_EQ(player.state().shop_inventory.items[19].dwDBIdx,8001u);
    const auto removed=player.remove_shop_inventory_item(original);
    ASSERT_TRUE(removed); EXPECT_EQ(removed->dwDBIdx,8001u);
    EXPECT_EQ(player.state().shop_inventory.items[19].dwDBIdx,0u);
    EXPECT_EQ(player.state().shop_inventory.items[19].Position,409u);
    EXPECT_FALSE(player.remove_shop_inventory_item(original));
    player.state().shop_inventory.items[19]=original;
    ASSERT_TRUE(player.begin_logout());
    EXPECT_FALSE(player.remove_shop_inventory_item(original));
}

TEST(PlayerShopInventory, PhysicalExpirySearchesOriginalContainersAndFailsClosedOnDuplicates) {
    struct Case {
        std::uint16_t expected_position;
        std::uint16_t actual_position;
        mxh::game::ItemBase* slot;
    };
    auto player = active_player();
    const auto expected = mxh::game::make_item(8001, 55001, 390);
    for (auto test : std::array{
             Case{390, 409, &player.state().shop_inventory.items[19]},
             Case{390, 82, &player.state().equipment.items[2]},
             Case{390, 491, &player.state().pet_wear.items[1]},
             Case{390, 502, &player.state().titan_shop_items.items[2]}}) {
        *test.slot = mxh::game::make_item(8001, 55001, test.actual_position);
        const auto removed = player.remove_physical_shop_item(expected);
        ASSERT_TRUE(removed);
        EXPECT_EQ(removed->Position, test.actual_position);
        EXPECT_EQ(test.slot->dwDBIdx, 0u);
        EXPECT_EQ(test.slot->Position, test.actual_position);
    }
    player.state().shop_inventory.items[0] = mxh::game::make_item(8001, 55001, 390);
    player.state().equipment.items[0] = mxh::game::make_item(8001, 55001, 80);
    EXPECT_FALSE(player.remove_physical_shop_item(expected));
    EXPECT_EQ(player.state().shop_inventory.items[0].dwDBIdx, 8001u);
    EXPECT_EQ(player.state().equipment.items[0].dwDBIdx, 8001u);
    player.state().equipment.items[0] = mxh::game::make_empty_item();
    player.state().shop_inventory.items[0].wIconIdx = 55002;
    EXPECT_FALSE(player.remove_physical_shop_item(expected));
    EXPECT_EQ(player.state().shop_inventory.items[0].dwDBIdx, 8001u);
}

TEST(PlayerDamage, NormalShieldAbsorbsBeforeLife) {
    auto player = active_player();
    player.state().vitals.current_shield = 50;
    player.state().vitals.current_hp = 100;
    const auto result = player.apply_damage(80);
    EXPECT_EQ(result.shield_damage, 50u);
    EXPECT_EQ(result.life_damage, 30u);
    EXPECT_EQ(player.state().vitals.current_hp, 70u);
    EXPECT_FALSE(result.died);
}

TEST(PlayerDamage, MussangUsesSeventyPercentShieldReduction) {
    auto player = active_player();
    player.state().mussang_mode = true;
    player.state().vitals.current_shield = 100;
    player.state().vitals.current_hp = 100;
    const auto result = player.apply_damage(100);
    EXPECT_EQ(result.shield_damage, 70u);
    EXPECT_EQ(result.life_damage, 0u);
    EXPECT_EQ(player.state().vitals.current_hp, 100u);
}

TEST(PlayerDamage, DeathAndReviveTransition) {
    auto player = active_player();
    player.state().vitals.max_hp = 341;
    player.state().vitals.max_shield = 103;
    player.state().vitals.current_mp = 57;
    player.state().vitals.current_shield = 0;
    player.state().vitals.current_hp = 10;
    const auto result = player.apply_damage(10);
    EXPECT_TRUE(result.died);
    EXPECT_EQ(player.lifecycle(), mxh::server::PlayerLifecycle::Dead);
    EXPECT_TRUE(player.revive());
    EXPECT_EQ(player.lifecycle(), mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(player.state().vitals.current_hp, 102u);
    EXPECT_EQ(player.state().vitals.current_shield, 30u);
    EXPECT_EQ(player.state().vitals.current_mp, 0u);
    player.state().vitals.current_hp = 99;
    EXPECT_FALSE(player.revive());
    EXPECT_EQ(player.state().vitals.current_hp, 99u);
}

TEST(PlayerRecovery, HealFullRestoresAllVitals) {
    auto player = active_player();
    player.state().vitals.current_hp = 1;
    player.state().vitals.current_shield = 0;
    player.state().vitals.current_mp = 0;
    player.heal_full();
    EXPECT_EQ(player.state().vitals.current_hp, player.state().vitals.max_hp);
    EXPECT_EQ(player.state().vitals.current_shield, player.state().vitals.max_shield);
    EXPECT_EQ(player.state().vitals.current_mp, player.state().vitals.max_mp);
}

TEST(PlayerDamage, FinalizeExternalDamageRequiresActiveZeroLifeAndIsIdempotent) {
    auto player = active_player();
    EXPECT_FALSE(player.mark_dead_if_zero_life());
    player.state().vitals.current_hp = 0;
    EXPECT_TRUE(player.mark_dead_if_zero_life());
    EXPECT_EQ(player.lifecycle(), mxh::server::PlayerLifecycle::Dead);
    EXPECT_FALSE(player.mark_dead_if_zero_life());
    EXPECT_TRUE(player.revive());
    EXPECT_TRUE(player.is_active());
    EXPECT_TRUE(player.is_alive());
    player.release();
    EXPECT_FALSE(player.mark_dead_if_zero_life());
}

TEST(PlayerDamage, DeadPlayerCanLogOutWithoutReviving) {
    auto player = active_player();
    player.state().vitals.current_hp = 0;
    ASSERT_TRUE(player.mark_dead_if_zero_life());
    ASSERT_TRUE(player.begin_logout());
    EXPECT_EQ(player.lifecycle(), mxh::server::PlayerLifecycle::LoggingOut);
    EXPECT_EQ(player.state().vitals.current_hp, 0u);
    EXPECT_FALSE(player.revive());
    EXPECT_FALSE(player.begin_logout());
    EXPECT_EQ(player.state().vitals.current_hp, 0u);
}

TEST(PlayerDamage, ReviveClearsOnlyCandidateDeathFlagsAndLogoutPreservesThem) {
    auto player=active_player();
    EXPECT_FALSE(player.state().death_flags.battle_channel);
    EXPECT_FALSE(player.state().death_flags.guild_field_war);
    EXPECT_FALSE(player.state().death_flags.special_map);
    player.state().vitals.current_hp=0;
    ASSERT_TRUE(player.mark_dead_if_zero_life());
    player.state().death_flags={true,true,true};
    auto candidate=player;
    ASSERT_TRUE(candidate.revive());
    EXPECT_FALSE(candidate.state().death_flags.battle_channel);
    EXPECT_FALSE(candidate.state().death_flags.guild_field_war);
    EXPECT_FALSE(candidate.state().death_flags.special_map);
    EXPECT_TRUE(player.state().death_flags.battle_channel);
    EXPECT_TRUE(player.state().death_flags.guild_field_war);
    EXPECT_TRUE(player.state().death_flags.special_map);
    ASSERT_TRUE(player.begin_logout());
    EXPECT_FALSE(player.revive());
    EXPECT_TRUE(player.state().death_flags.guild_field_war);
    player.release();
    EXPECT_FALSE(player.state().death_flags.guild_field_war);
}

TEST(PlayerDamage, OrdinaryReviveCandidateCombinesLossAndRecoveryWithoutChangingLivePlayer) {
    auto player=active_player();
    player.state().progress.level=2;
    player.state().progress.level_exp=100;
    player.state().progress.money=100;
    player.state().vitals.current_hp=0;
    ASSERT_TRUE(player.mark_dead_if_zero_life());
    std::string text;
    for (unsigned level=1;level<=121;++level) text+=std::to_string(level)+" 1000\n";
    const auto curve=mxh::game::ExperienceCurve::load_from_text(text);
    const auto candidate=mxh::server::prepare_ordinary_revive_candidate(player,curve,{},
        mxh::game::ReviveLocation::Login,{},0);
    ASSERT_TRUE(candidate);
    EXPECT_TRUE(candidate->actor.is_alive());
    EXPECT_EQ(candidate->actor.state().progress.money,96u);
    EXPECT_EQ(candidate->actor.state().progress.level_exp,80u);
    EXPECT_EQ(candidate->actor.state().vitals.current_mp,0u);
    EXPECT_EQ(player.lifecycle(),mxh::server::PlayerLifecycle::Dead);
    EXPECT_EQ(player.state().progress.money,100u);
    EXPECT_EQ(player.state().progress.level_exp,100u);
    EXPECT_FALSE(mxh::server::prepare_ordinary_revive_candidate(candidate->actor,curve,{},
        mxh::game::ReviveLocation::Login,{},0));
}

TEST(PlayerRecovery, PresentRevivePreservesExemptProgressAndProtection) {
    using namespace mxh::server;
    auto player=active_player();
    auto& state=player.state();
    state.progress.level=4;
    state.progress.money=100;
    state.progress.level_exp=100;
    state.shop_options.ProtectCount=2;
    state.vitals.current_hp=0;
    ASSERT_TRUE(player.mark_dead_if_zero_life());
    std::string text;
    for (unsigned level=1;level<=121;++level) text+=std::to_string(level)+" 1000\n";
    const auto curve=mxh::game::ExperienceCurve::load_from_text(text);
    mxh::db::LegacyShopAppearanceRows rows;
    rows.used_items={{55000,390,1,2,10,20}};
    PresentReviveContext context{false,true,false,false,false,false,false,false,false,4};
    ShopItemManager manager;
    UsingShopItemEntry entry{};
    entry.ItemIdx=55000; entry.Data.ShopItem.ItemBase.wIconIdx=55000;
    entry.Data.ShopItem.ItemBase.dwDBIdx=1; entry.Data.ShopItem.Param=2;
    ASSERT_TRUE(manager.add_using_item(entry)); manager.set_protect_item_idx(55000);
    auto candidate=prepare_present_revive_candidate(player,curve,{},rows,manager,context,{});
    ASSERT_TRUE(candidate);
    EXPECT_EQ(candidate->actor.state().progress.money,100u);
    EXPECT_EQ(candidate->actor.state().progress.level_exp,100u);
    EXPECT_EQ(candidate->actor.state().shop_options.ProtectCount,2);
    ASSERT_EQ(candidate->shop.rows.used_items.size(),1u);
    EXPECT_EQ(candidate->shop.rows.used_items[0].parameter,2u);
    EXPECT_FALSE(candidate->shop.protection.consume_combined);
    EXPECT_TRUE(candidate->actor.is_alive());
    EXPECT_EQ(player.lifecycle(),PlayerLifecycle::Dead);
    state.progress.level=5;
    context.level=5;
    candidate=prepare_present_revive_candidate(player,curve,{},rows,manager,context,{});
    ASSERT_TRUE(candidate);
    EXPECT_TRUE(candidate->shop.protection.consume_combined);
    EXPECT_EQ(candidate->actor.state().shop_options.ProtectCount,1);
    EXPECT_EQ(candidate->manager.find_using_item(55000)->Data.ShopItem.Param,1u);
    EXPECT_EQ(manager.find_using_item(55000)->Data.ShopItem.Param,2u);
    context.level=4; // stale or inconsistent context cannot authorize recovery
    EXPECT_FALSE(prepare_present_revive_candidate(player,curve,{},rows,manager,context,{}));
}

TEST(PlayerRecovery, PresentReviveGuardOrderAndIndependentExemptionsMatchSource) {
    using namespace mxh::server;
    using D=PresentReviveDecision;
    const PresentReviveContext ordinary{false,true,false,false,false,false,false,false,false,5};
    EXPECT_EQ(decide_present_revive(ordinary),D::RecoverWithLoss);
    auto c=ordinary;
    c.died_for_battle_channel=true;
    EXPECT_EQ(decide_present_revive(c),D::RecoverWithLoss);
    c.battle_channel=true;
    EXPECT_EQ(decide_present_revive(c),D::RecoverWithoutLoss);
    c=ordinary; c.battle_channel=true;
    EXPECT_EQ(decide_present_revive(c),D::RecoverWithLoss);
    c=ordinary; c.penalty_exempt_map=true;
    EXPECT_EQ(decide_present_revive(c),D::RecoverWithoutLoss);
    c=ordinary; c.died_for_guild_field_war=true;
    EXPECT_EQ(decide_present_revive(c),D::RecoverWithoutLoss);
    c=ordinary; c.korean_first_action_penalty=true;
    EXPECT_EQ(decide_present_revive(c),D::RecoverWithoutLoss);
    c=ordinary; c.exiting=true;
    EXPECT_EQ(decide_present_revive(c),D::RejectExiting);
    c.looted=true;
    EXPECT_EQ(decide_present_revive(c),D::RejectLooted);
    c.dead=false;
    EXPECT_EQ(decide_present_revive(c),D::RejectNotDead);
    c.event_map=true;
    EXPECT_EQ(decide_present_revive(c),D::IgnoreEventMap);
}

TEST(PlayerRecovery, PresentReviveRejectsStaleDeathFactsBeforeApplyingLoss) {
    using namespace mxh::server;
    auto player=active_player();
    player.state().progress.level=5;
    player.state().progress.level_exp=100;
    player.state().progress.money=100;
    player.state().vitals.current_hp=0;
    ASSERT_TRUE(player.mark_dead_if_zero_life());
    std::string text;
    for(unsigned level=1;level<=121;++level) text+=std::to_string(level)+" 1000\n";
    const auto curve=mxh::game::ExperienceCurve::load_from_text(text);
    PresentReviveContext context{false,true,false,false,false,true,false,false,false,5};
    for(const bool guild:{false,true}) {
        player.state().death_flags={};
        context.died_for_battle_channel=!guild;
        context.died_for_guild_field_war=guild;
        EXPECT_FALSE(prepare_present_revive_candidate(player,curve,{}, {},{},context,{}));
        player.state().death_flags.battle_channel=!guild;
        player.state().death_flags.guild_field_war=guild;
        const auto result=prepare_present_revive_candidate(player,curve,{}, {},{},context,{});
        ASSERT_TRUE(result);
        EXPECT_EQ(result->actor.state().progress.money,100u);
        EXPECT_EQ(result->actor.state().progress.level_exp,100u);
        EXPECT_FALSE(result->actor.state().death_flags.battle_channel);
        EXPECT_FALSE(result->actor.state().death_flags.guild_field_war);
        context.died_for_battle_channel=false;
        context.died_for_guild_field_war=false;
        EXPECT_FALSE(prepare_present_revive_candidate(player,curve,{}, {},{},context,{}));
        EXPECT_EQ(player.lifecycle(),PlayerLifecycle::Dead);
    }
}

TEST(PlayerRecovery, ContextUsesLootingRoomLifetimeAndResolvedMap) {
    using namespace mxh::server;
    auto player=active_player();
    player.state().vitals.current_hp=0;
    ASSERT_TRUE(player.mark_dead_if_zero_life());
    mxh::game::MapKindTable maps;
    LootingManagerState looting;
    const auto resolve=[&] { return resolve_present_revive_context(
        player,looting,maps,false,false,false); };
    EXPECT_FALSE(resolve());
    maps[player.state().map_num]=64;
    ASSERT_TRUE(resolve());
    EXPECT_EQ(decide_present_revive(*resolve()),PresentReviveDecision::RecoverWithLoss);
    create_looting_room(looting,make_looting_room(player.state().player_id,2002,0,100));
    EXPECT_EQ(decide_present_revive(*resolve()),PresentReviveDecision::RejectLooted);
    process_looting_timeouts(looting,15100); // Source uses strictly greater than.
    EXPECT_TRUE(resolve()->looted);
    process_looting_timeouts(looting,15101);
    EXPECT_FALSE(resolve()->looted);
    maps[player.state().map_num]=256;
    EXPECT_TRUE(resolve()->penalty_exempt_map);
    maps.unresolved_maps.insert(player.state().map_num);
    EXPECT_FALSE(resolve());
    maps.unresolved_maps.clear();
    player.state().death_flags.guild_field_war=true;
    EXPECT_TRUE(resolve()->died_for_guild_field_war);
    ASSERT_TRUE(player.begin_logout());
    EXPECT_TRUE(resolve()->exiting);
    player.state().map_num=58;
    EXPECT_EQ(maps.count(58),0u);
    ASSERT_TRUE(resolve());
    EXPECT_TRUE(resolve()->event_map);
    EXPECT_EQ(decide_present_revive(*resolve()),PresentReviveDecision::IgnoreEventMap);
    maps[58]=128;
    maps.unresolved_maps.insert(58);
    ASSERT_TRUE(resolve());
    EXPECT_EQ(decide_present_revive(*resolve()),PresentReviveDecision::IgnoreEventMap);
}

TEST(PlayerRecovery, ReviveShopManagerConsumesCopyAndRejectsStaleIdentity) {
    using namespace mxh::server;
    ShopItemManager manager;
    UsingShopItemEntry entry{};
    entry.ItemIdx=55000;
    entry.Data.ShopItem.ItemBase.wIconIdx=55000;
    entry.Data.ShopItem.ItemBase.dwDBIdx=901;
    entry.Data.ShopItem.Param=1;
    ASSERT_TRUE(manager.add_using_item(entry));
    manager.set_protect_item_idx(55000);
    mxh::db::LegacyShopAppearanceRows rows;
    rows.used_items={{55000,390,901,1,0,0}};
    const auto plan=prepare_revive_shop(rows,{6,30},1,55000);
    ASSERT_TRUE(plan);
    const auto candidate=prepare_revive_shop_manager(manager,rows,*plan);
    ASSERT_TRUE(candidate);
    EXPECT_EQ(candidate->using_item_count(),0u);
    EXPECT_EQ(candidate->protect_item_idx(),0u);
    EXPECT_EQ(manager.using_item_count(),1u);
    EXPECT_EQ(manager.protect_item_idx(),55000u);
    rows.used_items[0].database_id=902;
    EXPECT_FALSE(prepare_revive_shop_manager(manager,rows,*plan));
}

TEST(PlayerRecovery, PresentReviveIncludesPetCopyOnlyForUnprotectedLoss) {
    using namespace mxh::server;
    auto player=active_player();
    player.state().progress.level=5;
    player.state().progress.level_exp=100;
    player.state().progress.money=100;
    player.state().vitals.current_hp=0;
    ASSERT_TRUE(player.mark_dead_if_zero_life());
    std::string text;
    for (unsigned level=1;level<=121;++level) text+=std::to_string(level)+" 1000\n";
    const auto curve=mxh::game::ExperienceCurve::load_from_text(text);
    PetManagerState pets;
    PetTotalInfo pet{};
    pet.PetSummonItemDBIdx=901;
    pet.PetFriendly=10;
    pet.bAlive=1;
    pets.m_PetInfoList.push_back(pet);
    pets.m_curSummonItemDBIdx=901;
    pets.m_iFriendshipReduceAmount=-20;
    PresentReviveContext context{false,true,false,false,false,false,false,false,false,5};
    auto result=prepare_present_revive_candidate(player,curve,{}, {},{},context,pets);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->actor.state().progress.money,94u);
    EXPECT_EQ(result->pets.m_PetInfoList[0].PetFriendly,0u);
    EXPECT_TRUE(result->pet_effect.send_pet_death);
    EXPECT_EQ(result->pet_effect.release_delay_ms,5000u);
    EXPECT_EQ(pets.m_PetInfoList[0].PetFriendly,10u);
    EXPECT_EQ(pets.m_PetInfoList[0].bAlive,1u);
    EXPECT_EQ(player.lifecycle(),PlayerLifecycle::Dead);
    mxh::db::LegacyShopAppearanceRows rows;
    rows.used_items={{55312,390,123,0,0,0}};
    ShopItemManager manager;
    UsingShopItemEntry entry{};
    entry.ItemIdx=55312; entry.Data.ShopItem.ItemBase.wIconIdx=55312;
    entry.Data.ShopItem.ItemBase.dwDBIdx=123;
    ASSERT_TRUE(manager.add_using_item(entry));
    result=prepare_present_revive_candidate(player,curve,{},rows,manager,context,pets);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->pets.m_PetInfoList[0].PetFriendly,10u);
    EXPECT_FALSE(result->pet_effect.log_master_death);
    EXPECT_EQ(result->manager.using_item_count(),0u);
    EXPECT_EQ(manager.using_item_count(),1u);
    context.penalty_exempt_map=true;
    result=prepare_present_revive_candidate(player,curve,{}, {},{},context,pets);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->pets.m_PetInfoList[0].PetFriendly,10u);
    EXPECT_FALSE(result->pet_effect.log_master_death);
    context.penalty_exempt_map=false;
    pets.m_PetInfoList.clear();
    EXPECT_FALSE(prepare_present_revive_candidate(player,curve,{}, {},{},context,pets));
    pets.m_PetInfoList={pet,pet};
    EXPECT_FALSE(prepare_present_revive_candidate(player,curve,{}, {},{},context,pets));
}
