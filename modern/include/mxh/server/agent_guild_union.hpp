#pragma once
#include <cstdint>
#include <optional>
namespace mxh::server {
// MP_CATEGORY byte for MP_GUILD_UNION (MP_GUILD_UNION=61).
inline constexpr std::uint8_t guild_union_category=61;
// User-side sub-protocols (offset 0..N from MP_PROTOCOL_GUILD_UNION).
inline constexpr std::uint8_t guild_union_create_syn=2,guild_union_create_nack=4;
// Server-side notify sub-protocols (broadcast 2MapServerExceptOne).
inline constexpr std::uint8_t guild_union_create_notify_to_map=20;
inline constexpr std::uint8_t guild_union_destroy_notify_to_map=21;
inline constexpr std::uint8_t guild_union_invite_accept_notify_to_map=22;
inline constexpr std::uint8_t guild_union_add_notify_to_map=23;
inline constexpr std::uint8_t guild_union_remove_notify_to_map=24;
inline constexpr std::uint8_t guild_union_secede_notify_to_map=25;
inline constexpr std::uint8_t guild_union_mark_regist_notify_to_map=26;
// Legacy eGU_Not_ValidName error code (from #define).
inline constexpr std::uint32_t guild_union_err_not_valid_name=1;
enum class GuildUnionActionKind : std::uint8_t { forward_to_map, send_create_nack_to_user, drop_no_user };
struct GuildUnionRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; bool user_found=true; bool name_usable=true; bool has_invalid_char=false; };
struct GuildUnionAction { GuildUnionActionKind kind=GuildUnionActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t error_code=0; };
GuildUnionAction classify_guild_union_user(const GuildUnionRequest&);
enum class GuildUnionServerActionKind : std::uint8_t { broadcast_to_other_maps, default_forward_to_client, drop_unknown };
struct GuildUnionServerRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; };
struct GuildUnionServerAction { GuildUnionServerActionKind kind=GuildUnionServerActionKind::default_forward_to_client; std::uint8_t protocol=0; };
GuildUnionServerAction classify_guild_union_server(const GuildUnionServerRequest&);
}
