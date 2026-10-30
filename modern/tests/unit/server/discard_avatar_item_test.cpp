// discard_avatar_item_test.cpp - 1:1 data-plane tests for the
// legacy CShopItemManager::DiscardAvatarItem data-plane half
// from [Server]Map/ShopItemManager.cpp. Locks the 4 no-op
// conditions + the clear-and-default-fill semantics across
// the five-slot [Weared_Hair, Weared_Gum) default-fill range.

#include <mxh/server/discard_avatar_item.hpp>
#include <mxh/server/avatar_equip_catalog.hpp>
#include <mxh/server/avatar_equip_environment.hpp>
#include <mxh/server/shop_playtime_step.hpp>
#include <mxh/server/shop_event_rates.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

using namespace mxh::server;
using namespace mxh::game;

TEST(ShopPlaytimeStep, CountdownPauseWrapAndExpirationKeepSourceOrder) {
    ShopRateEnvironment::Rates baseline; baseline.fill(1);
    ShopRateEnvironment normal(ShopLocale::China,baseline);
    ItemInfo info{}; info.SellPrice=2;
    ShopItemWithTime item; item.ShopItem.Param=10; item.ShopItem.Remaintime=90000; item.LastCheckTime=1000;
    auto step=plan_shop_playtime_step(item,&info,normal,31000,true);
    EXPECT_TRUE(step.handled); EXPECT_FALSE(step.expired); EXPECT_TRUE(step.persist_remaining);
    EXPECT_TRUE(step.notify_one_minute); EXPECT_EQ(step.next.ShopItem.Remaintime,60000u);
    EXPECT_EQ(step.next.LastCheckTime,31000u); EXPECT_EQ(item.ShopItem.Remaintime,90000u);
    step=plan_shop_playtime_step(step.next,&info,normal,91000,false);
    EXPECT_TRUE(step.expired); EXPECT_FALSE(step.persist_remaining); EXPECT_FALSE(step.notify_one_minute);
    EXPECT_EQ(step.next.ShopItem.Remaintime,0u);
    info.ItemKind=258; info.MeleeAttackMin=1;
    auto changed=baseline; changed[1]=2;
    const auto paused=normal.with_current(changed); ASSERT_TRUE(paused);
    step=plan_shop_playtime_step(item,&info,*paused,31000,true);
    EXPECT_EQ(step.next.ShopItem.Remaintime,90000u); EXPECT_EQ(step.next.LastCheckTime,31000u);
    EXPECT_FALSE(step.persist_remaining); EXPECT_FALSE(step.notify_one_minute);
    item.LastCheckTime=0xfffffff0u; item.ShopItem.Remaintime=100;
    step=plan_shop_playtime_step(item,&info,normal,16,false);
    EXPECT_EQ(step.next.ShopItem.Remaintime,68u); EXPECT_FALSE(step.expired);
    item.ShopItem.Remaintime=0;
    step=plan_shop_playtime_step(item,&info,normal,16,true);
    EXPECT_FALSE(step.expired); EXPECT_FALSE(step.persist_remaining);
    info.SellPrice=1;
    EXPECT_FALSE(plan_shop_playtime_step(item,&info,normal,16,true).handled);
}

TEST(AvatarEquipEnvironment, ActualManagerAndAbsoluteSlotControlAdmission) {
    AvatarEquipCatalog avatars;
    AvatarEquipCatalog::Entry dress; dress.equip.position=6; dress.equip.item.fill(1);
    avatars.entries.emplace(55001,dress);
    ItemManager items; ItemInfo info{}; info.ItemIdx=55001; info.SellPrice=1; items.add(info);
    ShopItemManager used;
    std::array<ItemBase,1> slots{};
    slots[0].dwDBIdx=9001; slots[0].wIconIdx=55001; slots[0].Position=390;
    AvatarSlots current{}; for(std::size_t i=12;i<23;++i) current[i]=1;
    AvatarEquipEnvironment before(avatars,items,used,{}, {},slots);
    EXPECT_EQ(put_on_avatar_item(before,&current,55001,390,false,0,false).status,AvatarEquipStatus::UsingItemMissing);
    UsingShopItemEntry entry; entry.ItemIdx=55001; entry.Data.ShopItem.ItemBase=slots[0];
    ASSERT_TRUE(used.add_using_item(entry));
    AvatarEquipEnvironment after(avatars,items,used,{}, {},slots);
    const auto restored=put_on_avatar_item(after,&current,55001,390,false,0,false);
    ASSERT_EQ(restored.status,AvatarEquipStatus::Ok); EXPECT_EQ(restored.avatar[6],55001);
    EXPECT_FALSE(restored.calc_stats);
    EXPECT_EQ(put_on_avatar_item(before,&current,55001,390,false,0,false).status,AvatarEquipStatus::UsingItemMissing);
    slots[0].dwDBIdx=9002;
    AvatarEquipEnvironment mismatch(avatars,items,used,{}, {},slots);
    EXPECT_EQ(put_on_avatar_item(mismatch,&current,55001,390,false,0,false).status,AvatarEquipStatus::ItemBaseMismatch);
}

TEST(AvatarEquipCatalog, RealSourceLastRecordWinsLikeLegacyHashTable) {
    std::string error;
    const auto path=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path()/"data/PlayDH/Resource/AvatarEquip.bin";
    const auto catalog=load_avatar_equip_bin(path,error);
    ASSERT_TRUE(catalog) << error;
    EXPECT_EQ(catalog->entries.size(),462u);
    EXPECT_EQ(catalog->duplicate_ids,(std::vector<std::uint16_t>{55503,55504,57680,58060,58061,58062,58063}));
    EXPECT_EQ(catalog->entries.at(57680).equip.position,6);
    EXPECT_EQ(catalog->entries.at(57680).equip.item[15],0);
    EXPECT_EQ(catalog->entries.at(40501).equip.position,17);
    std::ifstream file(path,std::ios::binary);
    std::vector<std::uint8_t> raw((std::istreambuf_iterator<char>(file)),{});
    raw[13]^=1; EXPECT_FALSE(parse_avatar_equip_bin(raw,error));
    raw.pop_back(); EXPECT_FALSE(parse_avatar_equip_bin(raw,error));
}

TEST(AvatarEquipCatalog, ValidRowsAndMalformedFields) {
    const auto packed=[](std::string text) {
        std::vector<std::uint8_t> raw(13,0); raw[4]=1;
        const auto size=static_cast<std::uint32_t>(text.size());
        for(int i=0;i<4;++i) raw[8+i]=static_cast<std::uint8_t>(size>>(8*i));
        std::uint8_t crc=1;
        for(std::size_t i=0;i<text.size();++i) {
            const auto b=static_cast<std::uint8_t>(static_cast<unsigned char>(text[i])+i+1);
            raw.push_back(b); crc=static_cast<std::uint8_t>(crc+b);
        }
        raw[12]=crc; raw.push_back(crc); return raw;
    };
    const std::string mask=" 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1\n";
    const std::string row="40501 2 17"+mask;
    std::string error;
    const auto catalog=parse_avatar_equip_bin(packed(row+row),error);
    ASSERT_TRUE(catalog) << error;
    ASSERT_EQ(catalog->entries.size(),1u);
    EXPECT_EQ(catalog->entries.at(40501).equip.item[17],0);
    EXPECT_EQ(catalog->entries.at(40501).equip.item[18],1);
    EXPECT_FALSE(parse_avatar_equip_bin(packed("40501 2 23"+mask),error));
    EXPECT_FALSE(parse_avatar_equip_bin(packed("65536 2 17"+mask),error));
    EXPECT_FALSE(parse_avatar_equip_bin(packed("40501 256 17"+mask),error));
    EXPECT_FALSE(parse_avatar_equip_bin(packed("40501 2 17 1"),error));
    const auto replaced=parse_avatar_equip_bin(packed(row+"40501 2 6"+mask),error);
    ASSERT_TRUE(replaced); EXPECT_EQ(replaced->entries.at(40501).equip.position,6);
}

static AvatarEquipRow make_equip(std::uint8_t pos,
                              std::initializer_list<std::uint16_t> mask) {
    AvatarEquipRow e;
    e.position = pos;
    std::size_t i = 0;
    for (std::uint16_t v : mask) {
        if (i >= EAvatarCount) break;
        e.item[i++] = v;
    }
    return e;
}

static std::array<std::uint16_t, EAvatarCount> zero_avatar() {
    return {};
}

// ----- no-op conditions -----

TEST(DiscardAvatarItem, NullEquipIsNoOp) {
    auto av = zero_avatar();
    av[5] = 100;
    auto out = discard_avatar_item(nullptr, 100, av);
    EXPECT_EQ(out, av);
}

TEST(DiscardAvatarItem, PositionOutOfRangeIsNoOp) {
    AvatarEquipRow e = make_equip(/*pos=*/99, {});
    auto av = zero_avatar();
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out, av);
}

TEST(DiscardAvatarItem, MismatchedItemIdxIsNoOp) {
    AvatarEquipRow e = make_equip(/*pos=*/5, {});
    auto av = zero_avatar();
    av[5] = 999;  // wrong item
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out, av);
}

TEST(DiscardAvatarItem, ZeroInPositionIsNoOp) {
    // Legacy: if (pAvatar[Position] != ItemIdx) return;
    // av[pos] == 0 != ItemIdx, so no-op.
    AvatarEquipRow e = make_equip(/*pos=*/5, {});
    auto av = zero_avatar();  // av[5] = 0
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out, av);
}

// ----- clear-and-default-fill happy path -----

TEST(DiscardAvatarItem, MatchingSlotIsCleared) {
    AvatarEquipRow e = make_equip(/*pos=*/5, {});
    auto av = zero_avatar();
    av[5] = 100;
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out[5], 0u);
}

TEST(DiscardAvatarItem, DefaultFillZerosBecomeOnes) {
    // Position 5 is a cosmetic slot, no default-fill there.
    AvatarEquipRow e = make_equip(/*pos=*/5, {0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0});
    auto av = zero_avatar();
    av[5] = 100;
    auto out = discard_avatar_item(&e, 100, av);
    // Source enum: five worn flags (12..16); weapon avatars start at 17.
    EXPECT_EQ(out[5], 0u);
    for (std::size_t n = 12; n < 17; ++n) {
        EXPECT_EQ(out[n], 1u);
    }
}

TEST(DiscardAvatarItem, NonZeroMaskedSlotsPreserve) {
    // equip.Item[12] = 5 (non-zero) -> avatar[12] should NOT be
    // overwritten to 1; legacy only fills when mask == 0.
    std::array<std::uint16_t, EAvatarCount> mask{};
    mask[12] = 5;
    AvatarEquipRow e;
    e.position = 5;
    e.item = mask;
    auto av = zero_avatar();
    av[5] = 100;
    av[12] = 999;  // pre-existing value
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out[5], 0u);
    EXPECT_EQ(out[12], 999u);  // preserved
    for (std::size_t n = 13; n < 17; ++n) {
        EXPECT_EQ(out[n], 1u);
    }
}

TEST(DiscardAvatarItem, AllMaskedSlotsPreserve) {
    // All five worn flags have non-zero mask -> nothing changes
    // beyond the clear of position.
    std::array<std::uint16_t, EAvatarCount> mask{};
    for (std::size_t n = 12; n < 17; ++n) mask[n] = 100;
    AvatarEquipRow e;
    e.position = 5;
    e.item = mask;
    auto av = zero_avatar();
    av[5] = 100;
    for (std::size_t n = 12; n < 17; ++n) av[n] = 200;
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out[5], 0u);
    for (std::size_t n = 12; n < 17; ++n) {
        EXPECT_EQ(out[n], 200u);
    }
}

TEST(DiscardAvatarItem, NonWearedSlotsNotTouched) {
    // Cosmetic slots (0..11) outside position are never touched.
    AvatarEquipRow e = make_equip(/*pos=*/5, {});
    auto av = zero_avatar();
    av[0] = 1000;
    av[1] = 1001;
    av[5] = 100;  // discarded
    av[11] = 1011;
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out[0], 1000u);
    EXPECT_EQ(out[1], 1001u);
    EXPECT_EQ(out[5], 0u);
    EXPECT_EQ(out[11], 1011u);
}

TEST(DiscardAvatarItem, WearedGumNotIncluded) {
    // Default-fill range is [12, 17); all six weapon slots remain unchanged.
    AvatarEquipRow e = make_equip(/*pos=*/5, {});
    auto av = zero_avatar();
    av[5] = 100;
    av[17] = 0; // hidden sword must remain hidden
    av[18] = 55001; // equipped fist avatar must retain its identity
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out[17], 0u);
    EXPECT_EQ(out[18], 55001u);
    for (std::size_t n = 19; n < 23; ++n) EXPECT_EQ(out[n], av[n]);
}

TEST(DiscardAvatarItem, PositionAtMaxIsOutOfRange) {
    // Source eAvatar_Max is 23, the first invalid slot.
    AvatarEquipRow e = make_equip(/*pos=*/23, {});
    auto av = zero_avatar();
    av[5] = 100;
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out, av);  // no-op
}
