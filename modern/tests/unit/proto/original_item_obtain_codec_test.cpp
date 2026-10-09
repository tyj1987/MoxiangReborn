#include "mxh/proto/original_item_obtain_codec.hpp"
#include "mxh/proto/protocol.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

namespace {
using mxh::proto::original_item::ItemObtainArray;
using mxh::proto::original_item::decode_item_obtain_payload;
using mxh::proto::original_item::encode_item_obtain_payload;
using mxh::game::ItemBase;

TEST(OriginalItemProfile, ModernProfileRemainsSeparate) {
    using Original = mxh::proto::original_item::ItemProtocol;
    EXPECT_EQ(static_cast<std::uint16_t>(Original::MP_ITEM_BUY_SYN), 19u);
    EXPECT_EQ(static_cast<std::uint16_t>(Original::MP_ITEM_MONSTER_OBTAIN_NOTIFY), 32u);
    EXPECT_EQ(static_cast<std::uint16_t>(Original::MP_ITEM_PROTOCOL_MAX), 256u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupSyn), 19u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MonsterObtainNotify), 35u);
}

TEST(OriginalItemObtainCodec, EmptyArrayIsExactlyThreeBytes) {
    const auto encoded = encode_item_obtain_payload(ItemObtainArray{});
    ASSERT_TRUE(encoded);
    EXPECT_EQ(*encoded, (std::vector<std::uint8_t>{0u, 0u, 0u}));
    const auto decoded = decode_item_obtain_payload(*encoded);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->obtain_count, 0u);
    EXPECT_TRUE(decoded->items.empty());
}

TEST(OriginalItemObtainCodec, OneItemHasSourceFieldOrderAndIndependentQuantity) {
    ItemObtainArray value;
    value.obtain_count = 7u;
    value.items.push_back(ItemBase{0x78563412u, 0xABCDu, 7u,
                                   0x01020304u, 0x0A0B0C0Du,
                                   0xFFFEu, 0x10203040u});
    const auto encoded = encode_item_obtain_payload(value);
    ASSERT_TRUE(encoded);
    const std::vector<std::uint8_t> expected{
        1u, 7u, 0u,
        0x12u, 0x34u, 0x56u, 0x78u,
        0xCDu, 0xABu,
        7u, 0u,
        4u, 3u, 2u, 1u,
        0x0Du, 0x0Cu, 0x0Bu, 0x0Au,
        0xFEu, 0xFFu,
        0x40u, 0x30u, 0x20u, 0x10u,
    };
    EXPECT_EQ(*encoded, expected);
    const auto decoded = decode_item_obtain_payload(*encoded);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->obtain_count, 7u);
    ASSERT_EQ(decoded->items.size(), 1u);
    EXPECT_EQ(decoded->items[0].dwDBIdx, 0x78563412u);
    EXPECT_EQ(decoded->items[0].wIconIdx, 0xABCDu);
    EXPECT_EQ(decoded->items[0].Position, 7u);
    EXPECT_EQ(decoded->items[0].Durability, 0x01020304u);
    EXPECT_EQ(decoded->items[0].RareIdx, 0x0A0B0C0Du);
    EXPECT_EQ(decoded->items[0].QuickPosition, 0xFFFEu);
    EXPECT_EQ(decoded->items[0].ItemParam, 0x10203040u);
}

TEST(OriginalItemObtainCodec, MultipleChangedSlotsNeedNotEqualObtainQuantity) {
    ItemObtainArray value;
    value.obtain_count = 9u;
    value.items = {ItemBase{1001u, 11000u, 3u, 100u, 0u, 0xFFFFu, 1u},
                   ItemBase{1002u, 23000u, 80u, 55u, 0u, 0xFFFFu, 1u}};
    const auto encoded = encode_item_obtain_payload(value);
    ASSERT_TRUE(encoded);
    ASSERT_EQ(encoded->size(), 3u + 2u * 22u);
    EXPECT_EQ((*encoded)[0], 2u);
    EXPECT_EQ((*encoded)[1], 9u);
    EXPECT_EQ((*encoded)[2], 0u);
    const auto decoded = decode_item_obtain_payload(*encoded);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->obtain_count, 9u);
    ASSERT_EQ(decoded->items.size(), 2u);
    EXPECT_EQ(decoded->items[0].dwDBIdx, 1001u);
    EXPECT_EQ(decoded->items[1].wIconIdx, 23000u);
    EXPECT_EQ(decoded->items[1].Position, 80u);
}

TEST(OriginalItemObtainCodec, RejectsGroundTwentyBytesAndMalformedLengths) {
    EXPECT_FALSE(decode_item_obtain_payload(std::vector<std::uint8_t>(20u, 0u)));
    EXPECT_FALSE(decode_item_obtain_payload(std::vector<std::uint8_t>{0u, 0u}));
    auto one = *encode_item_obtain_payload(
        ItemObtainArray{1u, {ItemBase{1001u, 11000u, 3u, 100u, 0u, 0u, 1u}}});
    one.pop_back();
    EXPECT_FALSE(decode_item_obtain_payload(one));
    one.push_back(1u);
    one.push_back(0u);
    EXPECT_FALSE(decode_item_obtain_payload(one));
    one.pop_back();
    one[0] = 2u;
    EXPECT_FALSE(decode_item_obtain_payload(one));
}

TEST(OriginalItemObtainCodec, RejectsRecordAndQuantityRangeOverflow) {
    ItemObtainArray value;
    value.obtain_count = 65536u;
    EXPECT_FALSE(encode_item_obtain_payload(value));
    value.obtain_count = 1u;
    value.items.resize(81u);
    EXPECT_FALSE(encode_item_obtain_payload(value));
    std::vector<std::uint8_t> forged(3u + 81u * 22u, 0u);
    forged[0] = 81u;
    EXPECT_FALSE(decode_item_obtain_payload(forged));
    value.items.resize(80u);
    const auto maximum = encode_item_obtain_payload(value);
    ASSERT_TRUE(maximum);
    EXPECT_EQ(maximum->size(), 3u + 80u * 22u);
    EXPECT_TRUE(decode_item_obtain_payload(*maximum));
}
}  // namespace
