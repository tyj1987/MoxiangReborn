#include "mxh/server/murimnet_play_room_manager.hpp"
#include <algorithm>
namespace mxh::server {
bool MurimNetPlayRoomManager::init(std::uint32_t maxRooms, const MnPlayRoomCreateInfo& def) {
    release();
    if (maxRooms == 0) return false;
    m_maxRooms = maxRooms;
    auto info = def;
    if (info.room_index == 0) {
        info.title = "Default PlayRoom";
        info.max_players_per_team = 8;
        info.max_observers = 0;
    }
    m_defaultRoom = create_room(info);
    return m_defaultRoom != nullptr;
}
void MurimNetPlayRoomManager::release() noexcept {
    for (auto& r : m_rooms) if (r) r->release();
    m_rooms.clear();
    m_defaultRoom = nullptr;
    m_maxRooms = 0;
}
std::uint32_t MurimNetPlayRoomManager::allocate_index() const noexcept {
    for (std::uint32_t id = 1; id <= m_maxRooms; ++id) {
        if (!get_room(id)) return id;
    }
    return 0;
}
MurimNetPlayRoom* MurimNetPlayRoomManager::create_room(MnPlayRoomCreateInfo info) {
    if (m_maxRooms == 0 || m_rooms.size() >= m_maxRooms) return nullptr;
    if (info.room_index == 0) info.room_index = allocate_index();
    if (info.room_index == 0 || get_room(info.room_index)) return nullptr;
    auto room = std::make_unique<MurimNetPlayRoom>();
    if (!room->create(info)) return nullptr;
    auto* result = room.get();
    m_rooms.push_back(std::move(room));
    return result;
}
bool MurimNetPlayRoomManager::delete_room(std::uint32_t id) {
    auto it = std::find_if(m_rooms.begin(), m_rooms.end(), [&](const auto& r) { return r && r->room_index() == id; });
    if (it == m_rooms.end()) return false;
    if (it->get() == m_defaultRoom) m_defaultRoom = nullptr;
    (*it)->release();
    m_rooms.erase(it);
    return true;
}
MurimNetPlayRoom* MurimNetPlayRoomManager::get_room(std::uint32_t id) noexcept {
    for (auto& r : m_rooms) if (r && r->room_index() == id) return r.get();
    return nullptr;
}
const MurimNetPlayRoom* MurimNetPlayRoomManager::get_room(std::uint32_t id) const noexcept {
    for (const auto& r : m_rooms) if (r && r->room_index() == id) return r.get();
    return nullptr;
}
void MurimNetPlayRoomManager::for_each_room(const std::function<void(const MurimNetPlayRoom&)>& fn) const {
    for (const auto& r : m_rooms) if (r && fn) fn(*r);
}
bool MurimNetPlayRoomManager::enter_room(MnRoomPlayer& p, std::uint32_t id) {
    auto* room = get_room(id);
    return room && room->player_in(p);
}
bool MurimNetPlayRoomManager::exit_room(MnRoomPlayer& p) {
    if (p.location_index == 0) return false;
    auto* room = get_room(p.location_index);
    return room && room->player_out(p);
}
}
