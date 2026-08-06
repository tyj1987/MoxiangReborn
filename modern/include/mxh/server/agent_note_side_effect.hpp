//
// CItemManager::MP_NOTEMsgParser / MPSendNoteMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp:2355-2504.
//
// The MP_NOTE_* handlers on the agent run a small set of operations
// against a per-player note list. The flow per handler:
//   MP_NOTE_SENDNOTE_SYN:
//     1. FindUser(pmsg->FromId) -> userinfo (return if null).
//     2. SafeStrCpy FromName/ToName/Note into fixed buffers.
//     3. Filter check: IsInvalidCharInclude on From/To (with KR
//        local exceptions for [ì²­ë£¡]/[í™ë£¡] prefixes); on fail,
//        return.
//     4. NoteServerSendtoPlayer(FromId, FromName, ToName, Note).
//   MP_NOTE_SENDNOTEID_SYN:
//     1. NoteSendtoPlayerID(dwObjectID, FromName, TargetID, Note).
//   MP_NOTE_RECEIVENOTE:
//     1. FindUser(pmsg->dwObjectID) -> userinfo (return if null).
//     2. Send2User(userinfo->dwConnectionIndex, pMsg, sizeof(MSGBASE)).
//   MP_NOTE_DELALLNOTE_SYN:
//     1. NoteDelAll(pmsg->dwObjectID).
//     2. FindUser(dwObjectID) -> userinfo (return if null).
//     3. Send2User DELALLNOTE_ACK.
//   MP_NOTE_NOTELIST_SYN:
//     1. NoteList(pmsg->dwObjectID, wData1, wData2).
//   MP_NOTE_READNOTE_SYN:
//     1. FindUser(pmsg->dwObjectID) -> userinfo (return if null).
//     2. NoteRead(dwObjectID, dwData1, dwData2).
//   MP_NOTE_DELNOTE_SYN:
//     1. If bLast == 0 or bLast == 1: NoteDelete(dwObjectID, NoteID,
//        bLast).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

enum class AgentNoteAction : std::uint8_t {
    SendNoteByName,
    SendNoteById,
    ReceiveNote,
    DelAllNote,
    ListNote,
    ReadNote,
    DeleteNote,
};

enum class AgentNoteOutcome : std::uint8_t {
    Dispatched = 0,  // legacy: action performed
    NoUser     = 1,  // legacy: FindUser returned null
    Filtered   = 2,  // legacy: name contains invalid char
    Invalid    = 3,  // legacy: bLast out of {0, 1}
};

struct AgentNoteValidationInput final {
    AgentNoteAction action = AgentNoteAction::SendNoteByName;
    bool user_found = false;
    bool filter_passed = false;     // SendNoteByName gate (else Filtered)
    bool b_last_valid = false;      // DeleteNote gate
};

inline AgentNoteOutcome classify_agent_note_outcome(
    const AgentNoteValidationInput& in) noexcept {
    if (in.action == AgentNoteAction::SendNoteByName) {
        if (!in.user_found) return AgentNoteOutcome::NoUser;
        if (!in.filter_passed) return AgentNoteOutcome::Filtered;
        return AgentNoteOutcome::Dispatched;
    }
    if (in.action == AgentNoteAction::DeleteNote) {
        if (!in.b_last_valid) return AgentNoteOutcome::Invalid;
        return AgentNoteOutcome::Dispatched;
    }
    if (in.action == AgentNoteAction::SendNoteById) {
        return AgentNoteOutcome::Dispatched;
    }
    if (in.action == AgentNoteAction::ListNote) {
        return AgentNoteOutcome::Dispatched;
    }
    if (!in.user_found) return AgentNoteOutcome::NoUser;
    return AgentNoteOutcome::Dispatched;
}

enum class AgentNoteSideEffectKind : std::uint8_t {
    CopyNoteBuffers       = 0,  // legacy SafeStrCpy of FromName/ToName/Note
    FilterCheckNote       = 1,  // legacy FILTERTABLE->IsInvalidCharInclude
    NoteServerSendtoPlayer = 2, // legacy NoteServerSendtoPlayer
    NoteSendtoPlayerID    = 3,  // legacy NoteSendtoPlayerID
    Send2UserPassthrough  = 4,  // legacy g_Network.Send2User (RECEIVE)
    SendDelAllAckToUser   = 5,  // legacy g_Network.Send2User DELALLNOTE_ACK
    NoteDelAll            = 6,  // legacy NoteDelAll
    NoteList              = 7,  // legacy NoteList
    NoteRead              = 8,  // legacy NoteRead
    NoteDelete            = 9,  // legacy NoteDelete
};

struct AgentNoteSideEffect final {
    AgentNoteSideEffectKind kind =
        AgentNoteSideEffectKind::CopyNoteBuffers;
    std::uint32_t object_id = 0;
    std::uint16_t list_page = 0;
    std::uint16_t list_slot = 0;
    std::uint32_t note_id = 0;
    std::uint8_t  b_last = 0;
};

struct AgentNoteSideEffectPlan final {
    std::vector<AgentNoteSideEffect> effects;
    bool dispatched = false;
    bool filter_passed = false;
    bool b_last_valid = false;
};

inline AgentNoteSideEffectPlan agent_note_side_effect_plan(
    const AgentNoteValidationInput& in,
    std::uint32_t object_id,
    std::uint16_t list_page,
    std::uint16_t list_slot,
    std::uint32_t note_id,
    std::uint8_t b_last) {
    AgentNoteSideEffectPlan plan;
    const AgentNoteOutcome outcome =
        classify_agent_note_outcome(in);
    if (outcome != AgentNoteOutcome::Dispatched) {
        return plan;
    }
    plan.dispatched = true;
    plan.filter_passed = in.filter_passed;
    plan.b_last_valid = in.b_last_valid;

    switch (in.action) {
        case AgentNoteAction::SendNoteByName:
            plan.effects.reserve(3u);
            {
                AgentNoteSideEffect cp{};
                cp.kind = AgentNoteSideEffectKind::CopyNoteBuffers;
                cp.object_id = object_id;
                plan.effects.push_back(cp);
            }
            {
                AgentNoteSideEffect fc{};
                fc.kind = AgentNoteSideEffectKind::FilterCheckNote;
                fc.object_id = object_id;
                plan.effects.push_back(fc);
            }
            {
                AgentNoteSideEffect sn{};
                sn.kind = AgentNoteSideEffectKind::NoteServerSendtoPlayer;
                sn.object_id = object_id;
                plan.effects.push_back(sn);
            }
            return plan;
        case AgentNoteAction::SendNoteById:
            plan.effects.reserve(1u);
            {
                AgentNoteSideEffect sn{};
                sn.kind = AgentNoteSideEffectKind::NoteSendtoPlayerID;
                sn.object_id = object_id;
                plan.effects.push_back(sn);
            }
            return plan;
        case AgentNoteAction::ReceiveNote:
            plan.effects.reserve(1u);
            {
                AgentNoteSideEffect snd{};
                snd.kind = AgentNoteSideEffectKind::Send2UserPassthrough;
                snd.object_id = object_id;
                plan.effects.push_back(snd);
            }
            return plan;
        case AgentNoteAction::DelAllNote:
            plan.effects.reserve(2u);
            {
                AgentNoteSideEffect da{};
                da.kind = AgentNoteSideEffectKind::NoteDelAll;
                da.object_id = object_id;
                plan.effects.push_back(da);
            }
            {
                AgentNoteSideEffect ack{};
                ack.kind = AgentNoteSideEffectKind::SendDelAllAckToUser;
                ack.object_id = object_id;
                plan.effects.push_back(ack);
            }
            return plan;
        case AgentNoteAction::ListNote:
            plan.effects.reserve(1u);
            {
                AgentNoteSideEffect ls{};
                ls.kind = AgentNoteSideEffectKind::NoteList;
                ls.object_id = object_id;
                ls.list_page = list_page;
                ls.list_slot = list_slot;
                plan.effects.push_back(ls);
            }
            return plan;
        case AgentNoteAction::ReadNote:
            plan.effects.reserve(1u);
            {
                AgentNoteSideEffect rd{};
                rd.kind = AgentNoteSideEffectKind::NoteRead;
                rd.object_id = object_id;
                rd.note_id = note_id;
                rd.list_page = list_page;
                plan.effects.push_back(rd);
            }
            return plan;
        case AgentNoteAction::DeleteNote:
            plan.effects.reserve(1u);
            {
                AgentNoteSideEffect dl{};
                dl.kind = AgentNoteSideEffectKind::NoteDelete;
                dl.object_id = object_id;
                dl.note_id = note_id;
                dl.b_last = b_last;
                plan.effects.push_back(dl);
            }
            return plan;
    }
    return plan;
}

}  // namespace mxh::server

