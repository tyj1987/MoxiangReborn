#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_MOVE (MP_MOVE=8 in MP_CATEGORY, 1-based position 8).
inline constexpr std::uint8_t move_category=8;
// Sub-protocols within MP_PROTOCOL_MOVE (offset 0..19 from MP_PROTOCOL_MOVE enum).
inline constexpr std::uint8_t move_init=0;
inline constexpr std::uint8_t move_target=1;
inline constexpr std::uint8_t move_correction=2;
inline constexpr std::uint8_t move_walkmode=3,move_runmode=4;
inline constexpr std::uint8_t move_kyunggong_syn=5,move_kyunggong_ack=6,move_kyunggong_nack=7;
inline constexpr std::uint8_t move_stop=8;
inline constexpr std::uint8_t move_effectmove=9;
inline constexpr std::uint8_t move_monstermove_notify=10;
inline constexpr std::uint8_t move_forcestopkyunggong=11;
inline constexpr std::uint8_t move_warp=12;
inline constexpr std::uint8_t move_onetarget=13;
inline constexpr std::uint8_t move_pet_onetarget=14;
inline constexpr std::uint8_t move_pet_target=15,move_pet_stop=16,move_pet_correction=17;
inline constexpr std::uint8_t move_pet_warp_syn=18,move_pet_warp_ack=19;
enum class MoveActionKind : std::uint8_t { forward_to_map, forward_to_map_if_in_map, drop_no_map };
struct MoveRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; bool user_in_map=true; };
struct MoveAction { MoveActionKind kind=MoveActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; };
MoveAction classify_move(const MoveRequest&);
}
