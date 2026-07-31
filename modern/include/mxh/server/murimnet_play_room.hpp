#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace mxh::server {
enum class MnTeam : std::uint8_t { Left=0, Right=1, Observer=2, Max=3 };
enum class MnPlayerLocation : std::uint8_t { None=0, Channel=1, PlayRoom=2, Game=3, Result=4 };
struct MnRoomPlayer { std::uint32_t id=0; MnTeam team=MnTeam::Left; MnPlayerLocation location=MnPlayerLocation::None; std::uint32_t location_index=0; bool captain=false; };
struct MnPlayRoomCreateInfo { std::uint32_t room_index=0; std::string title; std::uint8_t kind=0; std::uint16_t max_players_per_team=0; std::uint16_t max_observers=0; std::uint32_t money_for_play=0; std::uint16_t map_num=0; };
class MurimNetPlayRoom final { public: bool create(const MnPlayRoomCreateInfo&); void release() noexcept; bool player_in(MnRoomPlayer&); bool player_out(MnRoomPlayer&); bool team_change(MnRoomPlayer&,MnTeam,MnTeam); void play_start(bool) noexcept; bool started() const noexcept{return m_started;} std::size_t player_count() const noexcept{return m_players.size();} std::size_t team_count(MnTeam) const noexcept; bool contains(std::uint32_t) const noexcept; std::uint32_t room_index() const noexcept{return m_info.room_index;} const std::string& title() const noexcept{return m_info.title;} private: MnPlayRoomCreateInfo m_info{}; std::vector<MnRoomPlayer*> m_players; std::array<std::vector<MnRoomPlayer*>,3> m_teams; bool m_started=false; };
}
