#include <gtest/gtest.h>
#include <mxh/server/restore_used_shop_item.hpp>
#include <string>
#include <vector>
#include <limits>

namespace {
using namespace mxh;
using namespace mxh::server;
struct RestoreEffects : ShopRestoreEffects {
    std::vector<std::string> calls;
    bool discard_ok = true;
    std::uint32_t calculated = 0, updated_param = 0, updated_remaining = 0;
    void update_use_info(const game::ItemBase& item, std::uint32_t p, std::uint32_t r) override {
        EXPECT_EQ(item.wIconIdx, 55299); calls.push_back("update"); updated_param=p; updated_remaining=r;
    }
    bool discard_item(const game::ItemBase&) override { calls.push_back("discard"); return discard_ok; }
    void delete_use_info(const game::ShopItemBase&) override { calls.push_back("delete"); }
    void log_expired(const game::ShopItemBase&) override { calls.push_back("log"); }
    void calculate_option(std::uint16_t, std::uint32_t p) override { calls.push_back("calculate"); calculated=p; }
    void add_dup_parameter(const game::ItemInfo* info) override { calls.push_back(info ? "dup" : "dup-null"); }
};
struct RestoreUsedShopItem : testing::Test {
    ShopItemManager manager;
    game::ItemManager catalog;
    game::ShopItemOption options{};
    RestoreEffects effects;
    game::ItemBase item{};
    void define(std::uint16_t id, std::uint32_t timer=2, std::uint16_t type=10,
                std::uint16_t kind=LEGACY_SHOP_ITEM_INCANTATION, std::uint16_t stat=0) {
        item.wIconIdx=id; item.dwDBIdx=801; item.Position=390;
        game::ItemInfo info{}; info.ItemIdx=id; info.SellPrice=timer; info.ItemType=type;
        info.ItemKind=kind; info.CheRyuk=stat; info.MeleeAttackMin=stat;
        catalog.add(info);
    }
    ShopRestoreStatus run(std::uint32_t param=10, std::uint32_t remaining=100,
                          std::uint32_t now=100, ShopRestoreLocale locale=ShopRestoreLocale::China) {
        return restore_used_shop_item(manager,catalog,options,effects,&item,param,
            game::PackedTime{45},remaining,777,game::PackedTime{now},locale);
    }
};
TEST_F(RestoreUsedShopItem, MissingDefinitionRejectsWithoutStateOrEffects) {
    item.wIconIdx=60000;
    EXPECT_EQ(run(),ShopRestoreStatus::MissingItemInfo);
    EXPECT_EQ(manager.using_item_count(),0u); EXPECT_TRUE(effects.calls.empty());
}
TEST_F(RestoreUsedShopItem, SpecialCountersRestoreWithoutCatalog) {
    item.wIconIdx=55300;
    ASSERT_EQ(run(70000,80000),ShopRestoreStatus::Restored);
    EXPECT_EQ(options.SkillPoint,70000u); EXPECT_EQ(options.UseSkillPoint,80000u);
    EXPECT_EQ(effects.calls,(std::vector<std::string>{"calculate","dup-null"}));
    item.wIconIdx=55299;
    ASSERT_EQ(run(65537,65538),ShopRestoreStatus::Restored);
    EXPECT_EQ(options.StatePoint,1u); EXPECT_EQ(options.UseStatePoint,2u);
}
TEST_F(RestoreUsedShopItem, StatePointThirtyConvertsIdentityButPreservesOriginalRowParameters) {
    define(55321);
    ASSERT_EQ(run(99,7),ShopRestoreStatus::Restored);
    EXPECT_EQ(item.wIconIdx,55299); EXPECT_EQ(options.StatePoint,7); EXPECT_EQ(options.UseStatePoint,23);
    EXPECT_EQ(effects.updated_param,7u); EXPECT_EQ(effects.updated_remaining,23u);
    const auto* row=manager.find_using_item(55299); ASSERT_NE(row,nullptr);
    EXPECT_EQ(row->Data.ShopItem.Param,99u); EXPECT_EQ(row->Data.ShopItem.Remaintime,7u);
    EXPECT_EQ(row->Data.LastCheckTime,777u);
    EXPECT_EQ(effects.calls,(std::vector<std::string>{"update","calculate","dup"}));
}
TEST_F(RestoreUsedShopItem, ExpiredAvatarDiscardFailureStopsDeletionAndAdmission) {
    define(55134,1,11); effects.discard_ok=false;
    EXPECT_EQ(run(10,100,101),ShopRestoreStatus::DiscardFailed);
    EXPECT_EQ(effects.calls,(std::vector<std::string>{"discard"})); EXPECT_EQ(manager.using_item_count(),0u);
}
TEST_F(RestoreUsedShopItem, ExpiredAvatarDeletesAndLogsOnlyAfterDiscard) {
    define(55134,1,11);
    EXPECT_EQ(run(10,100,101),ShopRestoreStatus::Expired);
    EXPECT_EQ(effects.calls,(std::vector<std::string>{"discard","delete","log"}));
    EXPECT_EQ(manager.using_item_count(),0u);
}
TEST_F(RestoreUsedShopItem, ExpiredNonAvatarDoesNotDiscardInventory) {
    define(55134,1,10);
    EXPECT_EQ(run(10,0,1),ShopRestoreStatus::Expired);
    EXPECT_EQ(effects.calls,(std::vector<std::string>{"delete","log"}));
}
TEST_F(RestoreUsedShopItem, EqualEndTimeRestoresAndDuplicateHasNoEffects) {
    define(55134,1);
    ASSERT_EQ(run(),ShopRestoreStatus::Restored);
    effects.calls.clear(); EXPECT_EQ(run(),ShopRestoreStatus::AlreadyPresent); EXPECT_TRUE(effects.calls.empty());
}
TEST_F(RestoreUsedShopItem, IncantationUsesParamAndEndedCharmSkipsOptionAndDup) {
    define(55134,2,10,LEGACY_SHOP_ITEM_INCANTATION,1);
    ASSERT_EQ(run(42,99),ShopRestoreStatus::Restored); EXPECT_EQ(effects.calculated,42u);
    define(55135,2,10,LEGACY_SHOP_ITEM_CHARM,1); effects.calls.clear();
    ASSERT_EQ(run(42,0),ShopRestoreStatus::Restored); EXPECT_TRUE(effects.calls.empty());
    EXPECT_TRUE(manager.has_using_item(55135));
}
TEST_F(RestoreUsedShopItem, LocaleExpansionExceptionsMatchSource) {
    for (const auto locale : {ShopRestoreLocale::China,ShopRestoreLocale::Korea,
                            ShopRestoreLocale::Japan,ShopRestoreLocale::HongKong,ShopRestoreLocale::Thailand}) {
        for (const auto id : {55361,57544,57542,57543,57957,57960,57958,57959}) {
            manager.release(); catalog.clear(); effects.calls.clear(); define(static_cast<std::uint16_t>(id));
            ASSERT_EQ(run(10,100,100,locale),ShopRestoreStatus::Restored);
            const bool skip=locale==ShopRestoreLocale::HongKong ||
                ((locale==ShopRestoreLocale::Japan || locale==ShopRestoreLocale::Thailand) && id<57957);
            EXPECT_EQ(effects.calls.empty(),skip) << id << " locale " << static_cast<int>(locale);
            EXPECT_TRUE(manager.has_using_item(id));
        }
    }
}

struct RestorePlayerHook : CalcShopItemOptionPlayerHook {
    int inventory_expansions=0;
    void on_expand_inven_slot() noexcept override { ++inventory_expansions; }
    void on_expand_pyoguk_slot() noexcept override {}
    void on_expand_mugong_slot() noexcept override {}
    void on_expand_character_slot() noexcept override {}
};
struct CalculatedEffects : ShopRestoreCalculatedEffects {
    using ShopRestoreCalculatedEffects::ShopRestoreCalculatedEffects;
    void update_use_info(const game::ItemBase&,std::uint32_t,std::uint32_t) override {}
    bool discard_item(const game::ItemBase&) override { ADD_FAILURE() << "unexpected discard"; return false; }
    void delete_use_info(const game::ShopItemBase&) override { ADD_FAILURE() << "unexpected delete"; }
    void log_expired(const game::ShopItemBase&) override { ADD_FAILURE() << "unexpected expiry"; }
};
struct RestoreDupLookup : DupParamLookup {
    bool try_get_dup_param(std::uint32_t index, std::uint32_t& param) const noexcept override {
        if (index != 7 && index != 0) return false;
        param=2; return true;
    }
};
TEST_F(RestoreUsedShopItem, BoundCalculatorRestoresActualProtectAndMixStats) {
    CalcShopItemOptionEnv environment; RestorePlayerHook player;
    RestoreDupLookup duplicates;
    CalculatedEffects bound(manager,catalog,options,environment,player,duplicates);
    define(55134,2,10,LEGACY_SHOP_ITEM_INCANTATION,4);
    ASSERT_EQ(restore_used_shop_item(manager,catalog,options,bound,&item,7,
        game::PackedTime{0},100,777,game::PackedTime{100},ShopRestoreLocale::China),ShopRestoreStatus::Restored);
    EXPECT_EQ(options.ProtectCount,7); EXPECT_EQ(manager.protect_item_idx(),55134u);
    define(55322);
    ASSERT_EQ(restore_used_shop_item(manager,catalog,options,bound,&item,0,
        game::PackedTime{0},100,777,game::PackedTime{100},ShopRestoreLocale::China),ShopRestoreStatus::Restored);
    EXPECT_EQ(options.ItemMixSuccess,10);
}
TEST_F(RestoreUsedShopItem, BoundCalculatorRequeriesConvertedStatePointIdentity) {
    CalcShopItemOptionEnv environment; RestorePlayerHook player;
    RestoreDupLookup duplicates;
    CalculatedEffects bound(manager,catalog,options,environment,player,duplicates);
    define(55321,2,10,LEGACY_SHOP_ITEM_INCANTATION,9);
    ASSERT_EQ(restore_used_shop_item(manager,catalog,options,bound,&item,99,
        game::PackedTime{0},7,777,game::PackedTime{100},ShopRestoreLocale::China),ShopRestoreStatus::Restored);
    EXPECT_EQ(options.StatePoint,7); EXPECT_EQ(options.UseStatePoint,23);
    EXPECT_EQ(options.ProtectCount,0); EXPECT_EQ(manager.protect_item_idx(),0u);
}
TEST(ShopOptionCatalogInput, FireUsesFirstElementAndDwordWidth) {
    game::ItemInfo info{}; info.ItemIdx=57000; info.ItemKind=LEGACY_SHOP_ITEM_CHARM;
    info.AttrRegist.Element[0]=65536.0f; info.AttrRegist.Element[1]=0.0f;
    const auto input=shop_option_info(info); ASSERT_TRUE(input);
    EXPECT_EQ(input->AttrFire,65536u);
    ShopItemManager manager; game::ShopItemOption options{};
    CalcShopItemOptionEnv environment; RestorePlayerHook player;
    apply_calc_shop_item_option(manager,options,info.ItemIdx,true,0,*input,environment,player);
    EXPECT_EQ(options.dwStreetStallDecoration,57000u);
    info.AttrRegist.Element[0]=0.75f;
    ASSERT_TRUE(shop_option_info(info)); EXPECT_EQ(shop_option_info(info)->AttrFire,0u);
}
TEST_F(RestoreUsedShopItem, BoundDuplicateRestorationWritesManagerAndStreetStall) {
    CalcShopItemOptionEnv environment; RestorePlayerHook player; RestoreDupLookup duplicates;
    CalculatedEffects bound(manager,catalog,options,environment,player,duplicates);
    game::ItemInfo info{}; info.ItemIdx=57001; info.ItemKind=LEGACY_SHOP_ITEM_SUNDRIES;
    info.MugongType=7; info.LifeRecover=7; info.LifeRecoverRate=0.5f;
    catalog.add(info); item.wIconIdx=info.ItemIdx;
    ASSERT_EQ(restore_used_shop_item(manager,catalog,options,bound,&item,0,
        game::PackedTime{0},100,777,game::PackedTime{100},ShopRestoreLocale::China),ShopRestoreStatus::Restored);
    EXPECT_EQ(manager.dup_incantation(),2u); EXPECT_EQ(manager.dup_sundries(),2u);
    EXPECT_EQ(manager.dup_pet_equip(),2u); EXPECT_EQ(options.bStreetStall,1);
}
TEST(ShopOptionCatalogInput, InvalidFloatCannotEnterIntegerConversion) {
    game::ItemInfo info{};
    for (const auto value : {-1.0f,std::numeric_limits<float>::infinity(),
                            std::numeric_limits<float>::quiet_NaN(),4294967296.0f}) {
        info.AttrRegist.Element[0]=value; EXPECT_FALSE(shop_option_info(info));
    }
}
} // namespace
