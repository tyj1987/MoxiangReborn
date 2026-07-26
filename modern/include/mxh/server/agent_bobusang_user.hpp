#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_BOBUSANG (MP_BOBUSANG=73).
inline constexpr std::uint8_t bobusang_user_category=73;
enum class BobusangUserActionKind : std::uint8_t { forward_to_map, drop_no_user, drop_wrong_gm_power };
struct BobusangUserRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; bool user_found=true; bool is_gm=false; bool gm_master_or_below=true; };
struct BobusangUserAction { BobusangUserActionKind kind=BobusangUserActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; };
BobusangUserAction classify_bobusang_user(const BobusangUserRequest&);
}
