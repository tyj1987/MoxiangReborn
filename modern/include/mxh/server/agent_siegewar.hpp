#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_SIEGEWAR (MP_SIEGEWAR=62).
inline constexpr std::uint8_t siegewar_category=62;
// Sub-protocols within MP_PROTOCOL_SIEGEWAR (offset 0..N).
inline constexpr std::uint8_t siegewar_movein_syn=1,siegewar_movein_nack=3;
inline constexpr std::uint8_t siegewar_battlejoin_syn=7,siegewar_battlejoin_nack=9,siegewar_observerjoin_syn=10,siegewar_observerjoin_nack=11;
inline constexpr std::uint8_t siegewar_leave_syn=12;
inline constexpr std::uint8_t siegewar_cheat=61;
enum class SiegeWarUserActionKind : std::uint8_t { cheat_fanout_to_map_servers, movein_to_user_map, battlejoin_to_target_map_or_nack, leave_syn_to_user_map, default_forward_to_map, drop_no_user };
struct SiegeWarUserRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t unique_connect_idx=0; std::uint32_t guild_idx=0; std::uint32_t return_map_num=0; std::uint8_t observer_flag=0; std::uint32_t target_map=0; bool user_found=true; bool target_map_found=false; };
struct SiegeWarUserAction { SiegeWarUserActionKind kind=SiegeWarUserActionKind::default_forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t unique_connect_idx=0; std::uint32_t data2_target_map=0; std::uint32_t data3_target_map=0; std::uint32_t guild_idx=0; std::uint32_t return_map_num=0; std::uint8_t observer_flag=0; std::uint8_t user_level=0; std::uint8_t channel=0; };
SiegeWarUserAction classify_siegewar_user(const SiegeWarUserRequest&);
}
