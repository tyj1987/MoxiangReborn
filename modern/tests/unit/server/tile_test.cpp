// tile_test.cpp - Phase D6 Tile 1:1 port tests.

#include "mxh/server/tile.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::Tile;
using mxh::server::tile_init;
using mxh::server::tile_increase_preoccupied;
using mxh::server::tile_decrease_preoccupied;
using mxh::server::tile_get_preoccupied;
using mxh::server::tile_inc_mu_collision;
using mxh::server::tile_dec_mu_collision;
using mxh::server::tile_get_mu_collision;
using mxh::server::tile_is_collision;

TEST(Tile, DefaultIsZero) {
    Tile t{};
    EXPECT_EQ(tile_get_preoccupied(t), 0);
    EXPECT_EQ(tile_get_mu_collision(t), 0);
    EXPECT_FALSE(tile_is_collision(t));
}

TEST(Tile, IncreasePreoccupiedGrowsUntil255) {
    Tile t{};
    for (int i = 0; i < 256; ++i) tile_increase_preoccupied(t);
    EXPECT_EQ(tile_get_preoccupied(t), 255);
    for (int i = 0; i < 10; ++i) tile_increase_preoccupied(t);
    EXPECT_EQ(tile_get_preoccupied(t), 255);  // saturated
}

TEST(Tile, DecreasePreoccupiedShrinks) {
    Tile t{};
    for (int i = 0; i < 5; ++i) tile_increase_preoccupied(t);
    tile_decrease_preoccupied(t);
    EXPECT_EQ(tile_get_preoccupied(t), 4);
}

TEST(Tile, DecreasePreoccupiedSaturatesAtZero) {
    Tile t{};
    tile_decrease_preoccupied(t);
    EXPECT_EQ(tile_get_preoccupied(t), 0);
}

TEST(Tile, MuCollisionCounterMatchesPreoccupied) {
    Tile t{};
    tile_inc_mu_collision(t);
    tile_inc_mu_collision(t);
    EXPECT_EQ(tile_get_mu_collision(t), 2);
    tile_dec_mu_collision(t);
    EXPECT_EQ(tile_get_mu_collision(t), 1);
}

TEST(Tile, IsCollisionTrueWhenPreoccupied) {
    Tile t{};
    tile_increase_preoccupied(t);
    EXPECT_TRUE(tile_is_collision(t));
}

TEST(Tile, IsCollisionTrueWhenMuCollision) {
    Tile t{};
    tile_inc_mu_collision(t);
    EXPECT_TRUE(tile_is_collision(t));
}

TEST(Tile, IsCollisionFalseWhenBothZero) {
    Tile t{};
    EXPECT_FALSE(tile_is_collision(t));
}

}  // namespace
