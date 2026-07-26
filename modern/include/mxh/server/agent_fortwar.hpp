#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_FORTWAR (MP_FORTWAR=76).
inline constexpr std::uint8_t fortwar_category=76;
// Sub-protocols within MP_PROTOCOL_FORTWAR (offset 0..50+).
inline constexpr std::uint8_t fortwar_info=0;
inline constexpr std::uint8_t fortwar_start_before10min=1,fortwar_start=2,fortwar_ing=3,fortwar_end=4;
inline constexpr std::uint8_t fortwar_start_before10min_to_map=5,fortwar_start_to_map=6,fortwar_ing_to_map=7,fortwar_end_to_map=8;
enum class FortWarActionKind : std::uint8_t { broadcast_to_all_users, broadcast_to_other_maps, forward_to_user_if_found, drop_no_user };
struct FortWarRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; bool user_object_found=false; };
struct FortWarAction { FortWarActionKind kind=FortWarActionKind::drop_no_user; std::uint8_t protocol=0; };
FortWarAction classify_fortwar(const FortWarRequest&);
}
