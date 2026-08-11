// protocol.hpp - Phase 4 protocol layer.
//
// Replaces [CC]Header/Protocol.h (3542 lines of #define MP_CATEGORY_*)
// with a typed enum. The byte values are 1:1 compatible with the
// original protocol so existing clients/servers can interoperate.
//
// Wire format (matches original MSGBASE/MSGROOT 1:1):
//   [checksum: u8] [code: i8] [category: u8] [protocol: u8] [object_id: u32] [length: u32]
//   [payload: u8 × length]

#pragma once

#include <cstdint>

namespace mxh::proto {

// ============================================================================
// Protocol version (Phase 4.3).
//
// Version negotiation happens BEFORE any game messages:
//   1. Client connects → sends CheckVersion with its protocol version
//   2. Server compares versions → sends NotifyVersionAck (compatible) or
//      NotifyVersionNack (incompatible) with server's version
//   3. If compatible, client proceeds with RequestLogin
//   4. If not, server disconnects
//
// The original legacy protocol has NO version field — all messages are
// implicitly version 0. Modern servers can set min_version > 0 to reject
// legacy clients, or keep min_version = 0 for backward compatibility.
// ============================================================================
constexpr std::uint16_t kProtocolVersion   = 1;   // current server version
constexpr std::uint16_t kMinProtocolVersion = 0;   // minimum accepted (0 = legacy compat)

// Version check payload format:
//   [client_version: u16] [client_name: u8 len + bytes]
// Version ack payload format:
//   [server_version: u16] [encryption_required: u8]
// Version nack payload format:
//   [server_version: u16] [reason: u8]
enum class VersionRejectReason : std::uint8_t {
    TooOld     = 0,
    TooNew     = 1,
    ServerFull = 2,
};

// Categories (1:1 with [CC]Header/Protocol.h MP_CATEGORY).
enum class Category : std::uint8_t {
    Server            = 1,
    PowerUp           = 2,
    Character         = 3,
    Map               = 4,
    Item              = 5,
    Chat              = 6,
    UserConn          = 7,
    Move              = 8,
    Mugong            = 9,
    AuctionBoard      = 10,
    Cheat             = 11,
    Quick             = 12,
    PackedData        = 13,
    Party             = 14,
    PeaceWarMode      = 15,
    UngiJosik         = 16,
    Auction           = 17,
    AutoPatch         = 18,
    Signal            = 19,
    Tactic            = 20,
    Munpa             = 21,
    Skill             = 22,
    KyungGong         = 23,
    SimBub            = 24,
    MornitorTool      = 25,
    MornitorServer    = 26,
    MornitorMapServer = 27,
    Exchange          = 28,
    StreetStall       = 29,
    Pyoguk            = 30,
    Battle            = 31,
    CharRevive        = 32,
    Friend            = 33,
    BossMonster       = 34,
    Monster           = 35,
    Option            = 36,
    Npc               = 37,
    MurimNet          = 38,
    Quest             = 39,
    Debug             = 40,
    Pk                = 41,
    HackCheck         = 42,
    RMTool_Connect    = 43,
    RMTool_User       = 44,
    RMTool_Munpa      = 45,
    RMTool_GameLog    = 46,
    RMTool_OperLog    = 47,
    RMTool_Statistics = 48,
    RMTool_Admin      = 49,
    RMTool_Character  = 50,
    RMTool_Item       = 51,
    Wanted            = 52,
    Journal           = 53,
    Suryun            = 54,
    SocietyAct        = 55,
    Guild             = 56,
    GuildFieldWar     = 57,
    Note              = 58,
    PartyWar          = 59,
    GTournament       = 60,
    Jackpot           = 61,
    GuildUnion        = 62,
    SiegeWar          = 63,
    SiegeWar_Profit   = 64,
    Weather           = 65,
    Pet               = 66,
    HackShield        = 67,
    RMTool_Pet        = 68,
    NProtect          = 69,
    RMTool_DelChar    = 70,
    Survival          = 71,
    Titan             = 72,
    ItemExt           = 73,
    Bobusang          = 74,
    ItemLimit         = 75,
    AutoNote          = 76,
    FortWar           = 77,
    Max               = 78,
};

inline const char* category_name(Category c) {
    switch (c) {
        case Category::Server: return "Server";
        case Category::PowerUp: return "PowerUp";
        case Category::Character: return "Character";
        case Category::Map: return "Map";
        case Category::Item: return "Item";
        case Category::Chat: return "Chat";
        case Category::UserConn: return "UserConn";
        case Category::Move: return "Move";
        case Category::Mugong: return "Mugong";
        case Category::AuctionBoard: return "AuctionBoard";
        case Category::Cheat: return "Cheat";
        case Category::Quick: return "Quick";
        case Category::PackedData: return "PackedData";
        case Category::Party: return "Party";
        case Category::PeaceWarMode: return "PeaceWarMode";
        case Category::UngiJosik: return "UngiJosik";
        case Category::Auction: return "Auction";
        case Category::AutoPatch: return "AutoPatch";
        case Category::Signal: return "Signal";
        case Category::Tactic: return "Tactic";
        case Category::Munpa: return "Munpa";
        case Category::Skill: return "Skill";
        case Category::KyungGong: return "KyungGong";
        case Category::SimBub: return "SimBub";
        case Category::MornitorTool: return "MornitorTool";
        case Category::MornitorServer: return "MornitorServer";
        case Category::MornitorMapServer: return "MornitorMapServer";
        case Category::Exchange: return "Exchange";
        case Category::StreetStall: return "StreetStall";
        case Category::Pyoguk: return "Pyoguk";
        case Category::Battle: return "Battle";
        case Category::CharRevive: return "CharRevive";
        case Category::Friend: return "Friend";
        case Category::BossMonster: return "BossMonster";
        case Category::Monster: return "Monster";
        case Category::Option: return "Option";
        case Category::Npc: return "Npc";
        case Category::MurimNet: return "MurimNet";
        case Category::Quest: return "Quest";
        case Category::Debug: return "Debug";
        case Category::Pk: return "Pk";
        case Category::HackCheck: return "HackCheck";
        case Category::RMTool_Connect: return "RMTool_Connect";
        case Category::RMTool_User: return "RMTool_User";
        case Category::RMTool_Munpa: return "RMTool_Munpa";
        case Category::RMTool_GameLog: return "RMTool_GameLog";
        case Category::RMTool_OperLog: return "RMTool_OperLog";
        case Category::RMTool_Statistics: return "RMTool_Statistics";
        case Category::RMTool_Admin: return "RMTool_Admin";
        case Category::RMTool_Character: return "RMTool_Character";
        case Category::RMTool_Item: return "RMTool_Item";
        case Category::Wanted: return "Wanted";
        case Category::Journal: return "Journal";
        case Category::Suryun: return "Suryun";
        case Category::SocietyAct: return "SocietyAct";
        case Category::Guild: return "Guild";
        case Category::GuildFieldWar: return "GuildFieldWar";
        case Category::Note: return "Note";
        case Category::PartyWar: return "PartyWar";
        case Category::GTournament: return "GTournament";
        case Category::Jackpot: return "Jackpot";
        case Category::GuildUnion: return "GuildUnion";
        case Category::SiegeWar: return "SiegeWar";
        case Category::SiegeWar_Profit: return "SiegeWar_Profit";
        case Category::Weather: return "Weather";
        case Category::Pet: return "Pet";
        case Category::HackShield: return "HackShield";
        case Category::RMTool_Pet: return "RMTool_Pet";
        case Category::NProtect: return "NProtect";
        case Category::RMTool_DelChar: return "RMTool_DelChar";
        case Category::Survival: return "Survival";
        case Category::Titan: return "Titan";
        case Category::ItemExt: return "ItemExt";
        case Category::Bobusang: return "Bobusang";
        case Category::ItemLimit: return "ItemLimit";
        case Category::AutoNote: return "AutoNote";
        case Category::FortWar: return "FortWar";
        default: return "Unknown";
    }
}

// ============================================================================
// UserConn sub-protocols (MP_USERCONN_*).
// This is the login flow: client → DistributeServer → AgentServer.
// Values are 1:1 with the original C enum MP_PROTOCOL_USERCONN (auto-increment
// from 0). DO NOT reorder or insert — every slot has a fixed wire meaning.
// ============================================================================
enum class UserConnProtocol : std::uint8_t {
    // Distribute phase (0-8)
    DistConnectSuccess     = 0,   // D → C: auth key in dwObjectID
    RequestLogin           = 1,   // C → D: login with id + password
    NotifyUserLoginAck     = 2,   // D → C: ack + AgentAddr + userIdx
    NotifyUserLoginNack    = 3,   // D → C: nack
    NotifyUserLoginSyn     = 4,   // D → A: user login notification
    NotifyUserLoginAck2    = 5,   // A → D: login notification ack
    NotifyUserLoginNack2   = 6,   // A → D: login notification nack
    NotifyOverlappedLogin  = 7,   // D → A: overlapped login
    AgentConnectSuccess    = 8,   // A → C: agent connection success

    // Character list & selection (9-18)
    CharacterListSyn       = 9,   // C → A: request character list
    DirectCharacterListSyn = 10,  // C → A: direct char list (for GM)
    CharacterListNack      = 11,  // A → C: char list nack
    CharacterListAck       = 12,  // A → C: list of chars
    DisconnectSyn          = 13,  // C → A/D: disconnect request
    DisconnectAck          = 14,  // A/D → C: disconnect ack
    DisconnectNack         = 15,  // A/D → C: disconnect nack
    CharacterSelectSyn     = 16,  // C → A: enter game with char
    CharacterSelectAck     = 17,  // A → C: char select ack (sends CHARACTER_TOTALINFO)
    CharacterSelectNack    = 18,  // A → C: char select nack

    // Character name check (19-21)
    CharacterNameCheckSyn  = 19,  // C → A: check if name is available
    CharacterNameCheckAck  = 20,  // A → C: name check result
    CharacterNameCheckNack = 21,  // A → C: name check nack

    // Character creation (22-27)
    CharacterMakeSyn       = 22,  // C → A: create character
    CharacterMakeAck       = 23,  // A → C: create ack
    CharacterMakeNack      = 24,  // A → C: create nack
    CharacterInfoSyn       = 25,  // C → A: request character info
    CharacterInfoAck       = 26,  // A → C: character info ack
    CharacterInfoNack      = 27,  // A → C: character info nack

    // Game entry (28-33) — MAP SERVER handles these
    GameInSyn              = 28,  // C → M: enter game (client sends GAMEIN_SYN)
    GameInAck              = 29,  // M → C: enter game ack (sends SEND_HERO_TOTALINFO)
    GameInNack             = 30,  // M → C: enter game nack
    GameOutSyn             = 31,  // C → M: exit game
    GameOutAck             = 32,  // M → C: exit game ack
    GameOutNack            = 33,  // M → C: exit game nack

    // Connection state (34+)
    Disconnected           = 34,  // client disconnected notification
    CharacterAdd           = 35,  // A → C: new character appeared
    PetAdd                 = 36,  // A → C: new pet appeared
    MonsterAdd             = 37,  // A → C: new monster appeared
    BossMonsterAdd         = 38,  // A → C: new boss appeared
    NpcAdd                 = 39,  // A → C: new NPC appeared
    ObjectRemove           = 40,  // A → C: remove object
    CharacterDie           = 41,  // A → C: character died
    MonsterDie             = 42,  // A → C: monster died
    PetDie                 = 43,  // A → C: pet died
    CharacterRevive        = 44,  // A → C: character revived
    CharacterRemoveSyn     = 45,  // C → A: remove (delete) character
    CharacterRemoveAck     = 46,  // A → C: remove ack
    CharacterRemoveNack    = 47,  // A → C: remove nack
    ChangeMapSyn           = 48,  // C → A: change map request
    ChangeMapAck           = 49,  // A → C: change map ack
    ChangeMapNack          = 50,  // A → C: change map nack
    MapOut                 = 51,  // A → C: map out notification
    MapOutWithMapNum       = 52,  // A → C: map out with map number
    CharacterTotalInfo     = 53,  // A → C: full character total info
    SavepointSyn           = 54,  // C → A: set save point
    SavepointAck           = 55,  // A → C: save point ack
    SavepointNack          = 56,  // A → C: save point nack
    BackToCharSelSyn       = 57,  // C → A: back to char select
    BackToCharSelAck       = 58,  // A → C: back to char select ack
    BackToCharSelNack      = 59,  // A → C: back to char select nack
    GridInit               = 60,  // A → C: grid initialization
    SetVisible             = 61,  // A → C: visibility setting
    OtherUserConnectNotify = 62,  // A → C: other user connection notify
    ConnectionCheck        = 63,  // A → C: connection check ping
    ConnectionCheckOk      = 64,  // C → A: connection check pong
    ChecksumError          = 65,  // A → C: checksum error
    ForceDisconnectOverlap = 66,  // A → C: force disconnect overlap
    DisconnectedByOverlap  = 67,  // A → C: disconnected by overlap
    ChannelInfoSyn         = 68,  // C → A: channel info request
    ChannelInfoAck         = 69,  // A → C: channel info ack
    ChannelInfoNack        = 70,  // A → C: channel info nack
    NotifyToAgentAlreadyOut= 71,  // D → A: already logged out notification
    RequestDistOut         = 72,  // C → D: request distribute logout
    DisconnectedOnLogin    = 73,  // A → C: disconnected on login
    ServerNotReady         = 74,  // A → C: server not ready
    MapDesc                = 75,  // A → C: map description
    CharacterReviveNack    = 76,  // A → C: revive nack
    ReadyToRevive          = 77,  // C → A: ready to revive
    CheatUsing             = 78,  // cheat usage
    CheatChangeMapAck      = 79,  // GM map change ack
    UseDynamicSyn          = 80,  // dynamic use syn
    UseDynamicAck          = 81,  // dynamic use ack
    UseDynamicNack         = 82,  // dynamic use nack
    LoginDynamicSyn        = 83,  // dynamic login syn
    LoginDynamicAck        = 84,  // dynamic login ack
    LoginDynamicNack       = 85,  // dynamic login nack
    LoginCheckDelete       = 86,  // login check delete
    ForceDisconnectAck2    = 87,  // force disconnect ack (overlap)
    MapOutToEventMap       = 88,  // event map out
    MapOutToEventBeforeMap = 89,  // event before-map out
    EnterEventMapSyn       = 90,  // enter event map
    EventReady             = 91,  // event ready
    EventStart             = 92,  // event start
    EventEnd               = 93,  // event end
    EventItemUse           = 94,  // event item use
    EventItemUse2          = 95,  // event item use 2
    GameInPosSyn           = 96,  // C → M: enter game at position
    GameInPosAck           = 97,  // M → C: enter game at position ack
    GameInPosNack          = 98,  // M → C: enter game at position nack
    RemainTimeNotify       = 99,  // remaining time notify
    BackToBeforeMapToUser  = 100, // back to before map (to user)
    BackToBeforeMapSyn     = 101, // back to before map syn
    BackToBeforeMapAck     = 102, // back to before map ack
    BackToBeforeMapNack    = 103, // back to before map nack
    EnterGTournamentSyn    = 104, // enter G-tournament
    CharacterSlot          = 105, // character slot
    CastleGateAdd          = 106, // castle gate add
    GameInOtherMapSyn      = 107, // enter game on other map
    NoWaitExitPlayer       = 108, // no-wait exit player
    FlagNpcOnOff           = 109, // flag NPC on/off
    LoginSynBuddy          = 110, // login syn buddy
    ChangeMapChannelInfoSyn= 111, // change map channel info syn
    ChangeMapChannelInfoAck= 112, // change map channel info ack
    ChangeMapChannelInfoNack=113, // change map channel info nack
    CurrentMapChannelInfo  = 114, // current map channel info
};

// ============================================================================
// Server sub-protocols (MP_SERVER).
// ============================================================================
enum class ServerProtocol : std::uint8_t {
    QueryClientInfoSyn    = 1,
    QueryClientInfoAck    = 2,
    QueryUserCountSyn     = 3,
    QueryUserCountAck     = 4,
};

// ============================================================================
// Modern-only protocol constants (NOT in the original enum).
// Used by the modern version-negotiation layer. These sit outside the
// original UserConnProtocol enum to avoid collisions with legacy values.
// ============================================================================
constexpr std::uint8_t kModernCheckVersion      = 200;  // C → D: version check
constexpr std::uint8_t kModernNotifyVersionAck  = 201;  // D → C: version ack
constexpr std::uint8_t kModernNotifyVersionNack = 202;  // D → C: version nack
constexpr std::uint8_t kModernHselKey           = 203;  // D → C: HSEL session key (64B HselInit)

// Modern-only NPC shop list (S -> C on SpeechSyn), Category::Item.
// Payload: [npc_id:u32][count:u16] + count x [item_id:u16][price:u32].
constexpr std::uint8_t kModernShopList          = 199;

// ============================================================================
// Move sub-protocols (MP_MOVE_*).
// 1:1 with the original C enum MP_PROTOCOL_MOVE.
// ============================================================================
enum class MoveProtocol : std::uint8_t {
    Init                = 0,   // initialize movement
    Target              = 1,   // movement target position
    Correction          = 2,   // position correction
    WalkMode            = 3,   // switch to walk mode
    RunMode             = 4,   // switch to run mode
    KyungGongSyn        = 5,   //轻功 request
    KyungGongAck        = 6,   //轻功 ack
    KyungGongNack       = 7,   //轻功 nack
    Stop                = 8,   // stop moving
    EffectMove          = 9,   // effect-based movement
    MonsterMoveNotify   = 10,  // monster movement notification
    ForceStopKyungGong  = 11,  // force stop轻功
    Warp                = 12,  // teleport/warp
    OneTarget           = 13,  // single target movement
    PetOneTarget        = 14,  // pet single target
    PetTarget           = 15,  // pet target
    PetStop             = 16,  // pet stop
    PetCorrection       = 17,  // pet correction
    PetWarpSyn          = 18,  // pet warp request
    PetWarpAck          = 19,  // pet warp ack
};

// ============================================================================
// Chat sub-protocols (MP_CHAT_*).
// 1:1 with the original C enum MP_PROTOCOL_CHAT.
// ============================================================================
enum class ChatProtocol : std::uint8_t {
    All                 = 0,   // general chat (all)
    Party               = 1,   // party chat
    Guild               = 2,   // guild chat
    Whisper             = 3,   // whisper
    WhisperSyn          = 4,   // whisper request
    WhisperAck          = 5,   // whisper ack
    WhisperNack         = 6,   // whisper nack
    FromMonsterAll      = 7,   // from monster (all)
    MonsterSpeech       = 8,   // monster speech
    FastChat            = 9,   // spam prevention
    GM                  = 10,  // GM chat
    WhisperGM           = 11,  // whisper GM (server → client)
    WhisperGMSyn        = 12,  // whisper GM (server → server)
    SmallShout          = 13,  // same-map same-channel shout
    GMSmallShout        = 14,  // GM shout
    ShoutSyn            = 15,  // shout request
    ShoutAck            = 16,  // shout ack
    ShoutNack           = 17,  // shout nack
    ShoutSendAll        = 18,  // shout send all
    ShoutSendServer     = 19,  // shout send server
    GuildUnion          = 20,  // guild union chat
};

// ============================================================================
// Item sub-protocols (MP_ITEM_*).
// 1:1 with the original C enum MP_PROTOCOL_ITEM.
// ============================================================================
enum class ItemProtocol : std::uint8_t {
    TotalInfoLocal       = 0,   // client-local dispatch from GameInAck
    PyogukItemInfo       = 1,   // warehouse item info

    UseSyn               = 2,   // C -> S: use item
    UseAck               = 3,   // S -> C: use success
    UseNack              = 4,   // S -> C: use failed

    CombineSyn           = 5,   // C -> S: combine items
    CombineAck           = 6,
    CombineNack          = 7,
    DivideSyn            = 8,   // C -> S: divide stack
    DivideAck            = 9,
    DivideNack           = 10,
    DivideNewItemNotify  = 11,
    DiscardSyn           = 12,  // C -> S: discard item
    DiscardAck           = 13,
    DiscardNack          = 14,

    ErrorNack            = 15,

    MoveSyn              = 16,  // C -> S: move item within inventory
    MoveAck              = 17,  // S -> C: move success
    MoveNack             = 18,

    BuySyn               = 22,  // C -> S: buy from NPC
    BuyAck               = 23,  // S -> C: buy success
    BuyNack              = 24,

    SellSyn              = 25,  // C -> S: sell to NPC
    SellAck              = 26,  // S -> C: sell success
    SellNack             = 27,

    DealerSyn            = 28,  // C -> S: open dealer (street stall)
    DealerAck            = 29,
    DealerNack           = 30,

    Money                = 31,  // S -> C: update money
    MoneyError           = 32,

    AppearanceChange     = 33,
    ObtainMoney          = 34,  // S -> C: obtained money
    MonsterObtainNotify  = 35,  // S -> C: monster obtained item

    UpgradeSyn           = 36,  // C -> S: upgrade item
    UpgradeSuccessAck    = 37,
    UpgradeFailedAck     = 38,
    UpgradeNack          = 39,

    MixSyn               = 40,
    MixSuccessAck        = 41,
    MixBigFailedAck      = 42,
    MixFailedAck         = 43,
    MixMsg                = 44,

    ReinforceSyn         = 45,
    ReinforceSuccessAck  = 46,
    ReinforceFailedAck   = 47,
    ReinforceNack        = 48,

    DissolveSyn          = 49,
    DissolveSuccessAck   = 50,
    DissolveFailedAck    = 51,
    DissolveNack         = 52,

    // 1:1 wire-byte compatibility with legacy MP_ITEM_SHOPITEM_CHASE_*
    // (positions 154/155/156 in [CC]Header/Protocol.h).
    ShopItemChaseSyn     = 154,
    ShopItemChaseAck     = 155,
    ShopItemChaseNack    = 156,

    // 1:1 wire-byte compatibility with legacy MP_ITEM_SHOPITEM_JOBCHANGE_*
    // (positions 175/176/177 in [CC]Header/Protocol.h). Modern skips the
    // intermediate entries 53..174 because the modern client/server does
    // not implement them yet, but the wire byte for the job-change syn /
    // ack / nack must remain stable so legacy clients/servers can talk to
    // the modern server (and vice versa) once Phase A/B wires real network
    // I/O.
    ShopItemJobChangeSyn  = 175,
    ShopItemJobChangeAck  = 176,
    ShopItemJobChangeNack = 177,
};

// ============================================================================
// Monster sub-protocols (MP_MONSTER_*).
// 1:1 with the original C enum MP_PROTOCOL_MONSTER.
// ============================================================================
enum class MonsterProtocol : std::uint8_t {
    LifeNotify        = 0,   // S -> C: monster HP changed
    RestStartNotify   = 1,   // S -> C: monster started resting
    RestEndNotify     = 2,   // S -> C: monster stopped resting
    RecallNotify      = 3,   // S -> C: monster recalled/respawned
};

// ============================================================================
// BossMonster sub-protocols (MP_BOSSMONSTER_*).
// 1:1 with the original C enum MP_PROTOCOL_BOSSMONSTER.
// ============================================================================
enum class BossMonsterProtocol : std::uint8_t {
    RestStartNotify   = 0,
    RecallNotify      = 1,
    LifeNotify        = 2,
    ShieldNotify      = 3,
    StandNotify       = 4,
    StandEndNotify    = 5,
    FieldLifeNotify   = 6,   // field boss life
    FieldShieldNotify = 7,
};

// ============================================================================
// NPC sub-protocols (MP_NPC_*).
// 1:1 with the original C enum MP_PROTOCOL_NPC.
// ============================================================================
enum class NpcProtocol : std::uint8_t {
    SpeechSyn         = 0,   // C -> S: talk to NPC
    SpeechAck         = 1,
    SpeechNack        = 2,
    CloseBomulSyn     = 3,
    CloseBomulAck     = 4,
    CloseBomulNack    = 5,
    OpenBomulSyn      = 6,
    OpenBomulAck      = 7,
    OpenBomulNack     = 8,
    DoJobSyn          = 9,
    DoJobAck          = 10,
    DoJobNack         = 11,
    DieAck            = 12,
};

// Skill protocol (Category = Skill = 22, MP_SKILL).
// Matches [CC]Header/Protocol.h MP_SKILL_* defines 1:1.
enum class SkillProtocol : std::uint8_t {
    StartSyn              = 0,   // C->S: request skill use
    StartAck              = 1,   // S->C: skill started
    StartNack             = 2,   // S->C: skill failed (error code in payload)
    SkillObjectAdd        = 3,   // S->C: skill object appeared on map
    SkillObjectRemove     = 4,   // S->C: skill object disappeared
    AddToAreaSyn          = 5,   // C->S: target entered skill area
    AddToAreaAck          = 6,
    AddToAreaNack         = 7,
    RemoveFromAreaSyn     = 8,   // C->S: target left skill area
    RemoveFromAreaAck     = 9,
    RemoveFromAreaNack    = 10,
    SingleResult          = 11,  // S->C: single-target skill result (damage)
    StartEffect           = 12,  // C->S: skill effect VFX (client broadcast)
    OperateSyn            = 13,  // C->S: operate continuous skill
    OperateAck            = 14,
    OperateNack           = 15,
    DelayDamagedTargetList= 16,  // S->C: delayed damage target list
    DelayNotify           = 17,  // S->C: delayed notification
    JobNack               = 18,  // S->C: job skill failed
};

// Battle protocol (Category = Battle = 29, MP_BATTLE).
// Matches [CC]Header/Protocol.h MP_BATTLE_* defines 1:1.
enum class BattleProtocol : std::uint8_t {
    Info                  = 0,
    ChatTeamSyn           = 1,
    ChatTeamAck           = 2,
    ChatTeamNack          = 3,
    ChatMasterSyn         = 4,
    ChatMasterAck         = 5,
    ChatMasterNack        = 6,
    StartNotify           = 7,   // S->C: battle started
    TeamMemberAddNotify   = 8,   // S->C: member joined battle
    TeamMemberDeleteNotify= 9,   // S->C: member left battle
    TeamMemberDieNotify   = 10,  // S->C: member died
    BattleObjectDestroyNotify = 11,
    BattleObjectCreateNotify  = 12,
    VictoryNotify         = 13,  // S->C: battle victory
    DrawNotify            = 14,  // S->C: battle draw
    DestroyNotify         = 15,  // S->C: battle destroyed
    Result                = 16,  // S->C: battle result
};

// Quest protocol (Category = Quest = 39, MP_QUEST).
// Matches [CC]Header/Protocol.h MP_PROTOCOL_QUEST offsets 1:1. Only the
// Start / End sub-protocols used by the T3 side-by-side replay harness
// are defined here; legacy also has TotalInfo / ChangeState / Notify
// sub-protocols that will be added when the modern quest manager lands.
enum class QuestProtocol : std::uint8_t {
    StartSyn              = 9,   // C -> S: start (accept) quest
    StartAck              = 10,  // S -> C: quest accepted
    StartNack             = 11,  // S -> C: quest rejected
    EndSyn                = 12,  // C -> S: complete quest
    EndAck                = 13,
    EndNack               = 14,
};

}  // namespace mxh::proto
