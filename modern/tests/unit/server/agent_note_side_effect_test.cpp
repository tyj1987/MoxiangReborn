// Tests for MP_NOTE_* agent side-effect dispatcher.

#include <mxh/server/agent_note_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

AgentNoteValidationInput send_by_name_ok() {
    AgentNoteValidationInput in{};
    in.action = AgentNoteAction::SendNoteByName;
    in.user_found = true;
    in.filter_passed = true;
    return in;
}

AgentNoteValidationInput send_by_id_ok() {
    AgentNoteValidationInput in{};
    in.action = AgentNoteAction::SendNoteById;
    return in;
}

AgentNoteValidationInput receive_ok() {
    AgentNoteValidationInput in{};
    in.action = AgentNoteAction::ReceiveNote;
    in.user_found = true;
    return in;
}

AgentNoteValidationInput del_all_ok() {
    AgentNoteValidationInput in{};
    in.action = AgentNoteAction::DelAllNote;
    in.user_found = true;
    return in;
}

AgentNoteValidationInput list_ok() {
    AgentNoteValidationInput in{};
    in.action = AgentNoteAction::ListNote;
    return in;
}

AgentNoteValidationInput read_ok() {
    AgentNoteValidationInput in{};
    in.action = AgentNoteAction::ReadNote;
    in.user_found = true;
    return in;
}

AgentNoteValidationInput del_ok() {
    AgentNoteValidationInput in{};
    in.action = AgentNoteAction::DeleteNote;
    in.b_last_valid = true;
    return in;
}

TEST(AgentNoteOutcome, SendByNameDispatchedWhenAllOk) {
    EXPECT_EQ(classify_agent_note_outcome(send_by_name_ok()),
              AgentNoteOutcome::Dispatched);
}

TEST(AgentNoteOutcome, SendByNameNoUserTakesPrecedence) {
    auto in = send_by_name_ok();
    in.user_found = false;
    EXPECT_EQ(classify_agent_note_outcome(in),
              AgentNoteOutcome::NoUser);
}

TEST(AgentNoteOutcome, SendByNameFiltered) {
    auto in = send_by_name_ok();
    in.filter_passed = false;
    EXPECT_EQ(classify_agent_note_outcome(in),
              AgentNoteOutcome::Filtered);
}

TEST(AgentNoteOutcome, SendByIdAlwaysDispatched) {
    EXPECT_EQ(classify_agent_note_outcome(send_by_id_ok()),
              AgentNoteOutcome::Dispatched);
}

TEST(AgentNoteOutcome, ReceiveDispatchedWhenUserFound) {
    EXPECT_EQ(classify_agent_note_outcome(receive_ok()),
              AgentNoteOutcome::Dispatched);
    auto in = receive_ok();
    in.user_found = false;
    EXPECT_EQ(classify_agent_note_outcome(in),
              AgentNoteOutcome::NoUser);
}

TEST(AgentNoteOutcome, DelAllDispatchedWhenUserFound) {
    EXPECT_EQ(classify_agent_note_outcome(del_all_ok()),
              AgentNoteOutcome::Dispatched);
    auto in = del_all_ok();
    in.user_found = false;
    EXPECT_EQ(classify_agent_note_outcome(in),
              AgentNoteOutcome::NoUser);
}

TEST(AgentNoteOutcome, ListAlwaysDispatched) {
    EXPECT_EQ(classify_agent_note_outcome(list_ok()),
              AgentNoteOutcome::Dispatched);
}

TEST(AgentNoteOutcome, ReadDispatchedWhenUserFound) {
    EXPECT_EQ(classify_agent_note_outcome(read_ok()),
              AgentNoteOutcome::Dispatched);
    auto in = read_ok();
    in.user_found = false;
    EXPECT_EQ(classify_agent_note_outcome(in),
              AgentNoteOutcome::NoUser);
}

TEST(AgentNoteOutcome, DeleteDispatchedWhenBLastValid) {
    EXPECT_EQ(classify_agent_note_outcome(del_ok()),
              AgentNoteOutcome::Dispatched);
    auto in = del_ok();
    in.b_last_valid = false;
    EXPECT_EQ(classify_agent_note_outcome(in),
              AgentNoteOutcome::Invalid);
}

TEST(AgentNotePlan, SendByNameEmitsThreeEffects) {
    auto in = send_by_name_ok();
    auto plan = agent_note_side_effect_plan(in, 100, 0, 0, 0, 0);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.filter_passed);
    EXPECT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind,
              AgentNoteSideEffectKind::CopyNoteBuffers);
    EXPECT_EQ(plan.effects[1].kind,
              AgentNoteSideEffectKind::FilterCheckNote);
    EXPECT_EQ(plan.effects[2].kind,
              AgentNoteSideEffectKind::NoteServerSendtoPlayer);
}

TEST(AgentNotePlan, SendByNameNoUserEmitsEmptyPlan) {
    auto in = send_by_name_ok();
    in.user_found = false;
    auto plan = agent_note_side_effect_plan(in, 100, 0, 0, 0, 0);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentNotePlan, SendByNameFilteredEmitsEmptyPlan) {
    auto in = send_by_name_ok();
    in.filter_passed = false;
    auto plan = agent_note_side_effect_plan(in, 100, 0, 0, 0, 0);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentNotePlan, SendByIdEmitsOneEffect) {
    auto in = send_by_id_ok();
    auto plan = agent_note_side_effect_plan(in, 100, 0, 0, 0, 0);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AgentNoteSideEffectKind::NoteSendtoPlayerID);
}

TEST(AgentNotePlan, ReceiveEmitsOneEffect) {
    auto in = receive_ok();
    auto plan = agent_note_side_effect_plan(in, 100, 0, 0, 0, 0);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AgentNoteSideEffectKind::Send2UserPassthrough);
}

TEST(AgentNotePlan, DelAllEmitsDelThenAck) {
    auto in = del_all_ok();
    auto plan = agent_note_side_effect_plan(in, 100, 0, 0, 0, 0);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              AgentNoteSideEffectKind::NoteDelAll);
    EXPECT_EQ(plan.effects[1].kind,
              AgentNoteSideEffectKind::SendDelAllAckToUser);
}

TEST(AgentNotePlan, ListEmitsNoteListWithPageAndSlot) {
    auto in = list_ok();
    auto plan = agent_note_side_effect_plan(in, 100, 5, 7, 0, 0);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentNoteSideEffectKind::NoteList);
    EXPECT_EQ(plan.effects[0].list_page, 5u);
    EXPECT_EQ(plan.effects[0].list_slot, 7u);
}

TEST(AgentNotePlan, ReadEmitsNoteReadWithNoteId) {
    auto in = read_ok();
    auto plan = agent_note_side_effect_plan(in, 100, 0, 0, 42, 0);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentNoteSideEffectKind::NoteRead);
    EXPECT_EQ(plan.effects[0].note_id, 42u);
}

TEST(AgentNotePlan, DeleteEmitsNoteDeleteWithBLast) {
    auto in = del_ok();
    auto plan = agent_note_side_effect_plan(in, 100, 0, 0, 42, 1);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.b_last_valid);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AgentNoteSideEffectKind::NoteDelete);
    EXPECT_EQ(plan.effects[0].note_id, 42u);
    EXPECT_EQ(plan.effects[0].b_last, 1u);
}

TEST(AgentNotePlan, DeleteInvalidBLastEmitsEmptyPlan) {
    auto in = del_ok();
    in.b_last_valid = false;
    auto plan = agent_note_side_effect_plan(in, 100, 0, 0, 42, 5);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_FALSE(plan.b_last_valid);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentNotePlan, PlanIsIdempotent) {
    auto in = send_by_name_ok();
    auto a = agent_note_side_effect_plan(in, 100, 0, 0, 0, 0);
    auto b = agent_note_side_effect_plan(in, 100, 0, 0, 0, 0);
    EXPECT_EQ(a.dispatched, b.dispatched);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].object_id, b.effects[i].object_id);
    }
}
