#pragma once
#include "mxh/server/murimnet_play_room.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
namespace mxh::server {
struct MurimNetPlayerInfo { std::uint32_t player_id=0; std::uint32_t agent_num=0; std::uint32_t unique_id_in_agent=0; std::uint32_t back_map_num=0; std::string name; std::uint16_t level=0; };
class MurimNetPlayer final { public: bool init(const MurimNetPlayerInfo&) noexcept; const MurimNetPlayerInfo& info() const noexcept{return m_info;} std::uint32_t id() const noexcept{return m_info.player_id;} MnRoomPlayer& room_state() noexcept{return m_roomState;} const MnRoomPlayer& room_state() const noexcept{return m_roomState;} bool connected() const noexcept{return m_connected;} void set_connected(bool value) noexcept{m_connected=value;} private: MurimNetPlayerInfo m_info{}; MnRoomPlayer m_roomState{}; bool m_connected=false; };
class MurimNetPlayerManager final { public: bool init(std::uint32_t max_players) noexcept; void release() noexcept; MurimNetPlayer* add_player(const MurimNetPlayerInfo&); bool delete_player(std::uint32_t); MurimNetPlayer* find_player(std::uint32_t) noexcept; const MurimNetPlayer* find_player(std::uint32_t) const noexcept; std::size_t player_count() const noexcept{return m_players.size();} std::uint32_t max_players() const noexcept{return m_maxPlayers;} private: std::uint32_t m_maxPlayers=0; std::unordered_map<std::uint32_t,std::unique_ptr<MurimNetPlayer>> m_players; };
}

