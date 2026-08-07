// D4.172 -- AgentNote side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_NOTEServerMsgParser (lines 2353-2397) and MP_NOTEMsgParser (lines 2398-2506).
// The data plane (classify_note_user / classify_note_server) decides the action;
// this header captures the ordered side-effect list the orchestrator must execute.
//
// Legacy semantics (preserved verbatim):
//   SERVER side (inter-map, lines 2353-2397):
//     MP_NOTE_SENDNOTE_SYN:
//         FindUser(fromId); !user -> drop.
//         FILTERTABLE.IsInvalidCharInclude(FromName) -> drop.
//         FILTERTABLE.IsInvalidCharInclude(ToName) -> drop.
//         NoteServerSendtoPlayer(fromId, fromName, toName, note).
//   USER side (client->agent, lines 2398-2506):
//     MP_NOTE_SENDNOTE_SYN:
//         FindUser(fromId); !user -> drop.
//         FILTERTABLE.IsInvalidCharInclude(FromName) -> drop.
//         FILTERTABLE.IsInvalidCharInclude(ToName) -> drop.
//         FILTERTABLE.IsCharInString(note, '') -> drop.
//         NoteSendtoPlayer(fromId, fromName, toName, note).
//     MP_NOTE_SENDNOTEID_SYN: NoteSendtoPlayerID(objectId, fromName, targetId, note).
//     MP_NOTE_RECEIVENOTE: FindUser; Send2User -> forward pmsg.
//     MP_NOTE_DELALLNOTE_SYN: NoteDelAll(objectId); FindUser; Send MP_NOTE_DELALLNOTE_ACK.
//     MP_NOTE_NOTELIST_SYN: NoteList(objectId, wData1, wData2) (unconditional).
//     MP_NOTE_READNOTE_SYN: FindUser; NoteRead(objectId, dwData1, dwData2).
//     MP_NOTE_DELNOTE_SYN: NoteDelete(objectId, noteId, bLast) (legacy quirk: same path as read_note).
//     default: drop.
//
// Side effects:
//   - Drop: silent no-op.
//   - SendNoteServer: NoteServerSendtoPlayer inter-map routing.
//   - SendNoteById: NoteSendtoPlayerID.
//   - SendNote: NoteSendtoPlayer (user-side).
//   - ForwardToUser: Send2User pmsg.
//   - DeleteAllWithAck: NoteDelAll + send MP_NOTE_DELALLNOTE_ACK.
//   - ListNotes: NoteList.
//   - ReadNote: NoteRead (covers both readnote_syn and delnote_syn legacy quirk).

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/agent_note.hpp"

namespace mxh::server {

// Side-effect kinds the AgentNote dispatcher must execute in order.
enum class NoteSideEffectKind : std::uint8_t {
    Drop,
    SendNoteServer,
    SendNoteById,
    SendNote,
    ForwardToUser,
    DeleteAllWithAck,
    ListNotes,
    ReadNote,
};

struct NoteSideEffect final {
    NoteSideEffectKind kind = NoteSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint32_t target_id = 0u;
    bool forward_payload = true;
};

struct NoteSideEffectPlan final {
    std::vector<NoteSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
    bool send_note_server = false;
    bool send_note_by_id = false;
    bool send_note = false;
    bool forward_to_user = false;
    bool delete_all_with_ack = false;
    bool list_notes = false;
    bool read_note = false;
};

inline NoteSideEffectPlan note_user_side_effect_plan(
    const NoteAction& a) {
    NoteSideEffectPlan plan;
    using K = NoteActionKind;
    using S = NoteSideEffectKind;
    switch (a.kind) {
        case K::drop_no_user: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.protocol, a.object_id, 0u, false});
            return plan;
        }
        case K::drop_with_invalid_filter: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.protocol, a.object_id, 0u, false});
            return plan;
        }
        case K::send_note_filtered: {
            plan.dispatched = true;
            plan.drop = false;
            plan.send_note = true;
            plan.effects.push_back({S::SendNote, a.protocol, a.object_id, 0u, true});
            return plan;
        }
        case K::send_note_by_id: {
            plan.dispatched = true;
            plan.drop = false;
            plan.send_note_by_id = true;
            plan.effects.push_back({S::SendNoteById, a.protocol, a.object_id, 0u, true});
            return plan;
        }
        case K::forward_to_user: {
            plan.dispatched = true;
            plan.drop = false;
            plan.forward_to_user = true;
            plan.effects.push_back({S::ForwardToUser, a.protocol, a.object_id, 0u, true});
            return plan;
        }
        case K::delete_all_with_ack: {
            plan.dispatched = true;
            plan.drop = false;
            plan.delete_all_with_ack = true;
            plan.effects.push_back({S::DeleteAllWithAck, a.protocol, a.object_id, 0u, true});
            return plan;
        }
        case K::list_notes: {
            plan.dispatched = true;
            plan.drop = false;
            plan.list_notes = true;
            plan.effects.push_back({S::ListNotes, a.protocol, a.object_id, 0u, true});
            return plan;
        }
        case K::read_note: {
            plan.dispatched = true;
            plan.drop = false;
            plan.read_note = true;
            plan.effects.push_back({S::ReadNote, a.protocol, a.object_id, 0u, true});
            return plan;
        }
    }
    return plan;
}

inline NoteSideEffectPlan note_server_side_effect_plan(
    const NoteAction& a) {
    NoteSideEffectPlan plan;
    using K = NoteActionKind;
    using S = NoteSideEffectKind;
    switch (a.kind) {
        case K::drop_no_user: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.protocol, a.object_id, 0u, false});
            return plan;
        }
        case K::drop_with_invalid_filter: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.protocol, a.object_id, 0u, false});
            return plan;
        }
        case K::send_note_filtered: {
            plan.dispatched = true;
            plan.drop = false;
            plan.send_note_server = true;
            plan.effects.push_back({S::SendNoteServer, a.protocol, a.object_id, 0u, true});
            return plan;
        }
        case K::send_note_by_id:
        case K::forward_to_user:
        case K::delete_all_with_ack:
        case K::list_notes:
        case K::read_note: {
            // Server-side only handles SENDNOTE_SYN -> send_note_filtered;
            // any other kind emitted (legacy quirk) silently drops.
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.protocol, a.object_id, 0u, false});
            return plan;
        }
    }
    return plan;
}

}  // namespace mxh::server

