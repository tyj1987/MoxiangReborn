#pragma once

//
// calc_avatar_option_test.cpp -- D4.125
//
// 1:1 lock the legacy CShopItemManager::CalcAvatarOption() data plane from
// [Server]Map/ShopItemManager.cpp. The accumulator walks the player 24-slot
// avatar[] array (m_pPlayer->GetShopItemStats()->Avatar) and, for every non-empty
// slot (wIconIdx >= 2), looks up the corresponding ITEM_INFO and folds the per-field
// deltas into an AVATARITEMOPTION. Each test pins one branch of the legacy accumulator
// so future drift triggers a test failure.
//

#include <gtest/gtest.h>

#include "mxh/server/avatar_calc.hpp"

#include <array>
#include <cstdint>

using namespace mxh::server;
using namespace mxh::game;

// ---------------------- empty / zero edge cases ----------------------

TEST(CalcAvatarOption, EmptyAvatarProducesZeroOutput) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    ItemManager mgr;
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Life, 0u);
    EXPECT_EQ(out.Shield, 0u);
    EXPECT_EQ(out.Naeruyk, 0u);
    EXPECT_EQ(out.Attack, 0u);
    EXPECT_EQ(out.Gengol, 0u);
    EXPECT_EQ(out.bKyungGong, 0u);
    EXPECT_EQ(out.NaeruykspendbyKG, 0u);
    EXPECT_EQ(out.MussangDamage, 0u);
}

TEST(CalcAvatarOption, CosmeticSlotsAreSkipped) {
    // Legacy: skip if (pAvatar[i] < 2). Cosmetic slots 0/1 are skipped even if the
    // manager has the icon (which it does for wIconIdx 1=base skin).
    std::array<std::uint16_t, EAvatarCount> avatar{};
    for (std::size_t i = 0; i < EAvatarCount; ++i) avatar[i] = 0u;
    avatar[0] = 0u;  // empty
    avatar[1] = 1u;  // base skin
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 1u;  // would match avatar[1]
    info.GenGol = 100;
    info.Life = 1000;
    info.EquipKind = 1;
    info.AllPlus_Kind = 1;
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Gengol, 0u);
    EXPECT_EQ(out.Life, 0u);
    EXPECT_EQ(out.bKyungGong, 0u);
    EXPECT_EQ(out.NaeruykspendbyKG, 0u);
}

TEST(CalcAvatarOption, MissingItemIsSkipped) {
    // Legacy: if (!ITEMMGR->GetItemInfo(wIconIdx)) continue;
    // try_get returns false for indices not in the table.
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 100u;  // not in manager
    ItemManager mgr;
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Gengol, 0u);
    EXPECT_EQ(out.Life, 0u);
    EXPECT_EQ(out.bKyungGong, 0u);
    EXPECT_EQ(out.NaeruykspendbyKG, 0u);
    EXPECT_EQ(out.MussangDamage, 0u);
}

TEST(CalcAvatarOption, ZeroStatsInItemContributeNothing) {
    // Legacy: if (info.X > 0) opt->Y += info.X; -- zero stats are skipped.
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 50u;
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 50u;
    info.GenGol = 0;
    info.MinChub = 0;
    info.Life = 0;
    info.EquipKind = 0;  // != 1, so NaeruykspendbyKG stays 0
    info.AllPlus_Kind = 0;  // != 1, so bKyungGong stays 0
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Gengol, 0u);
    EXPECT_EQ(out.Minchub, 0u);
    EXPECT_EQ(out.Life, 0u);
    EXPECT_EQ(out.bKyungGong, 0u);
    EXPECT_EQ(out.NaeruykspendbyKG, 0u);
}

TEST(CalcAvatarOption, ZeroValuesAcrossAll28FieldsProduceZeroOutput) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    for (std::size_t i = 0; i < EAvatarCount; ++i) avatar[i] = static_cast<std::uint16_t>(100 + i);
    ItemManager mgr;
    // Each avatar slot has a unique icon idx 100..123; add a matching empty info for each.
    for (std::uint32_t i = 0; i < EAvatarCount; ++i) {
        ItemInfo info;
        info.ItemIdx = 100u + i;
        mgr.add(info);
    }
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Life, 0u);
    EXPECT_EQ(out.Shield, 0u);
    EXPECT_EQ(out.Naeruyk, 0u);
    EXPECT_EQ(out.Attack, 0u);
    EXPECT_EQ(out.Critical, 0u);
    EXPECT_EQ(out.Decisive, 0u);
    EXPECT_EQ(out.Gengol, 0u);
    EXPECT_EQ(out.Minchub, 0u);
    EXPECT_EQ(out.Cheryuk, 0u);
    EXPECT_EQ(out.Simmek, 0u);
    EXPECT_EQ(out.CounterPercent, 0u);
    EXPECT_EQ(out.CounterDamage, 0u);
    EXPECT_EQ(out.bKyungGong, 0u);
    EXPECT_EQ(out.NeaRyukSpend, 0u);
    EXPECT_EQ(out.NeagongDamage, 0u);
    EXPECT_EQ(out.WoigongDamage, 0u);
    EXPECT_EQ(out.TargetPhyDefDown, 0u);
    EXPECT_EQ(out.TargetAttrDefDown, 0u);
    EXPECT_EQ(out.TargetAtkDown, 0u);
    EXPECT_EQ(out.RecoverRate, 0u);
    EXPECT_EQ(out.KyunggongSpeed, 0u);
    EXPECT_EQ(out.MussangCharge, 0u);
    EXPECT_EQ(out.NaeruykspendbyKG, 0u);
    EXPECT_EQ(out.ShieldRecoverRate, 0u);
    EXPECT_EQ(out.MussangDamage, 0u);
}

TEST(CalcAvatarOption, SingleItemAllFieldsPopulated) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 100u;
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 100u;
    info.GenGol = 10;
    info.MinChub = 20;
    info.CheRyuk = 30;
    info.SimMek = 40;
    info.Life = 100;
    info.Shield = 200;
    info.NaeRyuk = 50;
    info.MeleeAttackMin = 5;
    info.CriticalPercent = 7;
    info.Plus_MugongIdx = 11;
    info.Plus_Value = 13;
    info.AllPlus_Kind = 1;
    info.LimitCheRyuk = 22;
    info.LimitJob = 33;
    info.LimitGender = 44;
    info.LimitLevel = 55;
    info.LimitGenGol = 66;
    info.LimitMinChub = 77;
    info.LimitSimMek = 88;
    info.ItemGrade = 99;
    info.RangeType = 12;
    info.EquipKind = 1;
    info.NaeRyukRecover = 14;
    info.RangeAttackMin = 15;
    info.RangeAttackMax = 16;
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Gengol, 10u);
    EXPECT_EQ(out.Minchub, 20u);
    EXPECT_EQ(out.Cheryuk, 30u);
    EXPECT_EQ(out.Simmek, 40u);
    EXPECT_EQ(out.Life, 100u);
    EXPECT_EQ(out.Shield, 200u);
    EXPECT_EQ(out.Naeruyk, 50u);
    EXPECT_EQ(out.Attack, 5u);
    EXPECT_EQ(out.Critical, 7u);
    EXPECT_EQ(out.CounterPercent, 11u);
    EXPECT_EQ(out.CounterDamage, 13u);
    EXPECT_EQ(out.bKyungGong, 1u);
    EXPECT_EQ(out.NeaRyukSpend, 22u);
    EXPECT_EQ(out.NeagongDamage, 33u);
    EXPECT_EQ(out.WoigongDamage, 44u);
    EXPECT_EQ(out.TargetPhyDefDown, 55u);
    EXPECT_EQ(out.TargetAttrDefDown, 66u);
    EXPECT_EQ(out.TargetAtkDown, 77u);
    EXPECT_EQ(out.RecoverRate, 88u);
    EXPECT_EQ(out.KyunggongSpeed, 99u);
    EXPECT_EQ(out.MussangCharge, 12u);
    EXPECT_EQ(out.NaeruykspendbyKG, 1u);
    EXPECT_EQ(out.Decisive, 14u);
    EXPECT_EQ(out.ShieldRecoverRate, 15u);
    EXPECT_EQ(out.MussangDamage, 16u);
}

// ---------------------- multi-item accumulation ----------------------

TEST(CalcAvatarOption, MultipleItemsAccumulateAcrossFields) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[0] = 100u;
    avatar[1] = 101u;
    avatar[2] = 102u;
    ItemManager mgr;
    ItemInfo info1;
    info1.ItemIdx = 100u;
    info1.GenGol = 5;
    info1.Life = 100;
    ItemInfo info2;
    info2.ItemIdx = 101u;
    info2.GenGol = 7;
    info2.MinChub = 3;
    ItemInfo info3;
    info3.ItemIdx = 102u;
    info3.Life = 50;
    info3.Shield = 200;
    info3.NaeRyuk = 33;
    mgr.add(info1);
    mgr.add(info2);
    mgr.add(info3);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Gengol, 12u);
    EXPECT_EQ(out.Minchub, 3u);
    EXPECT_EQ(out.Life, 150u);
    EXPECT_EQ(out.Shield, 200u);
    EXPECT_EQ(out.Naeruyk, 33u);
}

TEST(CalcAvatarOption, MixedSlotsSomeSkippedSomeAccumulated) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[0] = 0u;
    avatar[1] = 1u;
    avatar[2] = 100u;
    avatar[5] = 200u;
    avatar[10] = 101u;
    ItemManager mgr;
    ItemInfo info100;
    info100.ItemIdx = 100u;
    info100.GenGol = 10;
    ItemInfo info101;
    info101.ItemIdx = 101u;
    info101.GenGol = 20;
    mgr.add(info100);
    mgr.add(info101);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Gengol, 30u);
}

TEST(CalcAvatarOption, AllTwentyFourSlotsPopulated) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    for (std::size_t i = 0; i < EAvatarCount; ++i) {
        avatar[i] = static_cast<std::uint16_t>(100 + i);
    }
    ItemManager mgr;
    for (std::uint32_t i = 100; i < 100 + EAvatarCount; ++i) {
        ItemInfo info;
        info.ItemIdx = i;
        info.GenGol = 1;
        mgr.add(info);
    }
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Gengol, static_cast<std::uint8_t>(EAvatarCount));
}

// ---------------------- flag predicate semantics ----------------------

TEST(CalcAvatarOption, AllPlusKindEqualsOneSetsKyungGongFlag) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 100u;
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 100u;
    info.AllPlus_Kind = 1;
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.bKyungGong, 1u);
}

TEST(CalcAvatarOption, AllPlusKindNotOneLeavesKyungGongZero) {
    // Legacy: if (info.AllPlus_Kind == 1) opt->bKyungGong = 1; -- only == 1 sets it.
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 100u;
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 100u;
    info.AllPlus_Kind = 2;
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.bKyungGong, 0u);
}

TEST(CalcAvatarOption, EquipKindEqualsOneSetsNaeruykspendByKGFlag) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 100u;
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 100u;
    info.EquipKind = 1;
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.NaeruykspendbyKG, 1u);
}

TEST(CalcAvatarOption, EquipKindZeroLeavesNaeruykspendByKGZero) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 100u;
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 100u;
    info.EquipKind = 0;
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.NaeruykspendbyKG, 0u);
}

TEST(CalcAvatarOption, EquipKindTwoLeavesNaeruykspendByKGZero) {
    // Only == 1 sets the flag (not == 2 or any other value).
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 100u;
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 100u;
    info.EquipKind = 2;
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.NaeruykspendbyKG, 0u);
}

TEST(CalcAvatarOption, FlagsAcrossMultipleSlotsSetOnce) {
    // Legacy: flag is set to 1 (not summed). Once set, multiple matching items keep it 1.
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[0] = 100u;
    avatar[1] = 101u;
    avatar[2] = 102u;
    ItemManager mgr;
    ItemInfo info1;
    info1.ItemIdx = 100u;
    info1.AllPlus_Kind = 1;
    ItemInfo info2;
    info2.ItemIdx = 101u;
    info2.AllPlus_Kind = 1;
    ItemInfo info3;
    info3.ItemIdx = 102u;
    info3.AllPlus_Kind = 1;
    mgr.add(info1);
    mgr.add(info2);
    mgr.add(info3);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.bKyungGong, 1u);
}

// ---------------------- truncation semantics ----------------------

TEST(CalcAvatarOption, GenGolByteCastWrapsAt256) {
    // Legacy: info.GenGol is uint16, opt.Gengol is uint8. Modern port uses static_cast<uint8_t>
    // for both operands then sums; same byte-level wrap as legacy implicit truncation.
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[0] = 100u;
    avatar[1] = 101u;
    ItemManager mgr;
    ItemInfo info1;
    info1.ItemIdx = 100u;
    info1.GenGol = 200;
    ItemInfo info2;
    info2.ItemIdx = 101u;
    info2.GenGol = 100;
    mgr.add(info1);
    mgr.add(info2);
    const auto out = calc_avatar_option(avatar, mgr);
    // 200 + 100 = 300, cast to uint8 = 44 (300 mod 256).
    EXPECT_EQ(out.Gengol, 44u);
}

TEST(CalcAvatarOption, LifeWordCastWrapsAt65536) {
    // Legacy: info.Life is uint32, opt.Life is uint16. Modern port uses static_cast<uint16_t>
    // for both operands then sums.
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[0] = 100u;
    avatar[1] = 101u;
    ItemManager mgr;
    ItemInfo info1;
    info1.ItemIdx = 100u;
    info1.Life = 50000;
    ItemInfo info2;
    info2.ItemIdx = 101u;
    info2.Life = 30000;
    mgr.add(info1);
    mgr.add(info2);
    const auto out = calc_avatar_option(avatar, mgr);
    // 50000 + 30000 = 80000, cast to uint16 = 14464 (80000 mod 65536).
    EXPECT_EQ(out.Life, 14464u);
}

TEST(CalcAvatarOption, NaeruykWordPassThroughUpTo65535) {
    // Legacy: info.NaeRyuk is uint16, opt.Naeruyk is uint16. No cast.
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[0] = 100u;
    avatar[1] = 101u;
    ItemManager mgr;
    ItemInfo info1;
    info1.ItemIdx = 100u;
    info1.NaeRyuk = 30000;
    ItemInfo info2;
    info2.ItemIdx = 101u;
    info2.NaeRyuk = 30000;
    mgr.add(info1);
    mgr.add(info2);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Naeruyk, 60000u);
}

TEST(CalcAvatarOption, SingleValueByteCastFitsCleanly) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 100u;
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 100u;
    info.GenGol = 255;
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Gengol, 255u);
}

TEST(CalcAvatarOption, SingleValueWordCastFitsCleanly) {
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 100u;
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 100u;
    info.Life = 60000;
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    EXPECT_EQ(out.Life, 60000u);
}

// ---------------------- 1:1 lock: 28-field single-item sanity ----------------------

TEST(CalcAvatarOption, SingleItemAllFieldsIndependentSums) {
    // Lock: each of the 28 accumulator fields is independently driven.
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[5] = 100u;
    ItemManager mgr;
    ItemInfo info;
    info.ItemIdx = 100u;
    info.GenGol = 1; info.MinChub = 2; info.CheRyuk = 3; info.SimMek = 4;
    info.Life = 5; info.Shield = 6; info.NaeRyuk = 7;
    info.MeleeAttackMin = 8; info.CriticalPercent = 9;
    info.Plus_MugongIdx = 10; info.Plus_Value = 11;
    info.AllPlus_Kind = 1;
    info.LimitCheRyuk = 12; info.LimitJob = 13; info.LimitGender = 14;
    info.LimitLevel = 15; info.LimitGenGol = 16; info.LimitMinChub = 17;
    info.LimitSimMek = 18;
    info.ItemGrade = 19; info.RangeType = 20; info.EquipKind = 1;
    info.NaeRyukRecover = 21; info.RangeAttackMin = 22; info.RangeAttackMax = 23;
    mgr.add(info);
    const auto out = calc_avatar_option(avatar, mgr);
    // stats
    EXPECT_EQ(out.Gengol, 1u); EXPECT_EQ(out.Minchub, 2u);
    EXPECT_EQ(out.Cheryuk, 3u); EXPECT_EQ(out.Simmek, 4u);
    EXPECT_EQ(out.Life, 5u); EXPECT_EQ(out.Shield, 6u);
    EXPECT_EQ(out.Naeruyk, 7u);
    EXPECT_EQ(out.Attack, 8u); EXPECT_EQ(out.Critical, 9u);
    EXPECT_EQ(out.CounterPercent, 10u); EXPECT_EQ(out.CounterDamage, 11u);
    EXPECT_EQ(out.bKyungGong, 1u);
    EXPECT_EQ(out.NeaRyukSpend, 12u);
    EXPECT_EQ(out.NeagongDamage, 13u); EXPECT_EQ(out.WoigongDamage, 14u);
    EXPECT_EQ(out.TargetPhyDefDown, 15u); EXPECT_EQ(out.TargetAttrDefDown, 16u);
    EXPECT_EQ(out.TargetAtkDown, 17u); EXPECT_EQ(out.RecoverRate, 18u);
    EXPECT_EQ(out.KyunggongSpeed, 19u); EXPECT_EQ(out.MussangCharge, 20u);
    EXPECT_EQ(out.NaeruykspendbyKG, 1u);
    EXPECT_EQ(out.Decisive, 21u); EXPECT_EQ(out.ShieldRecoverRate, 22u);
    EXPECT_EQ(out.MussangDamage, 23u);
}

TEST(CalcAvatarOption, TwoItemsAccumulateEachFieldIndependently) {
    // Lock: each of the 28 accumulator fields sums correctly across two items.
    std::array<std::uint16_t, EAvatarCount> avatar{};
    avatar[0] = 100u;
    avatar[1] = 101u;
    ItemManager mgr;
    ItemInfo info1;
    info1.ItemIdx = 100u;
    info1.GenGol = 1; info1.MinChub = 2; info1.CheRyuk = 3; info1.SimMek = 4;
    info1.Life = 5; info1.Shield = 6; info1.NaeRyuk = 7;
    info1.MeleeAttackMin = 8; info1.CriticalPercent = 9;
    info1.Plus_MugongIdx = 10; info1.Plus_Value = 11;
    info1.AllPlus_Kind = 1;
    info1.LimitCheRyuk = 12; info1.LimitJob = 13; info1.LimitGender = 14;
    info1.LimitLevel = 15; info1.LimitGenGol = 16; info1.LimitMinChub = 17;
    info1.LimitSimMek = 18;
    info1.ItemGrade = 19; info1.RangeType = 20; info1.EquipKind = 1;
    info1.NaeRyukRecover = 21; info1.RangeAttackMin = 22; info1.RangeAttackMax = 23;
    ItemInfo info2;
    info2.ItemIdx = 101u;
    info2.GenGol = 1; info2.MinChub = 2; info2.CheRyuk = 3; info2.SimMek = 4;
    info2.Life = 5; info2.Shield = 6; info2.NaeRyuk = 7;
    info2.MeleeAttackMin = 8; info2.CriticalPercent = 9;
    info2.Plus_MugongIdx = 10; info2.Plus_Value = 11;
    info2.AllPlus_Kind = 0;  // not == 1, no flag
    info2.LimitCheRyuk = 12; info2.LimitJob = 13; info2.LimitGender = 14;
    info2.LimitLevel = 15; info2.LimitGenGol = 16; info2.LimitMinChub = 17;
    info2.LimitSimMek = 18;
    info2.ItemGrade = 19; info2.RangeType = 20; info2.EquipKind = 0;  // not == 1, no flag
    info2.NaeRyukRecover = 21; info2.RangeAttackMin = 22; info2.RangeAttackMax = 23;
    mgr.add(info1);
    mgr.add(info2);
    const auto out = calc_avatar_option(avatar, mgr);
    // doubles of info1 values (with proper truncations on byte/word fields).
    EXPECT_EQ(out.Gengol, 2u); EXPECT_EQ(out.Minchub, 4u);
    EXPECT_EQ(out.Cheryuk, 6u); EXPECT_EQ(out.Simmek, 8u);
    EXPECT_EQ(out.Life, 10u); EXPECT_EQ(out.Shield, 12u);
    EXPECT_EQ(out.Naeruyk, 14u);
    EXPECT_EQ(out.Attack, 16u); EXPECT_EQ(out.Critical, 18u);
    EXPECT_EQ(out.CounterPercent, 20u); EXPECT_EQ(out.CounterDamage, 22u);
    EXPECT_EQ(out.bKyungGong, 1u);  // set by info1, info2 doesnt unset
    EXPECT_EQ(out.NeaRyukSpend, 24u);
    EXPECT_EQ(out.NeagongDamage, 26u); EXPECT_EQ(out.WoigongDamage, 28u);
    EXPECT_EQ(out.TargetPhyDefDown, 30u); EXPECT_EQ(out.TargetAttrDefDown, 32u);
    EXPECT_EQ(out.TargetAtkDown, 34u); EXPECT_EQ(out.RecoverRate, 36u);
    EXPECT_EQ(out.KyunggongSpeed, 38u); EXPECT_EQ(out.MussangCharge, 40u);
    EXPECT_EQ(out.NaeruykspendbyKG, 1u);  // set by info1, info2 doesnt unset
    EXPECT_EQ(out.Decisive, 42u); EXPECT_EQ(out.ShieldRecoverRate, 44u);
    EXPECT_EQ(out.MussangDamage, 46u);
}

// ---------------------- struct size verification ----------------------

TEST(CalcAvatarOption, AvatarItemOptionSizeMatchesLegacy) {
    // 1:1 lock: AVATARITEMOPTION under pack(1) is 39 bytes.
    EXPECT_EQ(sizeof(AvatarItemOption), 39u);
}

TEST(CalcAvatarOption, EAvatarCountIs24) {
    // 1:1 lock: eAvatar_Max = 24 (24 avatar slots, indices 0..23).
    EXPECT_EQ(EAvatarCount, 24u);
}
