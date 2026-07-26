#include <gtest/gtest.h>
#include <mxh/proto/legacy_wire.hpp>
#include <array>

using mxh::proto::LegacyWireMessage;
using mxh::proto::decode_legacy_wire;
using mxh::proto::encode_legacy_wire;

namespace {
LegacyWireMessage fixture(std::uint8_t category, std::uint8_t protocol,
                          std::uint32_t objectId, std::uint8_t seed) {
    LegacyWireMessage m;
    m.checksum = static_cast<std::uint8_t>(seed ^ 0x5a);
    m.code = static_cast<std::int8_t>(seed);
    m.category = category;
    m.protocol = protocol;
    m.object_id = objectId;
    m.payload = {seed, static_cast<std::uint8_t>(seed + 1),
                 static_cast<std::uint8_t>(seed + 2), 0};
    return m;
}
void verify(std::uint8_t category, std::uint8_t protocol, std::uint8_t seed) {
    const auto source = fixture(category, protocol, 0x10203040u + seed, seed);
    const auto wire = encode_legacy_wire(source);
    ASSERT_TRUE(wire.has_value());
    ASSERT_EQ(wire->size(), 14u);
    EXPECT_EQ((*wire)[0], 12u); EXPECT_EQ((*wire)[1], 0u);
    const auto decoded = decode_legacy_wire(*wire);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->checksum, source.checksum);
    EXPECT_EQ(decoded->code, source.code);
    EXPECT_EQ(decoded->category, category);
    EXPECT_EQ(decoded->protocol, protocol);
    EXPECT_EQ(decoded->object_id, source.object_id);
    EXPECT_EQ(decoded->payload, source.payload);
}
}

TEST(LegacyWire, UserConnLogin) { verify(7, 1, 1); }
TEST(LegacyWire, UserConnCharacterList) { verify(7, 4, 2); }
TEST(LegacyWire, UserConnCharacterSelect) { verify(7, 12, 3); }
TEST(LegacyWire, Move) { verify(8, 1, 4); }
TEST(LegacyWire, Mugong) { verify(9, 4, 5); }
TEST(LegacyWire, Item) { verify(5, 17, 6); }
TEST(LegacyWire, ItemUse) { verify(5, 1, 7); }
TEST(LegacyWire, Chat) { verify(6, 1, 8); }
TEST(LegacyWire, QuestAccept) { verify(39, 1, 9); }
TEST(LegacyWire, QuestComplete) { verify(39, 3, 10); }
TEST(LegacyWire, BattleAttack) { verify(31, 1, 11); }
TEST(LegacyWire, Party) { verify(14, 1, 12); }
TEST(LegacyWire, Guild) { verify(62, 1, 13); }
TEST(LegacyWire, Pet) { verify(72, 1, 14); }
TEST(LegacyWire, Titan) { verify(76, 1, 15); }

TEST(LegacyWire, RejectsTruncatedAndMismatchedLength) {
    EXPECT_FALSE(decode_legacy_wire(std::array<std::uint8_t, 9>{}).has_value());
    auto wire = encode_legacy_wire(fixture(7, 1, 1, 1));
    ASSERT_TRUE(wire.has_value());
    (*wire)[0] = 13;
    EXPECT_FALSE(decode_legacy_wire(*wire).has_value());
}

TEST(LegacyWire, RejectsBodyLargerThanSixteenBitLength) {
    auto source = fixture(7, 1, 1, 1);
    source.payload.resize(65528u);
    EXPECT_FALSE(encode_legacy_wire(source).has_value());
}
