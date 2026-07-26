#pragma once
#include <cstdint>
namespace mxh::server {
// Sub-protocols for SiegeWarServer (offset from MP_PROTOCOL_SIEGEWAR).
inline constexpr std::uint8_t siegewar_taxrate=60;
inline constexpr std::uint8_t siegewar_returntomap=50;
inline constexpr std::uint8_t siegewar_flagchange=62;
enum class SiegeWarServerActionKind : std::uint8_t { broadcast_taxrate_to_affected_maps, update_user_map_and_forward_to_client, broadcast_to_all_users, default_forward_to_client, drop_no_user };
struct SiegeWarServerRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t target_map=0; bool target_map_found=false; bool user_found=true; std::uint16_t affected_count=0; };
struct SiegeWarServerAction { SiegeWarServerActionKind kind=SiegeWarServerActionKind::default_forward_to_client; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t target_map=0; };
SiegeWarServerAction classify_siegewar_server(const SiegeWarServerRequest&);
}
