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

// Categories (1:1 with [CC]Header/Protocol.h MP_CATEGORY).
enum class Category : std::uint8_t {
    Server            = 1,
    PowerUp,
    Character,
    Map,
    Item,
    Chat,
    UserConn,
    Move,
    Mugong,
    AuctionBoard,
    Cheat,
    Quick,
    PackedData,
    Party,
    PeaceWarMode,
    UngiJosik,
    Auction,
    AutoPatch,
    Signal,
    Tactic,
    Munpa,
    Skill,
    KyungGong,
    SimBub,
    MornitorTool,
    MornitorServer,
    MornitorMapServer,
    Exchange,
    StreetStall,
    Pyoguk,
    Battle,
    CharRevive,
    Friend,
    BossMonster,
    Monster,
    Option,
    Npc,
    MurimNet,
    Quest,
    Debug,
    Pk,
    HackCheck,
    RMTool_Connect,
    RMTool_User,
    RMTool_Munpa,
    RMTool_Item,
    RMTool_Chat,
    RMTool_Dungeon,
    RMTool_Map,
    RMTool_Oper,
    RMTool_Money,
    RMTool_Monster,
    RMTool_Skill,
    RMTool_Character_Info,
    RMTool_Guild,
    RMTool_Notes,
    RMTool_DelChar,
    Wanted,
    Journal,
    Suryun,
    SocietyAct,
    Guild,
    GuildFieldWar,
    Note,
    PartyWar,
    GTournament,
    Jackpot,
    GuildUnion,
    SiegeWar,
    SiegeWar_Profit,
    Weather,
    Pet,
    HackShield,
    NProtect,
    Survival,
    Titan,
    ItemExt,
    Bobusang,
    ItemLimit,
    AutoNote,
    FortWar,
    Max,
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
        case Category::RMTool_Item: return "RMTool_Item";
        case Category::RMTool_Chat: return "RMTool_Chat";
        case Category::RMTool_Dungeon: return "RMTool_Dungeon";
        case Category::RMTool_Map: return "RMTool_Map";
        case Category::RMTool_Oper: return "RMTool_Oper";
        case Category::RMTool_Money: return "RMTool_Money";
        case Category::RMTool_Monster: return "RMTool_Monster";
        case Category::RMTool_Skill: return "RMTool_Skill";
        case Category::RMTool_Character_Info: return "RMTool_Character_Info";
        case Category::RMTool_Guild: return "RMTool_Guild";
        case Category::RMTool_Notes: return "RMTool_Notes";
        case Category::RMTool_DelChar: return "RMTool_DelChar";
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
        case Category::NProtect: return "NProtect";
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
// ============================================================================
enum class UserConnProtocol : std::uint8_t {
    // Distribute phase
    RequestLogin          = 1,   // Client → Distribute: ID + password + version
    NotifyUserLoginAck    = 2,   // Distribute → Client: ack + AgentAddr
    NotifyUserLoginNack   = 3,   // Distribute → Client: nack
    CheckVersion          = 4,
    NotifyVersionAck      = 5,
    NotifyVersionNack     = 6,

    // Agent phase
    CharacterListSyn     = 10,  // Client → Agent: request character list
    CharacterListAck      = 11,  // Agent → Client: list of chars
    CharacterListNack     = 12,
    CreateCharacterSyn    = 13,  // Client → Agent: create new char
    CreateCharacterAck     = 14,
    CreateCharacterNack    = 15,
    DeleteCharacterSyn    = 16,
    DeleteCharacterAck     = 17,
    DeleteCharacterNack    = 18,

    // Game start
    GameStartSyn          = 20,  // Client → Agent: enter game with char idx
    GameStartAck          = 21,
    GameStartNack         = 22,
    DisconnectSyn         = 30,
    DisconnectAck         = 31,

    // Heartbeat / status
    Heartbeat            = 50,
    Ping                 = 51,
    Pong                 = 52,
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

}  // namespace mxh::proto