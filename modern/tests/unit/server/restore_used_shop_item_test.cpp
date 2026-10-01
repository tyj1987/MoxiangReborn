#include <gtest/gtest.h>
#include <mxh/server/restore_used_shop_item.hpp>
#include <mxh/server/shop_dup_catalog.hpp>
#include <mxh/server/shop_event_rates.hpp>
#include <mxh/server/restore_shop_persistence.hpp>
#include <mxh/db/sqlite_adapter.hpp>
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
struct PersistenceEffects : ShopRestorePersistenceEffects {
    using ShopRestorePersistenceEffects::ShopRestorePersistenceEffects;
    bool discard_ok=true;
    int logs=0;
    bool discard_item(const game::ItemBase&) override { return discard_ok; }
    void log_expired(const game::ShopItemBase&) override { ++logs; }
};

TEST_F(RestoreUsedShopItem, ConvertedStatePersistsDistinctParametersAndHonorsRollback) {
    db::SqliteAdapter adapter; db::IDbAdapter& database=adapter;
    db::ConnectionConfig config; config.path=":memory:";
    ASSERT_TRUE(database.connect(config).ok());
    ASSERT_TRUE(database.execute("CREATE TABLE character_info(chrid INT PRIMARY KEY,userid TEXT,character_data BLOB)").ok());
    ASSERT_TRUE(database.execute("INSERT INTO character_info VALUES(42,'7',NULL)").ok());
    auto initial=db::load_modern_shop_state(database,42,7); ASSERT_TRUE(initial);
    db::LegacyShopAppearanceRows rows; rows.used_items.push_back({55321,390,801,99,45,7});
    ASSERT_TRUE(db::save_modern_shop_state(database,42,7,*initial,rows));
    auto saved=db::load_modern_shop_state(database,42,7); ASSERT_TRUE(saved);
    CalcShopItemOptionEnv environment; RestorePlayerHook player; RestoreDupLookup duplicates;
    PersistenceEffects persistent_effects(manager,catalog,options,environment,player,duplicates,*saved);
    define(55321);
    ASSERT_EQ(restore_used_shop_item(manager,catalog,options,persistent_effects,&item,99,
        game::PackedTime{45},7,777,game::PackedTime{100},ShopRestoreLocale::China),ShopRestoreStatus::Restored);
    ASSERT_EQ(persistent_effects.pending_rows().used_items.size(),1u);
    EXPECT_EQ(persistent_effects.pending_rows().used_items[0].item_id,55299);
    EXPECT_EQ(persistent_effects.pending_rows().used_items[0].parameter,7u);
    EXPECT_EQ(persistent_effects.pending_rows().used_items[0].remaining_time,23u);
    EXPECT_EQ(manager.find_using_item(55299)->Data.ShopItem.Param,99u);
    ASSERT_TRUE(database.begin_transaction().ok());
    ASSERT_TRUE(persistent_effects.save_shop_record(database,42,7));
    ASSERT_TRUE(database.rollback().ok());
    EXPECT_EQ(db::load_modern_shop_state(database,42,7)->rows.used_items[0].item_id,55321);
    ASSERT_TRUE(persistent_effects.save_shop_record(database,42,7));
    EXPECT_EQ(db::load_modern_shop_state(database,42,7)->rows.used_items[0].item_id,55299);
}

TEST_F(RestoreUsedShopItem, ExpiryDraftRetainsFailedDiscardAndRemovesSuccessfulDiscard) {
    db::ModernShopState snapshot;
    snapshot.rows.used_items.push_back({55134,390,801,10,45,100});
    CalcShopItemOptionEnv environment; RestorePlayerHook player; RestoreDupLookup duplicates;
    PersistenceEffects persistent_effects(manager,catalog,options,environment,player,duplicates,snapshot);
    define(55134,1,11); persistent_effects.discard_ok=false;
    EXPECT_EQ(restore_used_shop_item(manager,catalog,options,persistent_effects,&item,10,
        game::PackedTime{45},100,777,game::PackedTime{101},ShopRestoreLocale::China),ShopRestoreStatus::DiscardFailed);
    EXPECT_EQ(persistent_effects.pending_rows().used_items.size(),1u); EXPECT_EQ(persistent_effects.logs,0);
    persistent_effects.discard_ok=true;
    EXPECT_EQ(restore_used_shop_item(manager,catalog,options,persistent_effects,&item,10,
        game::PackedTime{45},100,777,game::PackedTime{101},ShopRestoreLocale::China),ShopRestoreStatus::Expired);
    EXPECT_TRUE(persistent_effects.pending_rows().used_items.empty()); EXPECT_EQ(persistent_effects.logs,1);
}
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
TEST(ShopEventRates, RealPlaydhDecodesEverySourceRateAndSnapshotsCurrentValues) {
    const auto modern=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
    std::string error;
    const auto environment=load_playdh_shop_rates(modern/"data/PlayDH/Resource/Server/DropRate.bin",ShopLocale::China,error);
    ASSERT_TRUE(environment) << error;
    const ShopRateEnvironment::Rates expected{1,6000,10,1,1,1,1,6,1.2f,2000,50,150};
    EXPECT_EQ(environment->baseline(),expected); EXPECT_EQ(environment->current(),expected);
    EXPECT_EQ(environment->locale(),ShopLocale::China);
    for(std::uint16_t id=0;id<12;++id) EXPECT_TRUE(environment->event_rate_active(id));
    EXPECT_FALSE(environment->event_rate_active(12)); EXPECT_FALSE(environment->event_rate_active(65535));
    auto changed=expected; changed[1]=12000;
    const auto during_event=environment->with_current(changed); ASSERT_TRUE(during_event);
    EXPECT_FALSE(during_event->event_rate_active(1)); EXPECT_TRUE(during_event->event_rate_active(9));
    EXPECT_TRUE(environment->event_rate_active(1)); // admission snapshot cannot be retroactively changed
    CalcShopItemOptionInfo item{}; item.ItemKind=LEGACY_SHOP_ITEM_CHARM;
    item.ItemIdx=55001; item.LimitSimMek=3; item.MeleeAttackMin=1;
    game::ShopItemOption normal{}, active{}; CalcShopItemOptionSideEffects effects;
    ASSERT_EQ(calc_shop_item_option(normal,item.ItemIdx,true,0,item,*environment,0,effects),CalcShopItemOptionStatus::Ok);
    ASSERT_EQ(calc_shop_item_option(active,item.ItemIdx,true,0,item,*during_event,0,effects),CalcShopItemOptionStatus::Ok);
    EXPECT_EQ(normal.PlustimeExp,3); EXPECT_EQ(active.PlustimeExp,0);
    changed[1]=std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(environment->with_current(changed));
}

TEST(ShopEventRates, CorruptContainerAndUnrecognizedPayloadCannotPublishRates) {
    const auto modern=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
    std::ifstream file(modern/"data/PlayDH/Resource/Server/DropRate.bin",std::ios::binary);
    ASSERT_TRUE(file.good());
    std::vector<std::uint8_t> raw((std::istreambuf_iterator<char>(file)),{});
    ASSERT_EQ(raw.size(),212u); std::string error;
    auto bad=raw; bad[25]^=1; // '#EXP' label, not a rate-value policy change
    EXPECT_FALSE(parse_playdh_shop_rates(bad,ShopLocale::China,error));
    bad=raw; bad.pop_back(); EXPECT_FALSE(parse_playdh_shop_rates(bad,ShopLocale::China,error));
    bad=raw; bad[12]=0; EXPECT_FALSE(parse_playdh_shop_rates(bad,ShopLocale::China,error));
    EXPECT_FALSE(parse_shop_rate_text("#EXP 1",ShopLocale::China,error));
    EXPECT_FALSE(parse_shop_rate_text("#EXP nan",ShopLocale::China,error));
    EXPECT_FALSE(parse_shop_rate_text("#EXP 1 #EXP 2",ShopLocale::China,error));
    EXPECT_FALSE(parse_shop_rate_text("#EXP 1garbage",ShopLocale::China,error));
}

TEST(ShopDupCatalog, RealPlaydhContainsOriginalTwentyEightEntries) {
    const auto modern=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
    std::string error;
    auto catalog=load_shop_dup_bin(modern/"data/PlayDH/Resource/ItemdupOption.bin",error);
    ASSERT_TRUE(catalog) << error; ASSERT_EQ(catalog->entries.size(),28u);
    std::uint32_t value=999;
    ASSERT_TRUE(catalog->try_get_dup_param(9,value)); EXPECT_EQ(value,2u);
    EXPECT_EQ(catalog->entries.at(9).category,"#SUNDRIES");
    ASSERT_TRUE(catalog->try_get_dup_param(28,value)); EXPECT_EQ(value,32768u);
    EXPECT_FALSE(catalog->try_get_dup_param(999,value));
    DupCounters counters{}; SundrySideEffects effects{};
    add_dup_param(counters,DupParamIndices{28,16,4,9,15},*catalog,effects);
    EXPECT_EQ(counters.charm,32768u); EXPECT_EQ(counters.herb,2u);
    EXPECT_EQ(counters.incantation,4u); EXPECT_EQ(counters.sundries,2u); EXPECT_EQ(counters.pet_equip,2u);
    EXPECT_TRUE(effects.set_b_street_stall);
}
TEST(ShopDupCatalog, CorruptAndTruncatedInputCannotPublishPartialCatalog) {
    const auto modern=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
    std::ifstream stream(modern/"data/PlayDH/Resource/ItemdupOption.bin",std::ios::binary);
    ASSERT_TRUE(stream);
    std::vector<std::uint8_t> raw((std::istreambuf_iterator<char>(stream)),{});
    ASSERT_GT(raw.size(),14u); std::string error;
    raw[13]^=1;
    EXPECT_FALSE(parse_shop_dup_bin(raw,error)); EXPECT_EQ(error,"ItemdupOption checksum mismatch");
    raw[13]^=1; raw.pop_back();
    EXPECT_FALSE(parse_shop_dup_bin(raw,error)); EXPECT_EQ(error,"ItemdupOption payload size mismatch");
}
} // namespace
