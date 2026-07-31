#pragma once
#include "mxh/server/murimnet_player.hpp"
#include "mxh/server/murimnet_play_room_manager.hpp"
namespace mxh::server {
enum class MurimNetRuntimeStatus : std::uint8_t { Ok=0, InvalidPlayer=1, DuplicatePlayer=2, CapacityFull=3, PlayerNotFound=4, RoomNotFound=5, RoomStarted=6, NotInRoom=7 };
class MurimNetRuntime final { public: bool init(std::uint32_t max_players,std::uint32_t max_rooms); void release() noexcept; MurimNetRuntimeStatus connect(const MurimNetPlayerInfo&); MurimNetRuntimeStatus reconnect(std::uint32_t); MurimNetRuntimeStatus transport_disconnected(std::uint32_t); MurimNetRuntimeStatus enter_default_room(std::uint32_t); MurimNetRuntimeStatus disconnect(std::uint32_t); MurimNetRuntimeStatus game_logout(std::uint32_t,std::uint32_t); MurimNetPlayerManager& players() noexcept{return m_players;} MurimNetPlayRoomManager& rooms() noexcept{return m_rooms;} private: MurimNetPlayerManager m_players; MurimNetPlayRoomManager m_rooms; };
}

