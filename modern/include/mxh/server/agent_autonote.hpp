#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_AUTONOTE (1-based, MP_AUTONOTE=75).
inline constexpr std::uint8_t autonote_category=75;
// Sub-protocols within MP_PROTOCOL_AUTONOTE (offset 0..16).
inline constexpr std::uint8_t autonote_asktoauto_syn=0,autonote_asktoauto_ack=1,autonote_asktoauto_nack=2;
inline constexpr std::uint8_t autonote_asktoauto=3;
inline constexpr std::uint8_t autonote_answer_syn=4,autonote_answer_ack=5,autonote_answer_nack=6;
inline constexpr std::uint8_t autonote_answer_fail=7,autonote_answer_timeout=8,autonote_answer_logout=9;
inline constexpr std::uint8_t autonote_notauto=10;
inline constexpr std::uint8_t autonote_killauto=11,autonote_disconnect=12;
inline constexpr std::uint8_t autonote_list_add=13,autonote_list_all=14;
inline constexpr std::uint8_t autonote_punish=15;
inline constexpr std::uint8_t autonote_asktoauto_image=16;
// GM-level block: GMs can use autonote without restriction.
// Legacy code: UserLevel > eUSERLEVEL_GM is unrestricted; == eUSERLEVEL_GM goes through punish check too.
inline constexpr std::uint8_t user_level_gm=8;
// 2-minute punish window on successful ask.
inline constexpr std::uint32_t autonote_punish_seconds_ask=120;
enum class AutonoteUserActionKind : std::uint8_t { forward_to_map, send_punish_to_user, drop_no_user };
enum class AutonoteServerActionKind : std::uint8_t { asktoauto_ack_send_and_punish, notauto_punish_and_send_to_user_if_character, answer_ack_punish_other, answer_fail_punish_count, answer_logout_punish_count, answer_timeout_punish_count_and_send, killauto_send_if_character, disconnect_if_user, forward_to_user_if_found, drop_no_user };
struct AutonoteUserRequest { std::uint8_t protocol=0; std::uint32_t connection_index=0; std::uint32_t character_id=0; bool user_found=true; std::uint8_t user_level=0; std::uint32_t punish_remaining_seconds=0; };
struct AutonoteServerRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t user_id=0; bool user_object_found=false; bool user_id_found=false; bool user_has_character=false; std::uint32_t auto_note_use_minutes=0; };
struct AutonoteUserAction { AutonoteUserActionKind kind=AutonoteUserActionKind::drop_no_user; std::uint8_t protocol=0; std::uint32_t connection_index=0; std::uint32_t punish_seconds=0; std::uint32_t object_id=0; };
struct AutonoteServerAction { AutonoteServerActionKind kind=AutonoteServerActionKind::drop_no_user; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t user_id=0; std::uint32_t punish_seconds=0; bool disconnect=false; };
AutonoteUserAction classify_autonote_user(const AutonoteUserRequest&);
AutonoteServerAction classify_autonote_server(const AutonoteServerRequest&);
}
