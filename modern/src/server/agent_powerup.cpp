// agent_powerup.cpp - Phase 6.3 AgentPowerUp 1:1 port.

#include "mxh/server/agent_powerup.hpp"

namespace mxh::server {

// Pure classifier. The legacy [Server]Agent/AgentNetworkMsgParser.cpp line
// 206-208 simply forwards every MP_POWERUP message to BOOTMNGR
// (BootManager singleton). Here we model the surrounding ServerSystem
// bookkeeping at lines 309-318 + MP_AGENTSERVERMsgParser lines 230-280
// for the AuxServer/PWRUP/REGISTMAP dispatch.
PowerUpAction classify_powerup(const PowerUpRequest& r) {
    using K = PowerUpActionKind;

    switch (r.protocol) {
    // BOOTING_NOTIFY: a downstream component has finished booting; forward.
    case powerup_booting_notify:
        return {K::forward_to_boot_manager, 0u, 0u, 0u, 0u, true, false};

    // BOOTLIST_SYN: legacy maps (and ms) send a request for our boot list;
    // the request is steered at BOOTMNGR which composes the reply.
    case powerup_bootlist_syn:
        return {K::forward_to_boot_manager, 0u, 0u, 0u, 0u, true, false};

    // BOOTLIST_ACK: an upstream component acks our boot list. Forward.
    case powerup_bootlist_ack:
        return {K::forward_to_boot_manager, 0u, 0u, 0u, 0u, true, false};

    // CONNECT_SYN: a server (ms/distribute/monitor) wants to connect. BOOTMNGR
    // pairs the SYN with the matching ACK back; we mark it forwarded.
    case powerup_connect_syn: {
        if (!r.ms_reachable) {
            // legacy at line 312: if MS connect-to failed the agent triggers
            // OnConnectServerFail with a MONITOR_SERVER stub. We classify
            // that as drop_unreachable_ms so the dispatcher can branch.
            return {K::drop_unreachable_ms, 0u, 0u, 0u, 0u, false, false};
        }
        return {K::forward_to_boot_manager, 0u, 0u, 0u, 0u, true, false};
    }
    case powerup_connect_ack:
        return {K::forward_to_boot_manager, 0u, 0u, 0u, 0u, true, false};

    default:
        return {K::forward_to_boot_manager, 0u, 0u, 0u, 0u, true, false};
    }
}

// Secondary classifier for the ServerSystem self-init phase at lines 309-318.
// Returns the kind of BOOTMNGR call. We split it out because the legacy
// MP_POWERUPMsgParser covers inbound only.
PowerUpAction classify_powerup_self_init(const PowerUpRequest& r) {
    using K = PowerUpActionKind;
    if (!r.ms_reachable) {
        return {K::drop_unreachable_ms, 0u, 0u, 0u, 0u, false, false};
    }
    // AddSelfBootList always runs first; StartServer and ConnectToMS chain.
    if (r.server_num < powerup_max_agent_servers) {
        return {K::add_self_boot_list, 0u, r.self_port, 0u, 0u, false, false};
    }
    return {K::start_server, 0u, r.self_port, 0u, 0u, false, false};
}

// Third classifier for the MP_AGENTSERVERMsgParser message dispatch
// (lines 230-280 legacy). It maps PWRUP/REGISTMAP/USERCNT requests to
// the matching SETPROTOCOL response based on the target's SERVER_KIND.
PowerUpAction classify_powerup_server_kind_dispatch(const PowerUpRequest& r) {
    using K = PowerUpActionKind;
    if (!r.target_server_found) {
        return {K::drop_unknown_server_kind, 0u, 0u, 0u, 0u, false, false};
    }
    if (r.is_agent_kind || r.is_monitor_kind) {
        // MP_SERVER_REGISTMAP_ACK with mapServerPort=self_port, mapnum=loadAgentNum.
        return {K::send_registmap_ack_to_ms, /*MP_SERVER_REGISTMAP_ACK*/1u, r.self_port, 0u, 0u, true, false};
    }
    if (r.is_distribute_kind) {
        // MP_SERVER_USERCNT: wPortForServer + agent user count.
        return {K::send_user_count_to_distribute, /*MP_SERVER_USERCNT*/6u, r.self_port, 0u,
                r.agent_user_count, true, false};
    }
    if (r.is_map_kind) {
        // MP_SERVER_REGISTMAP_SYN pushed at the map server, agent then receives
        // the ACK and sets the map registration.
        return {K::send_registmap_syn_to_map, /*MP_SERVER_REGISTMAP_SYN*/5u, r.target_port, 0u, 0u, true, false};
    }
    return {K::drop_unknown_server_kind, 0u, 0u, 0u, 0u, false, false};
}

// Fourth classifier for REGISTMAP_ACK inbound (mirror of the legacy
// MP_SERVER_REGISTMAP_ACK handler at lines 265-280). It marks SetMapRegist
// + the MapUserUnRegistLoginMapInfo cleanup.
PowerUpAction classify_powerup_registmap_ack(const PowerUpRequest& r, std::uint16_t map_num) {
    using K = PowerUpActionKind;
    if (map_num == 0u) {
        // Empty mapnum is acknowledged but no table update happens.
        return {K::forward_to_boot_manager, 0u, 0u, 0u, 0u, false, false};
    }
    PowerUpAction a{K::set_map_regist, 0u, r.self_port, map_num, 0u, false, false};
    if (r.self_port >= 8000u /*MAPSERVER_PORT*/ && r.self_port < 10000u /*MAXSERVER_PORT*/) {
        a.kind = K::map_user_unregist_login;
    }
    return a;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int agent_powerup_translation_unit_anchor = 0;
}