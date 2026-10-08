#include "mxh/server/player_equipment_stats.hpp"
#include "mxh/server/skill_caster.hpp"
#include "mxh/game/item_list_parser.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <map>

namespace {
using namespace mxh::server;
using namespace mxh::game;
PlayerState new_character() {
    CalcBaseStats base; base.gengol=base.minchub=base.cheryuk=base.simmek=12;
    return make_player_state(1,1,1,base,{});
}
ItemInfo sword() {
    ItemInfo i; i.ItemIdx=11000; i.ItemKind=2049; i.EquipKind=WEARED_WEAPON;
    i.WeaponType=1; i.MeleeAttackMin=13; i.MeleeAttackMax=17; i.LimitLevel=1;
    return i;
}
struct Catalog {
    std::map<std::uint16_t,ItemInfo> entries{{11000,sword()}};
    bool operator()(std::uint16_t id,ItemInfo& out) const {
        const auto it=entries.find(id); if (it==entries.end()) return false;
        out=it->second; return true;
    }
};
void wear(PlayerState& state,std::uint8_t slot,std::uint16_t icon) {
    state.equipment.items[slot]=make_item(100+slot,icon,TP_WEAREDITEM_START+slot);
}
}

TEST(PlayerEquipmentStats, NewEquipRepeatUnequipAndRehydrationAgree) {
    auto state=new_character(); PlayerCombatStats combat; Catalog catalog;
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(combat.phy_attack_min,16u); EXPECT_EQ(combat.phy_attack_max,16u);
    EXPECT_EQ(combat.phy_defence,8u); EXPECT_EQ(state.vitals.max_hp,125u);
    wear(state,WEARED_WEAPON,11000);
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(combat.phy_attack_min,29u); EXPECT_EQ(combat.phy_attack_max,35u);
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(combat.phy_attack_min,29u); EXPECT_EQ(state.attributes.gengol,12u);
    auto reloaded=new_character(); reloaded.equipment=state.equipment;
    PlayerCombatStats reconnected;
    ASSERT_TRUE(rebuild_equipment_stats(reloaded,reconnected,catalog));
    EXPECT_EQ(reconnected.phy_attack_min,combat.phy_attack_min);
    EXPECT_EQ(reconnected.phy_attack_max,combat.phy_attack_max);
    state.equipment.items[WEARED_WEAPON]={};
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(combat.phy_attack_min,16u);
}

TEST(PlayerEquipmentStats, ArmourAttributesAndResourcesRebuildWithoutHealing) {
    auto state=new_character(); PlayerCombatStats combat; Catalog catalog;
    ItemInfo armor; armor.ItemIdx=23000; armor.ItemKind=2050; armor.EquipKind=WEARED_DRESS;
    armor.CheRyuk=3; armor.PhyDef=20; armor.Life=15;
    catalog.entries[23000]=armor; wear(state,WEARED_DRESS,23000);
    state.vitals.current_hp=70;
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(state.vitals.max_hp,170u); EXPECT_EQ(state.vitals.current_hp,70u);
    EXPECT_EQ(combat.phy_defence,30u); EXPECT_EQ(state.attributes.cheryuk,12u);
    state.equipment.items[WEARED_DRESS]={};
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(state.vitals.max_hp,125u); EXPECT_EQ(state.vitals.current_hp,70u);
    EXPECT_EQ(combat.phy_defence,8u);
}

TEST(PlayerEquipmentStats, RangeUsesDexterityAndArmletIsPercentage) {
    auto state=new_character(); state.attributes.minchub=37;
    PlayerCombatStats combat; Catalog catalog;
    auto bow=sword(); bow.WeaponType=5; bow.RangeAttackMin=13; bow.RangeAttackMax=17;
    catalog.entries[11000]=bow; wear(state,WEARED_WEAPON,11000);
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(combat.phy_attack_min,76u);
    ItemInfo armlet; armlet.ItemIdx=30000; armlet.ItemKind=2058;
    armlet.EquipKind=WEARED_ARMLET; armlet.RangeAttackMin=10; armlet.RangeAttackMax=10;
    catalog.entries[30000]=armlet; wear(state,WEARED_ARMLET,30000);
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(combat.phy_attack_min,83u); // DWORD(76 * 1.1f), not 76+10.
}

TEST(PlayerEquipmentStats, MissingOrUnsupportedResourcesDoNotPartiallyPublish) {
    auto state=new_character(); PlayerCombatStats combat; Catalog catalog;
    state.equipment.items[WEARED_WEAPON].wIconIdx=11000; // only an appearance
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(combat.phy_attack_min,16u);
    wear(state,WEARED_WEAPON,999);
    EXPECT_FALSE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(combat.phy_attack_min,16u);
    wear(state,WEARED_WEAPON,11000); state.equipment.items[WEARED_WEAPON].RareIdx=7;
    EXPECT_FALSE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(state.bonuses.item_max_life,0);
}

TEST(PlayerEquipmentStats, ActualDamageUsesBothEndpointsRatherThanDefaultTen) {
    auto state=new_character(); wear(state,WEARED_WEAPON,11000);
    PlayerCombatStats attacker,defender; Catalog catalog;
    ASSERT_TRUE(rebuild_equipment_stats(state,attacker,catalog));
    attacker.critical_rate=0; defender.dodge_rate=0; defender.phy_defence=0;
    SkillInfo skill{}; std::mt19937 rng(42);
    int minimum=1000, maximum=0;
    for (int n=0;n<200;++n) {
        const auto result=skill_caster_calculate_damage(attacker,defender,skill,rng);
        minimum=std::min(minimum,result.damage); maximum=std::max(maximum,result.damage);
    }
    EXPECT_EQ(minimum,29); EXPECT_EQ(maximum,35);
}

TEST(PlayerEquipmentStats, CanonicalStarterTemplateProducesAuditedRange) {
    const auto path=std::filesystem::path(MXH_SOURCE_DIR)/"data/PlayDH/Resource/ItemList.bin";
    const auto parsed=load_item_list(path.string());
    const auto item=std::find_if(parsed.items.begin(),parsed.items.end(),[](const auto& row) { return row.ItemIdx==11000; });
    ASSERT_NE(item,parsed.items.end());
    Catalog catalog; catalog.entries[11000]=*item;
    auto state=new_character(); wear(state,WEARED_WEAPON,11000);
    PlayerCombatStats combat;
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(combat.phy_attack_min,29u); EXPECT_EQ(combat.phy_attack_max,35u);
    EXPECT_EQ(combat.phy_defence,8u);
}

TEST(PlayerEquipmentStats, ModernPlainDurability100AndZeroAreAcceptedButNotArbitraryOptionIds) {
    auto state=new_character(); PlayerCombatStats combat; Catalog catalog;
    wear(state,WEARED_WEAPON,11000);
    auto& item=state.equipment.items[WEARED_WEAPON];
    ASSERT_EQ(item.Durability,100u); // Real factory default, no test normalization.
    ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    EXPECT_EQ(combat.phy_attack_min,29u); EXPECT_EQ(combat.phy_attack_max,35u);
    for (auto invalid : {1u,99u,101u,UINT32_MAX}) {
        item.Durability=invalid;
        EXPECT_FALSE(rebuild_equipment_stats(state,combat,catalog));
        EXPECT_EQ(combat.phy_attack_min,29u); // Failed candidate cannot overwrite stats.
    }
    item.Durability=0;
    EXPECT_TRUE(rebuild_equipment_stats(state,combat,catalog));
    item.Durability=100; item.RareIdx=100;
    EXPECT_FALSE(rebuild_equipment_stats(state,combat,catalog));
    item.RareIdx=0; catalog.entries[11000].wSetItemKind=1;
    EXPECT_FALSE(rebuild_equipment_stats(state,combat,catalog));
}

TEST(PlayerEquipmentStats, RealtimeAppearanceIsNotOrdinaryDurabilityOrWeaponStats) {
    for (auto kind : {LEGACY_SHOP_ITEM_MAKEUP,LEGACY_SHOP_ITEM_EQUIP}) {
        auto state=new_character(); PlayerCombatStats combat; Catalog catalog;
        ItemInfo appearance; appearance.ItemIdx=55001; appearance.ItemKind=kind;
        appearance.ItemType=11; appearance.SellPrice=1; // SHOP_ITEM_PARAM_STORED_TIME
        catalog.entries[55001]=appearance;
        wear(state,WEARED_DRESS,55001);
        state.equipment.items[WEARED_DRESS].Durability=1;
        ASSERT_TRUE(rebuild_equipment_stats(state,combat,catalog));
        EXPECT_EQ(combat.phy_attack_min,16u); EXPECT_EQ(combat.phy_defence,8u);
        EXPECT_EQ(state.vitals.max_hp,125u);
        EXPECT_EQ(state.equipment.items[WEARED_DRESS].Durability,1u);
        auto reloaded=new_character(); reloaded.equipment=state.equipment;
        ASSERT_TRUE(rebuild_equipment_stats(reloaded,combat,catalog));
        reloaded.equipment.items[WEARED_DRESS]={}; // ShopItemManager expiration.
        ASSERT_TRUE(rebuild_equipment_stats(reloaded,combat,catalog));
        EXPECT_EQ(combat.phy_attack_min,16u); EXPECT_EQ(reloaded.vitals.max_hp,125u);
    }
}

TEST(PlayerEquipmentStats, TimedAppearanceExceptionCannotHideCombatOrOptionFields) {
    auto state=new_character(); PlayerCombatStats combat; Catalog catalog;
    ItemInfo appearance; appearance.ItemIdx=55001; appearance.ItemKind=LEGACY_SHOP_ITEM_EQUIP;
    appearance.ItemType=11; appearance.SellPrice=1; // SHOP_ITEM_PARAM_STORED_TIME
    wear(state,WEARED_DRESS,55001); auto& worn=state.equipment.items[WEARED_DRESS];
    worn.Durability=1;
    const auto rejects=[&](ItemInfo item) { catalog.entries[55001]=item; return !rebuild_equipment_stats(state,combat,catalog); };
    auto changed=appearance; changed.Life=1; EXPECT_TRUE(rejects(changed));
    changed=appearance; changed.GenGol=1; EXPECT_TRUE(rejects(changed));
    changed=appearance; changed.MeleeAttackMin=1; EXPECT_TRUE(rejects(changed));
    changed=appearance; changed.AttrAttack.Element[0]=.1f; EXPECT_TRUE(rejects(changed));
    changed=appearance; changed.Plus_MugongIdx=1; EXPECT_TRUE(rejects(changed));
    changed=appearance; changed.LifeRecover=1; EXPECT_TRUE(rejects(changed));
    changed=appearance; changed.wSetItemKind=1; EXPECT_TRUE(rejects(changed));
    changed=appearance; changed.ItemKind=2048; changed.EquipKind=WEARED_DRESS; EXPECT_TRUE(rejects(changed));
    changed=appearance; changed.SellPrice=2; EXPECT_TRUE(rejects(changed)); // playtime, not realtime
    changed=appearance; changed.ItemType=0; EXPECT_TRUE(rejects(changed));
    worn.RareIdx=1; EXPECT_TRUE(rejects(appearance));
}
