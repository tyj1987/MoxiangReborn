#include "mxh/server/present_revive.hpp"

#include <gtest/gtest.h>
#include <limits>

using mxh::server::PresentReviveContext;
using mxh::server::prepare_present_revive;

namespace {
PresentReviveContext ready_dead() {
    PresentReviveContext c;
    c.player_id = 100003;
    c.level = 1;
    c.map_num = 10;
    c.ordinary_map_verified = true;
    c.dead = true;
    c.ready = true;
    c.max_hp = 125;
    c.max_shield = 245;
    c.pos_x = 25000;
    c.pos_z = 25001;
    return c;
}
}

TEST(PresentRevive, OriginalThirtyPercentTruncationAndAbsoluteMp) {
    const auto candidate = prepare_present_revive(ready_dead());
    ASSERT_TRUE(candidate);
    EXPECT_EQ(candidate->hp, 37u);
    EXPECT_EQ(candidate->shield, 73u);
    EXPECT_EQ(candidate->mp, 0u);
    EXPECT_EQ(candidate->pos_x, 25000u);
    EXPECT_EQ(candidate->pos_z, 25001u);
    EXPECT_EQ(mxh::server::encode_naeryuk_absolute(candidate->mp),
              (std::array<std::uint8_t, 4>{0, 0, 0, 0}));
}

TEST(PresentRevive, RejectsUnsupportedOrUnknownCapability) {
    for (const auto level : {0u, 2u, 4u, 5u}) {
        auto c = ready_dead(); c.level = static_cast<std::uint16_t>(level);
        EXPECT_FALSE(prepare_present_revive(c));
    }
    auto c = ready_dead(); c.ordinary_map_verified = false;
    EXPECT_FALSE(prepare_present_revive(c));
    c = ready_dead(); c.looted = true;
    EXPECT_FALSE(prepare_present_revive(c));
    c = ready_dead(); c.exiting = true;
    EXPECT_FALSE(prepare_present_revive(c));
    c = ready_dead(); c.changing_map = true;
    EXPECT_FALSE(prepare_present_revive(c));
    c = ready_dead(); c.sos = true;
    EXPECT_FALSE(prepare_present_revive(c));
    c = ready_dead(); c.dead = false;
    EXPECT_FALSE(prepare_present_revive(c));
    c = ready_dead(); c.ready = false;
    EXPECT_FALSE(prepare_present_revive(c));
    c = ready_dead(); c.max_hp = 1; // DWORD(1 * 0.3) remains zero.
    EXPECT_FALSE(prepare_present_revive(c));
    c = ready_dead(); c.pos_x = -1;
    EXPECT_FALSE(prepare_present_revive(c));
    c = ready_dead(); c.pos_x = 65536;
    EXPECT_FALSE(prepare_present_revive(c));
    c = ready_dead(); c.pos_z = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(prepare_present_revive(c));
}

TEST(PresentRevive, RequestAndMovePayloadUseOriginalSizesAndEndian) {
    EXPECT_TRUE(mxh::server::is_present_spot_syn(0, {}));
    const std::array<std::uint8_t, 1> extra{0};
    EXPECT_FALSE(mxh::server::is_present_spot_syn(0, extra));
    EXPECT_FALSE(mxh::server::is_present_spot_syn(3, {}));
    const auto candidate = prepare_present_revive(ready_dead());
    ASSERT_TRUE(candidate);
    EXPECT_EQ(mxh::server::encode_character_revive_move(*candidate),
              (std::array<std::uint8_t, 8>{0xa3, 0x86, 0x01, 0x00,
                                           0xa8, 0x61, 0xa9, 0x61}));
    EXPECT_EQ(mxh::server::encode_naeryuk_absolute(0x12345678u),
              (std::array<std::uint8_t, 4>{0x78, 0x56, 0x34, 0x12}));
}
