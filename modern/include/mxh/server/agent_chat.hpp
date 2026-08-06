#pragma once
#include <cstdint>
namespace mxh::server {
enum class ChatRoute : std::uint8_t { broadcast, whisper, party, guild, guild_union, shout_server, fast_chat, rejected };
inline constexpr std::uint8_t chat_all=0,chat_smallshout=1,chat_gm_smallshout=2,chat_monster_speech=3,chat_whisper_syn=4,chat_whisper_gm_syn=5,chat_party=6,chat_guild=7,chat_guild_union=8,chat_shout_send_server=9,chat_fastchat=10;
inline constexpr std::uint8_t chat_whisper_ack=12,chat_whisper_nack=13;
struct ChatDispatch { ChatRoute route=ChatRoute::rejected; bool requires_target=false; bool gm_only=false; };
ChatDispatch classify_chat(std::uint8_t protocol,bool from_user,bool sender_is_gm=false);
}