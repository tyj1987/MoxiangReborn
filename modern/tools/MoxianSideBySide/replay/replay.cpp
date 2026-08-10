// replay/replay.cpp - deterministic packet sequences for the 5-op
// T3 side-by-side test set.
//
// Each scenario returns the same hard-coded client->server packets
// the legacy client would send at that point. The harness plays
// these against both old + modern server, then diffs the responses.
// We deliberately avoid timestamps / random IDs in the inputs;
// only the responses (server->client) may carry jitter, and the
// diff module masks those.
//
// Format contracts (locked against modern LoginHandler/AgentHandler/MapHandler):
//   login      -> LoginServer port  payload >=38B
//                 [AuthKey: u32][id: 17B][pw: 17B]
//   enter_game -> AgentServer port  two-step:
//                 proto=16 (CharacterSelectSyn) object_id=char_id payload=[channel:u16]
//                 proto=28 (GameInSyn)         object_id=char_id payload=[channel:u32]
//   attack     -> MapServer port    cat=22 (Skill) proto=0 (StartSyn)
//                 payload=[skill:u16][target:u32]
//   shop       -> MapServer port    cat=5 (Item) proto=22 (BuySyn)
//                 payload=[item:u16][qty:u16]
//   quest      -> MapServer port    cat=39 (Quest) proto=9 (StartSyn)
//                 payload=[quest:u16]   [modern quest manager not landed;
//                 handle_quest replies StartNack with payload echo]
#include "replay.hpp"
#include <cstring>

namespace mxh::tools::sidebyside {

namespace {

Packet mk(std::uint8_t cat, std::uint8_t proto,
          std::uint32_t object_id, std::vector<std::uint8_t> payload) {
    Packet p;
    p.category = cat;
    p.protocol = proto;
    p.object_id = object_id;
    p.payload = std::move(payload);
    p.length = static_cast<std::uint32_t>(p.payload.size());
    p.direction = "c->s";
    return p;
}

void put_u16(std::uint8_t* dst, std::uint16_t v) {
    dst[0] = static_cast<std::uint8_t>(v & 0xffu);
    dst[1] = static_cast<std::uint8_t>((v >> 8) & 0xffu);
}
void put_u32(std::uint8_t* dst, std::uint32_t v) {
    dst[0] = static_cast<std::uint8_t>(v & 0xffu);
    dst[1] = static_cast<std::uint8_t>((v >> 8) & 0xffu);
    dst[2] = static_cast<std::uint8_t>((v >> 16) & 0xffu);
    dst[3] = static_cast<std::uint8_t>((v >> 24) & 0xffu);
}

}  // namespace

ReplayScenario login_scenario() {
    ReplayScenario s;
    s.name = "login";
    s.endpoint = ReplayEndpoint::Login;
    // Category 7 = MP_USERCONN. Protocol 1 = MP_USERCONN_LOGIN_SYN.
    // Payload format: [AuthKey:u32][id:17][pw:17] = 38 bytes minimum.
    std::vector<std::uint8_t> creds(38, 0);
    std::uint32_t auth_key = 1000;
    put_u32(creds.data(), auth_key);
    std::memcpy(creds.data() + 4, "test", 4);
    std::memcpy(creds.data() + 21, "test", 4);
    s.client_packets.push_back(mk(7, 1, 0, std::move(creds)));
    return s;
}

ReplayScenario enter_game_scenario() {
    ReplayScenario s;
    s.name = "enter_game";
    s.endpoint = ReplayEndpoint::Agent;
    // Two-step: CharacterSelectSyn then GameInSyn to AgentServer.
    std::vector<std::uint8_t> sel(2, 0);
    put_u16(sel.data(), 0);
    s.client_packets.push_back(mk(7, 16, 1001u, std::move(sel)));
    std::vector<std::uint8_t> gin(4, 0);
    put_u32(gin.data(), 0);
    s.client_packets.push_back(mk(7, 28, 1001u, std::move(gin)));
    return s;
}

ReplayScenario attack_scenario() {
    ReplayScenario s;
    s.name = "attack";
    s.endpoint = ReplayEndpoint::Map;
    // Modern MapServer speaks cat=Skill (22), proto=StartSyn (0).
    // Legacy cat=Mugong (9) was migrated to cat=Skill during the modern
    // rewrite; the legacy "MP_MUGONG_USE_SYN = 4" maps to StartSyn=0.
    // Payload: [skill_idx:u32][main_target:u32][target_x:f32][target_z:f32] = 16B.
    std::vector<std::uint8_t> p(16, 0);
    put_u32(p.data(), 1);      // skill_idx
    put_u32(p.data() + 4, 2);  // main_target
    put_u32(p.data() + 8, 0);  // target_x (f32 zero)
    put_u32(p.data() + 12, 0); // target_z (f32 zero)
    s.client_packets.push_back(mk(22, 0, 0, std::move(p)));
    return s;
}

ReplayScenario shop_scenario() {
    ReplayScenario s;
    s.name = "shop";
    s.endpoint = ReplayEndpoint::Map;
    // Cat 5 MP_ITEM. Proto 22 MP_ITEM_BUY_SYN. (Proto 17 was a copy-paste bug:
    // 17 is MoveAck, not BuySyn.) Payload: item(u16) + qty(u16) = 4B.
    std::vector<std::uint8_t> p(4, 0);
    put_u16(p.data(), 1);
    put_u16(p.data() + 2, 1);
    s.client_packets.push_back(mk(5, 22, 0, std::move(p)));
    return s;
}

ReplayScenario quest_scenario() {
    ReplayScenario s;
    s.name = "quest";
    s.endpoint = ReplayEndpoint::Map;
    // Cat 39 MP_QUEST. Proto 9 MP_QUEST_START_SYN (the legacy "accept a new
    // quest" packet - there is no MP_QUEST_ACCEPT_SYN; legacy used StartSyn).
    // Modern MapServer handle_quest() answers with StartNack because the
    // modern quest manager is not implemented yet.
    // Payload: quest(u16) = 2B.
    std::vector<std::uint8_t> p(2, 0);
    put_u16(p.data(), 1);
    s.client_packets.push_back(mk(39, 9, 0, std::move(p)));
    return s;
}

const char* endpoint_name(ReplayEndpoint endpoint) noexcept {
    switch (endpoint) {
    case ReplayEndpoint::Login: return "login";
    case ReplayEndpoint::Agent: return "agent";
    case ReplayEndpoint::Map:   return "map";
    }
    return "unknown";
}

}  // namespace mxh::tools::sidebyside