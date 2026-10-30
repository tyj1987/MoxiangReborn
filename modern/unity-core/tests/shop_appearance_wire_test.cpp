#include "ShopAppearanceWire.hpp"
#include <gtest/gtest.h>
#include <array>
using mxh::unity::parse_shop_appearance_wire;
TEST(ShopAppearanceWire, DecodesOriginalCountsAndPreservesEntireBlock) {
    std::array<std::uint8_t,120> bytes{};
    for(std::size_t i=0;i<bytes.size();++i)bytes[i]=static_cast<std::uint8_t>(i);
    const auto parsed=parse_shop_appearance_wire(bytes);
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->raw,bytes);
    EXPECT_EQ(parsed->avatar.front(),0x0100);
    EXPECT_EQ(parsed->avatar.back(),0x2d2c);
    EXPECT_EQ(parsed->skin.front(),0x6b6a);
    EXPECT_EQ(parsed->skin.back(),0x7372);
    EXPECT_EQ(parsed->street_stall_decoration,0x77767574u);
}
TEST(ShopAppearanceWire, RejectsTruncationAndModernInternalLayout) {
    std::array<std::uint8_t,124> internal{};
    EXPECT_FALSE(parse_shop_appearance_wire(internal));
    EXPECT_FALSE(parse_shop_appearance_wire(std::span(internal).first(119)));
    EXPECT_FALSE(parse_shop_appearance_wire({}));
}
#include "mxh/proto/character_revive.hpp"
#include "mxh/proto/character_level.hpp"
#include "mxh/proto/protocol.hpp"
#include <limits>
TEST(CharacterReviveWire, MatchesOriginalMovePosBytesAndRejectsWrongIdentity) {
    static_assert(static_cast<unsigned>(mxh::proto::CharacterProtocol::NaeryukAck)==9);
    const auto payload=mxh::proto::encode_character_revive(0x12345678u,258.9f,65535.5f);
    ASSERT_TRUE(payload);
    const std::array<std::uint8_t,8> expected{0x78,0x56,0x34,0x12,0x02,0x01,0xff,0xff};
    EXPECT_EQ(*payload,expected);
    const auto decoded=mxh::proto::decode_character_revive(0x12345678u,*payload);
    ASSERT_TRUE(decoded); EXPECT_EQ(decoded->x,258u); EXPECT_EQ(decoded->z,65535u);
    EXPECT_FALSE(mxh::proto::decode_character_revive(1,*payload));
    EXPECT_FALSE(mxh::proto::decode_character_revive(0,*payload));
    for(std::size_t size=0;size<8;++size)
        EXPECT_FALSE(mxh::proto::decode_character_revive(0x12345678u,std::span(*payload).first(size)));
    std::array<std::uint8_t,9> too_long{};
    EXPECT_FALSE(mxh::proto::decode_character_revive(1,too_long));
    EXPECT_FALSE(mxh::proto::encode_character_revive(0,0,0));
    for(float bad:{-1.f,65536.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        EXPECT_FALSE(mxh::proto::encode_character_revive(1,bad,0));
        EXPECT_FALSE(mxh::proto::encode_character_revive(1,0,bad));
    }
}

TEST(CharacterLevelWire, PreservesPackedWidthsAndExcessExperience) {
    const auto bytes=mxh::proto::encode_character_level({120,0x0102030405060708LL,100});
    ASSERT_TRUE(bytes);
    const std::array<std::uint8_t,18> expected{120,0,8,7,6,5,4,3,2,1,100,0,0,0,0,0,0,0};
    EXPECT_EQ(*bytes,expected);
    const auto decoded=mxh::proto::decode_character_level(expected);
    ASSERT_TRUE(decoded); EXPECT_EQ(decoded->current_experience,0x0102030405060708LL);
    EXPECT_EQ(decoded->level,120); EXPECT_EQ(decoded->maximum_experience,100);
    for(std::size_t size=0;size<18;++size)
        EXPECT_FALSE(mxh::proto::decode_character_level(std::span(expected).first(size)));
    for(auto value:{mxh::proto::CharacterLevel{0,0,1},{121,0,1},{1,-1,1},{1,0,0},{1,0,-1}})
        EXPECT_FALSE(mxh::proto::encode_character_level(value));
    auto invalid=expected; invalid[9]=128;
    EXPECT_FALSE(mxh::proto::decode_character_level(invalid));
}
