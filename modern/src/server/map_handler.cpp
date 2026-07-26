// map_handler.cpp - MapServer handler (per-map instance).
//
// Phase 8 P0 minimal implementation: handles the minimum protocols needed
// for a client to enter a map and move around:
//
//   USERCONN (cat=7):
//     - GameInSyn (proto=28)  → GameInAck with SEND_HERO_TOTALINFO
//     - GameOutSyn (proto=31) → GameOutAck
//     - ConnectionCheck (proto=63) → ConnectionCheckOk
//     - CharacterAdd (proto=35) → notify others about new player
//     - ObjectRemove (proto=40) → notify others about player leaving
//
//   MOVE (cat=8):
//     - OneTarget/Target/Stop → broadcast to other players
//
//   CHAT (cat=6):
//     - All (proto=0) → echo to all connected players
//
// Wire format (legacy 4DyuchiNET):
//   [2B length LE] [8B MSGBASE: checksum+code+cat+proto+objID] [payload]

#include "mxh/server/server.hpp"
#include "mxh/game/item_effects.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <random>
#include <vector>

namespace mxh::server {

// ============================================================================
// Binary payload helpers (usable by both free functions and member functions)
// ============================================================================

void put_u32(std::vector<std::uint8_t>& buf, std::size_t off, std::uint32_t val) {
    if (off + 4 <= buf.size())
        std::memcpy(buf.data() + off, &val, 4);
}

void put_u16(std::vector<std::uint8_t>& buf, std::size_t off, std::uint16_t val) {
    if (off + 2 <= buf.size())
        std::memcpy(buf.data() + off, &val, 2);
}

void put_u8(std::vector<std::uint8_t>& buf, std::size_t off, std::uint8_t val) {
    if (off < buf.size())
        buf[off] = val;
}

void put_f32(std::vector<std::uint8_t>& buf, std::size_t off, float val) {
    if (off + 4 <= buf.size())
        std::memcpy(buf.data() + off, &val, 4);
}

void put_str(std::vector<std::uint8_t>& buf, std::size_t off,
                     const char* src, std::size_t max_len) {
    std::size_t len = std::strlen(src);
    if (len >= max_len) len = max_len - 1;
    std::memcpy(buf.data() + off, src, len);
    buf[off + len] = 0;
}

// ============================================================================
// SEND_HERO_TOTALINFO payload offsets (MSGBASE is in msg.header, NOT in payload)
// ============================================================================
constexpr std::size_t kPayloadBaseObjOff    = 0;    // BASEOBJECT_INFO start
constexpr std::size_t kPayloadCharTotalOff  = 35;   // CHARACTER_TOTALINFO start
constexpr std::size_t kPayloadHeroTotalOff  = 147;  // HERO_TOTALINFO start
constexpr std::size_t kPayloadMoveInfoOff   = 207;  // SEND_MOVEINFO start
constexpr std::size_t kPayloadUniqueAgentOff= 221;  // UniqueIDinAgent
constexpr std::size_t kPayloadShopOptOff    = 225;  // SHOPITEMOPTION start
constexpr std::size_t kPayloadMugongOff     = 345;  // MUGONG_TOTALINFO start
constexpr std::size_t kPayloadAbilityOff    = 695;  // ABILITY_TOTALINFO start
constexpr std::size_t kPayloadItemOff       = 1019; // ITEM_TOTALINFO start
constexpr std::size_t kPayloadOptNumOff     = 2955; // OptionNum..TitanEndrncNum
constexpr std::size_t kPayloadServerTimeOff = 2965; // SYSTEMTIME ServerTime
constexpr std::size_t kPayloadAddableOff    = 2981; // AddableInfoList start
constexpr std::size_t kMinGameInAckPayloadSize = 3000;

// ============================================================================
// Free helper functions for building message payloads
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// DB helper: extract typed values from a ResultSet row by column name.
// ---------------------------------------------------------------------------
std::int64_t get_int(const mxh::db::ResultSet& rs, std::size_t row,
                     const char* col, std::int64_t def = 0) {
    int idx = rs.column_index(col);
    if (idx < 0) return def;
    auto& v = rs.at(row, static_cast<std::size_t>(idx));
    if (auto* p = std::get_if<std::int64_t>(&v)) return *p;
    return def;
}

double get_double(const mxh::db::ResultSet& rs, std::size_t row,
                  const char* col, double def = 0.0) {
    int idx = rs.column_index(col);
    if (idx < 0) return def;
    auto& v = rs.at(row, static_cast<std::size_t>(idx));
    if (auto* p = std::get_if<double>(&v)) return *p;
    if (auto* p = std::get_if<std::int64_t>(&v)) return static_cast<double>(*p);
    return def;
}

std::string get_str(const mxh::db::ResultSet& rs, std::size_t row,
                    const char* col, const char* def = "") {
    int idx = rs.column_index(col);
    if (idx < 0) return def;
    auto& v = rs.at(row, static_cast<std::size_t>(idx));
    if (auto* p = std::get_if<std::string>(&v)) return *p;
    return def;
}

// ---------------------------------------------------------------------------
// CharData: real character data loaded from the database.
// ---------------------------------------------------------------------------
struct CharData {
    std::uint32_t chrid = 0;
    std::string name = "Player";
    std::uint32_t life = 100, max_life = 100;
    std::uint32_t shield = 0, max_shield = 0;
    std::uint32_t naeryuk = 50, max_naeryuk = 50;
    std::uint32_t exp = 0;
    std::uint8_t gender = 0;
    std::uint8_t face_type = 1;
    std::uint8_t hair_type = 1;
    std::uint16_t level = 1;
    std::uint16_t map_num = 12;
    float height = 1.0f, width = 1.0f;
};

CharData load_char_data(mxh::db::IDbAdapter& db, std::uint32_t chrid) {
    CharData cd;
    cd.chrid = chrid;
    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> params = { mxh::db::bind(static_cast<std::int64_t>(chrid)) };
    auto r = db.query(
        "SELECT charname, sex_type, face_type, hair_type, height, width, "
        "level, map_num FROM character_info WHERE chrid = ?",
        params, rs);
    if (r.ok() && !rs.empty()) {
        cd.name      = get_str(rs, 0, "charname", "Player");
        cd.gender    = static_cast<std::uint8_t>(get_int(rs, 0, "sex_type"));
        cd.face_type = static_cast<std::uint8_t>(get_int(rs, 0, "face_type", 1));
        cd.hair_type = static_cast<std::uint8_t>(get_int(rs, 0, "hair_type", 1));
        cd.height    = static_cast<float>(get_double(rs, 0, "height", 1.0));
        cd.width     = static_cast<float>(get_double(rs, 0, "width", 1.0));
        cd.level     = static_cast<std::uint16_t>(get_int(rs, 0, "level", 1));
        cd.map_num   = static_cast<std::uint16_t>(get_int(rs, 0, "map_num", 12));
    }
    return cd;
}

mxh::net::Message make_gamein_ack(std::uint32_t player_id, std::uint16_t map_num,
                                  const CharData& cd) {
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInAck);
    m.header.object_id = player_id;

    m.payload.resize(kMinGameInAckPayloadSize, 0);

    // --- BASEOBJECT_INFO [0..34] ---
    put_u32(m.payload, kPayloadBaseObjOff + 0, player_id);
    put_u32(m.payload, kPayloadBaseObjOff + 4, player_id);
    put_str(m.payload, kPayloadBaseObjOff + 8, cd.name.c_str(), 17);

    // --- CHARACTER_TOTALINFO [35..146] ---
    put_u32(m.payload, kPayloadCharTotalOff + 0, cd.life);
    put_u32(m.payload, kPayloadCharTotalOff + 4, cd.max_life);
    put_u32(m.payload, kPayloadCharTotalOff + 8, cd.shield);
    put_u32(m.payload, kPayloadCharTotalOff + 12, cd.max_shield);
    put_u8(m.payload, kPayloadCharTotalOff + 16, cd.gender);
    put_u8(m.payload, kPayloadCharTotalOff + 17, cd.face_type);
    put_u8(m.payload, kPayloadCharTotalOff + 18, cd.hair_type);
    put_u16(m.payload, kPayloadCharTotalOff + 40, cd.level);
    put_u16(m.payload, kPayloadCharTotalOff + 42, cd.map_num);
    put_u16(m.payload, kPayloadCharTotalOff + 44, cd.map_num);
    put_u8(m.payload, kPayloadCharTotalOff + 46, 1);  // bPeace
    put_u8(m.payload, kPayloadCharTotalOff + 60, 1);  // bVisible
    put_f32(m.payload, kPayloadCharTotalOff + 70, 170.0f);
    put_f32(m.payload, kPayloadCharTotalOff + 74, 60.0f);

    // --- HERO_TOTALINFO [147..206] ---
    put_u16(m.payload, kPayloadHeroTotalOff + 0, 10);
    put_u16(m.payload, kPayloadHeroTotalOff + 2, 10);
    put_u16(m.payload, kPayloadHeroTotalOff + 4, 10);
    put_u16(m.payload, kPayloadHeroTotalOff + 6, 10);
    put_u32(m.payload, kPayloadHeroTotalOff + 8, 50);
    put_u32(m.payload, kPayloadHeroTotalOff + 12, 50);
    put_u32(m.payload, kPayloadHeroTotalOff + 34, 1000);
    put_u16(m.payload, kPayloadHeroTotalOff + 56, 1);

    // --- UniqueIDinAgent [221..224] ---
    put_u32(m.payload, kPayloadUniqueAgentOff, player_id);

    // --- ServerTime [2965..2980] --- use real system clock
    {
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm lt{};
        localtime_s(&lt, &tt);
        put_u16(m.payload, kPayloadServerTimeOff + 0,
                static_cast<std::uint16_t>(lt.tm_year + 1900));
        put_u16(m.payload, kPayloadServerTimeOff + 2,
                static_cast<std::uint16_t>(lt.tm_mon + 1));
        put_u16(m.payload, kPayloadServerTimeOff + 4,
                static_cast<std::uint16_t>(lt.tm_wday));
        put_u16(m.payload, kPayloadServerTimeOff + 6,
                static_cast<std::uint16_t>(lt.tm_mday));
        put_u16(m.payload, kPayloadServerTimeOff + 8,
                static_cast<std::uint16_t>(lt.tm_hour));
    }

    return m;
}

mxh::net::Message make_gameout_ack() {
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameOutAck);
    return m;
}

mxh::net::Message make_connection_check_ok() {
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ConnectionCheckOk);
    return m;
}

}  // namespace

// ============================================================================
// MapHandler implementation
// ============================================================================

MapHandler::MapHandler(mxh::db::IDbAdapter& db, std::uint16_t map_num,
                       ReplyFn reply, bool use_legacy_framing)
    : db_(db), map_num_(map_num), reply_(std::move(reply)),
      use_legacy_framing_(use_legacy_framing) {
    init_skill_table();
}

bool MapHandler::on_connect(mxh::net::ConnectionId /*id*/,
                            const std::string& remote_addr) {
    std::cout << "[Map] client connected from " << remote_addr
              << " (map=" << map_num_ << ")\n";
    return true;
}

void MapHandler::on_disconnect(mxh::net::ConnectionId id,
                               mxh::net::NetError reason) {
    std::cout << "[Map] client disconnected (id=" << id.value
              << " reason=" << mxh::net::to_string(reason) << ")\n";
    // Remove ALL players on this TCP connection (AgentServer multiplexes).
    std::vector<std::uint32_t> removed_pids;
    std::vector<std::uint32_t> remaining_pids;
    {
        std::lock_guard<std::mutex> lk(players_mu_);
        for (auto it = connected_players_.begin(); it != connected_players_.end(); ) {
            if (it->second.conn_id == id.value) {
                removed_pids.push_back(it->first);
                it = connected_players_.erase(it);
            } else {
                ++it;
            }
        }
        for (auto& [pid, info] : connected_players_) {
            remaining_pids.push_back(pid);
        }
    }
    for (auto removed_pid : removed_pids) {
        for (auto remaining_pid : remaining_pids) {
            send_object_remove_locked(remaining_pid, removed_pid);
        }
        std::cout << "[Map] notified others about player=" << removed_pid << " leaving\n";
    }
}

void MapHandler::on_message(mxh::net::ConnectionId id,
                            const mxh::net::Message& msg) {
    auto cat = static_cast<mxh::proto::Category>(msg.header.category);
    std::cout << "[Map] on_message cat=" << mxh::proto::category_name(cat)
              << " proto=" << (int)msg.header.protocol
              << " obj=" << msg.header.object_id
              << " from conn=" << id.value << "\n";

    switch (cat) {
        case mxh::proto::Category::UserConn:
            handle_userconn(id, msg);
            break;
        case mxh::proto::Category::Move:
            handle_move(id, msg);
            break;
        case mxh::proto::Category::Chat:
            handle_chat(id, msg);
            break;
        case mxh::proto::Category::Item:
            handle_item(id, msg);
            break;
        case mxh::proto::Category::Monster:
            handle_monster(id, msg);
            break;
        case mxh::proto::Category::Npc:
            handle_npc(id, msg);
            break;
        case mxh::proto::Category::Skill:
            handle_skill(id, msg);
            break;
        case mxh::proto::Category::Battle:
            handle_battle(id, msg);
            break;
        default:
            std::cout << "[Map] unhandled category="
                      << mxh::proto::category_name(cat)
                      << " proto=" << (int)msg.header.protocol << "\n";
            break;
    }
}

void MapHandler::handle_userconn(mxh::net::ConnectionId id,
                                 const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::UserConnProtocol>(msg.header.protocol);

    switch (proto) {
        case mxh::proto::UserConnProtocol::GameInSyn: {
            handle_gamein(id, msg);
            break;
        }
        case mxh::proto::UserConnProtocol::GameOutSyn: {
            std::uint32_t player_id = msg.header.object_id;
            std::cout << "[Map] GameOutSyn from player=" << player_id << "\n";
            std::vector<std::uint32_t> remaining_pids;
            {
                std::lock_guard<std::mutex> lk(players_mu_);
                connected_players_.erase(player_id);
                for (auto& [pid, info] : connected_players_) {
                    remaining_pids.push_back(pid);
                }
            }
            for (auto rpid : remaining_pids) {
                send_object_remove(rpid, player_id);
            }
            if (!remaining_pids.empty()) {
                std::cout << "[Map] notified others about player=" << player_id << " leaving (GameOut)\n";
            }
            reply_(id, make_gameout_ack());
            break;
        }
        case mxh::proto::UserConnProtocol::ConnectionCheck: {
            std::cout << "[Map] ConnectionCheck from player=" << id.value << "\n";
            reply_(id, make_connection_check_ok());
            break;
        }
        default:
            std::cout << "[Map] unhandled userconn proto="
                      << (int)proto << " from player=" << id.value << "\n";
            break;
    }
}

void MapHandler::handle_gamein(mxh::net::ConnectionId id,
                               const mxh::net::Message& msg) {
    std::uint32_t player_id = msg.header.object_id;
    std::cout << "[Map] GAMEIN_SYN from player=" << player_id
              << " payload=" << msg.payload.size() << "B\n";

    // Phase 9.1: Load real character data from DB.
    CharData cd = load_char_data(db_, player_id);

    // Store player info for CHARACTER_ADD broadcasts to other players.
    PlayerInfo pi;
    pi.player_id = player_id;
    pi.map_num = map_num_;
    pi.pos_x = 0; pi.pos_z = 0;
    std::memset(pi.name, 0, sizeof(pi.name));
    auto name_bytes = cd.name.c_str();
    std::size_t name_len = std::strlen(name_bytes);
    if (name_len >= sizeof(pi.name)) name_len = sizeof(pi.name) - 1;
    std::memcpy(pi.name, name_bytes, name_len);
    pi.level = cd.level;
    pi.gender = cd.gender;
    pi.face_type = cd.face_type;
    pi.hair_type = cd.hair_type;

    // Store player info keyed by player_id (AgentServer multiplexes).
    pi.conn_id = id.value;
    {
        std::lock_guard<std::mutex> lk(players_mu_);
        connected_players_[player_id] = pi;
    }

    // Send GAMEIN_ACK to this player (full self info from DB).
    reply_(id, make_gamein_ack(player_id, map_num_, cd));
    std::cout << "[Map] sent GAMEIN_ACK to player=" << player_id
              << " name='" << cd.name << "' level=" << cd.level
              << " (connected=" << connected_players_.size() << ")\n";

    // Phase 10b: Send item total info after GAMEIN_ACK.
    // In the original, MapServer sends MP_ITEM_TOTALINFO_LOCAL right after
    // GAMEIN_ACK so the client can populate the inventory UI.
    {
        std::lock_guard<std::mutex> lk(players_mu_);
        auto it = connected_players_.find(player_id);
        if (it != connected_players_.end()) {
            mxh::net::Message item_msg;
            item_msg.header.category = static_cast<std::uint8_t>(
                mxh::proto::Category::Item);
            item_msg.header.protocol = static_cast<std::uint8_t>(
                mxh::proto::ItemProtocol::TotalInfoLocal);
            item_msg.header.object_id = player_id;

            // Payload: MONEYTYPE(4B) + ITEM_TOTALINFO(2420B)
            mxh::game::SendItemTotalInfoLocal item_data{};
            item_data.Money = it->second.money;
            item_data.Items = it->second.items;
            item_msg.payload.resize(sizeof(item_data), 0);
            std::memcpy(item_msg.payload.data(), &item_data, sizeof(item_data));
            reply_(id, item_msg);
            std::cout << "[Map] sent ITEM_TOTALINFO_LOCAL to player=" << player_id
                      << " money=" << it->second.money
                      << " (" << sizeof(item_data) << "B)\n";
        }
    }

    // Send existing players to this new player
    // And send this new player to all existing players
    {
        std::lock_guard<std::mutex> lk(players_mu_);
        for (auto& [pid, existing] : connected_players_) {
            if (pid == player_id) continue;

            send_character_add_locked(pid, pi);
            send_character_add_locked(player_id, existing);
        }
    }
    std::cout << "[Map] broadcast CHARACTER_ADD for player=" << player_id << "\n";

    // Phase 10c: Spawn monsters on first player entry and send existing monsters
    {
        std::lock_guard<std::mutex> lk(monsters_mu_);
        if (!monsters_spawned_) {
            // Load templates and spawn points (P0: use defaults)
            monster_templates_ = mxh::game::get_default_templates();
            spawn_points_ = mxh::game::get_default_spawn_points(map_num_);
            spawn_monsters();
            monsters_spawned_ = true;
            std::cout << "[Map] spawned " << monsters_.size() << " monsters\n";
        }
        // Send all existing monsters to this new player
        for (auto& m : monsters_) {
            if (!m.is_dead) {
                send_monster_add(player_id, m);
            }
        }
    }
    std::cout << "[Map] sent " << monsters_.size() << " monster adds to player=" << player_id << "\n";
}

void MapHandler::handle_move(mxh::net::ConnectionId /*id*/,
                             const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::MoveProtocol>(msg.header.protocol);
    std::uint32_t sender_pid = msg.header.object_id;

    switch (proto) {
        case mxh::proto::MoveProtocol::OneTarget:
        case mxh::proto::MoveProtocol::Target:
        case mxh::proto::MoveProtocol::Init:
        case mxh::proto::MoveProtocol::Stop: {
            mxh::net::Message fwd;
            fwd.header = msg.header;
            fwd.payload = msg.payload;
            broadcast_except(sender_pid, fwd);
            break;
        }
        case mxh::proto::MoveProtocol::Warp:
            std::cout << "[Map] Warp from player=" << sender_pid << "\n";
            {
                mxh::net::Message fwd;
                fwd.header = msg.header;
                fwd.payload = msg.payload;
                broadcast_except(sender_pid, fwd);
            }
            break;
        default:
            std::cout << "[Map] move proto=" << (int)proto
                      << " from player=" << sender_pid << "\n";
            break;
    }
}

void MapHandler::handle_chat(mxh::net::ConnectionId /*id*/,
                             const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::ChatProtocol>(msg.header.protocol);
    std::uint32_t sender_pid = msg.header.object_id;

    if (proto == mxh::proto::ChatProtocol::All) {
        std::cout << "[Map] Chat(All) from player=" << sender_pid
                  << " len=" << msg.payload.size() << "\n";

        // Phase 9.3: AgentServer multiplexes all players through a single
        // TCP connection, so we CANNOT filter by conn_id (all players share
        // the same conn).  Broadcast to ALL active connections and let
        // AgentServer filter by char_id in forward_from_map().
        std::lock_guard<std::mutex> lk(players_mu_);
        std::vector<std::uint64_t> active_conns;
        for (auto& [pid, pinfo] : connected_players_) {
            if (std::find(active_conns.begin(), active_conns.end(), pinfo.conn_id)
                == active_conns.end()) {
                active_conns.push_back(pinfo.conn_id);
            }
        }
        std::cout << "[Map] Chat broadcast players=" << connected_players_.size()
                  << " target_conns=" << active_conns.size() << "\n";
        for (auto conn : active_conns) {
            mxh::net::ConnectionId target{conn};
            mxh::net::Message echo;
            echo.header.category = msg.header.category;
            echo.header.protocol = msg.header.protocol;
            echo.header.object_id = msg.header.object_id;
            echo.payload = msg.payload;
            reply_(target, echo);
        }
    } else {
        std::cout << "[Map] chat proto=" << (int)proto
                  << " from player=" << sender_pid << "\n";
    }
}

// ============================================================================
// CHARACTER_ADD / OBJECT_REMOVE message construction
// ============================================================================

void MapHandler::send_character_add(std::uint32_t target_player_id,
                                   const PlayerInfo& info) {
    std::lock_guard<std::mutex> lk(players_mu_);
    send_character_add_locked(target_player_id, info);
}

void MapHandler::send_character_add_locked(std::uint32_t target_player_id,
                                           const PlayerInfo& info) {
    // Caller must hold players_mu_.
    // Phase 9.2: AgentServer multiplexes all players through a single TCP
    // connection.  Instead of looking up the target's stored conn_id (which
    // may be stale after a MapClient reconnect), send to ALL active
    // connections.  The AgentServer will route the message to the correct
    // test client based on char_to_client_.
    std::vector<std::uint64_t> active_conns;
    for (auto& [pid, pinfo] : connected_players_) {
        if (std::find(active_conns.begin(), active_conns.end(), pinfo.conn_id)
            == active_conns.end()) {
            active_conns.push_back(pinfo.conn_id);
        }
    }
    // Also send to the target's stored conn_id if present.
    auto it = connected_players_.find(target_player_id);
    if (it != connected_players_.end()) {
        auto c = it->second.conn_id;
        if (std::find(active_conns.begin(), active_conns.end(), c)
            == active_conns.end()) {
            active_conns.push_back(c);
        }
    }
    for (auto conn : active_conns) {
        mxh::net::ConnectionId target{conn};
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterAdd);
        m.header.object_id = info.player_id;

    constexpr std::size_t kPayloadSize = 35 + 112 + 14 + 120 + 4 + 1 + 2;
    m.payload.resize(kPayloadSize, 0);

    std::size_t off = 0;

    // BASEOBJECT_INFO [0..34]
    put_u32(m.payload, off + 0, info.player_id);
    put_u32(m.payload, off + 4, info.player_id);
    put_str(m.payload, off + 8, info.name, 17);
    off += 35;

    // CHARACTER_TOTALINFO [35..146]
    put_u16(m.payload, off + 16, info.gender);
    put_u8(m.payload, off + 17, info.face_type);
    put_u8(m.payload, off + 18, info.hair_type);
    put_u16(m.payload, off + 40, info.level);
    put_u16(m.payload, off + 42, info.map_num);
    put_u16(m.payload, off + 44, info.map_num);
    put_u8(m.payload, off + 46, 1);  // bPeace
    put_u8(m.payload, off + 60, 1);  // bVisible
    put_f32(m.payload, off + 70, 170.0f);
    put_f32(m.payload, off + 74, 60.0f);
    off += 112;

    // SEND_MOVEINFO [147..160]
    put_u16(m.payload, off + 0, static_cast<std::uint16_t>(info.pos_x));
    put_u16(m.payload, off + 2, static_cast<std::uint16_t>(info.pos_z));
    off += 14;

    // SHOPITEMOPTION [147..266] - all zeros
    off += 120;

    // bInTitan (4B) = 0
    off += 4;

    // bLogin (1B) = 1
    m.payload[off] = 1;
    off += 1;

    // AddableInfoList (2B) = empty list (size=0)

        reply_(target, m);
    }
}

void MapHandler::send_object_remove(std::uint32_t target_player_id,
                                   std::uint32_t object_id) {
    std::lock_guard<std::mutex> lk(players_mu_);
    send_object_remove_locked(target_player_id, object_id);
}

void MapHandler::send_object_remove_locked(std::uint32_t target_player_id,
                                           std::uint32_t object_id) {
    // Caller must hold players_mu_.
    std::uint64_t target_conn = 0;
    auto it = connected_players_.find(target_player_id);
    if (it != connected_players_.end()) target_conn = it->second.conn_id;
    if (target_conn == 0) return;
    mxh::net::ConnectionId target{target_conn};
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ObjectRemove);
    m.header.object_id = 0;

    m.payload.resize(4, 0);
    put_u32(m.payload, 0, object_id);

    reply_(target, m);
}

void MapHandler::broadcast_except(std::uint32_t except_player_id,
                                 const mxh::net::Message& msg) {
    std::lock_guard<std::mutex> lk(players_mu_);
    // Phase 10c fix: AgentServer multiplexes multiple players through a
    // single TCP connection.  We CANNOT exclude the sender's connection
    // because other players may share it.  Send to ALL connections and
    // let AgentServer's forward_from_map handle per-player filtering.
    std::vector<std::uint64_t> active_conns;
    for (auto& [pid, info] : connected_players_) {
        if (std::find(active_conns.begin(), active_conns.end(), info.conn_id)
            == active_conns.end()) {
            active_conns.push_back(info.conn_id);
        }
    }
    std::cout << "[Map] broadcast_except sender=" << except_player_id
              << " players=" << connected_players_.size()
              << " conns=" << active_conns.size() << "\n";
    for (auto conn : active_conns) {
        mxh::net::ConnectionId target{conn};
        reply_(target, msg);
    }
}

// ============================================================================
// Phase 10b: Item protocol handling
// ============================================================================

void MapHandler::handle_item(mxh::net::ConnectionId id,
                             const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::ItemProtocol>(msg.header.protocol);
    std::uint32_t player_id = msg.header.object_id;

    std::cout << "[Map] ITEM proto=" << static_cast<int>(proto)
              << " player=" << player_id
              << " payload=" << msg.payload.size() << "B\n";

    switch (proto) {
        // --- C -> S: Move item within inventory ---
        case mxh::proto::ItemProtocol::MoveSyn: {
            // Payload: ITEMBASE(22B) of the item being moved + target position(2B)
            // Original: dwDBIdx(4) + wIconIdx(2) + Position(2) + Dur(4) + Rare(4)
            //          + QuickPos(2) + ItemParam(4) + NewPosition(2)
            if (msg.payload.size() < 24) {
                std::cout << "[Map] ITEM_MOVE_SYN: payload too small\n";
                break;
            }
            // Read item DB idx and target position
            std::uint32_t item_db_idx = 0;
            std::uint16_t new_pos = 0;
            std::memcpy(&item_db_idx, msg.payload.data(), 4);
            std::memcpy(&new_pos, msg.payload.data() + 22, 2);

            std::cout << "[Map] ITEM_MOVE_SYN item_db=" << item_db_idx
                      << " new_pos=" << new_pos << "\n";

            // Update in-memory inventory
            bool found = false;
            {
                std::lock_guard<std::mutex> lk(players_mu_);
                auto it = connected_players_.find(player_id);
                if (it != connected_players_.end()) {
                    auto& inv = it->second.items;
                    // Find item in inventory (0..79) or equipment (80..89)
                    for (int i = 0; i < mxh::game::SLOT_INVENTORY_NUM + mxh::game::WEARED_ITEM_MAX; ++i) {
                        mxh::game::ItemBase* slot = nullptr;
                        if (i < mxh::game::SLOT_INVENTORY_NUM)
                            slot = &inv.Inventory[i];
                        else
                            slot = &inv.WearedItem[i - mxh::game::SLOT_INVENTORY_NUM];
                        if (slot && slot->dwDBIdx == item_db_idx) {
                            // Swap positions
                            slot->Position = new_pos;
                            found = true;
                            break;
                        }
                    }
                }
            }

            // Send MoveAck or MoveNack
            mxh::net::Message reply;
            reply.header.category = static_cast<std::uint8_t>(
                mxh::proto::Category::Item);
            reply.header.protocol = static_cast<std::uint8_t>(
                found ? mxh::proto::ItemProtocol::MoveAck
                       : mxh::proto::ItemProtocol::MoveNack);
            reply.header.object_id = player_id;
            reply.payload = msg.payload; // echo back the item data
            reply_(id, reply);
            std::cout << "[Map] sent ITEM_MOVE_"
                      << (found ? "ACK" : "NACK") << "\n";
            break;
        }

        // --- C -> S: Discard item ---
        case mxh::proto::ItemProtocol::DiscardSyn: {
            // Payload: position(2B) or ITEMBASE(22B)
            if (msg.payload.size() < 2) {
                std::cout << "[Map] ITEM_DISCARD_SYN: payload too small\n";
                break;
            }
            std::uint16_t pos = 0;
            std::memcpy(&pos, msg.payload.data(), 2);
            std::cout << "[Map] ITEM_DISCARD_SYN pos=" << pos << "\n";

            bool found = false;
            {
                std::lock_guard<std::mutex> lk(players_mu_);
                auto it = connected_players_.find(player_id);
                if (it != connected_players_.end()) {
                    auto& inv = it->second.items;
                    // Clear the slot at the given position
                    if (pos < mxh::game::TP_INVENTORY_END) {
                        inv.Inventory[pos] = mxh::game::make_empty_item();
                        found = true;
                    } else if (pos < mxh::game::TP_WEAREDITEM_END) {
                        inv.WearedItem[pos - mxh::game::TP_WEAREDITEM_START] =
                            mxh::game::make_empty_item();
                        found = true;
                    } else if (pos < mxh::game::TP_SHOPINVEN_END) {
                        inv.ShopInventory[pos - mxh::game::TP_SHOPINVEN_START] =
                            mxh::game::make_empty_item();
                        found = true;
                    }
                }
            }

            mxh::net::Message reply;
            reply.header.category = static_cast<std::uint8_t>(
                mxh::proto::Category::Item);
            reply.header.protocol = static_cast<std::uint8_t>(
                found ? mxh::proto::ItemProtocol::DiscardAck
                       : mxh::proto::ItemProtocol::DiscardNack);
            reply.header.object_id = player_id;
            reply.payload = msg.payload;
            reply_(id, reply);
            std::cout << "[Map] sent ITEM_DISCARD_"
                      << (found ? "ACK" : "NACK") << "\n";
            break;
        }

        // --- C -> S: Use item ---
        case mxh::proto::ItemProtocol::UseSyn: {
            // Payload: position(2B)
            if (msg.payload.size() < 2) {
                std::cout << "[Map] ITEM_USE_SYN: payload too small\n";
                break;
            }
            std::uint16_t pos = 0;
            std::memcpy(&pos, msg.payload.data(), 2);
            std::cout << "[Map] ITEM_USE_SYN pos=" << pos << "\n";

            // Phase 12.1: apply item effects on UseSyn.
            //
            // 1) Look up the item at the given position in the
            //    player's inventory. If no item / wrong range, send
            //    UseNack.
            // 2) Classify the item by wIconIdx. Non-consumables
            //    (wIconIdx >= 400 or wIconIdx == 0) also yield
            //    UseNack.
            // 3) Resolve the effect (HP / MP / buff delta) and
            //    apply to the player's combat stats. Clamp HP/MP
            //    to [0, max_*].
            // 4) Send UseAck with a small payload describing the
            //    applied effect (HP delta, MP delta, new HP, new
            //    MP). This is sufficient for clients to update
            //    their UI; a richer "UseEffectNotify" can be
            //    added later.
            mxh::game::ItemBase used_item{};
            bool found = false;
            {
                std::lock_guard<std::mutex> lk(players_mu_);
                auto it = connected_players_.find(player_id);
                if (it != connected_players_.end()) {
                    auto& inv = it->second.items;
                    if (pos < mxh::game::TP_INVENTORY_END) {
                        used_item = inv.Inventory[pos];
                        found = !mxh::game::is_empty_slot(used_item);
                    }
                }
            }
            if (!found) {
                mxh::net::Message reply;
                reply.header.category = static_cast<std::uint8_t>(
                    mxh::proto::Category::Item);
                reply.header.protocol = static_cast<std::uint8_t>(
                    mxh::proto::ItemProtocol::UseNack);
                reply.header.object_id = player_id;
                reply.payload = msg.payload;
                reply_(id, reply);
                std::cout << "[Map] sent ITEM_USE_NACK (no item at pos)\n";
                break;
            }

            const auto kind = mxh::game::classify_item(used_item.wIconIdx);
            const auto effect = mxh::game::resolve_item_effect(used_item.wIconIdx);
            if (kind == mxh::game::ItemEffectKind::None) {
                // Equipment / scroll / unknown — not consumable.
                mxh::net::Message reply;
                reply.header.category = static_cast<std::uint8_t>(
                    mxh::proto::Category::Item);
                reply.header.protocol = static_cast<std::uint8_t>(
                    mxh::proto::ItemProtocol::UseNack);
                reply.header.object_id = player_id;
                reply.payload = msg.payload;
                reply_(id, reply);
                std::cout << "[Map] sent ITEM_USE_NACK (non-consumable wIconIdx="
                          << used_item.wIconIdx << ")\n";
                break;
            }

            // Apply the effect to player combat stats.
            std::uint32_t new_hp = 0;
            std::uint32_t new_mp = 0;
            {
                std::lock_guard<std::mutex> lk(players_mu_);
                auto it = connected_players_.find(player_id);
                if (it != connected_players_.end()) {
                    auto& c = it->second.combat;
                    if (effect.hp_delta > 0) {
                        std::int64_t next = static_cast<std::int64_t>(c.current_hp)
                                          + effect.hp_delta;
                        if (next > c.max_hp) next = c.max_hp;
                        if (next < 0) next = 0;
                        c.current_hp = static_cast<std::uint32_t>(next);
                    }
                    if (effect.mp_delta > 0) {
                        std::int64_t next = static_cast<std::int64_t>(c.current_mp)
                                          + effect.mp_delta;
                        if (next > c.max_mp) next = c.max_mp;
                        if (next < 0) next = 0;
                        c.current_mp = static_cast<std::uint32_t>(next);
                    }
                    new_hp = c.current_hp;
                    new_mp = c.current_mp;
                }
            }

            // Build the UseAck reply.
            //
            // Payload layout (12 bytes — matches the conventional
            // GameIn reply shape used by other handlers in this
            // file; can be expanded with a follow-up "UseEffect"
            // notify if a real client needs more fields):
            //   u16  pos               (echoed)
            //   u16  wIconIdx          (echoed)
            //   i32  hp_delta          (signed)
            //   i32  mp_delta          (signed)
            //   u32  current_hp        (post-effect)
            //   u32  current_mp        (post-effect)
            mxh::net::Message reply;
            reply.header.category = static_cast<std::uint8_t>(
                mxh::proto::Category::Item);
            reply.header.protocol = static_cast<std::uint8_t>(
                mxh::proto::ItemProtocol::UseAck);
            reply.header.object_id = player_id;
            reply.payload.resize(2 + 2 + 4 + 4 + 4 + 4, 0);
            std::size_t off = 0;
            std::memcpy(reply.payload.data() + off, &pos, 2);            off += 2;
            std::memcpy(reply.payload.data() + off, &used_item.wIconIdx, 2); off += 2;
            std::memcpy(reply.payload.data() + off, &effect.hp_delta, 4); off += 4;
            std::memcpy(reply.payload.data() + off, &effect.mp_delta, 4); off += 4;
            std::memcpy(reply.payload.data() + off, &new_hp, 4);          off += 4;
            std::memcpy(reply.payload.data() + off, &new_mp, 4);          off += 4;
            reply_(id, reply);
            std::cout << "[Map] sent ITEM_USE_ACK pos=" << pos
                      << " wIconIdx=" << used_item.wIconIdx
                      << " hp+=" << effect.hp_delta
                      << " mp+=" << effect.mp_delta
                      << " (now hp=" << new_hp << " mp=" << new_mp << ")\n";
            break;
        }

        default:
            std::cout << "[Map] unhandled ITEM proto="
                      << static_cast<int>(proto) << "\n";
            break;
    }
}

// ============================================================================
// Phase 10c: Monster management
// ============================================================================

void MapHandler::spawn_monsters() {
    // Caller must hold monsters_mu_.
    std::mt19937 rng(static_cast<std::uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count()));

    for (auto& sp : spawn_points_) {
        // Find template
        const mxh::game::MonsterTemplate* tmpl = nullptr;
        for (auto& t : monster_templates_) {
            if (t.MonsterKind == sp.NpcKind) {
                tmpl = &t;
                break;
            }
        }
        if (!tmpl && !monster_templates_.empty()) {
            tmpl = &monster_templates_[0];  // fallback to first template
        }
        if (!tmpl) continue;

        mxh::game::MonsterInstance m;
        m.object_id     = next_monster_id_++;
        m.monster_kind  = tmpl->MonsterKind;
        m.object_kind   = tmpl->ObjectKind;
        std::strncpy(m.name, sp.Name, 16);
        m.name[16] = '\0';
        m.level        = tmpl->Level;
        m.group        = 0;
        m.map_num      = map_num_;

        m.max_life     = tmpl->Life;
        m.current_life = tmpl->Life;
        m.max_shield   = tmpl->Shield;
        m.current_shield = tmpl->Shield;

        // Spawn position with small random offset
        std::uniform_real_distribution<float> dist(-5.0f, 5.0f);
        m.pos_x = sp.PosX + dist(rng);
        m.pos_y = sp.PosY;
        m.pos_z = sp.PosZ + dist(rng);
        m.spawn_x = m.pos_x;
        m.spawn_y = m.pos_y;
        m.spawn_z = m.pos_z;
        m.angle   = sp.Angle;

        m.ai_state   = mxh::game::MonsterAIState::Idle;
        m.is_dead    = false;

        // Copy template stats
        m.exp_reward    = tmpl->ExpPoint;
        m.attack_min    = tmpl->AttackMin;
        m.attack_max    = tmpl->AttackMax;
        m.defense       = tmpl->Defense;
        m.walk_speed    = tmpl->WalkSpeed;
        m.run_speed     = tmpl->RunSpeed;
        m.search_range  = tmpl->SearchRange;
        m.domain_range  = tmpl->DomainRange;
        m.aggressive    = tmpl->Aggressive;

        monsters_.push_back(m);
        std::cout << "[Map] spawned monster id=" << m.object_id
                  << " name='" << m.name << "' kind=" << m.monster_kind
                  << " at (" << m.pos_x << "," << m.pos_z << ")\n";
    }
}

void MapHandler::send_monster_add(std::uint32_t player_id,
                                  const mxh::game::MonsterInstance& monster) {
    // Caller must hold monsters_mu_.
    // Find the player's connection
    std::uint64_t target_conn = 0;
    {
        std::lock_guard<std::mutex> lk(players_mu_);
        auto it = connected_players_.find(player_id);
        if (it != connected_players_.end()) {
            target_conn = it->second.conn_id;
        }
    }
    if (target_conn == 0) return;

    mxh::net::ConnectionId target{target_conn};
    mxh::net::Message m;
    // MonsterAdd is sent via UserConn category (proto=37)
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::MonsterAdd);
    m.header.object_id = monster.object_id;

    // Payload: BASEOBJECT_INFO(35B) + MONSTER_TOTALINFO(14B) + SEND_MOVEINFO(14B) + bLogin(1B)
    constexpr std::size_t kPayloadSize = 35 + 14 + 14 + 1;
    m.payload.resize(kPayloadSize, 0);

    std::size_t off = 0;

    // BASEOBJECT_INFO [0..34]
    put_u32(m.payload, off + 0, monster.object_id);   // dwObjectID
    put_u32(m.payload, off + 4, monster.object_id);   // dwUserID
    put_str(m.payload, off + 8, monster.name, 17);    // ObjectName
    // BattleID(4) + BattleTeam(1) + ObjectState(1) + SingleSpecialState(4) = 0
    off += 35;

    // MONSTER_TOTALINFO [35..48]
    put_u32(m.payload, off + 0, monster.current_life);
    put_u32(m.payload, off + 4, monster.current_shield);
    put_u16(m.payload, off + 8, monster.monster_kind);
    put_u16(m.payload, off + 10, monster.group);
    put_u16(m.payload, off + 12, monster.map_num);
    off += 14;

    // SEND_MOVEINFO [49..62]
    // COMPRESSEDPOS: x(2B) + z(2B) = 4B, MoveMode(1B), KyungGongIdx(2B),
    // AbilityKyungGongLevel(2B), Move_Direction(4B) = 14B total
    put_u16(m.payload, off + 0, static_cast<std::uint16_t>(monster.pos_x));
    put_u16(m.payload, off + 2, static_cast<std::uint16_t>(monster.pos_z));
    // MoveMode = 0 (idle)
    off += 14;

    // bLogin = 1
    m.payload[off] = 1;

    reply_(target, m);
}

void MapHandler::send_monster_remove(std::uint32_t player_id,
                                     std::uint32_t monster_object_id) {
    // Find the player's connection
    std::uint64_t target_conn = 0;
    {
        std::lock_guard<std::mutex> lk(players_mu_);
        auto it = connected_players_.find(player_id);
        if (it != connected_players_.end()) {
            target_conn = it->second.conn_id;
        }
    }
    if (target_conn == 0) return;

    mxh::net::ConnectionId target{target_conn};
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ObjectRemove);
    m.header.object_id = 0;
    m.payload.resize(4, 0);
    put_u32(m.payload, 0, monster_object_id);
    reply_(target, m);
}

void MapHandler::broadcast_monster_add(const mxh::game::MonsterInstance& monster) {
    std::lock_guard<std::mutex> lk(players_mu_);
    std::vector<std::uint32_t> player_ids;
    for (auto& [pid, info] : connected_players_) {
        player_ids.push_back(pid);
    }
    // Note: caller must NOT hold monsters_mu_ to avoid deadlock.
    // We call send_monster_add which acquires players_mu_ internally.
    // But we already hold players_mu_ here, so we inline the logic.
    for (auto pid : player_ids) {
        auto it = connected_players_.find(pid);
        if (it == connected_players_.end()) continue;
        mxh::net::ConnectionId target{it->second.conn_id};
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::MonsterAdd);
        m.header.object_id = monster.object_id;
        constexpr std::size_t kPayloadSize = 35 + 14 + 14 + 1;
        m.payload.resize(kPayloadSize, 0);
        std::size_t off = 0;
        put_u32(m.payload, off + 0, monster.object_id);
        put_u32(m.payload, off + 4, monster.object_id);
        put_str(m.payload, off + 8, monster.name, 17);
        off += 35;
        put_u32(m.payload, off + 0, monster.current_life);
        put_u32(m.payload, off + 4, monster.current_shield);
        put_u16(m.payload, off + 8, monster.monster_kind);
        put_u16(m.payload, off + 10, monster.group);
        put_u16(m.payload, off + 12, monster.map_num);
        off += 14;
        put_u16(m.payload, off + 0, static_cast<std::uint16_t>(monster.pos_x));
        put_u16(m.payload, off + 2, static_cast<std::uint16_t>(monster.pos_z));
        off += 14;
        m.payload[off] = 1;
        reply_(target, m);
    }
}

void MapHandler::broadcast_monster_remove(std::uint32_t monster_object_id) {
    std::lock_guard<std::mutex> lk(players_mu_);
    for (auto& [pid, info] : connected_players_) {
        mxh::net::ConnectionId target{info.conn_id};
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::ObjectRemove);
        m.header.object_id = 0;
        m.payload.resize(4, 0);
        put_u32(m.payload, 0, monster_object_id);
        reply_(target, m);
    }
}

void MapHandler::broadcast_monster_life(const mxh::game::MonsterInstance& monster) {
    std::lock_guard<std::mutex> lk(players_mu_);
    for (auto& [pid, info] : connected_players_) {
        mxh::net::ConnectionId target{info.conn_id};
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::Monster);
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::MonsterProtocol::LifeNotify);
        m.header.object_id = monster.object_id;
        // Payload: Life(4B) + Shield(4B)
        m.payload.resize(8, 0);
        put_u32(m.payload, 0, monster.current_life);
        put_u32(m.payload, 4, monster.current_shield);
        reply_(target, m);
    }
}

void MapHandler::tick_monster_ai() {
    // Called periodically from the server main loop or a timer.
    // Phase 10c P0: simple state machine.
    auto now_ms = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    std::lock_guard<std::mutex> lk(monsters_mu_);

    for (auto& m : monsters_) {
        if (m.is_dead) {
            // Check respawn timer (10 seconds after death)
            if (m.respawn_time_ms > 0 && now_ms >= m.respawn_time_ms) {
                m.is_dead = false;
                m.current_life = m.max_life;
                m.current_shield = m.max_shield;
                m.pos_x = m.spawn_x;
                m.pos_z = m.spawn_z;
                m.ai_state = mxh::game::MonsterAIState::Idle;
                m.respawn_time_ms = 0;
                std::cout << "[Map] monster id=" << m.object_id
                          << " respawned\n";
                broadcast_monster_add(m);
            }
            continue;
        }

        // AI tick every 1 second
        if (now_ms - m.last_ai_tick_ms < 1000) continue;
        m.last_ai_tick_ms = now_ms;

        switch (m.ai_state) {
            case mxh::game::MonsterAIState::Idle: {
                // Check for nearby players (aggro)
                if (m.aggressive) {
                    std::lock_guard<std::mutex> plk(players_mu_);
                    float search_sq = m.search_range * m.search_range;
                    for (auto& [pid, pinfo] : connected_players_) {
                        float d2 = mxh::game::distance_sq_2d(
                            m.pos_x, m.pos_z, pinfo.pos_x, pinfo.pos_z);
                        if (d2 < search_sq) {
                            m.ai_state = mxh::game::MonsterAIState::Chase;
                            m.target_object_id = pid;
                            std::cout << "[Map] monster id=" << m.object_id
                                      << " aggro player=" << pid << "\n";
                            break;
                        }
                    }
                }
                break;
            }
            case mxh::game::MonsterAIState::Chase: {
                // Move toward target player
                std::lock_guard<std::mutex> plk(players_mu_);
                auto it = connected_players_.find(m.target_object_id);
                if (it == connected_players_.end()) {
                    // Target gone, return to spawn
                    m.ai_state = mxh::game::MonsterAIState::Return;
                    break;
                }
                float dx = it->second.pos_x - m.pos_x;
                float dz = it->second.pos_z - m.pos_z;
                float dist = std::sqrt(dx * dx + dz * dz);
                if (dist < 3.0f) {
                    // In attack range
                    m.ai_state = mxh::game::MonsterAIState::Attack;
                } else {
                    // Move toward target
                    float speed = m.run_speed * 0.01f;  // per tick
                    m.pos_x += dx / dist * speed;
                    m.pos_z += dz / dist * speed;
                }
                break;
            }
            case mxh::game::MonsterAIState::Attack: {
                // Phase 10c P0: just log the attack, damage dealt in Phase 10d
                std::cout << "[Map] monster id=" << m.object_id
                          << " attacks player=" << m.target_object_id << "\n";
                // Return to chase (will re-evaluate next tick)
                m.ai_state = mxh::game::MonsterAIState::Chase;
                break;
            }
            case mxh::game::MonsterAIState::Return: {
                float dx = m.spawn_x - m.pos_x;
                float dz = m.spawn_z - m.pos_z;
                float dist = std::sqrt(dx * dx + dz * dz);
                if (dist < 2.0f) {
                    m.pos_x = m.spawn_x;
                    m.pos_z = m.spawn_z;
                    m.ai_state = mxh::game::MonsterAIState::Idle;
                    m.target_object_id = 0;
                } else {
                    float speed = m.walk_speed * 0.01f;
                    m.pos_x += dx / dist * speed;
                    m.pos_z += dz / dist * speed;
                }
                break;
            }
            default:
                break;
        }
    }
}

// ============================================================================
// Phase 10c: Monster/NPC protocol handlers
// ============================================================================

void MapHandler::handle_monster(mxh::net::ConnectionId /*id*/,
                                const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::MonsterProtocol>(msg.header.protocol);
    std::cout << "[Map] MONSTER proto=" << static_cast<int>(proto)
              << " obj=" << msg.header.object_id
              << " payload=" << msg.payload.size() << "B\n";

    // Phase 10c P0: Monster category messages are server->client only
    // (LifeNotify, RestNotify, RecallNotify). Client doesn't send to Monster cat.
    // Log and ignore for now.
}

void MapHandler::handle_npc(mxh::net::ConnectionId id,
                            const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::NpcProtocol>(msg.header.protocol);
    std::uint32_t player_id = msg.header.object_id;
    std::cout << "[Map] NPC proto=" << static_cast<int>(proto)
              << " player=" << player_id
              << " payload=" << msg.payload.size() << "B\n";

    switch (proto) {
        case mxh::proto::NpcProtocol::SpeechSyn: {
            // C -> S: talk to NPC
            // Phase 10c P0: echo SpeechAck
            mxh::net::Message reply;
            reply.header.category = static_cast<std::uint8_t>(
                mxh::proto::Category::Npc);
            reply.header.protocol = static_cast<std::uint8_t>(
                mxh::proto::NpcProtocol::SpeechAck);
            reply.header.object_id = player_id;
            reply.payload = msg.payload;
            reply_(id, reply);
            std::cout << "[Map] sent NPC_SPEECH_ACK\n";
            break;
        }
        default:
            std::cout << "[Map] unhandled NPC proto="
                      << static_cast<int>(proto) << "\n";
            break;
    }
}

// ============================================================================
// Phase 10d: Skill system implementation
// ============================================================================

namespace {

// Helper: project the simplified 15-field view onto the legacy
// 1:1 SKILLINFO struct so the combat path can read it through
// the same SkillInfo handle it will use after D1.3 init_from_bin
// lands.  Pulled out as a free function because MSVC can't deduce
// the return type of a 15-arg nested lambda inside init_skill_table.
void add_simple_skill(std::unordered_map<std::uint32_t, mxh::game::SkillInfo>& dst,
                      std::uint16_t idx, const char* name,
                      std::uint8_t kind, std::uint16_t range,
                      std::uint16_t target_range, std::uint32_t delay,
                      std::uint16_t duration, std::uint8_t weapon,
                      std::uint8_t attrib, std::uint16_t phy,
                      std::uint16_t att, std::uint16_t att_rate,
                      std::uint8_t crit, std::uint8_t stun,
                      std::uint16_t nearyuk) {
    using namespace mxh::game;
    SkillInfo full{};
    full.SkillIdx       = idx;
    const std::size_t n = std::strlen(name);
    for (std::size_t i = 0; i < n && i < SKILL_MAX_NAME - 1; ++i) {
        full.SkillName[i] = name[i];
    }
    full.SkillKind      = kind;
    full.SkillRange     = range;
    full.TargetRange    = target_range;
    full.DelayTime      = delay;
    full.Duration       = duration;
    full.WeaponKind     = weapon;
    full.Attrib         = attrib;
    full.UpPhyAttack[0]      = static_cast<float>(phy);
    full.FirstAttAttack[0]    = static_cast<float>(att);
    full.AttackSuccessRate[0] = static_cast<float>(att_rate);
    full.CriticalRate[0]      = static_cast<float>(crit);
    full.StunRate[0]          = static_cast<float>(stun);
    full.NeedNaeRyuk[0]       = nearyuk;
    dst[idx] = full;
}

}  // namespace

void MapHandler::init_skill_table() {
    // D1.3: SkillInfo is now the 1:1 legacy SKILLINFO layout.  The
    // hardcoded 4-skill table is still here for offline testing, but
    // the production path will eventually call SkillManager::init_from_bin
    // and forget this method.
    using namespace mxh::game;
    auto& t = skill_table_;
    add_simple_skill(t, 1,  "BasicSlash", 0, 3, 0, 500, 0, 0, 0, 15, 0,  100, 5,  0, 0);
    add_simple_skill(t, 2,  "FireBolt",   0, 5, 2, 1000, 0, 0, 1, 20, 10, 90, 8,  0, 5);
    add_simple_skill(t, 3,  "Heal",       1, 4, 0, 2000, 0, 0, 0, 0, 0,  100, 0,  0, 10);
    add_simple_skill(t, 10, "Whirlwind",  0, 2, 3, 1500, 0, 0, 0, 25, 0,  100, 10, 0, 8);
    std::cout << "[Map] skill_table initialized with " << skill_table_.size()
              << " skills\n";
}

const mxh::game::SkillInfo* MapHandler::find_skill(std::uint32_t skill_idx) const {
    auto it = skill_table_.find(skill_idx);
    if (it == skill_table_.end()) return nullptr;
    return &it->second;
}

mxh::game::DamageResult MapHandler::calculate_damage(
    const mxh::game::PlayerCombatStats& attacker,
    const mxh::game::PlayerCombatStats& defender,
    const mxh::game::SkillInfo& skill) {
    mxh::game::DamageResult result;

    // Dodge check
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uint8_t dodge_roll = static_cast<std::uint8_t>(rng() % 100);
    if (dodge_roll < defender.dodge_rate) {
        result.is_miss = true;
        result.hit_result = 0;
        result.damage = 0;
        return result;
    }

    // Base damage = skill phy_attack + attacker phy_attack - defender phy_defence
    const auto simple = mxh::game::to_simple(skill);
    std::int32_t base_damage = static_cast<std::int32_t>(simple.phy_attack)
                             + static_cast<std::int32_t>(attacker.phy_attack)
                             - static_cast<std::int32_t>(defender.phy_defence);
    if (base_damage < 1) base_damage = 1;

    // Attribute damage
    std::int32_t attr_damage = static_cast<std::int32_t>(simple.att_attack)
                             + static_cast<std::int32_t>(attacker.att_attack)
                             - static_cast<std::int32_t>(defender.att_defence);
    if (attr_damage < 0) attr_damage = 0;

    std::int32_t total_damage = base_damage + attr_damage;

    // Critical check
    std::uint8_t crit_roll = static_cast<std::uint8_t>(rng() % 100);
    if (crit_roll < (simple.critical_rate + attacker.critical_rate)) {
        result.is_critical = true;
        total_damage = static_cast<std::int32_t>(total_damage * 1.5);
        result.hit_result = 2;
    } else {
        result.hit_result = 1;
    }

    result.damage = total_damage;
    return result;
}

void MapHandler::broadcast_skill_object_add(
    std::uint32_t except_player_id,
    const mxh::game::SkillInstance& skill_obj,
    std::uint32_t caster_id) {
    // Build MP_SKILL_SKILLOBJECT_ADD message
    // Payload: [skill_idx:u32][skill_obj_id:u32][caster_id:u32][pos_x:f32][pos_z:f32][direction:u16]
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::SkillProtocol::SkillObjectAdd);
    msg.header.object_id = caster_id;
    msg.payload.resize(22);
    std::uint8_t* p = msg.payload.data();
    std::memcpy(p, &skill_obj.skill_idx, 4); p += 4;
    std::memcpy(p, &skill_obj.skill_object_id, 4); p += 4;
    std::memcpy(p, &skill_obj.caster_id, 4); p += 4;
    std::memcpy(p, &skill_obj.pos_x, 4); p += 4;
    std::memcpy(p, &skill_obj.pos_z, 4); p += 4;
    std::memcpy(p, &skill_obj.direction, 2);
    broadcast_except(except_player_id, msg);
}

void MapHandler::broadcast_skill_object_remove(
    std::uint32_t except_player_id,
    std::uint32_t skill_object_id) {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::SkillProtocol::SkillObjectRemove);
    msg.header.object_id = skill_object_id;
    broadcast_except(except_player_id, msg);
}

void MapHandler::send_skill_single_result(
    std::uint32_t target_player_id,
    std::uint32_t target_id,
    std::int32_t damage,
    std::uint8_t hit_result) {
    // MP_SKILL_SINGLE_RESULT payload: [target_id:u32][damage:i32][hit_result:u8]
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::SkillProtocol::SingleResult);
    msg.header.object_id = target_id;
    msg.payload.resize(9);
    std::uint8_t* p = msg.payload.data();
    std::memcpy(p, &target_id, 4); p += 4;
    std::memcpy(p, &damage, 4); p += 4;
    std::memcpy(p, &hit_result, 1);

    // Send to the target player's connection
    std::lock_guard<std::mutex> lk(players_mu_);
    auto it = connected_players_.find(target_player_id);
    if (it != connected_players_.end()) {
        reply_(mxh::net::ConnectionId{it->second.conn_id}, msg);
    }
}

void MapHandler::handle_skill(mxh::net::ConnectionId id,
                               const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::SkillProtocol>(msg.header.protocol);
    std::uint32_t caster_id = msg.header.object_id;
    std::cout << "[Map] Skill proto=" << static_cast<int>(proto)
              << " caster=" << caster_id
              << " payload=" << msg.payload.size() << "B\n";

    switch (proto) {
        case mxh::proto::SkillProtocol::StartSyn: {
            // Client requests skill use.
            // Payload: [skill_idx:u32][main_target:u32][target_x:f32][target_z:f32]
            if (msg.payload.size() < 16) {
                std::cout << "[Map] Skill StartSyn too short\n";
                break;
            }
            std::uint32_t skill_idx = 0;
            std::uint32_t main_target = 0;
            float target_x = 0, target_z = 0;
            std::memcpy(&skill_idx, msg.payload.data(), 4);
            std::memcpy(&main_target, msg.payload.data() + 4, 4);
            std::memcpy(&target_x, msg.payload.data() + 8, 4);
            std::memcpy(&target_z, msg.payload.data() + 12, 4);

            std::cout << "[Map] Skill StartSyn: skill=" << skill_idx
                      << " target=" << main_target
                      << " pos=(" << target_x << "," << target_z << ")\n";

            // Find skill info
            const auto* skill = find_skill(skill_idx);
            if (!skill) {
                std::cout << "[Map] Skill " << skill_idx << " not found\n";
                // Send StartNack
                mxh::net::Message nack;
                nack.header.category = static_cast<std::uint8_t>(
                    mxh::proto::Category::Skill);
                nack.header.protocol = static_cast<std::uint8_t>(
                    mxh::proto::SkillProtocol::StartNack);
                nack.header.object_id = caster_id;
                std::uint8_t err = 1;  // skill not found
                nack.payload.assign(reinterpret_cast<const std::uint8_t*>(&err),
                                    reinterpret_cast<const std::uint8_t*>(&err) + 1);
                reply_(id, nack);
                break;
            }

            // Find caster
            PlayerInfo* caster = nullptr;
            {
                std::lock_guard<std::mutex> lk(players_mu_);
                auto it = connected_players_.find(caster_id);
                if (it != connected_players_.end()) caster = &it->second;
            }
            if (!caster) {
                std::cout << "[Map] Skill caster " << caster_id << " not found\n";
                break;
            }

            // MP check
            if (caster->combat.current_mp < mxh::game::to_simple(*skill).need_nearyuk) {
                std::cout << "[Map] Skill not enough MP\n";
                mxh::net::Message nack;
                nack.header.category = static_cast<std::uint8_t>(
                    mxh::proto::Category::Skill);
                nack.header.protocol = static_cast<std::uint8_t>(
                    mxh::proto::SkillProtocol::StartNack);
                nack.header.object_id = caster_id;
                std::uint8_t err = 2;  // not enough MP
                nack.payload.assign(reinterpret_cast<const std::uint8_t*>(&err),
                                    reinterpret_cast<const std::uint8_t*>(&err) + 1);
                reply_(id, nack);
                break;
            }

            // Deduct MP
            const auto simple = mxh::game::to_simple(*skill);
            caster->combat.current_mp -= simple.need_nearyuk;

            // Create skill instance
            mxh::game::SkillInstance skill_obj;
            {
                std::lock_guard<std::mutex> lk(skills_mu_);
                skill_obj.skill_object_id = next_skill_obj_id_++;
                skill_obj.skill_idx = skill_idx;
                skill_obj.caster_id = caster_id;
                skill_obj.main_target_id = main_target;
                skill_obj.pos_x = target_x;
                skill_obj.pos_z = target_z;
                skill_obj.start_time = static_cast<std::uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count());
                skill_obj.duration = simple.duration;
                active_skills_.push_back(skill_obj);
            }

            // Send StartAck to caster
            mxh::net::Message ack;
            ack.header.category = static_cast<std::uint8_t>(
                mxh::proto::Category::Skill);
            ack.header.protocol = static_cast<std::uint8_t>(
                mxh::proto::SkillProtocol::StartAck);
            ack.header.object_id = caster_id;
            ack.payload.resize(8);
            std::memcpy(ack.payload.data(), &skill_idx, 4);
            std::uint32_t sobj_id = skill_obj.skill_object_id;
            std::memcpy(ack.payload.data() + 4, &sobj_id, 4);
            reply_(id, ack);

            // Broadcast SkillObjectAdd to other players
            broadcast_skill_object_add(caster_id, skill_obj, caster_id);

            // Calculate damage for target
            if (main_target != 0 && simple.skill_kind == mxh::game::SkillKind::Combo) {
                // Attack skill - calculate damage
                PlayerInfo* target = nullptr;
                {
                    std::lock_guard<std::mutex> lk(players_mu_);
                    auto it = connected_players_.find(main_target);
                    if (it != connected_players_.end()) target = &it->second;
                }

                if (target) {
                    auto dmg = calculate_damage(caster->combat, target->combat,
                                               *skill);
                    target->combat.current_hp -= dmg.damage;
                    if (target->combat.current_hp < 0)
                        target->combat.current_hp = 0;

                    // Send SingleResult to target
                    send_skill_single_result(main_target, main_target,
                                            dmg.damage, dmg.hit_result);

                    // Also send to caster for display
                    send_skill_single_result(caster_id, main_target,
                                            dmg.damage, dmg.hit_result);

                    std::cout << "[Map] Skill damage: " << dmg.damage
                              << " hit=" << (int)dmg.hit_result
                              << " target_hp=" << target->combat.current_hp
                              << "\n";
                } else {
                    // Target might be a monster - just log
                    std::cout << "[Map] Skill target " << main_target
                              << " not a player (monster?)\n";
                }
            } else if (simple.skill_kind == mxh::game::SkillKind::OuterMugong) {
                // Heal skill
                if (main_target != 0) {
                    PlayerInfo* target = nullptr;
                    {
                        std::lock_guard<std::mutex> lk(players_mu_);
                        auto it = connected_players_.find(main_target);
                        if (it != connected_players_.end()) target = &it->second;
                    }
                    if (target) {
                        std::int32_t heal = simple.phy_attack + caster->combat.level * 5;
                        target->combat.current_hp += heal;
                        if (target->combat.current_hp > target->combat.max_hp)
                            target->combat.current_hp = target->combat.max_hp;
                        send_skill_single_result(main_target, main_target,
                                                -heal, 1);  // negative = heal
                        std::cout << "[Map] Skill heal: " << heal
                                  << " target_hp=" << target->combat.current_hp
                                  << "\n";
                    }
                }
            }

            // Remove skill object after duration (instant for now)
            if (simple.duration == 0) {
                std::lock_guard<std::mutex> lk(skills_mu_);
                active_skills_.erase(
                    std::remove_if(active_skills_.begin(), active_skills_.end(),
                        [&](const mxh::game::SkillInstance& s) {
                            return s.skill_object_id == skill_obj.skill_object_id;
                        }),
                    active_skills_.end());
                // Broadcast remove
                broadcast_skill_object_remove(caster_id, skill_obj.skill_object_id);
            }
            break;
        }
        case mxh::proto::SkillProtocol::StartEffect: {
            // Client-to-client VFX broadcast, server just relays
            broadcast_except(caster_id, msg);
            break;
        }
        case mxh::proto::SkillProtocol::OperateSyn: {
            // Continuous skill operate - treat like StartSyn for now
            std::cout << "[Map] Skill OperateSyn (not implemented)\n";
            break;
        }
        default:
            std::cout << "[Map] unhandled Skill proto="
                      << static_cast<int>(proto) << "\n";
            break;
    }
}

void MapHandler::handle_battle(mxh::net::ConnectionId id,
                                const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::BattleProtocol>(msg.header.protocol);
    std::uint32_t player_id = msg.header.object_id;
    std::cout << "[Map] Battle proto=" << static_cast<int>(proto)
              << " player=" << player_id << "\n";

    // Phase 10d P0: Battle is mostly client-side for PvP.
    // For PvE, the damage is handled through Skill protocol.
    // Just acknowledge basic battle messages.
    switch (proto) {
        case mxh::proto::BattleProtocol::Info: {
            // Client requesting battle info - send empty for now
            mxh::net::Message reply;
            reply.header.category = static_cast<std::uint8_t>(
                mxh::proto::Category::Battle);
            reply.header.protocol = static_cast<std::uint8_t>(
                mxh::proto::BattleProtocol::Info);
            reply.header.object_id = player_id;
            reply_(id, reply);
            break;
        }
        default:
            std::cout << "[Map] unhandled Battle proto="
                      << static_cast<int>(proto) << "\n";
            break;
    }
}

}  // namespace mxh::server
