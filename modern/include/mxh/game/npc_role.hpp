// mxh/game/npc_role.hpp
//
// Mirror of the legacy NPC_ROLE enum from
// [CC]Header/CommonGameDefine.h:575.  We mirror it here so the modern
// client/server can route NPC role-specific UI / interaction logic
// (shop vs. quest vs. warp) without dragging in the legacy shared
// header (which is read-only per AGENTS.md §0).
//
// 1:1 with legacy; the wire npc_kind value comes from
// [CC]Header/ServerGameStruct.h NPC_LIST.NpcKind and is broadcast in
// UserConn::NpcAdd payload offset 35 (map_handler.cpp:1182).  This
// enum must stay bit-compatible with the legacy values forever.
//
// Reference: 墨香《[CC]Header/CommonGameDefine.h:575 enum NPC_ROLE》
//            OBJECT_ROLE=0, DEALER_ROLE=1, ..., MAPCHANGE_ROLE=27.

#pragma once
#include <cstdint>

namespace mxh::game {

enum class NpcRole : std::uint16_t {
    Object     = 0,   // OBJECT_ROLE  — 普通 object / decoration
    Dealer     = 1,   // DEALER_ROLE  — 商店 NPC (item buy/sell)
    Auction    = 2,   // AUCTION_ROLE
    Munpa      = 3,   // MUNPA_ROLE   — 帮派相关
    Changgo    = 4,   // CHANGGO_ROLE — 仓库
    Fyokuk     = 5,   // FYOKUK_ROLE  — 寄售
    Talker     = 6,   // TALKER_ROLE  — 任务 / 对话 NPC (quest giver)
    Wanted     = 9,   // WANTED_ROLE  — 悬赏任务 NPC
    Suryun     = 10,  // SURYUN_ROLE  — 修练 NPC (skill training)
    Symbol     = 11,  // SYMBOL_ROLE  — 门派标志
    Castle     = 12,  // CASTLE_ROLE
    Guide      = 13,  // GUIDE_ROLE
    Titan      = 14,  // TITAN_ROLE
    Bobusang   = 15,  // BOBUSANG_ROLE — 流动商人
    FortwarSymbol = 16,
    Bomul      = 23,  // BOMUL_ROLE
    MapChange  = 27,  // MAPCHANGE_ROLE — 传送 NPC (warp)
};

// Convert legacy wire value (WORD from NpcAdd payload offset 35) to
// the modern NpcRole enum.  Unknown values are reported via the
// optional and caller should fall back to Object-role rendering.
constexpr NpcRole role_from_wire(std::uint16_t kind) noexcept {
    switch (kind) {
        case 0:  return NpcRole::Object;
        case 1:  return NpcRole::Dealer;
        case 2:  return NpcRole::Auction;
        case 3:  return NpcRole::Munpa;
        case 4:  return NpcRole::Changgo;
        case 5:  return NpcRole::Fyokuk;
        case 6:  return NpcRole::Talker;
        case 9:  return NpcRole::Wanted;
        case 10: return NpcRole::Suryun;
        case 11: return NpcRole::Symbol;
        case 12: return NpcRole::Castle;
        case 13: return NpcRole::Guide;
        case 14: return NpcRole::Titan;
        case 15: return NpcRole::Bobusang;
        case 16: return NpcRole::FortwarSymbol;
        case 23: return NpcRole::Bomul;
        case 27: return NpcRole::MapChange;
        default: return NpcRole::Object;
    }
}

// True for NPC roles that grant, advance, or complete a player quest.
// Drives the "!" indicator above the NPC sprite.  1:1 with the
// (TALKER_ROLE=6, WANTED_ROLE=9, SURYUN_ROLE=10) subset of the legacy
// enum — these are the only NPC types that surface quest dialogs in
// the original client.
constexpr bool role_has_quest_indicator(NpcRole r) noexcept {
    return r == NpcRole::Talker
        || r == NpcRole::Wanted
        || r == NpcRole::Suryun;
}

// Map-change NPCs are routed by AgentServer's UserConn handler rather than
// the generic Npc forwarder.  Keeping this decision beside the wire-role
// mapping prevents the client from emitting a ChangeMapSyn under Category::Npc.
constexpr bool role_uses_agent_route(NpcRole r) noexcept {
    return r == NpcRole::MapChange;
}

}  // namespace mxh::game
