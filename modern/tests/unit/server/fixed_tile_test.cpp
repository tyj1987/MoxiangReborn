// fixed_tile_test.cpp - Phase D6 FixedTile 1:1 port tests.

#include "mxh/server/fixed_tile.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::FixedTile;
using mxh::server::fixed_tile_init;
using mxh::server::fixed_tile_is_collision;
using mxh::server::TFA_COLLISON;
using mxh::server::TFA_FLYCOLLISON;
using mxh::server::TFA_PEACEZONE;
using mxh::server::TFA_JSAZONE;

TEST(FixedTile, DefaultZeroIsNotCollision) {
    FixedTile t{};
    EXPECT_FALSE(fixed_tile_is_collision(t));
}

TEST(FixedTile, InitSetsAttr) {
    FixedTile t{};
    fixed_tile_init(t, 0u);
    EXPECT_FALSE(fixed_tile_is_collision(t));
    fixed_tile_init(t, TFA_COLLISON);
    EXPECT_TRUE(fixed_tile_is_collision(t));
}

TEST(FixedTile, MultipleFlagsRetainCollision) {
    FixedTile t{};
    fixed_tile_init(t, TFA_COLLISON | TFA_PEACEZONE | TFA_FLYCOLLISON);
    EXPECT_TRUE(fixed_tile_is_collision(t));
}

TEST(FixedTile, JsaZoneDoesNotImplyCollision) {
    FixedTile t{};
    fixed_tile_init(t, TFA_JSAZONE);
    EXPECT_FALSE(fixed_tile_is_collision(t));
}

TEST(FixedTile, ConstantsMatchLegacyBitLayout) {
    EXPECT_EQ(TFA_COLLISON, 0x01);
    EXPECT_EQ(TFA_FLYCOLLISON, 0x10);
    EXPECT_EQ(TFA_PEACEZONE, 0x08);
    EXPECT_EQ(TFA_JSAZONE, 0x40);
}

}  // namespace
