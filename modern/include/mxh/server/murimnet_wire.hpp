#pragma once
// 1:1 wire-byte port of legacy [CC]Header/CommonStruct.h MSGBASE family
// and [CC]Header/CommonStruct.h MNPLAYER_BASEINFO/CHANNEL_BASEINFO/PLAYROOM_BASEINFO.
// All fields use exact legacy byte width; struct layout is #pragma pack(1).
// This is the only acceptable replacement for legacy CommonStruct.h on the wire.
#include <array>
#include <cstddef>
#include <cstdint>
namespace mxh::server {
// Legacy CommonGameDefine.h name widths.
inline constexpr std::size_t MNH_NAME_LENGTH = 16;
inline constexpr std::size_t MNH_CHANNELTITLE_LENGTH = 64;
inline constexpr std::size_t MNH_PLAYROOMTITLE_LENGTH = 64;
inline constexpr std::size_t MNH_MAX_PLAYER_IN_CHANNEL = 300;
inline constexpr std::size_t MNH_MAX_CHANNEL_IN_MURIMNET = 10000;
inline constexpr std::size_t MNH_MAX_PLAYROOM_IN_MURIMNET = 10000;
// Legacy Protocol.h MP_MURIMNET category byte (1-based, value = 38).
inline constexpr std::uint8_t MNH_CATEGORY_MURIMNET = 38;

#pragma pack(push, 1)
// MSGROOT: 3 bytes header (CheckSum, Category, Protocol).
struct MnhWireRoot { std::uint8_t CheckSum; std::uint8_t Category; std::uint8_t Protocol; };
// MSGBASE: MSGROOT + dwObjectID = 7 bytes.
struct MnhWireBase { std::uint8_t CheckSum; std::uint8_t Category; std::uint8_t Protocol; std::uint32_t dwObjectID; };

// Common short payload wrappers around MSGBASE.
struct MnhWireByte : public MnhWireBase { std::uint8_t bData; };
struct MnhWireDword : public MnhWireBase { std::uint32_t dwData; };
struct MnhWireDword2 : public MnhWireBase { std::uint32_t dwData1; std::uint32_t dwData2; };
struct MnhWireDword3 : public MnhWireBase { std::uint32_t dwData1; std::uint32_t dwData2; std::uint32_t dwData3; };
struct MnhWireDword4 : public MnhWireBase { std::uint32_t dwData1; std::uint32_t dwData2; std::uint32_t dwData3; std::uint32_t dwData4; };
struct MnhWireWord : public MnhWireBase { std::uint16_t wData; };
struct MnhWireWord2 : public MnhWireBase { std::uint16_t wData1; std::uint16_t wData2; };

// MNPLAYER_BASEINFO payload (legacy size = 4+2+16+2+16+2+2+2+16+1+1 = 64 bytes).
struct MnhPlayerBaseInfo { std::uint32_t dwObjectID; std::uint16_t wRankPoint; std::array<char, MNH_NAME_LENGTH> strPlayerName; std::uint16_t Level; std::array<char, MNH_NAME_LENGTH> strNick; std::uint16_t wPlayCount; std::uint16_t wWin; std::uint16_t wLose; std::array<char, MNH_NAME_LENGTH> strMunpa; std::int8_t cbPositionInMunpa; std::int8_t cbTeam; };
struct MnhMsgPlayerBaseInfo : public MnhWireBase { MnhPlayerBaseInfo Info; };
struct MnhMsgPlayerBaseInfoList : public MnhWireBase { std::uint32_t dwTotalPlayerNum; std::array<MnhPlayerBaseInfo, MNH_MAX_PLAYER_IN_CHANNEL> PlayerInfo; };

// CHANNEL_BASEINFO payload (legacy size = 4+64+1+2+2 = 73 bytes).
struct MnhChannelBaseInfo { std::uint32_t dwChannelIndex; std::array<char, MNH_CHANNELTITLE_LENGTH> strChannelTitle; std::int8_t cbChannelKind; std::uint16_t wMaxPlayer; std::uint16_t wPlayerNum; };
struct MnhMsgChannelBaseInfo : public MnhWireBase { MnhChannelBaseInfo Info; };
struct MnhMsgChannelBaseInfoList : public MnhWireBase { std::uint32_t dwTotalChannelNum; std::array<MnhChannelBaseInfo, MNH_MAX_CHANNEL_IN_MURIMNET> ChannelInfo; };

// PLAYROOM_BASEINFO payload (legacy size = 4+64+1+2+2+4+2+1+2 = 82 bytes).
struct MnhPlayRoomBaseInfo { std::uint32_t dwPlayRoomIndex; std::array<char, MNH_PLAYROOMTITLE_LENGTH> strPlayRoomTitle; std::int8_t cbPlayRoomKind; std::uint16_t wMaxObserver; std::uint16_t wMaxPlayerPerTeam; std::uint32_t MoneyForPlay; std::uint16_t wPlayerNum; std::int8_t cbStart; std::uint16_t wMapNum; };
struct MnhMsgPlayRoomBaseInfo : public MnhWireBase { MnhPlayRoomBaseInfo Info; };
struct MnhMsgPlayRoomBaseInfoList : public MnhWireBase { std::uint32_t dwTotalPlayRoomNum; std::array<MnhPlayRoomBaseInfo, MNH_MAX_PLAYROOM_IN_MURIMNET> PlayRoomInfo; };

// MP_MURIMNET_PR_TEAMCHANGE payload (legacy size = 4+1+1 = 6 bytes after MSGBASE).
struct MnhMsgTeamChange : public MnhWireBase { std::uint32_t dwMoverID; std::uint8_t cbFromTeam; std::uint8_t cbToTeam; };
#pragma pack(pop)
}
