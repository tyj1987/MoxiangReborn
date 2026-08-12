// replay/replay.hpp - deterministic packet sequences.
#pragma once
#include "../packet.hpp"
#include <cstdint>
#include <vector>
#include <string>

namespace mxh::tools::sidebyside {

// A ReplayScenario returns the deterministic client->server packets
// for one operation. The harness sends these to BOTH servers
// (old + modern), then captures and diffs the s->c responses.
enum class ReplayEndpoint : std::uint8_t {
    Login,
    Agent,
    Map,
};

struct ReplayScenario {
    std::string name;
    ReplayEndpoint endpoint = ReplayEndpoint::Login;
    std::vector<Packet> client_packets;
};

const char* endpoint_name(ReplayEndpoint endpoint) noexcept;

ReplayScenario login_scenario();
ReplayScenario enter_game_scenario();
ReplayScenario attack_scenario();
ReplayScenario shop_scenario();
ReplayScenario quest_scenario();
ReplayScenario chat_scenario();
ReplayScenario move_scenario();
ReplayScenario item_use_scenario();

}  // namespace mxh::tools::sidebyside
