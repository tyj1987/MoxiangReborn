#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_BATTLE (MP_BATTLE=31 in MP_CATEGORY, position 31 1-based).
inline constexpr std::uint8_t battle_category=31;
// Sub-protocols within MP_PROTOCOL_BATTLE (offset 0..42 from MP_PROTOCOL_BATTLE enum).
inline constexpr std::uint8_t battle_info=0;
inline constexpr std::uint8_t battle_chat_team_syn=1,battle_chat_team_ack=2,battle_chat_team_nack=3;
inline constexpr std::uint8_t battle_chat_master_syn=4,battle_chat_master_ack=5,battle_chat_master_nack=6;
inline constexpr std::uint8_t battle_start_notify=7;
inline constexpr std::uint8_t battle_teammember_add_notify=8;
inline constexpr std::uint8_t battle_teammember_delete_notify=9;
inline constexpr std::uint8_t battle_teammember_die_notify=10;
inline constexpr std::uint8_t battle_battleobject_destroy_notify=11;
inline constexpr std::uint8_t battle_battleobject_create_notify=12;
inline constexpr std::uint8_t battle_victory_notify=13;
inline constexpr std::uint8_t battle_draw_notify=14;
inline constexpr std::uint8_t battle_destroy_notify=15;
inline constexpr std::uint8_t battle_result=16;
inline constexpr std::uint8_t battle_change_objectbattle=17;
inline constexpr std::uint8_t battle_vimu_request_syn=18,battle_vimu_request_ack=19,battle_vimu_request_nack=20;
inline constexpr std::uint8_t battle_vimu_start=21,battle_vimu_end=22;
inline constexpr std::uint8_t battle_vimu_apply=23;
inline constexpr std::uint8_t battle_vimu_apply_syn=24,battle_vimu_apply_ack=25,battle_vimu_apply_nack=26;
inline constexpr std::uint8_t battle_vimu_waiting_cancel=27;
inline constexpr std::uint8_t battle_vimu_waiting_cancel_syn=28,battle_vimu_waiting_cancel_ack=29;
inline constexpr std::uint8_t battle_vimu_waiting_cancel_nack=30;
enum class BattleActionKind : std::uint8_t { forward_to_map, drop_protocol };
struct BattleRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; };
struct BattleAction { BattleActionKind kind=BattleActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; };
BattleAction classify_battle(const BattleRequest&);
}
