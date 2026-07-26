
#include "mxh/server/agent_guild.hpp"
namespace mxh::server {
GuildAction classify_guild_user(const GuildUserRequest& req){if(req.has_invalid_char||!req.usable_name){return {GuildActionKind::send_nack,req.is_nickname_path?guild_givenickname_nack:guild_create_nack,req.object_id,req.is_nickname_path?guild_err_nick_filter:guild_err_create_name};}return {GuildActionKind::forward,static_cast<std::uint8_t>(req.is_nickname_path?guild_givenickname_syn:guild_create_syn),req.object_id,0};}
GuildAction classify_guild_server_default(std::uint8_t p){return {GuildActionKind::forward,p,0,0};}
}
[[maybe_unused]] constexpr int agent_guild_translation_unit_anchor=0;
