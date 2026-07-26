#pragma once
#include <cstdint>
#include <span>
#include <vector>
namespace mxh::server {
// MP_CATEGORY byte for MP_PACKEDDATA (1-based, MP_PACKEDDATA=13 in MP_CATEGORY).
inline constexpr std::uint8_t packed_category=13;
// Sub-protocols within MP_PROTOCOL_PACKEDDATA (MP_PACKEDDATA_NORMAL=0 ... TOBROADMAPSERVER=2).
inline constexpr std::uint8_t packed_normal=0;
inline constexpr std::uint8_t packed_to_mapserver=1;
inline constexpr std::uint8_t packed_to_broad_mapserver=2;
enum class PackedActionKind : std::uint8_t { fanout_to_users, send_to_map_server_by_port, broadcast_to_other_maps, unknown };
struct PackedRequest { std::uint8_t protocol=0; std::uint16_t receiver_count=0; std::uint16_t data_size=0; std::uint16_t target_map_num=0; bool target_map_port_found=false; std::vector<std::uint32_t> receivers_present{}; };
struct PackedAction { PackedActionKind kind=PackedActionKind::unknown; std::uint8_t protocol=0; std::uint32_t target_object_id=0; std::size_t receiver_count=0; std::uint16_t data_size=0; };
PackedAction classify_packed_user(const PackedRequest&);
}
