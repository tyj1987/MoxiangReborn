#pragma once
#include "mxh/server/murimnet_play_room.hpp"
#include <cstdint>
#include <memory>
#include <vector>
namespace mxh::server { class MurimNetPlayRoomManager final { public: bool init(std::uint32_t max_rooms,const MnPlayRoomCreateInfo& default_info={}); void release() noexcept; MurimNetPlayRoom* create_room(MnPlayRoomCreateInfo info); bool delete_room(std::uint32_t); MurimNetPlayRoom* get_room(std::uint32_t) noexcept; const MurimNetPlayRoom* get_room(std::uint32_t) const noexcept; bool enter_room(MnRoomPlayer&,std::uint32_t); bool exit_room(MnRoomPlayer&); std::size_t room_count() const noexcept{return m_rooms.size();} std::uint32_t max_rooms() const noexcept{return m_maxRooms;} MurimNetPlayRoom* default_room() const noexcept{return m_defaultRoom;} private: std::uint32_t allocate_index() const noexcept; std::uint32_t m_maxRooms=0; std::vector<std::unique_ptr<MurimNetPlayRoom>> m_rooms; MurimNetPlayRoom* m_defaultRoom=nullptr; }; }

