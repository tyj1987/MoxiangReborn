#include "mxh/server/agent_note.hpp"
namespace mxh::server {
// MP_NOTEMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 2352-2396.
NoteAction classify_note_user(const NoteRequest& r){
    switch(r.protocol){
    case note_sendnote_syn:
        if(!r.user_found){return {NoteActionKind::drop_no_user,note_sendnote_syn,r.object_id};}
        if(r.from_invalid||r.to_invalid){return {NoteActionKind::drop_with_invalid_filter,note_sendnote_syn,r.object_id};}
        if(r.note_has_quote){return {NoteActionKind::drop_with_invalid_filter,note_sendnote_syn,r.object_id};}
        return {NoteActionKind::send_note_filtered,note_sendnote_syn,r.object_id};
    case note_sendnoteid_syn:return {NoteActionKind::send_note_by_id,note_sendnoteid_syn,r.object_id};
    case note_receivenote:return r.user_found?NoteAction{NoteActionKind::forward_to_user,note_receivenote,r.object_id}:NoteAction{NoteActionKind::drop_no_user,note_receivenote,r.object_id};
    case note_delallnote_syn:return r.user_found?NoteAction{NoteActionKind::delete_all_with_ack,note_delallnote_syn,r.object_id}:NoteAction{NoteActionKind::drop_no_user,note_delallnote_syn,r.object_id};
    case note_notelist_syn:return {NoteActionKind::list_notes,note_notelist_syn,r.object_id};
    case note_readnote_syn:return r.user_found?NoteAction{NoteActionKind::read_note,note_readnote_syn,r.object_id}:NoteAction{NoteActionKind::drop_no_user,note_readnote_syn,r.object_id};
    case note_delnote_syn:return r.user_found?NoteAction{NoteActionKind::read_note,note_delnote_syn,r.object_id}:NoteAction{NoteActionKind::drop_no_user,note_delnote_syn,r.object_id};
    default:return {NoteActionKind::drop_no_user,r.protocol,r.object_id};
    }
}
// MP_NOTEServerMsgParser routes only SENDNOTE_SYN with the same name-filter logic.
NoteAction classify_note_server(const NoteRequest& r){
    switch(r.protocol){
    case note_sendnote_syn:
        if(!r.user_found){return {NoteActionKind::drop_no_user,note_sendnote_syn,r.object_id};}
        if(r.from_invalid||r.to_invalid){return {NoteActionKind::drop_with_invalid_filter,note_sendnote_syn,r.object_id};}
        return {NoteActionKind::send_note_filtered,note_sendnote_syn,r.object_id};
    default:return {NoteActionKind::drop_no_user,r.protocol,r.object_id};
    }
}
}
[[maybe_unused]] constexpr int agent_note_translation_unit_anchor=0;
