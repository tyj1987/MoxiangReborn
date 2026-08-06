// get_item_kind_type_test.cpp - 1:1 data-plane tests for
// legacy CItemManager::GetItemKindType from [Server]Map/ItemManager.cpp.

#include <mxh/server/get_item_kind_type.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace mxh::server;
using namespace mxh::game;

static void make_info(ItemInfo& info, std::uint16_t kind, std::uint16_t type) {
    info = ItemInfo{};
    info.ItemKind = kind;
    info.ItemType = type;
}

TEST(GetItemKindType, WritesBothFieldsOnHit) {
    ItemInfo info;
    make_info(info, 259, 11);
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 259u);
    EXPECT_EQ(type, 11u);
}

TEST(GetItemKindType, WritesZerosOnNull) {
    std::uint16_t kind = 99, type = 88;
    get_item_kind_type(nullptr, kind, type);
    EXPECT_EQ(kind, 0u);
    EXPECT_EQ(type, 0u);
}

TEST(GetItemKindType, KindAndTypeCanBeIndependent) {
    ItemInfo info;
    make_info(info, 512, 0);  // youngyak, no type
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 512u);
    EXPECT_EQ(type, 0u);
    make_info(info, 0, 10);  // no kind, has type
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 0u);
    EXPECT_EQ(type, 10u);
}
