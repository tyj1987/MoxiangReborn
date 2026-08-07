// agent_note_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// agent_note_side_effect_plan(). The data plane returns an empty plan
// (no user / filtered / invalid bLast) or the action's effect chain
// (1-3 entries); this header walks the plan and dispatches each entry
// to a virtual AgentNoteSideEffectSink.
//
// 1:1 invariants (1:1 with legacy MP_NOTEMsgParser /
// MPSendNoteMsgParser from [Server]Agent/AgentNetworkMsgParser.cpp:
// 2355-2504):
//   - SendNoteByName: CopyNoteBuffers -> FilterCheckNote ->
//     NoteServerSendtoPlayer.
//   - SendNoteById: NoteSendtoPlayerID.
//   - ReceiveNote: Send2User passthrough.
//   - DelAllNote: NoteDelAll -> SendDelAllAckToUser.
//   - ListNote: NoteList(page, slot); ReadNote: NoteRead(id);
//     DeleteNote: NoteDelete(id, bLast).
//
// Pattern mirrors agent_friend_side_effect_runtime.hpp (D4.96) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/agent_note_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the AgentNote side-effect chain.
class AgentNoteSideEffectSink {
public:
    virtual ~AgentNoteSideEffectSink() = default;

    // Legacy: SafeStrCpy of FromName/ToName/Note into fixed buffers.
    virtual void copy_note_buffers(std::uint32_t object_id) = 0;

    // Legacy: FILTERTABLE->IsInvalidCharInclude on From/To.
    virtual void filter_check_note(std::uint32_t object_id) = 0;

    // Legacy: NoteServerSendtoPlayer(FromId, FromName, ToName, Note).
    virtual void note_server_sendto_player(std::uint32_t object_id) = 0;

    // Legacy: NoteSendtoPlayerID(dwObjectID, FromName, TargetID, Note).
    virtual void note_sendto_player_id(std::uint32_t object_id) = 0;

    // Legacy: g_Network.Send2User(userinfo->dwConnectionIndex, pMsg,
    // sizeof(MSGBASE)).
    virtual void send_2_user_passthrough(std::uint32_t object_id) = 0;

    // Legacy: g_Network.Send2User DELALLNOTE_ACK.
    virtual void send_del_all_ack_to_user(std::uint32_t object_id) = 0;

    // Legacy: NoteDelAll(dwObjectID).
    virtual void note_del_all(std::uint32_t object_id) = 0;

    // Legacy: NoteList(dwObjectID, wData1, wData2).
    virtual void note_list(std::uint32_t object_id,
                           std::uint16_t list_page,
                           std::uint16_t list_slot) = 0;

    // Legacy: NoteRead(dwObjectID, dwData1, dwData2).
    virtual void note_read(std::uint32_t object_id,
                           std::uint32_t note_id,
                           std::uint16_t list_page) = 0;

    // Legacy: NoteDelete(dwObjectID, NoteID, bLast).
    virtual void note_delete(std::uint32_t object_id,
                             std::uint32_t note_id,
                             std::uint8_t b_last) = 0;
};

struct AgentNoteRuntimeOutcome {
    std::size_t effects_applied  = 0;
    std::size_t copies           = 0;
    std::size_t filters          = 0;
    std::size_t server_sends     = 0;
    std::size_t id_sends         = 0;
    std::size_t passthroughs     = 0;
    std::size_t delall_acks      = 0;
    std::size_t delalls          = 0;
    std::size_t lists            = 0;
    std::size_t reads            = 0;
    std::size_t deletes          = 0;
    bool dispatched_flag_consumed = false;
    bool filter_flag_consumed     = false;
    bool blast_flag_consumed      = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline AgentNoteRuntimeOutcome apply_agent_note_side_effects(
    const AgentNoteSideEffectPlan& plan,
    AgentNoteSideEffectSink& sink) {
    AgentNoteRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case AgentNoteSideEffectKind::CopyNoteBuffers:
            sink.copy_note_buffers(effect.object_id);
            ++out.copies;
            ++out.effects_applied;
            break;
        case AgentNoteSideEffectKind::FilterCheckNote:
            sink.filter_check_note(effect.object_id);
            ++out.filters;
            ++out.effects_applied;
            break;
        case AgentNoteSideEffectKind::NoteServerSendtoPlayer:
            sink.note_server_sendto_player(effect.object_id);
            ++out.server_sends;
            ++out.effects_applied;
            break;
        case AgentNoteSideEffectKind::NoteSendtoPlayerID:
            sink.note_sendto_player_id(effect.object_id);
            ++out.id_sends;
            ++out.effects_applied;
            break;
        case AgentNoteSideEffectKind::Send2UserPassthrough:
            sink.send_2_user_passthrough(effect.object_id);
            ++out.passthroughs;
            ++out.effects_applied;
            break;
        case AgentNoteSideEffectKind::SendDelAllAckToUser:
            sink.send_del_all_ack_to_user(effect.object_id);
            ++out.delall_acks;
            ++out.effects_applied;
            break;
        case AgentNoteSideEffectKind::NoteDelAll:
            sink.note_del_all(effect.object_id);
            ++out.delalls;
            ++out.effects_applied;
            break;
        case AgentNoteSideEffectKind::NoteList:
            sink.note_list(effect.object_id, effect.list_page,
                           effect.list_slot);
            ++out.lists;
            ++out.effects_applied;
            break;
        case AgentNoteSideEffectKind::NoteRead:
            sink.note_read(effect.object_id, effect.note_id,
                           effect.list_page);
            ++out.reads;
            ++out.effects_applied;
            break;
        case AgentNoteSideEffectKind::NoteDelete:
            sink.note_delete(effect.object_id, effect.note_id,
                             effect.b_last);
            ++out.deletes;
            ++out.effects_applied;
            break;
        }
    }
    out.dispatched_flag_consumed = plan.dispatched;
    out.filter_flag_consumed = plan.filter_passed;
    out.blast_flag_consumed = plan.b_last_valid;
    return out;
}

}  // namespace mxh::server
