#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_GTOURNAMENT (MP_GTOURNAMENT=59).
inline constexpr std::uint8_t gtournament_category=59;
// Sub-protocols used by Agent routing (subset).
inline constexpr std::uint8_t gtournament_movetobattlemap_syn=7,gtournament_movetobattlemap_nack=9;
inline constexpr std::uint8_t gtournament_standinginfo_syn=18,gtournament_standinginfo_nack=20;
inline constexpr std::uint8_t gtournament_battlejoin_syn=13,gtournament_observerjoin_syn=10;
inline constexpr std::uint8_t gtournament_battlejoin_nack=15;
inline constexpr std::uint8_t gtournament_leave_syn=17;
inline constexpr std::uint8_t gtournament_cheat=41;
inline constexpr std::uint8_t gtournament_event_start=43,gtournament_event_end=46;
// eGTError_ERROR code written into dwData when NACK occurs.
inline constexpr std::uint32_t gt_error_code_error=0;
// Tournament map number constant (#define GTMAPNUM 28).
inline constexpr std::uint16_t gt_map_num=28;
enum class GtournamentActionKind : std::uint8_t { forward_to_map_server, send_movetobattle_to_user_map, send_standing_info_to_gt_map, send_battlejoin_nack_to_user, send_standing_info_nack_to_user, send_movetobattle_nack_to_user, send_leave_syn_to_user_map, send_cheat_to_user_map, send_cheat_to_gt_map, send_event_to_gt_map, drop_no_user };
struct GtournamentRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; bool user_found=true; bool gt_map_found=false; bool user_map_found=false; std::uint8_t user_level=0; std::uint32_t guild_idx=0; std::uint32_t battle_idx=0; std::uint32_t return_map_num=0; std::uint32_t cheat_data=0; std::uint16_t wdata=0; std::uint32_t unique_connect_idx=0; };
struct GtournamentAction { GtournamentActionKind kind=GtournamentActionKind::drop_no_user; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t error_code=0; std::uint16_t target_map=0; };
GtournamentAction classify_gtournament_user(const GtournamentRequest&);
}
