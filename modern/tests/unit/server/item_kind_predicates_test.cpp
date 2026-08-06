// item_kind_predicates_test.cpp - 1:1 data-plane tests for
// legacy CItemManager::IsPetSummonItem / IsTitanCallItem /
// IsTitanEquipItem from [Server]Map/ItemManager.cpp.

#include <mxh/server/item_kind_predicates.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace mxh::server;
using namespace mxh::game;

static void make_info(ItemInfo& info, std::uint16_t kind) {
    info = ItemInfo{};
    info.ItemKind = kind;
}

// ----- IsPetSummonItem -----

TEST(IsPetSummonItem, QuestPetIsSummon) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_QUEST_PET);
    EXPECT_TRUE(is_pet_summon_item(&info));
}

TEST(IsPetSummonItem, ShopPetIsSummon) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_PET);
    EXPECT_TRUE(is_pet_summon_item(&info));
}

TEST(IsPetSummonItem, OtherKindsAreNotSummon) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_TITAN_PAPER);
    EXPECT_FALSE(is_pet_summon_item(&info));
    make_info(info, 0);
    EXPECT_FALSE(is_pet_summon_item(&info));
    make_info(info, 9999);
    EXPECT_FALSE(is_pet_summon_item(&info));
}

TEST(IsPetSummonItem, NullReturnsFalse) {
    EXPECT_FALSE(is_pet_summon_item(nullptr));
}

// ----- IsTitanCallItem -----

TEST(IsTitanCallItem, TitanPaperIsCallItem) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_TITAN_PAPER);
    EXPECT_TRUE(is_titan_call_item(&info));
}

TEST(IsTitanCallItem, NonTitanPaperIsNotCallItem) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_TITAN_EQUIP_UMBRELLA);
    EXPECT_FALSE(is_titan_call_item(&info));
    make_info(info, LEGACY_ITEM_KIND_SHOP_PET);
    EXPECT_FALSE(is_titan_call_item(&info));
}

TEST(IsTitanCallItem, NullReturnsFalse) {
    EXPECT_FALSE(is_titan_call_item(nullptr));
}

// ----- IsTitanEquipItem -----

TEST(IsTitanEquipItem, TitanEquipBitIsSet) {
    ItemInfo info;
    // Legacy eTITAN_EQUIPITEM = 128 (the umbrella bit).
    make_info(info, LEGACY_ITEM_KIND_TITAN_EQUIP_UMBRELLA);
    EXPECT_TRUE(is_titan_equip_item(&info));
    // Any kind with bit 7 set (128..255).
    make_info(info, 129);  // eTITAN_EQUIPITEM_HELMET
    EXPECT_TRUE(is_titan_equip_item(&info));
    make_info(info, 200);
    EXPECT_TRUE(is_titan_equip_item(&info));
    make_info(info, 255);
    EXPECT_TRUE(is_titan_equip_item(&info));
}

TEST(IsTitanEquipItem, NonTitanKindIsNotEquip) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_PET);
    EXPECT_FALSE(is_titan_equip_item(&info));
    make_info(info, LEGACY_ITEM_KIND_TITAN_PAPER);  // 65, bit 7 unset
    EXPECT_FALSE(is_titan_equip_item(&info));
    make_info(info, 0);
    EXPECT_FALSE(is_titan_equip_item(&info));
    make_info(info, 127);  // bit 7 unset
    EXPECT_FALSE(is_titan_equip_item(&info));
}

TEST(IsTitanEquipItem, NullReturnsFalse) {
    EXPECT_FALSE(is_titan_equip_item(nullptr));
}
