#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_MUGONG (MP_MUGONG=9 in MP_CATEGORY, 1-based position 9).
inline constexpr std::uint8_t mugong_category=9;
// Sub-protocols within MP_PROTOCOL_MUGONG (offset 0..25 from MP_PROTOCOL_MUGONG enum).
inline constexpr std::uint8_t mugong_totalinfo_local=0;
inline constexpr std::uint8_t mugong_move_syn=1,mugong_move_ack=2,mugong_move_nack=3;
inline constexpr std::uint8_t mugong_rem_syn=4,mugong_rem_ack=5,mugong_rem_nack=6;
inline constexpr std::uint8_t mugong_add_syn=7,mugong_add_ack=8,mugong_add_nack=9;
inline constexpr std::uint8_t mugong_deletegroundadd_syn=10,mugong_deletegroundadd_ack=11,mugong_deletegroundadd_nack=12;
inline constexpr std::uint8_t mugong_deleteinventoryadd_syn=13,mugong_deleteinventoryadd_ack=14,mugong_deleteinventoryadd_nack=15;
inline constexpr std::uint8_t mugong_exppoint_notify=16;
inline constexpr std::uint8_t mugong_sung_notify=17;
inline constexpr std::uint8_t mugong_sung_levelup=18;
inline constexpr std::uint8_t mugong_option_syn=19,mugong_option_ack=20,mugong_option_nack=21;
inline constexpr std::uint8_t mugong_option_clear_syn=22,mugong_option_clear_ack=23,mugong_option_clear_nack=24;
enum class MugongActionKind : std::uint8_t { forward_to_map, forward_to_map_if_level_ok, send_nack };
struct MugongRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t mugong_index=0; std::uint32_t mugong_level=0; std::uint32_t required_level=0; };
struct MugongAction { MugongActionKind kind=MugongActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint8_t error_code=0; };
MugongAction classify_mugong(const MugongRequest&);
}
