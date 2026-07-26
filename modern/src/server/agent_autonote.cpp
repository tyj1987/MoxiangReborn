#include "mxh/server/agent_autonote.hpp"
namespace mxh::server {
// MP_AUTONOTEUserMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5236-5252.
AutonoteUserAction classify_autonote_user(const AutonoteUserRequest& r){
    if(!r.user_found){return {AutonoteUserActionKind::drop_no_user,r.protocol,r.connection_index,0,0};}
    if(r.protocol==autonote_asktoauto_syn){
        if(r.user_level<=user_level_gm&&r.punish_remaining_seconds>0){return {AutonoteUserActionKind::send_punish_to_user,autonote_punish,r.connection_index,r.punish_remaining_seconds,0};}
        return {AutonoteUserActionKind::forward_to_map,autonote_asktoauto_syn,r.connection_index,0,r.character_id};
    }
    return {AutonoteUserActionKind::forward_to_map,r.protocol,r.connection_index,0,r.character_id};
}
// MP_AUTONOTEServerMsgParser routing per legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5252-5290.
AutonoteServerAction classify_autonote_server(const AutonoteServerRequest& r){
    if(r.protocol==autonote_asktoauto_ack){
        if(r.user_object_found){return {AutonoteServerActionKind::asktoauto_ack_send_and_punish,autonote_asktoauto_ack,r.object_id,r.user_id,autonote_punish_seconds_ask,false};}
        return {AutonoteServerActionKind::drop_no_user,autonote_asktoauto_ack,r.object_id,r.user_id,0,false};
    }
    if(r.protocol==autonote_notauto){
        if(!r.user_id_found){return {AutonoteServerActionKind::drop_no_user,autonote_notauto,r.object_id,r.user_id,0,false};}
        return {AutonoteServerActionKind::notauto_punish_and_send_to_user_if_character,autonote_notauto,r.object_id,r.user_id,r.auto_note_use_minutes*60,false};
    }
    if(r.protocol==autonote_answer_ack){return {AutonoteServerActionKind::answer_ack_punish_other,autonote_answer_ack,r.object_id,r.user_id,r.auto_note_use_minutes*60,false};}
    if(r.protocol==autonote_answer_fail){return {AutonoteServerActionKind::answer_fail_punish_count,autonote_answer_fail,r.object_id,r.user_id,0,false};}
    if(r.protocol==autonote_answer_logout){return {AutonoteServerActionKind::answer_logout_punish_count,autonote_answer_logout,r.object_id,r.user_id,0,false};}
    if(r.protocol==autonote_answer_timeout){
        return r.user_object_found?AutonoteServerAction{AutonoteServerActionKind::answer_timeout_punish_count_and_send,autonote_answer_timeout,r.object_id,r.user_id,0,false}:AutonoteServerAction{AutonoteServerActionKind::answer_fail_punish_count,autonote_answer_timeout,r.object_id,r.user_id,0,false};
    }
    if(r.protocol==autonote_killauto){
        if(!r.user_id_found){return {AutonoteServerActionKind::drop_no_user,autonote_killauto,r.object_id,r.user_id,0,false};}
        return {AutonoteServerActionKind::killauto_send_if_character,autonote_killauto,r.object_id,r.user_id,0,false};
    }
    if(r.protocol==autonote_disconnect){
        if(!r.user_id_found){return {AutonoteServerActionKind::drop_no_user,autonote_disconnect,r.object_id,r.user_id,0,false};}
        return {AutonoteServerActionKind::disconnect_if_user,autonote_disconnect,r.object_id,r.user_id,0,true};
    }
    if(r.user_object_found){return {AutonoteServerActionKind::forward_to_user_if_found,r.protocol,r.object_id,r.user_id,0,false};}
    return {AutonoteServerActionKind::drop_no_user,r.protocol,r.object_id,r.user_id,0,false};
}
}
[[maybe_unused]] constexpr int agent_autonote_translation_unit_anchor=0;
