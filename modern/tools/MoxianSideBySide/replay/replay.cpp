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
    // caster_id=1001 (matches enter_game_scenario) so the M3 dev-stub-caster
    // on the modern MapServer can inject a real PlayerInfo + PlayerRuntime
    // and we exercise the full StartAck + damage path instead of the
    // deterministic Nack trace.  main_target=2 (must be a real monster on
    // map 12; the spawn list uses object_ids >= 50000 so target 2 will
    // miss the monster branch and the caster-only damage ack is emitted).
    s.client_packets.push_back(mk(22, 0, 1001u, std::move(p)));
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

ReplayScenario chat_scenario() {
    ReplayScenario s;
    s.name = "chat";
    s.endpoint = ReplayEndpoint::Map;
    // Cat 6 MP_CHAT. Proto 0 MP_CHAT_ALL (the global broadcast the legacy
    // client sends on F1). Modern agent_network_msg_parser registers
    // MP_CHATServerMsgParser but the handler is currently a stub that
    // returns no reply; we still send a deterministic 5B payload hello
    // to confirm the wire format is accepted and the server does not
    // crash on it. Capture is masked as no_response_ok so the diff
    // does not flag a missing Ack (matches the legacy server
    // behaviour for the same harness input).
    std::vector<std::uint8_t> payload = {0x68, 0x65, 0x6c, 0x6c, 0x6f};
    s.client_packets.push_back(mk(6, 0, 0, std::move(payload)));
    return s;
}

ReplayScenario move_scenario() {
    ReplayScenario s;
    s.name = "move";
    s.endpoint = ReplayEndpoint::Map;
    // Cat 8 MP_MOVE. Proto 0 Init (movement initialization). The legacy
    // client sends 4B target_x:u16 + target_z:u16 (total 4B payload).
    // Modern handle_move echoes the same packet back to the sender
    // (see handle_move sender-echo fix), so the capture is a 1-packet
    // 7B wire frame: 2B length + 1B checksum + 1B code + 1B cat + 1B proto
    // + 4B object_id + 4B payload. object_id is the player_id (we use
    // 0 since the side-by-side has no player context).
    std::vector<std::uint8_t> p(4, 0);
    put_u16(p.data(), 0x1234);  // target_x
    put_u16(p.data() + 2, 0x5678);  // target_z
    s.client_packets.push_back(mk(8, 0, 0, std::move(p)));
    return s;
}

ReplayScenario item_use_scenario() {
    ReplayScenario s;
    s.name = "item";
    s.endpoint = ReplayEndpoint::Map;
    // Cat 5 MP_ITEM. Proto 12 DiscardSyn. Modern handle_item looks
    // up the player; with no player context it sends DiscardNack.
    // Payload: position:u16 (which slot to discard).
    std::vector<std::uint8_t> p(2, 0);
    put_u16(p.data(), 0);
    s.client_packets.push_back(mk(5, 12, 0, std::move(p)));
    return s;
}

ReplayScenario party_scenario() {
    ReplayScenario s;
    s.name = "party";
    s.endpoint = ReplayEndpoint::Map;
    // Cat 14 MP_PARTY. Proto 1 MP_PARTY_CREATE_SYN.
    // Payload: party_name:char[16] + target_pid:u32 = 20B.
    std::vector<std::uint8_t> p(20, 0);
    const char name[] = { 'P','A','R','T','Y' };
    for (int i = 0; i < 5; ++i) p[i] = static_cast<std::uint8_t>(name[i]);
    put_u32(p.data() + 16, 0xDEADBEEFu);
    s.client_packets.push_back(mk(14, 1, 0, std::move(p)));
    return s;
}

ReplayScenario guild_scenario() {
    ReplayScenario s;
    s.name = "guild";
    s.endpoint = ReplayEndpoint::Map;
    // Cat 56 MP_GUILD. Proto 1 MP_GUILD_CREATE_SYN.
    // Payload: guild_name:char[16] + guild_motto:char[50] + flag:u32 = 70B.
    std::vector<std::uint8_t> p(70, 0);
    const char n2[] = { 'G','U','I','L','D' };
    for (int i = 0; i < 5; ++i) p[i] = static_cast<std::uint8_t>(n2[i]);
    const char m[] = { 'M','O','T','T','O' };
    for (int i = 0; i < 5; ++i) p[16 + i] = static_cast<std::uint8_t>(m[i]);
    put_u32(p.data() + 66, 0xCAFEBABEu);
    s.client_packets.push_back(mk(56, 1, 0, std::move(p)));
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