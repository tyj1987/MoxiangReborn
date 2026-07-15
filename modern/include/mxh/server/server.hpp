// server.hpp - Server framework.
//
// Provides high-level handlers for the major game-server subsystems
// (Distribute, Agent, Map) using the net/crypto/proto/db stack.

#pragma once

#include "mxh/db/db_adapter.hpp"
#include "mxh/net/net.hpp"
#include "mxh/proto/protocol.hpp"
#include "mxh/game/item_types.hpp"
#include "mxh/game/monster_types.hpp"
#include "mxh/game/skill_types.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace mxh::server {

// Forward declare TcpServer so handlers can send replies.
namespace net_detail { class TcpServerBase; }

// Send-reply callback type. Used by handlers to reply to clients.
using ReplyFn = std::function<void(mxh::net::ConnectionId, const mxh::net::Message&)>;

// Forward declare TcpServer so handlers can use it in ReplyFn.
namespace net { class TcpServer; }

// LoginServer = phase 1 of client connection (Distribute).
// Validates user credentials and tells client which AgentServer to connect to.
class LoginHandler final : public mxh::net::IConnectionHandler {
public:
    LoginHandler(mxh::db::IDbAdapter& db,
                 std::string agent_addr,
                 std::uint16_t agent_port,
                 ReplyFn reply,
                 bool use_legacy_framing = false);
    ~LoginHandler() override = default;

    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override;
    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg) override;
    void on_disconnect(mxh::net::ConnectionId id,
                       mxh::net::NetError reason) override;

private:
    void handle_userconn(mxh::net::ConnectionId id,
                         const mxh::net::Message& msg);
    void handle_version_check(mxh::net::ConnectionId id,
                              const mxh::net::Message& msg);
    void handle_login(mxh::net::ConnectionId id,
                      const mxh::net::Message& msg);
    void handle_legacy_login(mxh::net::ConnectionId id,
                             const mxh::net::Message& msg);

    mxh::db::IDbAdapter& db_;
    std::string agent_addr_;
    std::uint16_t agent_port_;
    ReplyFn reply_;
    bool use_legacy_framing_;

    // Phase 4.3: track connections that passed version check.
    std::mutex version_mu_;
    std::unordered_set<std::uint64_t> version_verified_;

    // Phase 7.6: per-connection auth keys for legacy protocol.
    std::mutex auth_mu_;
    std::unordered_map<std::uint64_t, std::uint32_t> auth_keys_;
    std::uint32_t next_auth_key_ = 1000;
};

// AgentHandler - serves the per-user agent connection.
// Returns character list and game-start entry points.
// Phase 9: forwards GameInSyn to MapServer and relays responses.
class AgentHandler final : public mxh::net::IConnectionHandler {
public:
    AgentHandler(mxh::db::IDbAdapter& db, ReplyFn reply,
                 bool use_legacy_framing = false);
    ~AgentHandler() override = default;

    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override;
    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg) override;
    void on_disconnect(mxh::net::ConnectionId id,
                       mxh::net::NetError reason) override;

    // Phase 9: connect to MapServer for GameIn forwarding.
    // Phase 12.1 P2-13: takes ITcpSender* (was TcpClient*) so tests can
    // inject a mock without subclassing TcpClient. Production code passes
    // a real mxh::net::TcpClient; tests pass a MockTcpSender.
    void set_map_server(mxh::net::ITcpSender* client,
                        mxh::net::ConnectionId map_conn_id);
    mxh::net::ConnectionId get_map_connection() const;

    // Phase 9: forward MapServer response to the correct client.
    void forward_from_map(mxh::net::ConnectionId map_id,
                          const mxh::net::Message& msg);

private:
    void handle_userconn(mxh::net::ConnectionId id,
                         const mxh::net::Message& msg);
    void handle_legacy_character_list(mxh::net::ConnectionId id,
                                      const mxh::net::Message& msg);
    void handle_legacy_name_check(mxh::net::ConnectionId id,
                                  const mxh::net::Message& msg);
    void handle_legacy_character_make(mxh::net::ConnectionId id,
                                      const mxh::net::Message& msg);
    void handle_legacy_character_select(mxh::net::ConnectionId id,
                                        const mxh::net::Message& msg);
    void handle_legacy_gamein_syn(mxh::net::ConnectionId id,
                                   const mxh::net::Message& msg);
    std::uint32_t get_user_id(mxh::net::ConnectionId id);
    std::uint32_t get_char_id(mxh::net::ConnectionId id);

    mxh::db::IDbAdapter& db_;
    ReplyFn reply_;
    bool use_legacy_framing_;

    // Track user_id per connection (set during CharacterListSyn).
    std::mutex user_mu_;
    std::unordered_map<std::uint64_t, std::uint32_t> conn_user_ids_;
    // Track character_id per connection (set during CharacterSelectSyn).
    std::unordered_map<std::uint64_t, std::uint32_t> conn_char_ids_;
    // Track map_num per connection (set during CharacterSelectSyn).
    std::unordered_map<std::uint64_t, std::uint16_t> conn_map_nums_;

    // Phase 9: MapServer forwarding.
    // Phase 12.1 P2-13: map_client_ now holds an ITcpSender* (was
    // TcpClient*) so unit tests can inject a MockTcpSender and verify
    // GameOutSyn / GameInSyn forwarding without a real map server.
    mxh::net::ITcpSender* map_client_ = nullptr;
    mxh::net::ConnectionId map_conn_id_{};
    // char_id → client_connection_id mapping for routing MapServer responses.
    std::mutex map_route_mu_;
    std::unordered_map<std::uint32_t, std::uint64_t> char_to_client_;
};

// MapHandler - serves per-map instance.
// Each MapServer process manages one map. Handles:
//   - GAMEIN_SYN → GAMEIN_ACK (enter game with SEND_HERO_TOTALINFO)
//   - CHARACTER_ADD → notify other players about new player
//   - OBJECT_REMOVE → notify other players about player leaving
//   - MOVE category (movement sync, broadcast to others)
//   - CHAT category (P0: echo to all connected)
//   - USERCONN category (grid, visibility, etc.)
// Legacy framing is mandatory for MapServer (original clients connect
// directly after receiving Agent address from DistributeServer).
class MapHandler final : public mxh::net::IConnectionHandler {
public:
    // Per-player state stored on the server.
    struct PlayerInfo {
        std::uint32_t player_id = 0;
        std::uint64_t conn_id = 0;  // TCP connection for reply routing
        std::uint16_t map_num = 0;
        float pos_x = 0, pos_z = 0;  // current position
        char name[17] = {};           // character name
        std::uint16_t level = 1;
        std::uint8_t gender = 0;
        std::uint8_t face_type = 0;
        std::uint8_t hair_type = 0;
        // Phase 10b: inventory & money
        std::uint32_t money = 0;
        mxh::game::ItemTotalInfo items{};
        // Phase 10d: combat stats
        mxh::game::PlayerCombatStats combat{};
    };

    MapHandler(mxh::db::IDbAdapter& db, std::uint16_t map_num,
               ReplyFn reply, bool use_legacy_framing = true);
    ~MapHandler() override = default;

    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override;
    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg) override;
    void on_disconnect(mxh::net::ConnectionId id,
                       mxh::net::NetError reason) override;

private:
    void handle_userconn(mxh::net::ConnectionId id,
                         const mxh::net::Message& msg);
    void handle_move(mxh::net::ConnectionId id,
                     const mxh::net::Message& msg);
    void handle_chat(mxh::net::ConnectionId id,
                     const mxh::net::Message& msg);
    void handle_gamein(mxh::net::ConnectionId id,
                       const mxh::net::Message& msg);
    // Phase 10b: Item protocol handling
    void handle_item(mxh::net::ConnectionId id,
                     const mxh::net::Message& msg);
    // Phase 10c: Monster/NPC protocol handling
    void handle_monster(mxh::net::ConnectionId id,
                        const mxh::net::Message& msg);
    void handle_npc(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg);
    // Phase 10d: Skill/Battle protocol handling
    void handle_skill(mxh::net::ConnectionId id,
                      const mxh::net::Message& msg);
    void handle_battle(mxh::net::ConnectionId id,
                       const mxh::net::Message& msg);

    // Phase 10c: Monster management
    void spawn_monsters();
    void send_monster_add(std::uint32_t player_id,
                          const mxh::game::MonsterInstance& monster);
    void send_monster_remove(std::uint32_t player_id,
                             std::uint32_t monster_object_id);
    void broadcast_monster_add(const mxh::game::MonsterInstance& monster);
    void broadcast_monster_remove(std::uint32_t monster_object_id);
    void broadcast_monster_life(const mxh::game::MonsterInstance& monster);
    void tick_monster_ai();

    // Phase 10d: Skill management
    void init_skill_table();
    const mxh::game::SkillInfo* find_skill(std::uint32_t skill_idx) const;
    mxh::game::DamageResult calculate_damage(
        const mxh::game::PlayerCombatStats& attacker,
        const mxh::game::PlayerCombatStats& defender,
        const mxh::game::SkillInfo& skill);
    void broadcast_skill_object_add(
        std::uint32_t except_player_id,
        const mxh::game::SkillInstance& skill_obj,
        std::uint32_t caster_id);
    void broadcast_skill_object_remove(
        std::uint32_t except_player_id,
        std::uint32_t skill_object_id);
    void send_skill_single_result(
        std::uint32_t target_player_id,
        std::uint32_t target_id,
        std::int32_t damage,
        std::uint8_t hit_result);

    // Broadcast Helpers
    void broadcast_except(std::uint32_t except_player_id,
                          const mxh::net::Message& msg);
    void send_character_add(std::uint32_t target_player_id,
                            const PlayerInfo& info);
    void send_object_remove(std::uint32_t target_player_id,
                            std::uint32_t object_id);
    // Internal: caller must hold players_mu_
    void send_character_add_locked(std::uint32_t target_player_id,
                                   const PlayerInfo& info);
    void send_object_remove_locked(std::uint32_t target_player_id,
                                   std::uint32_t object_id);

    mxh::db::IDbAdapter& db_;
    std::uint16_t map_num_;
    ReplyFn reply_;
    bool use_legacy_framing_;

    // Connected players: player_id (char_id) → player info
    // Keyed by player_id because AgentServer multiplexes multiple players
    // through a single TCP connection to MapServer.
    std::mutex players_mu_;
    std::unordered_map<std::uint32_t, PlayerInfo> connected_players_;

    // Phase 10c: Monster management
    std::mutex monsters_mu_;
    std::vector<mxh::game::MonsterInstance> monsters_;
    std::vector<mxh::game::MonsterTemplate> monster_templates_;
    std::vector<mxh::game::NpcRegen> spawn_points_;
    std::uint32_t next_monster_id_ = 50000;  // reserved range for monsters
    bool monsters_spawned_ = false;

    // Phase 10d: Skill management
    std::unordered_map<std::uint32_t, mxh::game::SkillInfo> skill_table_;
    std::mutex skills_mu_;
    std::vector<mxh::game::SkillInstance> active_skills_;
    std::uint32_t next_skill_obj_id_ = 80000;  // reserved range for skill objects
};

}  // namespace mxh::server