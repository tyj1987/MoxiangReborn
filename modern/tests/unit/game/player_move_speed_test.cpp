#include "mxh/game/player_move_speed.hpp"
#include <gtest/gtest.h>
#include <limits>
using namespace mxh::game;
TEST(PlayerMoveSpeed, PlainModesIgnoreInactiveBonuses) {
    PlayerMoveSpeedInput p; p.avatar_lightness=999;
    EXPECT_EQ(player_move_speed(p),400); p.run=false; EXPECT_EQ(player_move_speed(p),200);
}
TEST(PlayerMoveSpeed, LightnessAddsAllThreeBonusesAndMissingSkillReturnsZero) {
    PlayerMoveSpeedInput p; p.kyunggong_id=2602;
    EXPECT_FALSE(player_move_speed(p));
    p.special_resources_resolved=true;
    p.ability_lightness=10; p.avatar_lightness=20; p.shop_lightness=30;
    EXPECT_EQ(player_move_speed(p),0);
    p.lightness_base=600.0f; EXPECT_EQ(player_move_speed(p),660);
}
TEST(PlayerMoveSpeed, TitanRunWalkAndGradeRulesPreserveOriginalFallbacks) {
    PlayerMoveSpeedInput p; p.in_titan=true; p.special_resources_resolved=true;
    EXPECT_FALSE(player_move_speed(p));
    p.titan_run_speed=550.0f; EXPECT_EQ(player_move_speed(p),550);
    p.run=false; EXPECT_EQ(player_move_speed(p),300);
    p.special_resources_resolved=false; EXPECT_EQ(player_move_speed(p),300);
    p.special_resources_resolved=true;
    p.kyunggong_id=2602; p.avatar_lightness=20; p.shop_lightness=30; p.ability_lightness=999;
    EXPECT_EQ(player_move_speed(p),300);
    p.titan_grade_lightness=std::array<float,3>{500,600,700};
    EXPECT_EQ(player_move_speed(p),650);
    p.kyunggong_id=2604; EXPECT_EQ(player_move_speed(p),750);
    p.kyunggong_id=2600; EXPECT_EQ(player_move_speed(p),550);
}
TEST(PlayerMoveSpeed, OrderedStatusOverridesAreNotSummedOrCompounded) {
    PlayerMoveSpeedInput p;
    const std::array<MoveSpeedStatus,3> statuses{{{50,0},{25,10},{0,20}}};
    EXPECT_FLOAT_EQ(*player_move_speed(p,statuses),420);
    const std::array<MoveSpeedStatus,3> reversed{{{0,20},{25,10},{50,0}}};
    EXPECT_FLOAT_EQ(*player_move_speed(p,reversed),560);
    const std::array<MoveSpeedStatus,1> slow{{{0,150}}};
    EXPECT_FLOAT_EQ(*player_move_speed(p,slow),-200); // No invented clamp in original formula.
}
TEST(PlayerMoveSpeed, NonfiniteActiveDataCannotProduceUsableSpeed) {
    PlayerMoveSpeedInput p; p.kyunggong_id=1; p.special_resources_resolved=true;
    p.lightness_base=std::numeric_limits<float>::infinity(); EXPECT_FALSE(player_move_speed(p));
    p.lightness_base=std::numeric_limits<float>::max();
    const std::array<MoveSpeedStatus,1> boost{{{65535,0}}};
    EXPECT_FALSE(player_move_speed(p,boost));
}
