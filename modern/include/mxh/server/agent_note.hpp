#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_NOTE (1-based, MP_NOTE=58 in MP_CATEGORY).
inline constexpr std::uint8_t note_category=58;
// Sub-protocols within MP_PROTOCOL_NOTE (offset 0..17 from MP_PROTOCOL_NOTE enum).
inline constexpr std::uint8_t note_sendnote_syn=0,note_sendnote_ack=1,note_sendnote_nack=2;
inline constexpr std::uint8_t note_sendnoteid_syn=3;
inline constexpr std::uint8_t note_receivenote=4;
inline constexpr std::uint8_t note_delnote_syn=5,note_delnote_ack=6,note_delnote_nack=7;
inline constexpr std::uint8_t note_delallnote_syn=8,note_delallnote_ack=9,note_delallnote_nack=10;
inline constexpr std::uint8_t note_notelist_syn=11,note_notelist_ack=12,note_notelist_nack=13;
inline constexpr std::uint8_t note_readnote_syn=14,note_readnote_ack=15,note_readnote_nack=16;
inline constexpr std::uint8_t note_new_note=17;
enum class NoteActionKind : std::uint8_t { send_note_filtered, send_note_by_id, forward_to_user, delete_all_with_ack, list_notes, read_note, drop_no_user, drop_with_invalid_filter };
struct NoteRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t target_id=0; bool user_found=true; bool from_invalid=false; bool to_invalid=false; bool note_has_quote=false; };
struct NoteAction { NoteActionKind kind=NoteActionKind::drop_no_user; std::uint8_t protocol=0; std::uint32_t object_id=0; };
NoteAction classify_note_user(const NoteRequest&);
NoteAction classify_note_server(const NoteRequest&);
}
