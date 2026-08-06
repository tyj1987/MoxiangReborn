// avatar_calc_test.cpp
//
// 1:1 tests for mxh::server::calc_avatar_option (D4.23).
//
// Covers the data-plane accumulator in modern/include/mxh/server/
// avatar_calc.hpp, which mirrors legacy CShopItemManager::CalcAvatarOption()
// from [Server]Map/ShopItemManager.cpp.

#include <mxh/game/avatar_item_option.hpp>
#include <mxh/game/item_list_types.hpp>
#include <mxh/game/item_manager.hpp>
#include <mxh/server/avatar_calc.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

using mxh::game::AvatarItemOption;
using mxh::game::AvatarSlot;
using mxh::game::EAvatarCount;
using mxh::game::ItemInfo;
using mxh::game::ItemManager;

namespace {

// 1:1 size assertion (mirrors avatar_item_option.hpp internal static_assert).
TEST(AvatarItemOptionLayout, StructIsThirtyNineBytes) {
    EXPECT_EQ(sizeof(AvatarItemOption), 39u);
}

// eAvatar_Max sentinel matches the legacy 24-slot array.
TEST(AvatarItemOptionLayout, MaxSlotEqualsTwentyFour) {
    EXPECT_EQ(static_cast<std::size_t>(AvatarSlot::Max), 24u);
    EXPECT_EQ(EAvatarCount, 24u);
}

// Cosmetic slots are 0..11, weapon slots are 12..22, sentinel is 23.
TEST(AvatarItemOptionLayout, SlotIndicesMatchLegacy) {
    EXPECT_EQ(static_cast<std::size_t>(AvatarSlot::Hat),         0u);
    EXPECT_EQ(static_cast<std::size_t>(AvatarSlot::Hair),        1u);
    EXPECT_EQ(static_cast<std::size_t>(AvatarSlot::Dress),       6u);
    EXPECT_EQ(static_cast<std::size_t>(AvatarSlot::Hand),       11u);
    EXPECT_EQ(static_cast<std::size_t>(AvatarSlot::Weared_Hair), 12u);
    EXPECT_EQ(static_cast<std::size_t>(AvatarSlot::Weared_Gum),  17u);
    EXPECT_EQ(static_cast<std::size_t>(AvatarSlot::Weared_Amgi), 22u);
}

// Helpers
ItemInfo mk_info(std::uint16_t idx) {
    ItemInfo it{};
    it.ItemIdx = idx;
    return it;
}

std::array<std::uint16_t, EAvatarCount> empty_avatar() {
    std::array<std::uint16_t, EAvatarCount> a{};
    return a;
}

}  // namespace

// ===========================================================================
// Empty inputs
// ===========================================================================

TEST(CalcAvatarOption, EmptyAvatarArrayReturnsZeroedStruct) {
    ItemManager mgr;
    auto a = empty_avatar();
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
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

TEST(CalcAvatarOption, CosmeticBaseIndexOneIsSkipped) {
    // Legacy: skip if pAvatar[i] < 2. Even if the ItemManager has an
    // ItemInfo for wIconIdx == 1, the slot should not contribute.
    ItemManager mgr;
    ItemInfo info = mk_info(1);
    info.GenGol = 99;  // would otherwise add to Gengol.
    mgr.add(info);
    auto a = empty_avatar();
    a.fill(1);  // every slot is the cosmetic base skin.
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, 0u);
}

TEST(CalcAvatarOption, UnknownItemIndexIsSkipped) {
    // Legacy: if (pItemInfo == NULL) continue;
    ItemManager mgr;
    auto a = empty_avatar();
    a[0] = 1234;  // not in mgr -> lookup miss -> skip.
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Life, 0u);
    EXPECT_EQ(out.Shield, 0u);
    EXPECT_EQ(out.Gengol, 0u);
}

// ===========================================================================
// Single-field accumulation
// ===========================================================================

TEST(CalcAvatarOption, SingleGenGolAccumulatesToGengolByte) {
    ItemManager mgr;
    ItemInfo info = mk_info(100);
    info.GenGol = 5;
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 100;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, 5u);
}

TEST(CalcAvatarOption, SingleLifeAccumulatesToLifeWord) {
    ItemManager mgr;
    ItemInfo info = mk_info(101);
    info.Life = 1000;
    mgr.add(info);
    auto a = empty_avatar();
    a[6] = 101;  // dress slot
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Life, 1000u);
}

TEST(CalcAvatarOption, GenGolZeroDoesNotContribute) {
    ItemManager mgr;
    ItemInfo info = mk_info(102);
    info.GenGol = 0;  // legacy predicate: if (info.GenGol > 0) ...
    info.Life = 500;
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 102;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, 0u);
    EXPECT_EQ(out.Life, 500u);
}

// ===========================================================================
// Multi-field accumulation per item
// ===========================================================================

TEST(CalcAvatarOption, SingleItemContributesAllTwentyEightFields) {
    ItemManager mgr;
    ItemInfo info = mk_info(200);
    info.GenGol = 1;
    info.MinChub = 2;
    info.CheRyuk = 3;
    info.SimMek = 4;
    info.Life = 500;
    info.Shield = 600;
    info.NaeRyuk = 700;
    info.MeleeAttackMin = 8;
    info.CriticalPercent = 9;
    info.Plus_MugongIdx = 10;
    info.Plus_Value = 11;
    info.AllPlus_Kind = 1;
    info.LimitCheRyuk = 12;
    info.LimitJob = 13;
    info.LimitGender = 14;
    info.LimitLevel = 15;
    info.LimitGenGol = 16;
    info.LimitMinChub = 17;
    info.LimitSimMek = 18;
    info.ItemGrade = 19;
    info.RangeType = 20;
    info.EquipKind = 1;
    info.NaeRyukRecover = 21;
    info.RangeAttackMin = 22;
    info.RangeAttackMax = 23;
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 200;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, 1u);
    EXPECT_EQ(out.Minchub, 2u);
    EXPECT_EQ(out.Cheryuk, 3u);
    EXPECT_EQ(out.Simmek, 4u);
    EXPECT_EQ(out.Life, 500u);
    EXPECT_EQ(out.Shield, 600u);
    EXPECT_EQ(out.Naeruyk, 700u);
    EXPECT_EQ(out.Attack, 8u);
    EXPECT_EQ(out.Critical, 9u);
    EXPECT_EQ(out.Decisive, 21u);
    EXPECT_EQ(out.CounterPercent, 10u);
    EXPECT_EQ(out.CounterDamage, 11u);
    EXPECT_EQ(out.bKyungGong, 1u);
    EXPECT_EQ(out.NeaRyukSpend, 12u);
    EXPECT_EQ(out.NeagongDamage, 13u);
    EXPECT_EQ(out.WoigongDamage, 14u);
    EXPECT_EQ(out.TargetPhyDefDown, 15u);
    EXPECT_EQ(out.TargetAttrDefDown, 16u);
    EXPECT_EQ(out.TargetAtkDown, 17u);
    EXPECT_EQ(out.RecoverRate, 18u);
    EXPECT_EQ(out.KyunggongSpeed, 19u);
    EXPECT_EQ(out.MussangCharge, 20u);
    EXPECT_EQ(out.NaeruykspendbyKG, 1u);
    EXPECT_EQ(out.ShieldRecoverRate, 22u);
    EXPECT_EQ(out.MussangDamage, 23u);
}

// ===========================================================================
// Multi-item accumulation across slots
// ===========================================================================

TEST(CalcAvatarOption, MultiSlotAccumulatesAcrossAllSlots) {
    ItemManager mgr;
    ItemInfo a1 = mk_info(301);
    a1.GenGol = 10;
    ItemInfo a2 = mk_info(302);
    a2.GenGol = 20;
    ItemInfo a3 = mk_info(303);
    a3.GenGol = 5;
    mgr.add(a1);
    mgr.add(a2);
    mgr.add(a3);
    auto a = empty_avatar();
    a[0] = 301;  // Hat -> Gengol += 10
    a[6] = 302;  // Dress -> Gengol += 20
    a[17] = 303; // Weared_Gum -> Gengol += 5
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, 35u);  // 10 + 20 + 5
}

TEST(CalcAvatarOption, AllPlusKindFlagIsSetBySingleMatchingItem) {
    // Legacy: if (info.AllPlus_Kind == 1) opt->bKyungGong = 1;
    // The flag is set unconditionally - multiple matching items do not
    // accumulate, just stay at 1.
    ItemManager mgr;
    ItemInfo info1 = mk_info(401);
    info1.AllPlus_Kind = 1;
    ItemInfo info2 = mk_info(402);
    info2.AllPlus_Kind = 1;
    mgr.add(info1);
    mgr.add(info2);
    auto a = empty_avatar();
    a[0] = 401;
    a[1] = 402;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.bKyungGong, 1u);
}

TEST(CalcAvatarOption, AllPlusKindNonOneDoesNotSetFlag) {
    ItemManager mgr;
    ItemInfo info = mk_info(403);
    info.AllPlus_Kind = 7;  // not == 1 -> legacy predicate fails.
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 403;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.bKyungGong, 0u);
}

TEST(CalcAvatarOption, EquipKindFlagIsSetBySingleMatchingItem) {
    ItemManager mgr;
    ItemInfo info = mk_info(404);
    info.EquipKind = 1;
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 404;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.NaeruykspendbyKG, 1u);
}

TEST(CalcAvatarOption, LimitSimMekUsesTruthyPredicate) {
    // Legacy: if (pItemInfo->LimitSimMek) - non-zero is enough (legacy C++
    // does implicit bool conversion, including negative values which we
    // cannot represent here since the field is uint16).
    ItemManager mgr;
    ItemInfo info = mk_info(405);
    info.LimitSimMek = 1;  // non-zero triggers the add.
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 405;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.RecoverRate, 1u);
}

TEST(CalcAvatarOption, LimitSimMekZeroDoesNotContribute) {
    ItemManager mgr;
    ItemInfo info = mk_info(406);
    info.LimitSimMek = 0;
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 406;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.RecoverRate, 0u);
}

// ===========================================================================
// Narrow-cast (uint16 -> uint8) semantics match legacy
// ===========================================================================

TEST(CalcAvatarOption, GengolByteCastPreservesValueWhenInRange) {
    ItemManager mgr;
    ItemInfo info = mk_info(501);
    info.GenGol = 200;  // fits in uint8.
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 501;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, 200u);
}

TEST(CalcAvatarOption, GengolByteCastTruncatesOverflow) {
    // Legacy implicit uint16 -> uint8 truncation. The modern port
    // mirrors this exactly via static_cast. We assert that 1000 & 0xFF
    // (= 232) is what legacy and modern both produce.
    ItemManager mgr;
    ItemInfo info = mk_info(502);
    info.GenGol = 1000;  // 0x03E8, truncates to 0xE8 = 232.
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 502;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, static_cast<std::uint8_t>(1000));  // 232
}

TEST(CalcAvatarOption, LifeWordCastTruncatesOverflow) {
    ItemManager mgr;
    ItemInfo info = mk_info(503);
    info.Life = 70000;  // truncates to 70000 & 0xFFFF = 4464.
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 503;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Life, static_cast<std::uint16_t>(70000));  // 4464
}

// ===========================================================================
// Mixed slot content - empty, base-skin, equipped, missing
// ===========================================================================

TEST(CalcAvatarOption, MixedSlotsAccumulateOnlyValidEntries) {
    ItemManager mgr;
    ItemInfo info = mk_info(601);
    info.GenGol = 7;
    info.MinChub = 11;
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 0;      // empty -> skip
    a[1] = 1;      // base skin -> skip
    a[2] = 601;    // equipped + in mgr -> contributes
    a[3] = 999;    // equipped but missing from mgr -> skip
    a[4] = 2;      // wIconIdx 2 but no entry -> skip
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, 7u);
    EXPECT_EQ(out.Minchub, 11u);
}

// ===========================================================================
// Determinism (idempotent on second call)
// ===========================================================================

TEST(CalcAvatarOption, RepeatedCallsProduceIdenticalResults) {
    ItemManager mgr;
    ItemInfo info = mk_info(701);
    info.GenGol = 3;
    info.Life = 100;
    info.CriticalPercent = 5;
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 701;
    a[6] = 701;
    auto out1 = mxh::server::calc_avatar_option(a, mgr);
    auto out2 = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out1.Gengol, out2.Gengol);
    EXPECT_EQ(out1.Life, out2.Life);
    EXPECT_EQ(out1.Critical, out2.Critical);
}

TEST(CalcAvatarOption, TwoSlotsSameItemSumsDoublesTheDelta) {
    // Same item equipped in two slots -> both contribute independently.
    ItemManager mgr;
    ItemInfo info = mk_info(801);
    info.GenGol = 10;
    info.Life = 100;
    mgr.add(info);
    auto a = empty_avatar();
    a[0] = 801;
    a[6] = 801;
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, 20u);
    EXPECT_EQ(out.Life, 200u);
}

// ===========================================================================
// All 24 slots exercised at once
// ===========================================================================

TEST(CalcAvatarOption, AllTwentyFourSlotsCanContribute) {
    ItemManager mgr;
    for (std::uint16_t i = 0; i < 24; ++i) {
        ItemInfo info = mk_info(static_cast<std::uint16_t>(100 + i));
        info.GenGol = 1;
        mgr.add(info);
    }
    auto a = empty_avatar();
    for (std::uint16_t i = 0; i < 24; ++i) {
        a[i] = static_cast<std::uint16_t>(100 + i);
    }
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, 24u);  // 24 slots * 1 each
}

TEST(CalcAvatarOption, AllTwentyFourSlotsAtBaseSkinContributeZero) {
    // Every slot at base skin (wIconIdx == 1) -> all skipped.
    ItemManager mgr;
    ItemInfo info = mk_info(901);
    info.GenGol = 99;
    mgr.add(info);
    auto a = empty_avatar();
    a.fill(1);
    AvatarItemOption out = mxh::server::calc_avatar_option(a, mgr);
    EXPECT_EQ(out.Gengol, 0u);
}
