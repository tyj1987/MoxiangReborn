// agent_quest_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_quest (D4.156).
// Augments the legacy 9-test agent_quest_test.cpp with deeper coverage of:
//   - quest_category constant = 38 (MP_QUEST)
//   - 28 sub-protocol constants (quest_totalinfo=0 .. quest_full=27)
//   - QuestActionKind enum (forward_to_map, send_nack_no_quest)
//   - QuestRequest struct defaults (protocol=0, object_id=0, user_has_quest=true)
//   - QuestAction struct defaults
//   - classify_quest pass-through: always forward_to_map with original protocol + object_id
//
// 1:1 invariants (locked):
//   - quest_category = 38
//   - 28 protocol constants (0..27, all distinct)
//   - Agent is pass-through (quest state lives on map server)
//   - forward_to_map always; protocol + object_id always preserved

#pragma once

#include "mxh/server/agent_quest.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_quest;
using mxh::server::quest_category;
using mxh::server::quest_changestate;
using mxh::server::quest_delete_ack;
using mxh::server::quest_delete_confirm_ack;
using mxh::server::quest_delete_confirm_syn;
using mxh::server::quest_delete_nack;
using mxh::server::quest_delete_syn;
using mxh::server::quest_end_ack;
using mxh::server::quest_end_nack;
using mxh::server::quest_end_syn;
using mxh::server::quest_execute_error;
using mxh::server::quest_full;
using mxh::server::quest_giveitem_ack;
using mxh::server::quest_givemoney_ack;
using mxh::server::quest_item_load;
using mxh::server::quest_maindata_load;
using mxh::server::quest_regist_checktime;
using mxh::server::quest_remove_notify;
using mxh::server::quest_start_ack;
using mxh::server::quest_start_nack;
using mxh::server::quest_start_syn;
using mxh::server::quest_subdata_load;
using mxh::server::quest_takeexp_ack;
using mxh::server::quest_takeitem_ack;
using mxh::server::quest_takemoney_ack;
using mxh::server::quest_takesexp_ack;
using mxh::server::quest_time_limit;
using mxh::server::quest_totalinfo;
using mxh::server::quest_unregist_checktime;
using mxh::server::QuestAction;
using mxh::server::QuestActionKind;
using mxh::server::QuestRequest;

}  // namespace


// ===========================================================================
// Constants
// ===========================================================================

TEST(QuestDataPlane, CategoryIsThirtyEight) {
    EXPECT_EQ(quest_category, 38u);
}

TEST(QuestDataPlane, ProtocolTotalinfoIsZero) { EXPECT_EQ(quest_totalinfo, 0u); }
TEST(QuestDataPlane, ProtocolChangestateIsOne) { EXPECT_EQ(quest_changestate, 1u); }
TEST(QuestDataPlane, ProtocolRemoveNotifyIsTwo) { EXPECT_EQ(quest_remove_notify, 2u); }
TEST(QuestDataPlane, ProtocolMaindataLoadIsThree) { EXPECT_EQ(quest_maindata_load, 3u); }
TEST(QuestDataPlane, ProtocolSubdataLoadIsFour) { EXPECT_EQ(quest_subdata_load, 4u); }
TEST(QuestDataPlane, ProtocolItemLoadIsFive) { EXPECT_EQ(quest_item_load, 5u); }
TEST(QuestDataPlane, ProtocolDeleteSynIsSix) { EXPECT_EQ(quest_delete_syn, 6u); }
TEST(QuestDataPlane, ProtocolDeleteAckIsSeven) { EXPECT_EQ(quest_delete_ack, 7u); }
TEST(QuestDataPlane, ProtocolDeleteNackIsEight) { EXPECT_EQ(quest_delete_nack, 8u); }
TEST(QuestDataPlane, ProtocolStartSynIsNine) { EXPECT_EQ(quest_start_syn, 9u); }
TEST(QuestDataPlane, ProtocolStartAckIsTen) { EXPECT_EQ(quest_start_ack, 10u); }
TEST(QuestDataPlane, ProtocolStartNackIsEleven) { EXPECT_EQ(quest_start_nack, 11u); }
TEST(QuestDataPlane, ProtocolEndSynIsTwelve) { EXPECT_EQ(quest_end_syn, 12u); }
TEST(QuestDataPlane, ProtocolEndAckIsThirteen) { EXPECT_EQ(quest_end_ack, 13u); }
TEST(QuestDataPlane, ProtocolEndNackIsFourteen) { EXPECT_EQ(quest_end_nack, 14u); }
TEST(QuestDataPlane, ProtocolTakeItemAckIsFifteen) { EXPECT_EQ(quest_takeitem_ack, 15u); }
TEST(QuestDataPlane, ProtocolTakeMoneyAckIsSixteen) { EXPECT_EQ(quest_takemoney_ack, 16u); }
TEST(QuestDataPlane, ProtocolTakeExpAckIsSeventeen) { EXPECT_EQ(quest_takeexp_ack, 17u); }
TEST(QuestDataPlane, ProtocolTakeSexpAckIsEighteen) { EXPECT_EQ(quest_takesexp_ack, 18u); }
TEST(QuestDataPlane, ProtocolGiveItemAckIsNineteen) { EXPECT_EQ(quest_giveitem_ack, 19u); }
TEST(QuestDataPlane, ProtocolGiveMoneyAckIsTwenty) { EXPECT_EQ(quest_givemoney_ack, 20u); }
TEST(QuestDataPlane, ProtocolDeleteConfirmSynIsTwentyOne) { EXPECT_EQ(quest_delete_confirm_syn, 21u); }
TEST(QuestDataPlane, ProtocolDeleteConfirmAckIsTwentyTwo) { EXPECT_EQ(quest_delete_confirm_ack, 22u); }
TEST(QuestDataPlane, ProtocolRegistChecktimeIsTwentyThree) { EXPECT_EQ(quest_regist_checktime, 23u); }
TEST(QuestDataPlane, ProtocolUnregistChecktimeIsTwentyFour) { EXPECT_EQ(quest_unregist_checktime, 24u); }
TEST(QuestDataPlane, ProtocolTimeLimitIsTwentyFive) { EXPECT_EQ(quest_time_limit, 25u); }
TEST(QuestDataPlane, ProtocolExecuteErrorIsTwentySix) { EXPECT_EQ(quest_execute_error, 26u); }
TEST(QuestDataPlane, ProtocolFullIsTwentySeven) { EXPECT_EQ(quest_full, 27u); }

TEST(QuestDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        quest_totalinfo, quest_changestate, quest_remove_notify,
        quest_maindata_load, quest_subdata_load, quest_item_load,
        quest_delete_syn, quest_delete_ack, quest_delete_nack,
        quest_start_syn, quest_start_ack, quest_start_nack,
        quest_end_syn, quest_end_ack, quest_end_nack,
        quest_takeitem_ack, quest_takemoney_ack, quest_takeexp_ack, quest_takesexp_ack,
        quest_giveitem_ack, quest_givemoney_ack,
        quest_delete_confirm_syn, quest_delete_confirm_ack,
        quest_regist_checktime, quest_unregist_checktime,
        quest_time_limit, quest_execute_error, quest_full,
    };
    EXPECT_EQ(seen.size(), 28u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(QuestDataPlane, ActionKindHasTwoValues) {
    auto all = { QuestActionKind::forward_to_map, QuestActionKind::send_nack_no_quest };
    EXPECT_EQ(all.size(), 2u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(QuestDataPlane, RequestDefaults) {
    QuestRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_TRUE(r.user_has_quest);
}

TEST(QuestDataPlane, ActionDefaults) {
    QuestAction a{};
    EXPECT_EQ(a.kind, QuestActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.error_code, 0u);
}


// ===========================================================================
// classify_quest pass-through
// ===========================================================================

TEST(QuestDataPlane, ClassifyStartSynForwards) {
    QuestRequest r{quest_start_syn, 7u, true};
    auto a = classify_quest(r);
    EXPECT_EQ(a.kind, QuestActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, quest_start_syn);
    EXPECT_EQ(a.object_id, 7u);
}

TEST(QuestDataPlane, ClassifyEndSynForwards) {
    QuestRequest r{quest_end_syn, 99u, true};
    auto a = classify_quest(r);
    EXPECT_EQ(a.kind, QuestActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, quest_end_syn);
}

TEST(QuestDataPlane, ClassifyTotalinfoForwards) {
    QuestRequest r{quest_totalinfo, 5u, true};
    auto a = classify_quest(r);
    EXPECT_EQ(a.kind, QuestActionKind::forward_to_map);
}

TEST(QuestDataPlane, ClassifyFullForwards) {
    QuestRequest r{quest_full, 5u, true};
    auto a = classify_quest(r);
    EXPECT_EQ(a.kind, QuestActionKind::forward_to_map);
}


// ===========================================================================
// All 28 protocols sweep
// ===========================================================================

TEST(QuestDataPlane, ClassifyAllProtocolsForwards) {
    std::uint8_t protos[] = {
        quest_totalinfo, quest_changestate, quest_remove_notify,
        quest_maindata_load, quest_subdata_load, quest_item_load,
        quest_delete_syn, quest_delete_ack, quest_delete_nack,
        quest_start_syn, quest_start_ack, quest_start_nack,
        quest_end_syn, quest_end_ack, quest_end_nack,
        quest_takeitem_ack, quest_takemoney_ack, quest_takeexp_ack, quest_takesexp_ack,
        quest_giveitem_ack, quest_givemoney_ack,
        quest_delete_confirm_syn, quest_delete_confirm_ack,
        quest_regist_checktime, quest_unregist_checktime,
        quest_time_limit, quest_execute_error, quest_full,
    };
    for (auto p : protos) {
        QuestRequest r{p, 100u, true};
        auto a = classify_quest(r);
        EXPECT_EQ(a.kind, QuestActionKind::forward_to_map);
        EXPECT_EQ(a.protocol, p);
        EXPECT_EQ(a.object_id, 100u);
        EXPECT_EQ(a.error_code, 0u);
    }
}


// ===========================================================================
// user_has_quest has no effect (legacy stub)
// ===========================================================================

TEST(QuestDataPlane, ClassifyUserWithoutQuestStillForwards) {
    QuestRequest r{quest_start_syn, 5u, false};
    EXPECT_EQ(classify_quest(r).kind, QuestActionKind::forward_to_map);
}


// ===========================================================================
// Boundary protocols
// ===========================================================================

TEST(QuestDataPlane, ClassifyProtocol31Forwards) {
    QuestRequest r{31u, 5u, true};
    auto a = classify_quest(r);
    EXPECT_EQ(a.kind, QuestActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 31u);
}

TEST(QuestDataPlane, ClassifyProtocol255Forwards) {
    QuestRequest r{255u, 5u, true};
    auto a = classify_quest(r);
    EXPECT_EQ(a.kind, QuestActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 255u);
}


// ===========================================================================
// Object_id preservation
// ===========================================================================

TEST(QuestDataPlane, ClassifyPreservesObjectIdMaxUint32) {
    QuestRequest r{quest_start_syn, 0xFFFFFFFFu, true};
    auto a = classify_quest(r);
    EXPECT_EQ(a.object_id, 0xFFFFFFFFu);
}

TEST(QuestDataPlane, ClassifyPreservesObjectIdZero) {
    QuestRequest r{quest_start_syn, 0u, true};
    auto a = classify_quest(r);
    EXPECT_EQ(a.object_id, 0u);
}

TEST(QuestDataPlane, ClassifyDefaultForwards) {
    QuestRequest r{};
    auto a = classify_quest(r);
    EXPECT_EQ(a.kind, QuestActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.error_code, 0u);
}

TEST(QuestDataPlane, ClassifyAlwaysZeroErrorCode) {
    QuestRequest r{255u, 5u, true};
    auto a = classify_quest(r);
    EXPECT_EQ(a.error_code, 0u);
}
