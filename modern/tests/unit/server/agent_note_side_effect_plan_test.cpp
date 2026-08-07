// D4.172 -- 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_NOTEServerMsgParser (lines 2353-2397) and MP_NOTEMsgParser (lines 2398-2506).
// Each test pins one branch of the legacy dispatch to its modern side-effect plan
// output so future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_note.hpp"
#include "mxh/server/agent_note_side_effect_plan.hpp"

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0xDEADBEEFu;
constexpr std::uint32_t kTargetId = 0xCAFEBABEu;
}

TEST(NotePlan, UserSendnoteNoUserEmitsDrop) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.object_id = kObjectId;
    r.user_found = false;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::drop_no_user);
    const auto plan = note_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, note_sendnote_syn);
    EXPECT_EQ(plan.effects[0].object_id, kObjectId);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(NotePlan, UserSendnoteInvalidFilterEmitsDrop) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = true;
    r.from_invalid = true;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::drop_with_invalid_filter);
    const auto plan = note_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::Drop);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(NotePlan, UserSendnoteValidEmitsSendNote) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.object_id = kObjectId;
    r.user_found = true;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::send_note_filtered);
    const auto plan = note_user_side_effect_plan(a);
    EXPECT_FALSE(plan.drop);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.send_note);
    EXPECT_FALSE(plan.send_note_server);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::SendNote);
    EXPECT_EQ(plan.effects[0].reply_protocol, note_sendnote_syn);
    EXPECT_EQ(plan.effects[0].object_id, kObjectId);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(NotePlan, UserSendnoteIdSynEmitsSendNoteById) {
    NoteRequest r;
    r.protocol = note_sendnoteid_syn;
    r.object_id = kObjectId;
    r.target_id = kTargetId;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::send_note_by_id);
    const auto plan = note_user_side_effect_plan(a);
    EXPECT_TRUE(plan.send_note_by_id);
    EXPECT_FALSE(plan.send_note);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::SendNoteById);
    EXPECT_EQ(plan.effects[0].reply_protocol, note_sendnoteid_syn);
    EXPECT_EQ(plan.effects[0].object_id, kObjectId);
}

TEST(NotePlan, UserReceivenoteWithUserForwardsToUser) {
    NoteRequest r;
    r.protocol = note_receivenote;
    r.object_id = kObjectId;
    r.user_found = true;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::forward_to_user);
    const auto plan = note_user_side_effect_plan(a);
    EXPECT_TRUE(plan.forward_to_user);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, note_receivenote);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(NotePlan, UserDelAllNoteSynEmitsDeleteAllWithAck) {
    NoteRequest r;
    r.protocol = note_delallnote_syn;
    r.object_id = kObjectId;
    r.user_found = true;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::delete_all_with_ack);
    const auto plan = note_user_side_effect_plan(a);
    EXPECT_TRUE(plan.delete_all_with_ack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::DeleteAllWithAck);
    EXPECT_EQ(plan.effects[0].reply_protocol, note_delallnote_syn);
}

TEST(NotePlan, UserNoteListSynEmitsListNotes) {
    NoteRequest r;
    r.protocol = note_notelist_syn;
    r.object_id = kObjectId;
    // legacy: unconditional (user_found ignored).
    r.user_found = false;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::list_notes);
    const auto plan = note_user_side_effect_plan(a);
    EXPECT_TRUE(plan.list_notes);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::ListNotes);
}

TEST(NotePlan, UserReadnoteSynEmitsReadNote) {
    NoteRequest r;
    r.protocol = note_readnote_syn;
    r.object_id = kObjectId;
    r.user_found = true;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::read_note);
    const auto plan = note_user_side_effect_plan(a);
    EXPECT_TRUE(plan.read_note);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::ReadNote);
    EXPECT_EQ(plan.effects[0].reply_protocol, note_readnote_syn);
}

TEST(NotePlan, UserDelnoteSynEmitsReadNoteLegacyQuirk) {
    NoteRequest r;
    r.protocol = note_delnote_syn;
    r.object_id = kObjectId;
    r.user_found = true;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::read_note);
    const auto plan = note_user_side_effect_plan(a);
    EXPECT_TRUE(plan.read_note);
    EXPECT_EQ(plan.effects[0].reply_protocol, note_delnote_syn);
}

TEST(NotePlan, UserDefaultProtocolEmitsDrop) {
    NoteRequest r;
    r.protocol = note_new_note;
    r.object_id = kObjectId;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::drop_no_user);
    const auto plan = note_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].reply_protocol, note_new_note);
}

TEST(NotePlan, ServerSendnoteValidEmitsSendNoteServer) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.object_id = kObjectId;
    r.user_found = true;
    auto a = classify_note_server(r);
    EXPECT_EQ(a.kind, NoteActionKind::send_note_filtered);
    const auto plan = note_server_side_effect_plan(a);
    EXPECT_TRUE(plan.send_note_server);
    EXPECT_FALSE(plan.send_note);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::SendNoteServer);
    EXPECT_EQ(plan.effects[0].reply_protocol, note_sendnote_syn);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(NotePlan, ServerSendnoteNoUserEmitsDrop) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.object_id = kObjectId;
    r.user_found = false;
    auto a = classify_note_server(r);
    EXPECT_EQ(a.kind, NoteActionKind::drop_no_user);
    const auto plan = note_server_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.send_note_server);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::Drop);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(NotePlan, ServerSendnoteInvalidFilterEmitsDrop) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.object_id = kObjectId;
    r.user_found = true;
    r.from_invalid = true;
    auto a = classify_note_server(r);
    EXPECT_EQ(a.kind, NoteActionKind::drop_with_invalid_filter);
    const auto plan = note_server_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, NoteSideEffectKind::Drop);
}

TEST(NotePlan, PlanDefaultsAreConservative) {
    NoteSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.send_note_server);
    EXPECT_FALSE(plan.send_note_by_id);
    EXPECT_FALSE(plan.send_note);
    EXPECT_FALSE(plan.forward_to_user);
    EXPECT_FALSE(plan.delete_all_with_ack);
    EXPECT_FALSE(plan.list_notes);
    EXPECT_FALSE(plan.read_note);
    EXPECT_TRUE(plan.effects.empty());
}

