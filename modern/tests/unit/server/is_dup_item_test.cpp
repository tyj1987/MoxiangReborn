// is_dup_item_test.cpp - 1:1 data-plane tests for legacy
// CItemManager::IsDupItem(WORD wItemIdx) from
// [Server]Map/ItemManager.cpp. Locks the item-kind switch + the
// Sundries / Incantation exception branches across the 13 always-
// dup-able kinds, the 30-entry incantation non-dup list, and the
// skin no-dup block.

#include <mxh/server/is_dup_item.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace mxh::server;
using namespace mxh::game;

// Populate a caller-owned ItemInfo with the fields the data plane
// reads (ItemKind, SimMek, CheRyuk, LimitLevel, SellPrice). Tests
// declare an ItemInfo at function scope, then pass &info to
// is_dup_item so the pointer is to an lvalue.
static void make_info(ItemInfo& info, std::uint16_t kind,
                      std::uint16_t sim_mek = 0,
                      std::uint16_t che_ryuk = 0,
                      std::uint16_t limit_level = 0,
                      std::uint32_t sell_price = 0) {
    info = ItemInfo{};
    info.ItemKind = kind;
    info.SimMek = sim_mek;
    info.CheRyuk = che_ryuk;
    info.LimitLevel = limit_level;
    info.SellPrice = sell_price;
}

// ----- always-dup-able kinds -----

TEST(IsDupItem, YoungyakAllFourKindsAreDupAble) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_YOUNGYAK_ITEM); EXPECT_TRUE(is_dup_item(1, &info));
    make_info(info, LEGACY_ITEM_KIND_YOUNGYAK_ITEM_PET); EXPECT_TRUE(is_dup_item(2, &info));
    make_info(info, LEGACY_ITEM_KIND_YOUNGYAK_ITEM_UPGRADE_PET); EXPECT_TRUE(is_dup_item(3, &info));
    make_info(info, LEGACY_ITEM_KIND_YOUNGYAK_ITEM_TITAN); EXPECT_TRUE(is_dup_item(4, &info));
}

TEST(IsDupItem, ExtraAllSevenKindsAreDupAble) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_EXTRA_JEWEL); EXPECT_TRUE(is_dup_item(1, &info));
    make_info(info, LEGACY_ITEM_KIND_EXTRA_MATERIAL); EXPECT_TRUE(is_dup_item(2, &info));
    make_info(info, LEGACY_ITEM_KIND_EXTRA_METAL); EXPECT_TRUE(is_dup_item(3, &info));
    make_info(info, LEGACY_ITEM_KIND_EXTRA_BOOK); EXPECT_TRUE(is_dup_item(4, &info));
    make_info(info, LEGACY_ITEM_KIND_EXTRA_HERB); EXPECT_TRUE(is_dup_item(5, &info));
    make_info(info, LEGACY_ITEM_KIND_EXTRA_ETC); EXPECT_TRUE(is_dup_item(6, &info));
    make_info(info, LEGACY_ITEM_KIND_EXTRA_USABLE); EXPECT_TRUE(is_dup_item(7, &info));
}

TEST(IsDupItem, ShopCharmAndHerbAreDupAble) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_CHARM); EXPECT_TRUE(is_dup_item(1, &info));
    make_info(info, LEGACY_ITEM_KIND_SHOP_HERB); EXPECT_TRUE(is_dup_item(2, &info));
}

// ----- Sundries exception branches -----

TEST(IsDupItem, SundriesZeroFieldsIsDupAble) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_SUNDRIES);
    EXPECT_TRUE(is_dup_item(55640, &info));
}

TEST(IsDupItem, SundriesSimMekBlocks) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_SUNDRIES, /*sim_mek=*/1);
    EXPECT_FALSE(is_dup_item(55640, &info));
}

TEST(IsDupItem, SundriesCheRyukBlocks) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_SUNDRIES, /*sim_mek=*/0, /*che_ryuk=*/1);
    EXPECT_FALSE(is_dup_item(55640, &info));
}

TEST(IsDupItem, SundriesShoutIdxBlocks) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_SUNDRIES);
    EXPECT_FALSE(is_dup_item(LEGACY_SUNDRIES_SHOUT, &info));
}

TEST(IsDupItem, SundriesShoutOnceIsDupAble) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_SUNDRIES);
    EXPECT_TRUE(is_dup_item(55632, &info));
}

// ----- Incantation non-dup list -----

TEST(IsDupItem, IncantationNonDupListAll30Entries) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_INCANTATION);
    // TownMove15 (55303)
    EXPECT_FALSE(is_dup_item(55303, &info));
    // MemoryMove15 (55304)
    EXPECT_FALSE(is_dup_item(55304, &info));
    // TownMove7 (57508)
    EXPECT_FALSE(is_dup_item(57508, &info));
    // TownMove7NoTrade (57509)
    EXPECT_FALSE(is_dup_item(57509, &info));
    // MemoryMove7 (57510)
    EXPECT_FALSE(is_dup_item(57510, &info));
    // MemoryMove7NoTrade (57511)
    EXPECT_FALSE(is_dup_item(57511, &info));
    // Item55357 (55357)
    EXPECT_FALSE(is_dup_item(55357, &info));
    // Item55362 (55362)
    EXPECT_FALSE(is_dup_item(55362, &info));
    // MemoryMoveExtend (55365)
    EXPECT_FALSE(is_dup_item(55365, &info));
    // MemoryMoveExtend7 (55390)
    EXPECT_FALSE(is_dup_item(55390, &info));
    // MemoryMove2 (55371)
    EXPECT_FALSE(is_dup_item(55371, &info));
    // MemoryMoveExtend30 (58010)
    EXPECT_FALSE(is_dup_item(58010, &info));
    // ShowPyoguk (55351)
    EXPECT_FALSE(is_dup_item(55351, &info));
    // ChangeName (55352)
    EXPECT_FALSE(is_dup_item(55352, &info));
    // ChangeNameDntrade (57799)
    EXPECT_FALSE(is_dup_item(57799, &info));
    // Tracking (55353)
    EXPECT_FALSE(is_dup_item(55353, &info));
    // TrackingJin (55387)
    EXPECT_FALSE(is_dup_item(55387, &info));
    // ChangeJob (55360)
    EXPECT_FALSE(is_dup_item(55360, &info));
    // ShowPyoguk7 (57506)
    EXPECT_FALSE(is_dup_item(57506, &info));
    // ShowPyoguk7NoTrade (57507)
    EXPECT_FALSE(is_dup_item(57507, &info));
    // Tracking7 (57504)
    EXPECT_FALSE(is_dup_item(57504, &info));
    // Tracking7NoTrade (57505)
    EXPECT_FALSE(is_dup_item(57505, &info));
    // MugongExtend (55361)
    EXPECT_FALSE(is_dup_item(55361, &info));
    // PyogukExtend (57544)
    EXPECT_FALSE(is_dup_item(57544, &info));
    // InvenExtend (57542)
    EXPECT_FALSE(is_dup_item(57542, &info));
    // CharacterSlot (57543)
    EXPECT_FALSE(is_dup_item(57543, &info));
    // MugongExtend2 (57957)
    EXPECT_FALSE(is_dup_item(57957, &info));
    // PyogukExtend2 (57960)
    EXPECT_FALSE(is_dup_item(57960, &info));
    // InvenExtend2 (57958)
    EXPECT_FALSE(is_dup_item(57958, &info));
    // CharacterSlot2 (57959)
    EXPECT_FALSE(is_dup_item(57959, &info));
}

TEST(IsDupItem, IncantationLimitLevelAndSellPriceBlocks) {
    ItemInfo info;
    // Neither set: dup-able
    make_info(info, LEGACY_ITEM_KIND_SHOP_INCANTATION);
    EXPECT_TRUE(is_dup_item(55301, &info));
    // LimitLevel only: dup-able (legacy: BOTH must be non-zero)
    make_info(info, LEGACY_ITEM_KIND_SHOP_INCANTATION, /*sim_mek=*/0, /*che_ryuk=*/0, /*limit_level=*/10);
    EXPECT_TRUE(is_dup_item(55301, &info));
    // SellPrice only: dup-able
    make_info(info, LEGACY_ITEM_KIND_SHOP_INCANTATION, /*sim_mek=*/0, /*che_ryuk=*/0, /*limit_level=*/0, /*sell_price=*/1000);
    EXPECT_TRUE(is_dup_item(55301, &info));
    // Both: NOT dup-able
    make_info(info, LEGACY_ITEM_KIND_SHOP_INCANTATION, /*sim_mek=*/0, /*che_ryuk=*/0, /*limit_level=*/10, /*sell_price=*/1000);
    EXPECT_FALSE(is_dup_item(55301, &info));
}

TEST(IsDupItem, SkinNeverDupAble) {
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_NOMALCLOTHES_SKIN); EXPECT_FALSE(is_dup_item(1, &info));
    make_info(info, LEGACY_ITEM_KIND_SHOP_COSTUME_SKIN); EXPECT_FALSE(is_dup_item(2, &info));
    // Skin ignores LimitLevel/SellPrice etc.
    make_info(info, LEGACY_ITEM_KIND_SHOP_NOMALCLOTHES_SKIN, /*sim_mek=*/1, /*che_ryuk=*/1);
    EXPECT_FALSE(is_dup_item(3, &info));
}

TEST(IsDupItem, NullInfoReturnsFalse) {
    // Legacy: Sundries/Incantation nullptr returns FALSE; data
    // plane generalises that to ALL kinds (NULL = lookup miss = no dup).
    EXPECT_FALSE(is_dup_item(1, nullptr));
    EXPECT_FALSE(is_dup_item(55640, nullptr));
}

TEST(IsDupItem, UnknownKindReturnsFalse) {
    // Legacy: default case in switch returns FALSE (e.g. ItemKind=999).
    ItemInfo info;
    make_info(info, /*kind=*/999);
    EXPECT_FALSE(is_dup_item(1, &info));
}

TEST(IsDupItem, SundriesSimMekCheckedFirstIgnoresCheRyuk) {
    // SimMek != 0 takes precedence over CheRyuk != 0; both block.
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_SUNDRIES, /*sim_mek=*/5, /*che_ryuk=*/5);
    EXPECT_FALSE(is_dup_item(55640, &info));
}

TEST(IsDupItem, SundriesCheRyukCheckedSecondIgnoresShout) {
    // CheRyuk != 0 takes precedence over Shout idx; both block.
    ItemInfo info;
    make_info(info, LEGACY_ITEM_KIND_SHOP_SUNDRIES, /*sim_mek=*/0, /*che_ryuk=*/1);
    EXPECT_FALSE(is_dup_item(LEGACY_SUNDRIES_SHOUT, &info));
}

TEST(IsDupItem, SundriesShoutConstantMatchesLegacy) {
    EXPECT_EQ(LEGACY_SUNDRIES_SHOUT, 55631u);
}
