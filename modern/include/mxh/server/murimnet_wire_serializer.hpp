#pragma once
// MurimNet wire-byte serializer for SendMsg_ChannelList / SendMsg_PlayRoomList.
// Produces byte-identical output to legacy CommonStruct.h packed wire layout,
// trimmed to legacy GetMsgLength() = sizeof(header) + 4 + count * sizeof(BaseInfo).
#include <cstdint>
#include <functional>
#include <vector>
namespace mxh::server {
class MurimNetPlayer;
class MurimNetChannelManager;
class MurimNetPlayRoomManager;
// Build wire bytes for SendMsg_ChannelList reply (MP_MURIMNET_CHNL_CHANNELINFOLIST = 34).
// Returned vector size = sizeof(MnhWireBase) + 4 + channel_count * sizeof(MnhChannelBaseInfo).
std::vector<std::uint8_t> mnh_build_channel_list_wire(const MurimNetChannelManager& mgr);
// Build wire bytes for SendMsg_PlayRoomList reply (MP_MURIMNET_CHNL_PLAYROOMINFOLIST = 35).
// Returned vector size = sizeof(MnhWireBase) + 4 + room_count * sizeof(MnhPlayRoomBaseInfo).
std::vector<std::uint8_t> mnh_build_playroom_list_wire(const MurimNetPlayRoomManager& mgr);
// Build wire bytes for player list reply (MP_MURIMNET_CHNL_PLAYERLIST = 33 or MP_MURIMNET_PR_PLAYERLIST = 16).
// visitor is called with a sink that should invoke once per player (e.g. for_each_member adapter).
// Returned vector size = sizeof(MnhWireBase) + 4 + emitted_count * sizeof(MnhPlayerBaseInfo).
std::vector<std::uint8_t> mnh_build_player_list_wire(const std::function<void(const std::function<void(const MurimNetPlayer&)>&)>& visitor, std::uint8_t protocol);
}
#include "mxh/server/murimnet_wire.hpp"
#include "mxh/server/murimnet_protocol.hpp"
