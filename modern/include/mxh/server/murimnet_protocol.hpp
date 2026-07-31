#pragma once
#include <cstdint>
namespace mxh::server {
// 1:1 wire-byte port of legacy [CC]Header/Protocol.h MP_PROTOCOL_MURIMNET.
// Each enumerator is the exact byte value the legacy client/server uses on
// the wire, so a modern server can interoperate with a legacy client and
// vice versa without any protocol translation layer.
enum MurimNetProtocol : std::uint8_t {
    ChangetoMurimNet_Syn = 0,
    ChangetoMurimNet_Ack = 1,
    ChangetoMurimNet_Nack = 2,
    ReturntoMurimNet_Syn = 3,
    ReturntoMurimNet_Ack = 4,
    ReturntoMurimNet_Nack = 5,
    Connect_Syn = 6,
    Connect_Ack = 7,
    Connect_Nack = 8,
    Reconnect_Syn = 9,
    Reconnect_Ack = 10,
    Reconnect_Nack = 11,
    Disconnect_Syn = 12,
    Disconnect_Ack = 13,
    Disconnect_Nack = 14,
    Pr_PlayRoomInfo = 15,
    Pr_PlayerList = 16,
    Pr_PlayerIn = 17,
    Pr_PlayerOut = 18,
    Pr_Captain = 19,
    Pr_PlayerRein = 20,
    Pr_TeamChange_Syn = 21,
    Pr_TeamChange_Ack = 22,
    Pr_TeamChange_Nack = 23,
    Pr_Start = 24,
    Pr_Start_Syn = 25,
    Pr_Start_Ack = 26,
    Pr_Start_Nack = 27,
    Chnl_ChannelInfo = 28,
    Chnl_PlayerIn = 29,
    Chnl_PlayerOut = 30,
    Chnl_Captain = 31,
    Chnl_ModeChange = 32,
    Chnl_PlayerList = 33,
    Chnl_ChannelInfoList = 34,
    Chnl_PlayRoomInfoList = 35,
    Chnl_JoinPlayRoom_Syn = 36,
    Chnl_JoinPlayRoom_Ack = 37,
    Chnl_JoinPlayRoom_Nack = 38,
    Chnl_CreatePlayRoom_Syn = 39,
    Chnl_CreatePlayRoom_Ack = 40,
    Chnl_CreatePlayRoom_Nack = 41,
    Chnl_JoinChannel_Syn = 42,
    Chnl_JoinChannel_Ack = 43,
    Chnl_JoinChannel_Nack = 44,
    Chnl_CreateChannel_Syn = 45,
    Chnl_CreateChannel_Ack = 46,
    Chnl_CreateChannel_Nack = 47,
    Chat_All = 48,
    MurimServerIn_Syn = 49,
    MurimServerIn_Ack = 50,
    MurimServerIn_Nack = 51,
    PlayRoom_Info_Syn = 52,
    PlayRoom_Info_Ack = 53,
    PlayRoom_Info_Nack = 54,
    Player_Info_Syn = 55,
    Player_Info_Ack = 56,
    Player_Info_Nack = 57,
    NotifyToMn_PlayerLogout = 58,
    NotifyToMn_GameEnd = 59,
};
inline constexpr std::uint8_t murimnet_changetomurimnet_syn = static_cast<std::uint8_t>(MurimNetProtocol::ChangetoMurimNet_Syn);
inline constexpr std::uint8_t murimnet_changetomurimnet_ack = static_cast<std::uint8_t>(MurimNetProtocol::ChangetoMurimNet_Ack);
inline constexpr std::uint8_t murimnet_changetomurimnet_nack = static_cast<std::uint8_t>(MurimNetProtocol::ChangetoMurimNet_Nack);
inline constexpr std::uint8_t murimnet_returntomurimnet_syn = static_cast<std::uint8_t>(MurimNetProtocol::ReturntoMurimNet_Syn);
inline constexpr std::uint8_t murimnet_returntomurimnet_ack = static_cast<std::uint8_t>(MurimNetProtocol::ReturntoMurimNet_Ack);
inline constexpr std::uint8_t murimnet_returntomurimnet_nack = static_cast<std::uint8_t>(MurimNetProtocol::ReturntoMurimNet_Nack);
inline constexpr std::uint8_t murimnet_connect_syn = static_cast<std::uint8_t>(MurimNetProtocol::Connect_Syn);
inline constexpr std::uint8_t murimnet_connect_ack = static_cast<std::uint8_t>(MurimNetProtocol::Connect_Ack);
inline constexpr std::uint8_t murimnet_reconnect_syn = static_cast<std::uint8_t>(MurimNetProtocol::Reconnect_Syn);
inline constexpr std::uint8_t murimnet_reconnect_ack = static_cast<std::uint8_t>(MurimNetProtocol::Reconnect_Ack);
inline constexpr std::uint8_t murimnet_disconnect_syn = static_cast<std::uint8_t>(MurimNetProtocol::Disconnect_Syn);
inline constexpr std::uint8_t murimnet_disconnect_ack = static_cast<std::uint8_t>(MurimNetProtocol::Disconnect_Ack);
inline constexpr std::uint8_t murimnet_pr_teamchange_syn = static_cast<std::uint8_t>(MurimNetProtocol::Pr_TeamChange_Syn);
inline constexpr std::uint8_t murimnet_pr_start = static_cast<std::uint8_t>(MurimNetProtocol::Pr_Start);
inline constexpr std::uint8_t murimnet_pr_start_syn = static_cast<std::uint8_t>(MurimNetProtocol::Pr_Start_Syn);
inline constexpr std::uint8_t murimnet_pr_start_ack = static_cast<std::uint8_t>(MurimNetProtocol::Pr_Start_Ack);
inline constexpr std::uint8_t murimnet_chnl_modechange = static_cast<std::uint8_t>(MurimNetProtocol::Chnl_ModeChange);
inline constexpr std::uint8_t murimnet_chat_all = static_cast<std::uint8_t>(MurimNetProtocol::Chat_All);
inline constexpr std::uint8_t murimnet_notifytomn_player_logout = static_cast<std::uint8_t>(MurimNetProtocol::NotifyToMn_PlayerLogout);
inline constexpr std::uint8_t murimnet_notifytomn_gameend = static_cast<std::uint8_t>(MurimNetProtocol::NotifyToMn_GameEnd);
}
