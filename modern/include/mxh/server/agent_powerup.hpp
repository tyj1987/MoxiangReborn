// agent_powerup.hpp - Phase 6.3 AgentPowerUp 1:1 port.
//
// Source-of-truth: legacy [Server]Agent/AgentNetworkMsgParser.cpp lines
// 206-208. The legacy MP_POWERUPMsgParser is a pass-through to BOOTMNGR
// (the BootManager singleton). The classifier below identifies which
// of the 5 PowerUp sub-protocols the message belongs to, plus the
// agent-side bookkeeping fields (ServerNum, ServerKind, ServerPort,
// ConnIdx) that BOOTMNGR consumes.
//
// Sub-protocol values are 0-indexed from MP_PROTOCOL_POWERUP enums
// in [CC]Header/Protocol.h (line 392-398).

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_POWERUP (MP_POWERUP=2 in MP_CATEGORY).
inline constexpr std::uint8_t powerup_category = 2u;

// Sub-protocols within MP_PROTOCOL_POWERUP (offset 0..4).
inline constexpr std::uint8_t powerup_booting_notify = 0u;
inline constexpr std::uint8_t powerup_bootlist_syn = 1u;
inline constexpr std::uint8_t powerup_bootlist_ack = 2u;
inline constexpr std::uint8_t powerup_connect_syn = 3u;
inline constexpr std::uint8_t powerup_connect_ack = 4u;

// Maximum number of MS-mappable agent servers (legacy ServerNum 0..MAX_AGENT-1).
inline constexpr std::uint32_t powerup_max_agent_servers = 100u;

// Input contract for classify_powerup.
struct PowerUpRequest {
    std::uint8_t protocol = 0u;
    std::uint32_t server_num = 0u;
    std::uint32_t self_port = 0u;
    std::uint32_t target_port = 0u;
    std::uint16_t agent_user_count = 0u;
    bool is_agent_kind = false;
    bool is_monitor_kind = false;
    bool is_distribute_kind = false;
    bool is_map_kind = false;
    bool target_server_found = true;
    bool ms_reachable = true;
};

// Dispatch intent.
enum class PowerUpActionKind : std::uint8_t {
    forward_to_boot_manager,                  // legacy BOOTMNGR->NetworkMsgParse
    add_self_boot_list,                       // BOOTMNGR->AddSelfBootList
    start_server,                             // BOOTMNGR->StartServer
    connect_to_ms,                            // BOOTMNGR->ConnectToMS
    send_registmap_ack_to_ms,                 // MP_SERVER_REGISTMAP_ACK -> MS (agent -> ms)
    send_user_count_to_distribute,            // MP_SERVER_USERCNT -> distribute
    send_registmap_syn_to_map,                // MP_SERVER_REGISTMAP_SYN -> map server
    set_map_regist,                           // ServerTable->SetMapRegist(wHaveMapNum, port)
    map_user_unregist_login,                  // MapUserUnRegistLoginMapInfo(port)
    drop_unknown_server_kind,                 // kind not recognized
    drop_unreachable_ms,                      // MS unreachable -> shutdown hook
};

struct PowerUpAction {
    PowerUpActionKind kind = PowerUpActionKind::forward_to_boot_manager;
    std::uint8_t reply_protocol = 0u;     // outgoing MP_SERVER sub-protocol
    std::uint32_t target_port = 0u;       // ServerTable port (for routing)
    std::uint16_t map_num = 0u;           // 0 = unset, else MP_SERVER_REGISTMAP_ACK payload
    std::uint16_t user_count = 0u;        // for MP_SERVER_USERCNT
    bool forward_payload = false;
    bool need_assert = false;             // legacy ASSERT(0) on StartServer fail
};

PowerUpAction classify_powerup(const PowerUpRequest& r);
// Self-init classifier (ServerSystem.cpp lines 309-318): whether to add
// self to BOOTMNGR, start server, or abort.
PowerUpAction classify_powerup_self_init(const PowerUpRequest& r);

// MP_AGENTSERVERMsgParser dispatch (lines 230-280): dispatch by target
// SERVER_KIND to REGISTMAP_ACK / USERCNT / REGISTMAP_SYN.
PowerUpAction classify_powerup_server_kind_dispatch(const PowerUpRequest& r);

// MP_SERVER_REGISTMAP_ACK inbound (lines 265-280): set map registration.
PowerUpAction classify_powerup_registmap_ack(const PowerUpRequest& r, std::uint16_t map_num);

}  // namespace mxh::server