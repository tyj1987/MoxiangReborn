#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_GUILD_UNION (MP_GUILD_UNION=61).
inline constexpr std::uint8_t guild_union_category=61;
// Sub-protocol within MP_PROTOCOL_GUILD_UNION (offset 0..N).
inline constexpr std::uint8_t guild_union_create_syn=2,guild_union_create_nack=4;
// Legacy eGU_Not_ValidName error code (from #define).
inline constexpr std::uint32_t guild_union_err_not_valid_name=1;
enum class GuildUnionActionKind : std::uint8_t { forward_to_map, send_create_nack_to_user, drop_no_user };
struct GuildUnionRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; bool user_found=true; bool name_usable=true; bool has_invalid_char=false; };
struct GuildUnionAction { GuildUnionActionKind kind=GuildUnionActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t error_code=0; };
GuildUnionAction classify_guild_union_user(const GuildUnionRequest&);
}
