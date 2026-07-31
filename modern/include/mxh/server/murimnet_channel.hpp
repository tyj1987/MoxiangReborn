#pragma once
#include "mxh/server/murimnet_player.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace mxh::server {
enum class MnChannelKind : std::uint8_t { Public=0, Private=1 };
enum class MnChannelMode : std::uint8_t { Id=0, Channel=1, PlayRoom=2, Max=3 };
struct MnChannelCreateInfo { std::uint32_t channel_index=0; MnChannelKind kind=MnChannelKind::Public; std::uint16_t max_players=0; std::string title; };
class MurimNetChannel final { public: bool create(const MnChannelCreateInfo&); void release() noexcept; bool player_in(MnRoomPlayer&); bool player_out(MnRoomPlayer&); bool contains(std::uint32_t) const noexcept; std::uint32_t channel_index() const noexcept{return m_info.channel_index;} std::size_t player_count() const noexcept{return m_players.size();} std::uint16_t max_players() const noexcept{return m_info.max_players;} const std::string& title() const noexcept{return m_info.title;} MnChannelKind kind() const noexcept{return m_info.kind;} void for_each_member(const std::function<void(const MnRoomPlayer&)>&) const; private: MnChannelCreateInfo m_info{}; std::vector<MnRoomPlayer*> m_players; };
class MurimNetChannelManager final { public: bool init(std::uint32_t max_channels,const MnChannelCreateInfo& default_info={}); void release() noexcept; MurimNetChannel* create_channel(MnChannelCreateInfo); bool delete_channel(std::uint32_t); MurimNetChannel* get_channel(std::uint32_t) noexcept; const MurimNetChannel* default_channel() const noexcept{return m_defaultChannel;} bool enter_default(MnRoomPlayer&); bool exit(MnRoomPlayer&); std::size_t channel_count() const noexcept{return m_channels.size();} void for_each_channel(const std::function<void(const MurimNetChannel&)>&) const; private: std::uint32_t m_maxChannels=0; std::vector<std::unique_ptr<MurimNetChannel>> m_channels; MurimNetChannel* m_defaultChannel=nullptr; };
}
