#pragma once
// MurimNet wire-byte serializer for SendMsg_ChannelList / SendMsg_PlayRoomList.
// Produces byte-identical output to legacy CommonStruct.h packed wire layout,
// trimmed to legacy GetMsgLength() = sizeof(header) + 4 + count * sizeof(BaseInfo).
#include <cstdint>
#include <vector>
namespace mxh::server {
class MurimNetChannelManager;
class MurimNetPlayRoomManager;
// Build wire bytes for SendMsg_ChannelList reply (MP_MURIMNET_CHNL_CHANNELINFOLIST = 34).
// Returned vector size = sizeof(MnhWireBase) + 4 + channel_count * sizeof(MnhChannelBaseInfo).
std::vector<std::uint8_t> mnh_build_channel_list_wire(const MurimNetChannelManager& mgr);
// Build wire bytes for SendMsg_PlayRoomList reply (MP_MURIMNET_CHNL_PLAYROOMINFOLIST = 35).
// Returned vector size = sizeof(MnhWireBase) + 4 + room_count * sizeof(MnhPlayRoomBaseInfo).
std::vector<std::uint8_t> mnh_build_playroom_list_wire(const MurimNetPlayRoomManager& mgr);
}
#include "mxh/server/murimnet_wire.hpp"
#include "mxh/server/murimnet_protocol.hpp"
