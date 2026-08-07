// agent_note_side_effect_runtime_test.cpp
//
// Verifies apply_agent_note_side_effects() (the runtime orchestrator
// for the legacy MP_NOTEMsgParser / MPSendNoteMsgParser side-effect
// chains) walks the data-plane plan and dispatches each entry: the
// per-action effect chains in legacy order / empty plan on gate
// failure.

#include <mxh/server/agent_note_side_effect.hpp>
#include <mxh/server/agent_note_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::AgentNoteAction;
using mxh::server::AgentNoteSideEffectSink;
using mxh::server::AgentNoteValidationInput;
using mxh::server::apply_agent_note_side_effects;
using mxh::server::agent_note_side_effect_plan;

class RecordingSink final : public AgentNoteSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_object_id = 0;
    std::uint16_t last_list_page = 0;
    std::uint16_t last_list_slot = 0;
    std::uint32_t last_note_id = 0;
    std::uint8_t last_b_last = 0;

    void copy_note_buffers(std::uint32_t object_id) override {
        calls.push_back("copy");
        last_object_id = object_id;
    }
    void filter_check_note(std::uint32_t object_id) override {
        calls.push_back("filter");
        last_object_id = object_id;
    }
    void note_server_sendto_player(std::uint32_t object_id) override {
        calls.push_back("send");
        last_object_id = object_id;
    }
    void note_sendto_player_id(std::uint32_t object_id) override {
        calls.push_back("sendid");
        last_object_id = object_id;
    }
    void send_2_user_passthrough(std::uint32_t object_id) override {
        calls.push_back("recv");
        last_object_id = object_id;
    }
    void send_del_all_ack_to_user(std::uint32_t object_id) override {
        calls.push_back("delallack");
        last_object_id = object_id;
    }
    void note_del_all(std::uint32_t object_id) override {
        calls.push_back("delall");
        last_object_id = object_id;
    }
    void note_list(std::uint32_t object_id,
                   std::uint16_t list_page,
                   std::uint16_t list_slot) override {
        calls.push_back("list");
        last_object_id = object_id;
        last_list_page = list_page;
        last_list_slot = list_slot;
    }
    void note_read(std::uint32_t object_id,
                   std::uint32_t note_id,
                   std::uint16_t list_page) override {
        calls.push_back("read");
        last_object_id = object_id;
        last_note_id = note_id;
        last_list_page = list_page;
    }
    void note_delete(std::uint32_t object_id,
                     std::uint32_t note_id,
                     std::uint8_t b_last) override {
        calls.push_back("del");
        last_object_id = object_id;
        last_note_id = note_id;
        last_b_last = b_last;
    }
};

}  // namespace

TEST(ApplyAgentNoteSideEffects, SendByNameEmitsThreeEffectsInOrder) {
    AgentNoteValidationInput in;
    in.action = AgentNoteAction::SendNoteByName;
    in.user_found = true;
    in.filter_passed = true;
    auto plan = agent_note_side_effect_plan(
        in, /*object_id=*/0x00240025u, 1, 2, 3, 0);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 3u);

    RecordingSink sink;
    auto out = apply_agent_note_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 3u);
    EXPECT_EQ(out.copies, 1u);
    EXPECT_EQ(out.filters, 1u);
    EXPECT_EQ(out.server_sends, 1u);
    EXPECT_TRUE(out.dispatched_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"copy", "filter", "send"}));
    EXPECT_EQ(sink.last_object_id, 0x00240025u);
}

TEST(ApplyAgentNoteSideEffects, PerActionSingleEffectChains) {
    {
        AgentNoteValidationInput in;
        in.action = AgentNoteAction::SendNoteById;
        auto plan = agent_note_side_effect_plan(in, 7, 0, 0, 0, 0);
        RecordingSink sink;
        (void)apply_agent_note_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"sendid"}));
    }
    {
        AgentNoteValidationInput in;
        in.action = AgentNoteAction::ReceiveNote;
        in.user_found = true;
        auto plan = agent_note_side_effect_plan(in, 8, 0, 0, 0, 0);
        RecordingSink sink;
        (void)apply_agent_note_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"recv"}));
    }
    {
        AgentNoteValidationInput in;
        in.action = AgentNoteAction::ListNote;
        auto plan = agent_note_side_effect_plan(
            in, 9, /*list_page=*/2, /*list_slot=*/3, 0, 0);
        RecordingSink sink;
        (void)apply_agent_note_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"list"}));
        EXPECT_EQ(sink.last_list_page, 2u);
        EXPECT_EQ(sink.last_list_slot, 3u);
    }
    {
        AgentNoteValidationInput in;
        in.action = AgentNoteAction::ReadNote;
        in.user_found = true;
        auto plan = agent_note_side_effect_plan(
            in, 10, /*list_page=*/4, 0, /*note_id=*/555, 0);
        RecordingSink sink;
        (void)apply_agent_note_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"read"}));
        EXPECT_EQ(sink.last_note_id, 555u);
        EXPECT_EQ(sink.last_list_page, 4u);
    }
}

TEST(ApplyAgentNoteSideEffects, DelAllEmitsDelThenAckInOrder) {
    AgentNoteValidationInput in;
    in.action = AgentNoteAction::DelAllNote;
    in.user_found = true;
    auto plan = agent_note_side_effect_plan(in, 42, 0, 0, 0, 0);
    ASSERT_EQ(plan.effects.size(), 2u);

    RecordingSink sink;
    auto out = apply_agent_note_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.delalls, 1u);
    EXPECT_EQ(out.delall_acks, 1u);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"delall", "delallack"}));
}

TEST(ApplyAgentNoteSideEffects, DeleteEmitsNoteDeleteWithBLast) {
    AgentNoteValidationInput in;
    in.action = AgentNoteAction::DeleteNote;
    in.b_last_valid = true;
    auto plan = agent_note_side_effect_plan(
        in, 7, 0, 0, /*note_id=*/123, /*b_last=*/1);
    EXPECT_TRUE(plan.b_last_valid);
    ASSERT_EQ(plan.effects.size(), 1u);

    RecordingSink sink;
    auto out = apply_agent_note_side_effects(plan, sink);
    EXPECT_EQ(out.deletes, 1u);
    EXPECT_TRUE(out.blast_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"del"}));
    EXPECT_EQ(sink.last_note_id, 123u);
    EXPECT_EQ(sink.last_b_last, 1u);
}

TEST(ApplyAgentNoteSideEffects, GateFailuresEmitEmptyPlan) {
    {
        AgentNoteValidationInput in;
        in.action = AgentNoteAction::SendNoteByName;
        in.user_found = false;
        auto plan = agent_note_side_effect_plan(in, 1, 0, 0, 0, 0);
        EXPECT_FALSE(plan.dispatched);
        EXPECT_TRUE(plan.effects.empty());
    }
    {
        AgentNoteValidationInput in;
        in.action = AgentNoteAction::SendNoteByName;
        in.user_found = true;
        in.filter_passed = false;
        auto plan = agent_note_side_effect_plan(in, 1, 0, 0, 0, 0);
        EXPECT_TRUE(plan.effects.empty());
    }
    {
        AgentNoteValidationInput in;
        in.action = AgentNoteAction::DeleteNote;
        in.b_last_valid = false;
        auto plan = agent_note_side_effect_plan(in, 1, 0, 0, 0, 2);
        EXPECT_TRUE(plan.effects.empty());
    }
    {
        AgentNoteValidationInput in;
        in.action = AgentNoteAction::ReceiveNote;
        in.user_found = false;
        auto plan = agent_note_side_effect_plan(in, 1, 0, 0, 0, 0);
        EXPECT_TRUE(plan.effects.empty());
    }
}

TEST(ApplyAgentNoteSideEffects, EmptyPlanIsNoOp) {
    mxh::server::AgentNoteSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_agent_note_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.copies, 0u);
    EXPECT_EQ(out.filters, 0u);
    EXPECT_EQ(out.server_sends, 0u);
    EXPECT_EQ(out.id_sends, 0u);
    EXPECT_EQ(out.passthroughs, 0u);
    EXPECT_EQ(out.delall_acks, 0u);
    EXPECT_EQ(out.delalls, 0u);
    EXPECT_EQ(out.lists, 0u);
    EXPECT_EQ(out.reads, 0u);
    EXPECT_EQ(out.deletes, 0u);
    EXPECT_FALSE(out.dispatched_flag_consumed);
    EXPECT_FALSE(out.filter_flag_consumed);
    EXPECT_FALSE(out.blast_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyAgentNoteSideEffects, BoundaryFieldsPassthrough) {
    AgentNoteValidationInput in;
    in.action = AgentNoteAction::ListNote;
    auto plan = agent_note_side_effect_plan(
        in, /*object_id=*/0xFFFFFFFFu,
        /*list_page=*/0xFFFFu, /*list_slot=*/0xFFFFu, 0, 0);
    RecordingSink sink;
    (void)apply_agent_note_side_effects(plan, sink);
    EXPECT_EQ(sink.last_object_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_list_page, 0xFFFFu);
    EXPECT_EQ(sink.last_list_slot, 0xFFFFu);
}
