// replay/replay.hpp - deterministic packet sequences.
#pragma once
#include "../packet.hpp"
#include <vector>
#include <string>

namespace mxh::tools::sidebyside {

// A ReplayScenario returns the deterministic client->server packets
// for one operation. The harness sends these to BOTH servers
// (old + modern), then captures and diffs the s->c responses.
struct ReplayScenario {
    std::string name;
    std::vector<Packet> client_packets;
};

ReplayScenario login_scenario();
ReplayScenario enter_game_scenario();
ReplayScenario attack_scenario();
ReplayScenario shop_scenario();
ReplayScenario quest_scenario();

}  // namespace mxh::tools::sidebyside
