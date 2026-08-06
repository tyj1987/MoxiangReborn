// agent_move_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_move (D4.144).
// Augments the legacy 11-test agent_move_test.cpp with deeper coverage of:
//   - move_category constant = 8 (MP_MOVE)
//   - 20 sub-protocol constants (move_init=0 .. move_pet_warp_ack=19)
//   - MoveActionKind enum (forward_to_map, forward_to_map_if_in_map, drop_no_map)
//   - MoveRequest struct defaults (protocol=0, object_id=0, user_in_map=true)
//   - MoveAction struct defaults
//   - classify_move truth table:
//       user_in_map=false -> drop_no_map
//       user_in_map=true  -> forward_to_map
//       protocol and object_id are always preserved
//   - all 20 protocols sweep: forward/drop depends only on user_in_map flag
//
// 1:1 invariants (locked):
//   - move_category = 8
//   - 20 protocol constants: move_init=0, move_target=1, move_correction=2,
//     move_walkmode=3, move_runmode=4, move_kyunggong_syn=5,
//     move_kyunggong_ack=6, move_kyunggong_nack=7, move_stop=8,
//     move_effectmove=9, move_monstermove_notify=10,
//     move_forcestopkyunggong=11, move_warp=12, move_onetarget=13,
//     move_pet_onetarget=14, move_pet_target=15, move_pet_stop=16,
//     move_pet_correction=17, move_pet_warp_syn=18, move_pet_warp_ack=19
//   - Forward path requires user_in_map=true; false -> drop_no_map
//   - Protocol + object_id are always preserved (legacy pass-through)

#pragma once

#include "mxh/server/agent_move.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_move;
using mxh::server::MoveAction;
using mxh::server::MoveActionKind;
using mxh::server::MoveRequest;
using mxh::server::move_category;
using mxh::server::move_correction;
using mxh::server::move_effectmove;
using mxh::server::move_forcestopkyunggong;
using mxh::server::move_init;
using mxh::server::move_kyunggong_ack;
using mxh::server::move_kyunggong_nack;
using mxh::server::move_kyunggong_syn;
using mxh::server::move_monstermove_notify;
using mxh::server::move_onetarget;
using mxh::server::move_pet_correction;
using mxh::server::move_pet_onetarget;
using mxh::server::move_pet_stop;
using mxh::server::move_pet_target;
using mxh::server::move_pet_warp_ack;
using mxh::server::move_pet_warp_syn;
using mxh::server::move_runmode;
using mxh::server::move_stop;
using mxh::server::move_target;
using mxh::server::move_walkmode;
using mxh::server::move_warp;

}  // namespace


// ===========================================================================
// move_category constant
// ===========================================================================

TEST(AgentMoveDataPlane, CategoryIsEight) {
    EXPECT_EQ(move_category, 8u);
}


// ===========================================================================
// Protocol constants -- 20 values 0..19
// ===========================================================================

TEST(AgentMoveDataPlane, ProtocolInitIsZero) { EXPECT_EQ(move_init, 0u); }
TEST(AgentMoveDataPlane, ProtocolTargetIsOne) { EXPECT_EQ(move_target, 1u); }
TEST(AgentMoveDataPlane, ProtocolCorrectionIsTwo) { EXPECT_EQ(move_correction, 2u); }
TEST(AgentMoveDataPlane, ProtocolWalkmodeIsThree) { EXPECT_EQ(move_walkmode, 3u); }
TEST(AgentMoveDataPlane, ProtocolRunmodeIsFour) { EXPECT_EQ(move_runmode, 4u); }
TEST(AgentMoveDataPlane, ProtocolKyunggongSynIsFive) { EXPECT_EQ(move_kyunggong_syn, 5u); }
TEST(AgentMoveDataPlane, ProtocolKyunggongAckIsSix) { EXPECT_EQ(move_kyunggong_ack, 6u); }
TEST(AgentMoveDataPlane, ProtocolKyunggongNackIsSeven) { EXPECT_EQ(move_kyunggong_nack, 7u); }
TEST(AgentMoveDataPlane, ProtocolStopIsEight) { EXPECT_EQ(move_stop, 8u); }
TEST(AgentMoveDataPlane, ProtocolEffectmoveIsNine) { EXPECT_EQ(move_effectmove, 9u); }
TEST(AgentMoveDataPlane, ProtocolMonstermoveNotifyIsTen) { EXPECT_EQ(move_monstermove_notify, 10u); }
TEST(AgentMoveDataPlane, ProtocolForceStopKyunggongIsEleven) { EXPECT_EQ(move_forcestopkyunggong, 11u); }
TEST(AgentMoveDataPlane, ProtocolWarpIsTwelve) { EXPECT_EQ(move_warp, 12u); }
TEST(AgentMoveDataPlane, ProtocolOneTargetIsThirteen) { EXPECT_EQ(move_onetarget, 13u); }
TEST(AgentMoveDataPlane, ProtocolPetOneTargetIsFourteen) { EXPECT_EQ(move_pet_onetarget, 14u); }
TEST(AgentMoveDataPlane, ProtocolPetTargetIsFifteen) { EXPECT_EQ(move_pet_target, 15u); }
TEST(AgentMoveDataPlane, ProtocolPetStopIsSixteen) { EXPECT_EQ(move_pet_stop, 16u); }
TEST(AgentMoveDataPlane, ProtocolPetCorrectionIsSeventeen) { EXPECT_EQ(move_pet_correction, 17u); }
TEST(AgentMoveDataPlane, ProtocolPetWarpSynIsEighteen) { EXPECT_EQ(move_pet_warp_syn, 18u); }
TEST(AgentMoveDataPlane, ProtocolPetWarpAckIsNineteen) { EXPECT_EQ(move_pet_warp_ack, 19u); }

TEST(AgentMoveDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        move_init, move_target, move_correction, move_walkmode, move_runmode,
        move_kyunggong_syn, move_kyunggong_ack, move_kyunggong_nack,
        move_stop, move_effectmove, move_monstermove_notify,
        move_forcestopkyunggong, move_warp, move_onetarget,
        move_pet_onetarget, move_pet_target, move_pet_stop, move_pet_correction,
        move_pet_warp_syn, move_pet_warp_ack,
    };
    EXPECT_EQ(seen.size(), 20u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(AgentMoveDataPlane, ActionKindHasThreeValues) {
    auto all = {
        MoveActionKind::forward_to_map,
        MoveActionKind::forward_to_map_if_in_map,
        MoveActionKind::drop_no_map,
    };
    EXPECT_EQ(all.size(), 3u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(AgentMoveDataPlane, RequestDefaults) {
    MoveRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_TRUE(r.user_in_map);
}

TEST(AgentMoveDataPlane, ActionDefaults) {
    MoveAction a{};
    EXPECT_EQ(a.kind, MoveActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
}


// ===========================================================================
// classify_move truth table
// ===========================================================================

TEST(AgentMoveDataPlane, ClassifyUserInMapForwards) {
    MoveRequest r{move_target, 5u, true};
    auto a = classify_move(r);
    EXPECT_EQ(a.kind, MoveActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, move_target);
    EXPECT_EQ(a.object_id, 5u);
}

TEST(AgentMoveDataPlane, ClassifyUserNotInMapDrops) {
    MoveRequest r{move_target, 5u, false};
    auto a = classify_move(r);
    EXPECT_EQ(a.kind, MoveActionKind::drop_no_map);
    EXPECT_EQ(a.object_id, 5u);
}

TEST(AgentMoveDataPlane, ClassifyWarpInMapForwards) {
    MoveRequest r{move_warp, 7u, true};
    auto a = classify_move(r);
    EXPECT_EQ(a.kind, MoveActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, move_warp);
}

TEST(AgentMoveDataPlane, ClassifyWarpNotInMapDrops) {
    MoveRequest r{move_warp, 7u, false};
    EXPECT_EQ(classify_move(r).kind, MoveActionKind::drop_no_map);
}

TEST(AgentMoveDataPlane, ClassifyUserInMapDefaultIsTrue) {
    // Default user_in_map=true
    MoveRequest r{};
    EXPECT_EQ(classify_move(r).kind, MoveActionKind::forward_to_map);
}


// ===========================================================================
// Protocol sweep -- all 20 protocols behave the same based on user_in_map
// ===========================================================================

TEST(AgentMoveDataPlane, ClassifyAllProtocolsInMapForwards) {
    std::uint8_t protos[] = {
        move_init, move_target, move_correction, move_walkmode, move_runmode,
        move_kyunggong_syn, move_kyunggong_ack, move_kyunggong_nack,
        move_stop, move_effectmove, move_monstermove_notify,
        move_forcestopkyunggong, move_warp, move_onetarget,
        move_pet_onetarget, move_pet_target, move_pet_stop, move_pet_correction,
        move_pet_warp_syn, move_pet_warp_ack,
    };
    for (auto p : protos) {
        MoveRequest r{p, 100u, true};
        auto a = classify_move(r);
        EXPECT_EQ(a.kind, MoveActionKind::forward_to_map);
        EXPECT_EQ(a.protocol, p);
        EXPECT_EQ(a.object_id, 100u);
    }
}

TEST(AgentMoveDataPlane, ClassifyAllProtocolsNotInMapDrops) {
    std::uint8_t protos[] = {
        move_init, move_target, move_correction, move_walkmode, move_runmode,
        move_kyunggong_syn, move_kyunggong_ack, move_kyunggong_nack,
        move_stop, move_effectmove, move_monstermove_notify,
        move_forcestopkyunggong, move_warp, move_onetarget,
        move_pet_onetarget, move_pet_target, move_pet_stop, move_pet_correction,
        move_pet_warp_syn, move_pet_warp_ack,
    };
    for (auto p : protos) {
        MoveRequest r{p, 100u, false};
        auto a = classify_move(r);
        EXPECT_EQ(a.kind, MoveActionKind::drop_no_map);
        EXPECT_EQ(a.protocol, p);
        EXPECT_EQ(a.object_id, 100u);
    }
}


// ===========================================================================
// Boundary protocols
// ===========================================================================

TEST(AgentMoveDataPlane, ClassifyProtocol255InMapForwards) {
    MoveRequest r{255, 1u, true};
    auto a = classify_move(r);
    EXPECT_EQ(a.kind, MoveActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 255u);
}

TEST(AgentMoveDataPlane, ClassifyProtocol255NotInMapDrops) {
    MoveRequest r{255, 1u, false};
    EXPECT_EQ(classify_move(r).kind, MoveActionKind::drop_no_map);
}


// ===========================================================================
// Object_id preservation
// ===========================================================================

TEST(AgentMoveDataPlane, ClassifyPreservesObjectIdMaxUint32) {
    MoveRequest r{move_target, 0xFFFFFFFFu, true};
    auto a = classify_move(r);
    EXPECT_EQ(a.object_id, 0xFFFFFFFFu);
}

TEST(AgentMoveDataPlane, ClassifyPreservesObjectIdZero) {
    MoveRequest r{move_target, 0u, false};
    auto a = classify_move(r);
    EXPECT_EQ(a.object_id, 0u);
}

TEST(AgentMoveDataPlane, ClassifyAlwaysPreservesProtocol) {
    // Regardless of user_in_map, protocol is always preserved.
    MoveRequest in{move_pet_warp_syn, 42u, true};
    MoveRequest out{move_pet_warp_syn, 42u, false};
    EXPECT_EQ(classify_move(in).protocol, move_pet_warp_syn);
    EXPECT_EQ(classify_move(out).protocol, move_pet_warp_syn);
}
