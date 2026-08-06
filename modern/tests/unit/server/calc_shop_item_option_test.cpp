// calc_shop_item_option_test.cpp - 1:1 data-plane tests for the legacy
// CShopItemManager::CalcShopItemOption() from [Server]Map/ShopItemManager.cpp:1246.
// Locks the +/- calc = (bAdd ? 1 : -1) accumulator semantics, the
// clamp-to-zero underflow behavior, and the locale-bounded incantation
// side effects so the modern dispatcher can wire orchestrator code
// without semantic drift.

#include <mxh/game/shop_item_option.hpp>
#include <mxh/server/calc_shop_item_option.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using mxh::game::ShopItemOption;
using mxh::server::CalcShopItemOptionEnv;
using mxh::server::CalcShopItemOptionInfo;
using mxh::server::CalcShopItemOptionSideEffects;
using mxh::server::CalcShopItemOptionStatus;
using mxh::server::calc_shop_item_option;
using mxh::server::IncantationId;
using mxh::server::LEGACY_SHOP_ITEM_CHARM;
using mxh::server::LEGACY_SHOP_ITEM_DECORATION;
using mxh::server::LEGACY_SHOP_ITEM_HERB;
using mxh::server::LEGACY_SHOP_ITEM_INCANTATION;
using mxh::server::LEGACY_SHOP_ITEM_MAKEUP;
using mxh::server::LEGACY_SHOP_ITEM_SUNDRIES;

namespace {

// Test env that lets the test flip the event_rate_active gate.
class TestEnv final : public CalcShopItemOptionEnv {
public:
    bool event_rate_active(std::uint16_t rate_id) const noexcept override {
        (void)rate_id;
        return rate_active;
    }
    bool rate_active = true;
};

CalcShopItemOptionInfo BaseCharmInfo() {
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_CHARM;
    info.ItemIdx  = 0;
    info.ItemType = 0;
    return info;
}

}  // namespace

//---------------------------------------------------------------------
// Layout / struct invariants
//---------------------------------------------------------------------

TEST(ShopItemOptionLayout, SizeMatchesLegacy) {
    EXPECT_EQ(sizeof(ShopItemOption), 124u);
}

TEST(ShopItemOptionLayout, AvatarArrayIs24Words) {
    ShopItemOption s;
    EXPECT_EQ(s.Avatar.size(), 24u);
}

TEST(ShopItemOptionLayout, SkinItemArrayIs6Words) {
    ShopItemOption s;
    EXPECT_EQ(s.wSkinItem.size(), 6u);
}

//---------------------------------------------------------------------
// Early returns / guards
//---------------------------------------------------------------------

TEST(CalcShopItemOption, WIdxZeroReturnsInvalidIcon) {
    ShopItemOption s;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_CHARM;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 0, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::InvalidIcon);
    // No mutation.
    EXPECT_EQ(s.Gengol, 0);
    EXPECT_EQ(s.Minchub, 0);
}

TEST(CalcShopItemOption, NonSpecialMissingInfoReturnsItemInfoMissing) {
    ShopItemOption s;
    CalcShopItemOptionInfo info;  // default zeros: ItemKind=0, ItemIdx=0
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1 /*not special*/, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::ItemInfoMissing);
    EXPECT_EQ(s.Gengol, 0);
}

TEST(CalcShopItemOption, SpecialIncantationMissingInfoReturnsItemInfoMissing) {
    // The legacy code returns FALSE on missing info for special
    // incantations too (the special "UsedShopItem" path is the only
    // caller that produces the StatePoint/SkillPoint side effects).
    ShopItemOption s;
    CalcShopItemOptionInfo info;  // default zeros
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(
        s, static_cast<std::uint32_t>(IncantationId::SkPointRedist),
        true, 5, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::ItemInfoMissing);
    EXPECT_EQ(s.StatePoint, 0);
    EXPECT_EQ(s.SkillPoint, 0u);
}

TEST(CalcShopItemOption, MakeupDecorationIsNoOp) {
    ShopItemOption s;
    s.Gengol = 5;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_MAKEUP;
    info.ItemIdx = 9999;
    info.GenGol = 100;  // would have been applied if Makeup were treated as Charm
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 9999, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Gengol, 5);  // untouched
}

//---------------------------------------------------------------------
// Incantation branch
//---------------------------------------------------------------------

TEST(CalcShopItemOption, IncantationMixUpAdds10OnAdd) {
    ShopItemOption s;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = static_cast<std::uint32_t>(IncantationId::MixUp);
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, info.ItemIdx, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.ItemMixSuccess, 10);
}

TEST(CalcShopItemOption, IncantationMixUpSubtracts10OnRemove) {
    ShopItemOption s;
    s.ItemMixSuccess = 25;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = static_cast<std::uint32_t>(IncantationId::MixUp);
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, info.ItemIdx, false, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.ItemMixSuccess, 15);
}

TEST(CalcShopItemOption, IncantationMixUpClampsToZeroOnUnderflow) {
    ShopItemOption s;
    s.ItemMixSuccess = 5;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = static_cast<std::uint32_t>(IncantationId::MixUp);
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, info.ItemIdx, false, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.ItemMixSuccess, 0);
}

TEST(CalcShopItemOption, IncantationGenGolAddsStatePointWhenParamNonZero) {
    ShopItemOption s;
    s.StatePoint = 7;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = 42;
    info.GenGol   = 1;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 42, true, 100, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.StatePoint, 107);
}

TEST(CalcShopItemOption, IncantationGenGolZeroParamDoesNotTouchStatePoint) {
    ShopItemOption s;
    s.StatePoint = 7;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = 42;
    info.GenGol   = 1;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 42, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.StatePoint, 7);
}

TEST(CalcShopItemOption, IncantationLifeAddsSkillPointWhenParamNonZero) {
    ShopItemOption s;
    s.SkillPoint = 3;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = 43;
    info.Life     = 10;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 43, true, 1, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.SkillPoint, 13u);
}

TEST(CalcShopItemOption, IncantationLifeZeroParamDoesNotTouchSkillPoint) {
    ShopItemOption s;
    s.SkillPoint = 3;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = 43;
    info.Life     = 10;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 43, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.SkillPoint, 3u);
}

TEST(CalcShopItemOption, IncantationCheRyukProtectsItemOnAdd) {
    ShopItemOption s;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = 44;
    info.CheRyuk  = 5;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 44, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.ProtectCount, 5);
    EXPECT_EQ(fx.new_protect_item_idx, 44u);
}

TEST(CalcShopItemOption, IncantationCheRyukPrefersParamOnAdd) {
    ShopItemOption s;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = 44;
    info.CheRyuk  = 5;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 44, true, 9, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.ProtectCount, 9);
    EXPECT_EQ(fx.new_protect_item_idx, 44u);
}

TEST(CalcShopItemOption, IncantationCheRyukZeroesProtectOnRemove) {
    ShopItemOption s;
    s.ProtectCount = 5;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = 44;
    info.CheRyuk  = 5;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 44, false, 0, info, env, 99, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.ProtectCount, 5);  // not touched on remove
    EXPECT_EQ(fx.new_protect_item_idx, 0u);
}

TEST(CalcShopItemOption, IncantationLimitJobAddsEquipLevelFree) {
    ShopItemOption s;
    s.EquipLevelFree = 3;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = 45;
    info.LimitJob = 7;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 45, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.EquipLevelFree, 10);
}

TEST(CalcShopItemOption, IncantationLimitJobSubtractsEquipLevelFree) {
    ShopItemOption s;
    s.EquipLevelFree = 10;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = 45;
    info.LimitJob = 7;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 45, false, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.EquipLevelFree, 3);
}

TEST(CalcShopItemOption, IncantationInvenExtendSetsExpandedFlag) {
    ShopItemOption s;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = static_cast<std::uint32_t>(IncantationId::InvenExtend);
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, info.ItemIdx, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_TRUE(fx.expanded_inven_slot);
    EXPECT_FALSE(fx.expanded_pyoguk_slot);
    EXPECT_FALSE(fx.expanded_mugong_slot);
    EXPECT_FALSE(fx.expanded_character_slot);
}

//---------------------------------------------------------------------
// Charm branch
//---------------------------------------------------------------------

TEST(CalcShopItemOption, CharmGenGolAddsOnAdd) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.GenGol = 10;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Gengol, 10);
}

TEST(CalcShopItemOption, CharmGenGolSubtractsOnRemove) {
    ShopItemOption s;
    s.Gengol = 15;
    auto info = BaseCharmInfo();
    info.GenGol = 10;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, false, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Gengol, 5);
}

TEST(CalcShopItemOption, CharmGenGolClampsToZeroOnUnderflow) {
    ShopItemOption s;
    s.Gengol = 5;
    auto info = BaseCharmInfo();
    info.GenGol = 10;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, false, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Gengol, 0);
}

TEST(CalcShopItemOption, CharmMinChubCheryukSimMekAdd) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.MinChub = 5;
    info.CheRyuk = 6;
    info.SimMek  = 7;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Minchub, 5);
    EXPECT_EQ(s.Cheryuk, 6);
    EXPECT_EQ(s.Simmek, 7);
}

TEST(CalcShopItemOption, CharmLifeShieldNarrowToSigned8) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.Life   = 100;
    info.Shield = 80;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.NeagongDamage, 100);
    EXPECT_EQ(s.WoigongDamage, 80);
}

TEST(CalcShopItemOption, CharmLifeClampsToZeroOnUnderflow) {
    ShopItemOption s;
    s.NeagongDamage = 30;
    auto info = BaseCharmInfo();
    info.Life = 100;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, false, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.NeagongDamage, 0);
}

TEST(CalcShopItemOption, CharmNaeRyukAddsToAddSung) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.NaeRyuk = 50;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.AddSung, 50);
}

TEST(CalcShopItemOption, CharmLimitJobAddsComboDamage) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.LimitJob = 30;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.ComboDamage, 30);
}

TEST(CalcShopItemOption, CharmLimitGenderAddsCritical) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.LimitGender = 20;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Critical, 20);
}

TEST(CalcShopItemOption, CharmLimitLevelAddsStunByCri) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.LimitLevel = 25;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.StunByCri, 25);
}

TEST(CalcShopItemOption, CharmLimitGenGolAddsRegistPhys) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.LimitGenGol = 33;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.RegistPhys, 33);
}

TEST(CalcShopItemOption, CharmLimitMinChubAddsRegistAttr) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.LimitMinChub = 44;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.RegistAttr, 44);
}

TEST(CalcShopItemOption, CharmLimitCheRyukPlustimeWhenMeleeAttackMin) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.LimitCheRyuk   = 60;
    info.MeleeAttackMin = 7;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.PlustimeNaeruyk, 60);
    EXPECT_EQ(s.NeaRyukSpend, 0);
}

TEST(CalcShopItemOption, CharmLimitCheRyukPlustimeZeroWhenRemoveCalled) {
    ShopItemOption s;
    s.PlustimeNaeruyk = 60;
    auto info = BaseCharmInfo();
    info.LimitCheRyuk   = 60;
    info.MeleeAttackMin = 7;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, false, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.PlustimeNaeruyk, 0);
}

TEST(CalcShopItemOption, CharmLimitCheRyukPlustimeZeroWhenEventRateInactive) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.LimitCheRyuk   = 60;
    info.MeleeAttackMin = 7;
    TestEnv env;
    env.rate_active = false;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.PlustimeNaeruyk, 0);
}

TEST(CalcShopItemOption, CharmLimitCheRyukAddsNeaRyukSpendWhenNoMeleeAttackMin) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.LimitCheRyuk   = 60;
    info.MeleeAttackMin = 0;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.NeaRyukSpend, 60);
    EXPECT_EQ(s.PlustimeNaeruyk, 0);
}

TEST(CalcShopItemOption, CharmLimitSimMekPlustimeActive) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.LimitSimMek    = 22;
    info.MeleeAttackMin = 8;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.PlustimeExp, 22);
    EXPECT_EQ(s.AddExp, 0);
}

TEST(CalcShopItemOption, CharmLimitSimMekAddsAddExpWhenNoMeleeAttackMin) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.LimitSimMek    = 22;
    info.MeleeAttackMin = 0;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.AddExp, 22);
    EXPECT_EQ(s.PlustimeExp, 0);
}

TEST(CalcShopItemOption, CharmItemGradePlustimeActive) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.ItemGrade      = 11;
    info.MeleeAttackMin = 9;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.PlustimeAbil, 11);
    EXPECT_EQ(s.AddAbility, 0);
}

TEST(CalcShopItemOption, CharmItemGradeAddsAddAbilityWhenNoMeleeAttackMin) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.ItemGrade      = 11;
    info.MeleeAttackMin = 0;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.AddAbility, 11);
    EXPECT_EQ(s.PlustimeAbil, 0);
}

TEST(CalcShopItemOption, CharmRangeTypeAddsAddMugongExp) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.RangeType = 12;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.AddMugongExp, 12);
}

TEST(CalcShopItemOption, CharmPlusMugongIdxAddsLifeNoClampAtZero) {
    ShopItemOption s;
    s.Life = 5;
    auto info = BaseCharmInfo();
    info.Plus_MugongIdx = 13;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Life, 18);
}

TEST(CalcShopItemOption, CharmPlusValueAddsShieldNoClampAtZero) {
    ShopItemOption s;
    s.Shield = 5;
    auto info = BaseCharmInfo();
    info.Plus_Value = 14;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Shield, 19);
}

TEST(CalcShopItemOption, CharmAllPlusKindAddsNaeryukNoClampAtZero) {
    ShopItemOption s;
    s.Naeryuk = 5;
    auto info = BaseCharmInfo();
    info.AllPlus_Kind = 16;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Naeryuk, 21);
}

TEST(CalcShopItemOption, CharmRangeAttackMinAddsBKyungGong) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.RangeAttackMin = 17;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.bKyungGong, 17);
}

TEST(CalcShopItemOption, CharmRangeAttackMaxAddsKyungGongSpeed) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.RangeAttackMax = 18;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.KyungGongSpeed, 18);
}

TEST(CalcShopItemOption, CharmCriticalPercentAddsReinforceAmp) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.CriticalPercent = 19;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.ReinforceAmp, 19);
}

TEST(CalcShopItemOption, CharmPhyDefAddsAddItemDropNoClamp) {
    ShopItemOption s;
    s.AddItemDrop = 30;
    auto info = BaseCharmInfo();
    info.PhyDef = 20;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.AddItemDrop, 50);
}

TEST(CalcShopItemOption, CharmNaeRyukRecoverAddsDecisive) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.NaeRyukRecover = 21;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Decisive, 21);
}

TEST(CalcShopItemOption, CharmAttrFireSetsDecorationOnAdd) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.ItemIdx  = 12345;
    info.AttrFire = 1;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 12345, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.dwStreetStallDecoration, 12345u);
}

TEST(CalcShopItemOption, CharmAttrFireClearsDecorationOnRemove) {
    ShopItemOption s;
    s.dwStreetStallDecoration = 12345;
    auto info = BaseCharmInfo();
    info.ItemIdx  = 12345;
    info.AttrFire = 1;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 12345, false, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.dwStreetStallDecoration, 0u);
}

//---------------------------------------------------------------------
// Herb branch
//---------------------------------------------------------------------

TEST(CalcShopItemOption, HerbLifeAddsLifeWithZeroClamp) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.ItemKind = LEGACY_SHOP_ITEM_HERB;
    info.Life = 25;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Life, 25);
}

TEST(CalcShopItemOption, HerbLifeClampsToZeroOnUnderflow) {
    ShopItemOption s;
    s.Life = 5;
    auto info = BaseCharmInfo();
    info.ItemKind = LEGACY_SHOP_ITEM_HERB;
    info.Life = 25;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, false, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Life, 0);
}

TEST(CalcShopItemOption, HerbShieldAddsShieldWithZeroClamp) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.ItemKind = LEGACY_SHOP_ITEM_HERB;
    info.Shield = 30;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Shield, 30);
}

TEST(CalcShopItemOption, HerbNaeRyukAddsNaeryukWithZeroClamp) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.ItemKind = LEGACY_SHOP_ITEM_HERB;
    info.NaeRyuk = 35;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.Naeryuk, 35);
}

//---------------------------------------------------------------------
// Sundries branch (HK_LOCAL semantics)
//---------------------------------------------------------------------

TEST(CalcShopItemOption, SundriesCheRyukAddsBStreetStall) {
    ShopItemOption s;
    auto info = BaseCharmInfo();
    info.ItemKind = LEGACY_SHOP_ITEM_SUNDRIES;
    info.CheRyuk  = 40;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.bStreetStall, 40);
}

TEST(CalcShopItemOption, SundriesCheRyukZeroIsNoOp) {
    ShopItemOption s;
    s.bStreetStall = 5;
    auto info = BaseCharmInfo();
    info.ItemKind = LEGACY_SHOP_ITEM_SUNDRIES;
    info.CheRyuk  = 0;
    TestEnv env;
    CalcShopItemOptionSideEffects fx;

    auto status = calc_shop_item_option(s, 1, true, 0, info, env, 0, fx);
    EXPECT_EQ(status, CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(s.bStreetStall, 5);
}
