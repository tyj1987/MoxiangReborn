
#pragma once
#include <cstdint>
#include <string>
namespace mxh::server {
inline constexpr std::uint8_t guild_category=63;
inline constexpr std::uint8_t guild_create_syn=2,guild_givenickname_syn=20;
inline constexpr std::uint8_t guild_givenickname_nack=21,guild_create_nack=3;
inline constexpr std::uint8_t guild_err_create_name=4,guild_err_nick_filter=1;
enum class GuildActionKind : std::uint8_t { forward, send_nack };
struct GuildUserRequest { std::uint32_t object_id=0; bool usable_name=true; bool has_invalid_char=false; bool has_quote_space=false; bool is_nickname_path=false; };
struct GuildAction { GuildActionKind kind=GuildActionKind::forward; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint8_t error_code=0; };
GuildAction classify_guild_user(const GuildUserRequest& req);
GuildAction classify_guild_server_default(std::uint8_t protocol);
}
