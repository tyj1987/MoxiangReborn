// agent_handler.cpp - AgentServer handler (per-user agent).
//
// Phase 4 demo: handles MP_USERCONN_CHARACTERLIST_SYN -> returns
// character list from DB. Limited implementation; full Agent logic
// is in [Server]Agent/AgentNetworkMsgParser.cpp (50K+ LOC).
//
// Phase 7.6: adds legacy 4DyuchiNET framing support:
//   - Sends AGENT_CONNECTSUCCESS on connect (with auth key)
//   - Handles CHARACTERLIST_SYN from original client
//   - Returns SEND_CHARSELECT_INFO with real character data from DB
//
// Phase 8: character creation flow:
//   - CharacterNameCheckSyn (proto=19) -> check name availability
//   - CharacterMakeSyn (proto=22) -> create character in DB
//   - CharacterListSyn (proto=9) -> return real character list from DB
//
// Phase 8.5: character selection & game entry:
//   - CharacterSelectSyn (proto=16) -> select character, get map number
//   - GameInSyn (proto=28) -> forward to MapServer (Phase 9)
//
// Phase 9: MapServer integration:
//   - GameInSyn forwarded to MapServer via TcpClient
//   - MapServer responses relayed back to original client
//   - CharacterAdd/ObjectRemove forwarded to other players

#include "mxh/server/server.hpp"

#include <cstring>
#include <iostream>
#include <random>

namespace mxh::server {

namespace {

// ============================================================================
// Binary packing helpers (little-endian, packed)
// ============================================================================

void put_u8(std::vector<std::uint8_t>& buf, std::uint8_t v) {
    buf.push_back(v);
}

void put_u16(std::vector<std::uint8_t>& buf, std::uint16_t v) {
    buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void put_i32(std::vector<std::uint8_t>& buf, std::int32_t v) {
    auto u = static_cast<std::uint32_t>(v);
    buf.push_back(static_cast<std::uint8_t>(u & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((u >> 8) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((u >> 16) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((u >> 24) & 0xFF));
}

void put_u32(std::vector<std::uint8_t>& buf, std::uint32_t v) {
    buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

void put_f32(std::vector<std::uint8_t>& buf, float v) {
    const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
    buf.insert(buf.end(), p, p + 4);
}

// Write a fixed-length string (null-padded to max_len).
void put_str(std::vector<std::uint8_t>& buf, const char* s, std::size_t max_len) {
    std::size_t len = strnlen(s, max_len);
    buf.insert(buf.end(), s, s + len);
    buf.resize(buf.size() + (max_len - len), 0);
}

// Zero-fill N bytes.
void put_zeros(std::vector<std::uint8_t>& buf, std::size_t n) {
    buf.resize(buf.size() + n, 0);
}

// Write a HselInit structure (64 bytes) to the payload.
// HselInit = { iDesCount, iEncryptType, iSwapFlag, iCustomize, Keys[12] }
// All fields are __int32 (4 bytes each), total = 4*4 + 12*4 = 64 bytes.
void put_hsel_init(std::vector<std::uint8_t>& buf, std::mt19937& rng) {
    std::uniform_int_distribution<std::int32_t> dist(1, 0x7FFFFFFF);
    // Header fields
    put_i32(buf, 3);   // iDesCount = HSEL_DES_TRIPLE
    put_i32(buf, 1);   // iEncryptType = HSEL_ENCRYPTTYPE_1
    put_i32(buf, 1);   // iSwapFlag = HSEL_SWAP_FLAG_ON
    put_i32(buf, 0);   // iCustomize = HSEL_KEY_TYPE_DEFAULT
    // Keys (12 x int32)
    for (int i = 0; i < 12; ++i) {
        put_i32(buf, dist(rng));
    }
}


// ============================================================================
// Constants matching CommonStruct.h (with #pragma pack(push,1))
// ============================================================================

constexpr int kMaxNameLength    = 16;
constexpr int kMaxCharSlots     = 5;
constexpr int kBaseObjectInfoSize = 35;  // 4+4+17+4+1+1+4
constexpr int kCharTotalInfoSize  = 140; // KOR version, no extra ifdef fields

// Default starting map (jangan / 长安 = 12, from CommonGameDefine.h enum)
// NOTE: CharacterSelectAck uses MSG_BYTE (1B), so map numbers must be 0-255.
constexpr std::uint16_t kDefaultMapNum = 12;

// Build MP_USERCONN_AGENT_CONNECTSUCCESS for legacy client.
mxh::net::Message make_agent_connect_success(std::uint32_t auth_key) {
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::AgentConnectSuccess);
    m.header.object_id = auth_key;
    return m;
}

// Helper: get int64 from ResultSet row, with default.
int64_t get_int(const mxh::db::ResultSet& rs, std::size_t row, const char* col,
                 int64_t def = 0) {
    int idx = rs.column_index(col);
    if (idx < 0) return def;
    const auto& v = rs.rows[row][idx];
    if (std::holds_alternative<std::monostate>(v)) return def;
    if (std::holds_alternative<std::int64_t>(v)) return std::get<std::int64_t>(v);
    if (std::holds_alternative<double>(v)) return static_cast<std::int64_t>(std::get<double>(v));
    if (std::holds_alternative<std::string>(v)) {
        try { return std::stoll(std::get<std::string>(v)); }
        catch (...) { return def; }
    }
    return def;
}

double get_double(const mxh::db::ResultSet& rs, std::size_t row, const char* col,
                  double def = 0.0) {
    int idx = rs.column_index(col);
    if (idx < 0) return def;
    const auto& v = rs.rows[row][idx];
    if (std::holds_alternative<std::monostate>(v)) return def;
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    if (std::holds_alternative<std::int64_t>(v)) return static_cast<double>(std::get<std::int64_t>(v));
    return def;
}

std::string get_str(const mxh::db::ResultSet& rs, std::size_t row, const char* col,
                    const std::string& def = "") {
    int idx = rs.column_index(col);
    if (idx < 0) return def;
    const auto& v = rs.rows[row][idx];
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    return def;
}

}  // namespace

// ============================================================================
// AgentHandler
// ============================================================================

AgentHandler::AgentHandler(mxh::db::IDbAdapter& db, ReplyFn reply,
                           bool use_legacy_framing)
    : db_(db), reply_(std::move(reply)),
      use_legacy_framing_(use_legacy_framing) {}

bool AgentHandler::on_connect(mxh::net::ConnectionId id,
                              const std::string& remote_addr) {
    std::cout << "[Agent] client connected from " << remote_addr << "\n";

    // Phase 7.6: In legacy mode, immediately send AGENT_CONNECTSUCCESS.
    if (use_legacy_framing_) {
        static std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<std::uint32_t> dist(50000, 99999);
        std::uint32_t agent_auth_key = dist(rng);
        reply_(id, make_agent_connect_success(agent_auth_key));
        std::cout << "[Agent] legacy: sent AgentConnectSuccess auth_key="
                  << agent_auth_key << "\n";
    }
    return true;
}

void AgentHandler::on_disconnect(mxh::net::ConnectionId id,
                                 mxh::net::NetError reason) {
    std::cout << "[Agent] client disconnected (id=" << id.value
              << " reason=" << mxh::net::to_string(reason) << ")\n";
    // Clean up connection state.
    std::uint32_t removed_char_id = 0;
    {
        std::lock_guard<std::mutex> lk(user_mu_);
        auto char_it = conn_char_ids_.find(id.value);
        if (char_it != conn_char_ids_.end()) {
            removed_char_id = char_it->second;
        }
        conn_user_ids_.erase(id.value);
        conn_char_ids_.erase(id.value);
        conn_map_nums_.erase(id.value);
    }
    // Phase 9: do NOT erase char_to_client_ here.
    // MapServer still considers this player connected (we never sent
    // GameOutSyn), so broadcast messages will still arrive for this
    // char_id. Removing the routing entry would drop those messages.
    // The entry will be overwritten when the client reconnects and
    // sends GameInSyn again.
    // TODO: send GameOutSyn to MapServer on client disconnect to
    // properly clean up both sides.
}

void AgentHandler::on_message(mxh::net::ConnectionId id,
                              const mxh::net::Message& msg) {
    auto cat = static_cast<mxh::proto::Category>(msg.header.category);
    if (cat == mxh::proto::Category::UserConn) {
        handle_userconn(id, msg);
    } else if (cat == mxh::proto::Category::Move ||
               cat == mxh::proto::Category::Chat ||
               cat == mxh::proto::Category::Item ||
               cat == mxh::proto::Category::Monster ||
               cat == mxh::proto::Category::Npc ||
               cat == mxh::proto::Category::Skill ||
               cat == mxh::proto::Category::Battle) {
        std::cout << "[Agent] " << mxh::proto::category_name(cat)
                  << " proto=" << (int)msg.header.protocol
                  << " from conn=" << id.value << "\n";
        // Snapshot map_client_ under lock to avoid race with set_map_server().
        mxh::net::TcpClient* mc = nullptr;
        {
            std::lock_guard<std::mutex> lk(map_route_mu_);
            mc = map_client_;
        }
        if (mc && mc->is_connected()) {
            std::uint32_t char_id = get_char_id(id);
            if (char_id != 0) {
                mxh::net::Message fwd;
                fwd.header = msg.header;
                fwd.header.object_id = char_id;
                fwd.payload = msg.payload;
                auto err = mc->send(fwd);
                if (err != mxh::net::NetError::Ok) {
                    std::cerr << "[Agent] failed to forward "
                              << mxh::proto::category_name(cat)
                              << " to MapServer: "
                              << mxh::net::to_string(err) << "\n";
                }
            }
        }
    } else {
        std::cout << "[Agent] unhandled category: "
                  << mxh::proto::category_name(cat)
                  << " proto=" << (int)msg.header.protocol << "\n";
    }
}

void AgentHandler::handle_userconn(mxh::net::ConnectionId id,
                                   const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::UserConnProtocol>(msg.header.protocol);

    if (use_legacy_framing_) {
        switch (proto) {
        case mxh::proto::UserConnProtocol::CharacterListSyn:
            handle_legacy_character_list(id, msg);
            break;
        case mxh::proto::UserConnProtocol::CharacterNameCheckSyn:
            handle_legacy_name_check(id, msg);
            break;
        case mxh::proto::UserConnProtocol::CharacterMakeSyn:
            handle_legacy_character_make(id, msg);
            break;
        case mxh::proto::UserConnProtocol::CharacterSelectSyn:
            handle_legacy_character_select(id, msg);
            break;
        case mxh::proto::UserConnProtocol::GameInSyn:
            handle_legacy_gamein_syn(id, msg);
            break;
        default:
            std::cout << "[Agent] legacy: unhandled proto="
                      << static_cast<int>(proto) << "\n";
            break;
        }
        return;
    }

    // Modern mode: same protocol numbers but different wire format.
    if (proto == mxh::proto::UserConnProtocol::CharacterListSyn) {
        std::cout << "[Agent] CharacterListSyn -> sending dummy list\n";
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterListAck);
        m.payload.resize(5, 0);
        m.payload[0] = 1;
        reply_(id, m);
    } else {
        std::cout << "[Agent] unhandled userconn proto="
                  << static_cast<int>(msg.header.protocol) << "\n";
    }
}

// ============================================================================
// Get user_id for a connection (stored during CharacterListSyn).
// ============================================================================

std::uint32_t AgentHandler::get_user_id(mxh::net::ConnectionId id) {
    std::lock_guard<std::mutex> lk(user_mu_);
    auto it = conn_user_ids_.find(id.value);
    if (it != conn_user_ids_.end()) return it->second;
    return 0;
}

std::uint32_t AgentHandler::get_char_id(mxh::net::ConnectionId id) {
    std::lock_guard<std::mutex> lk(user_mu_);
    auto it = conn_char_ids_.find(id.value);
    if (it != conn_char_ids_.end()) return it->second;
    return 0;
}

// ============================================================================
// Phase 9: MapServer integration
// ============================================================================

void AgentHandler::set_map_server(mxh::net::TcpClient* client,
                                  mxh::net::ConnectionId map_conn_id) {
    std::lock_guard<std::mutex> lk(map_route_mu_);
    map_client_ = client;
    map_conn_id_ = map_conn_id;
}

mxh::net::ConnectionId AgentHandler::get_map_connection() const {
    return map_conn_id_;
}

void AgentHandler::forward_from_map(mxh::net::ConnectionId /*map_id*/,
                                    const mxh::net::Message& msg) {
    auto cat = static_cast<mxh::proto::Category>(msg.header.category);

    // Phase 9.2: Move/Chat are broadcast messages — forward to ALL clients
    // except the sender (whose char_id matches msg.header.object_id).
    // Phase 10c: Monster/Npc/UserConn(MonsterAdd/ObjectRemove) are also
    // broadcast to all clients since they use monster object_id, not char_id.
    if (cat == mxh::proto::Category::Move ||
        cat == mxh::proto::Category::Chat ||
        cat == mxh::proto::Category::Monster ||
        cat == mxh::proto::Category::Npc ||
        cat == mxh::proto::Category::Skill) {
        std::uint32_t sender_char_id = msg.header.object_id;
        std::size_t routed = 0;
        {
            std::lock_guard<std::mutex> lk(map_route_mu_);
            for (auto& [chrid, conn_val] : char_to_client_) {
                if (chrid == sender_char_id) continue;
                reply_(mxh::net::ConnectionId{conn_val}, msg);
                ++routed;
            }
        }
        std::cout << "[Agent] broadcast " << mxh::proto::category_name(cat)
                  << " sender=" << sender_char_id
                  << " routed=" << routed << "\n";
        return;
    }

    // Phase 10c: UserConn messages with MonsterAdd/ObjectRemove (when
    // object_id >= 50000, i.e. a monster) need broadcast to all clients.
    if (cat == mxh::proto::Category::UserConn) {
        auto proto = static_cast<mxh::proto::UserConnProtocol>(msg.header.protocol);
        if (proto == mxh::proto::UserConnProtocol::MonsterAdd ||
            proto == mxh::proto::UserConnProtocol::BossMonsterAdd ||
            proto == mxh::proto::UserConnProtocol::NpcAdd) {
            // Broadcast to ALL clients
            std::size_t routed = 0;
            std::lock_guard<std::mutex> lk(map_route_mu_);
            for (auto& [chrid, conn_val] : char_to_client_) {
                reply_(mxh::net::ConnectionId{conn_val}, msg);
                ++routed;
            }
            std::cout << "[Agent] broadcast " << mxh::proto::category_name(cat)
                      << " proto=" << static_cast<int>(proto)
                      << " routed=" << routed << "\n";
            return;
        }
        if (proto == mxh::proto::UserConnProtocol::ObjectRemove) {
            // ObjectRemove could be for a monster — broadcast to all
            std::size_t routed = 0;
            std::lock_guard<std::mutex> lk(map_route_mu_);
            for (auto& [chrid, conn_val] : char_to_client_) {
                reply_(mxh::net::ConnectionId{conn_val}, msg);
                ++routed;
            }
            std::cout << "[Agent] broadcast ObjectRemove routed=" << routed << "\n";
            return;
        }
    }

    // Default: route to the single client that owns this char_id.
    std::uint32_t target_char_id = msg.header.object_id;
    std::uint64_t client_conn_value = 0;
    {
        std::lock_guard<std::mutex> lk(map_route_mu_);
        auto it = char_to_client_.find(target_char_id);
        if (it != char_to_client_.end()) {
            client_conn_value = it->second;
        }
    }

    if (client_conn_value != 0) {
        mxh::net::ConnectionId client_id{client_conn_value};
        std::cout << "[Agent] forwarding map response proto="
                  << static_cast<int>(msg.header.protocol)
                  << " to client=" << client_conn_value
                  << " charid=" << target_char_id << "\n";
        reply_(client_id, msg);
    } else {
        std::cout << "[Agent] map response proto="
                  << static_cast<int>(msg.header.protocol)
                  << " charid=" << target_char_id
                  << " no client routing found\n";
    }
}

// ============================================================================
// handle_legacy_character_list - proto=9
//
// Original client sends MSG_DWORD2:
//   MSGBASE(8B) + dwData1(4B)=UserID + dwData2(4B)=DistAuthKey
//
// We query the DB and return SEND_CHARSELECT_INFO with real data.
//
// SEND_CHARSELECT_INFO layout (KOR, no _CRYPTCHECK_):
//   MSGBASE(8B) [already in Message.header]
//   + CharNum(4B)
//   + StandingArrayNum[5](10B)
//   + BaseObjectInfo[5](5 * 35 = 175B)
//   + ChrTotalInfo[5](5 * 140 = 700B)
//   = payload total: 889B
// ============================================================================

void AgentHandler::handle_legacy_character_list(
    mxh::net::ConnectionId id, const mxh::net::Message& msg) {

    // Extract user_id from payload.
    std::uint32_t user_id = 0, dist_auth_key = 0;
    if (msg.payload.size() >= 4) {
        std::memcpy(&user_id, msg.payload.data(), 4);
    }
    if (msg.payload.size() >= 8) {
        std::memcpy(&dist_auth_key, msg.payload.data() + 4, 4);
    }

    std::cout << "[Agent] legacy: CHARACTERLIST_SYN user_id=" << user_id
              << " dist_auth_key=" << dist_auth_key << "\n";

    // Store user_id for this connection.
    {
        std::lock_guard<std::mutex> lk(user_mu_);
        conn_user_ids_[id.value] = user_id;
    }

    // Query DB for characters belonging to this user.
    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> params = {mxh::db::bind(std::to_string(user_id))};
    auto result = db_.query(
        "SELECT chrid, charname, sex_type, hair_type, face_type, body_type, "
        "start_area, height, width, level, map_num, standing_idx "
        "FROM character_info WHERE userid = ?",
        params, rs);

    int char_count = 0;
    if (result.ok()) {
        char_count = static_cast<int>(rs.size());
        if (char_count > kMaxCharSlots) char_count = kMaxCharSlots;
    }

    std::cout << "[Agent] legacy: found " << char_count << " character(s)\n";

    // Build response.
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListAck);

    auto& payload = m.payload;
    // Pre-allocate: 128 (2x HselInit) + 4 + 10 + 5*35 + 5*140 = 1017 bytes
    payload.reserve(1017);

    // Phase 11a fix: Add HselInit encryption keys (eninit + deinit = 128 bytes)
    // before CharNum. The client's SEND_CHARSELECT_INFO struct starts with
    // these fields when _CRYPTCHECK_ is defined.
    // Server eninit -> client uses as deinit; server deinit -> client uses as eninit.
    std::random_device rd;
    std::mt19937 rng(rd());
    put_hsel_init(payload, rng);  // eninit (server encrypt -> client decrypt)
    put_hsel_init(payload, rng);  // deinit (server decrypt -> client encrypt)

    // CharNum (int32, little-endian)
    put_i32(payload, char_count);

    // StandingArrayNum[5] (WORD each)
    for (int i = 0; i < kMaxCharSlots; ++i) {
        if (i < char_count) {
            put_u16(payload, static_cast<std::uint16_t>(
                get_int(rs, i, "standing_idx", 0)));
        } else {
            put_u16(payload, 0);
        }
    }

    // BaseObjectInfo[5] (35 bytes each)
    for (int i = 0; i < kMaxCharSlots; ++i) {
        if (i < char_count) {
            put_u32(payload, static_cast<std::uint32_t>(get_int(rs, i, "chrid")));
            put_u32(payload, user_id);
            put_str(payload, get_str(rs, i, "charname").c_str(), kMaxNameLength + 1);
            put_u32(payload, 0);   // BattleID
            put_u8(payload, 0);    // BattleTeam
            put_u8(payload, 0);    // ObjectState
            // SingleSpecialState[4] (bool[4])
            put_zeros(payload, 4);
        } else {
            put_zeros(payload, kBaseObjectInfoSize);
        }
    }

    // ChrTotalInfo[5] (140 bytes each, KOR version)
    for (int i = 0; i < kMaxCharSlots; ++i) {
        if (i < char_count) {
            // Life / MaxLife / Shield / MaxShield
            put_u32(payload, 100);  // Life (default for char select)
            put_u32(payload, 100);  // MaxLife
            put_u32(payload, 0);    // Shield
            put_u32(payload, 0);    // MaxShield

            // Appearance
            put_u8(payload, static_cast<std::uint8_t>(get_int(rs, i, "sex_type")));
            put_u8(payload, static_cast<std::uint8_t>(get_int(rs, i, "face_type")));
            put_u8(payload, static_cast<std::uint8_t>(get_int(rs, i, "hair_type")));

            // WearedItemIdx[10] (WORD each = 20 bytes)
            put_zeros(payload, 20);

            // Stage / Level / CurMapNum / LoginMapNum
            put_u8(payload, 0);    // Stage
            put_u16(payload, static_cast<std::uint16_t>(get_int(rs, i, "level", 1)));
            std::uint16_t map_num = static_cast<std::uint16_t>(
                get_int(rs, i, "map_num", kDefaultMapNum));
            put_u16(payload, map_num);  // CurMapNum
            put_u16(payload, map_num);  // LoginMapNum

            // bPeace
            put_u8(payload, 0);
            // MapChangePoint_Index / LoginPoint_Index
            put_u16(payload, 0);
            put_u16(payload, 0);
            // MunpaID / PositionInMunpa / MarkName
            put_u32(payload, 0);
            put_u8(payload, 0);
            put_u32(payload, 0);
            // bVisible / bPKMode / bMussangMode / BadFame
            put_u8(payload, 1);  // bVisible = true
            put_u8(payload, 0);  // bPKMode
            put_i32(payload, 0); // bMussangMode (BOOL = 4B)
            put_i32(payload, 0); // BadFame (FAMETYPE = int = 4B)

            // Height / Width
            put_f32(payload, static_cast<float>(get_double(rs, i, "height", 1.0)));
            put_f32(payload, static_cast<float>(get_double(rs, i, "width", 1.0)));

            // NickName[17] + GuildName[17]
            put_zeros(payload, 17 + 17);
            // dwGuildUnionIdx (4B)
            put_u32(payload, 0);
            // sGuildUnionName[17]
            put_zeros(payload, 17);
            // dwGuildUnionMarkIdx (4B)
            put_u32(payload, 0);
            // bRestraint / EventIndex / bNoAvatarView
            put_u8(payload, 0);
            put_u8(payload, 0);
            put_u8(payload, 0);
        } else {
            put_zeros(payload, kCharTotalInfoSize);
        }
    }

    std::cout << "[Agent] legacy: sending CHARACTERLIST_ACK payload="
              << payload.size() << "B (char_num=" << char_count << ")\n";
    reply_(id, m);
}

// ============================================================================
// handle_legacy_name_check - proto=19
//
// Client sends MSG_NAME:
//   MSGBASE(8B) + Name[17]
//
// We query DB for name uniqueness:
//   - Available: send MSGBASE-only ACK (proto=20)
//   - Taken: send MSG_WORD NACK (proto=21, wData=2)
// ============================================================================

void AgentHandler::handle_legacy_name_check(
    mxh::net::ConnectionId id, const mxh::net::Message& msg) {

    // MSG_NAME: MSGBASE(8B) + Name[17]
    char name[kMaxNameLength + 1] = {};
    if (msg.payload.size() >= kMaxNameLength) {
        std::memcpy(name, msg.payload.data(), kMaxNameLength);
    }
    name[kMaxNameLength] = '\0';

    std::cout << "[Agent] legacy: NAMECHECK_SYN name='" << name << "'\n";

    // Query DB for name.
    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> name_params = {mxh::db::bind(std::string(name))};
    auto result = db_.query(
        "SELECT 1 FROM character_info WHERE charname = ?",
        name_params, rs);

    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);

    if (!result.ok()) {
        // Query failed -> NACK with wData=1 (internal error)
        std::cerr << "[Agent] name check query error: "
                  << result.error_message << "\n";
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterNameCheckNack);
        put_u16(m.payload, 1);  // wData = 1 (internal error)
    } else if (rs.empty()) {
        // Name is available -> ACK (MSGBASE only, no payload)
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterNameCheckAck);
        std::cout << "[Agent] legacy: name '" << name << "' is AVAILABLE\n";
    } else {
        // Name is taken -> NACK with MSG_WORD format (wData=2)
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterNameCheckNack);
        put_u16(m.payload, 2);  // wData = 2 (name already exists)
        std::cout << "[Agent] legacy: name '" << name << "' is TAKEN\n";
    }

    reply_(id, m);
}

// ============================================================================
// handle_legacy_character_make - proto=22
//
// Client sends CHARACTERMAKEINFO:
//   MSGBASE(8B) + Name[17] + UserID(4B) + SexType(1B) + BodyType(1B)
//   + HairType(1B) + FaceType(1B) + StartArea(1B) + bDuplCheck(4B)
//   + WearedItemIdx[10](20B) + StandingArrayNum(1B) + Height(4B) + Width(4B)
//   Total payload after MSGBASE: 59 bytes
//
// On success: re-send character list (same as original behavior).
// On failure: send CharacterMakeNack (proto=24).
// ============================================================================

void AgentHandler::handle_legacy_character_make(
    mxh::net::ConnectionId id, const mxh::net::Message& msg) {

    // Get stored user_id for this connection.
    std::uint32_t user_id = get_user_id(id);

    // Parse CHARACTERMAKEINFO payload.
    constexpr std::size_t kMinPayload = 59;  // minimum required bytes
    if (msg.payload.size() < kMinPayload || user_id == 0) {
        std::cout << "[Agent] legacy: CHARACTERMAKE_SYN invalid (payload="
                  << msg.payload.size() << " user_id=" << user_id << ")\n";
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterMakeNack);
        reply_(id, m);
        return;
    }

    const auto* data = msg.payload.data();

    // Name[17] at offset 0
    char name[kMaxNameLength + 1] = {};
    std::memcpy(name, data, kMaxNameLength);
    name[kMaxNameLength] = '\0';

    // UserID at offset 17 (overwritten by server)
    // DWORD client_user_id;
    // std::memcpy(&client_user_id, data + 17, 4);

    // SexType at offset 21
    std::uint8_t sex_type = data[21];
    // BodyType at offset 22
    std::uint8_t body_type = data[22];
    // HairType at offset 23
    std::uint8_t hair_type = data[23];
    // FaceType at offset 24
    std::uint8_t face_type = data[24];
    // StartArea at offset 25
    std::uint8_t start_area = data[25];
    // bDuplCheck at offset 26 (4B BOOL)
    // WearedItemIdx[10] at offset 30 (20B)
    // StandingArrayNum at offset 50
    std::uint8_t standing_idx = data[50];
    // Height at offset 51 (float)
    float height = 0;
    std::memcpy(&height, data + 51, 4);
    // Width at offset 55 (float)
    float width = 0;
    std::memcpy(&width, data + 55, 4);

    std::cout << "[Agent] legacy: CHARACTERMAKE_SYN name='" << name
              << "' sex=" << (int)sex_type
              << " hair=" << (int)hair_type
              << " face=" << (int)face_type
              << " h=" << height << " w=" << width << "\n";

    // Validate (same as original CheckCharacterMakeInfo).
    if (sex_type > 1 || hair_type > 4 || face_type > 4) {
        std::cout << "[Agent] legacy: invalid character params\n";
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterMakeNack);
        reply_(id, m);
        return;
    }

    // Check name uniqueness.
    {
        mxh::db::ResultSet rs;
        std::vector<mxh::db::Bind> dup_params = {mxh::db::bind(std::string(name))};
        auto dup_result = db_.query(
            "SELECT 1 FROM character_info WHERE charname = ?",
            dup_params, rs);
        if (!rs.empty()) {
            std::cout << "[Agent] legacy: name '" << name << "' already exists\n";
            mxh::net::Message m;
            m.header.category = static_cast<std::uint8_t>(
                mxh::proto::Category::UserConn);
            m.header.protocol = static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::CharacterMakeNack);
            reply_(id, m);
            return;
        }
    }

    // Generate unique chrid.
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::int64_t> dist(100000, 9999999);
    std::int64_t chrid = dist(rng);

    // Insert into character_info.
    std::vector<mxh::db::Bind> ins_params = {
        mxh::db::bind(chrid),
        mxh::db::bind(std::string(name)),
        mxh::db::bind(std::to_string(user_id)),
        mxh::db::bind(static_cast<std::int64_t>(sex_type)),
        mxh::db::bind(static_cast<std::int64_t>(hair_type)),
        mxh::db::bind(static_cast<std::int64_t>(face_type)),
        mxh::db::bind(static_cast<std::int64_t>(body_type)),
        mxh::db::bind(static_cast<std::int64_t>(start_area)),
        mxh::db::bind(static_cast<double>(height)),
        mxh::db::bind(static_cast<double>(width)),
        mxh::db::bind(static_cast<std::int64_t>(kDefaultMapNum)),
        mxh::db::bind(static_cast<std::int64_t>(standing_idx))
    };
    auto result = db_.execute(
        "INSERT INTO character_info "
        "(chrid, charname, userid, sex_type, hair_type, face_type, "
        "body_type, start_area, height, width, level, map_num, standing_idx) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1, ?, ?)",
        ins_params);

    if (!result.ok()) {
        std::cerr << "[Agent] CreateCharacter DB error: "
                  << result.error_message << "\n";
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterMakeNack);
        reply_(id, m);
        return;
    }

    std::cout << "[Agent] Created character '" << name << "' chrid=" << chrid
              << " user_id=" << user_id << "\n";

    // Re-send character list (same as original: RCreateCharacter calls
    // UserIDXSendAndCharacterBaseInfo on success).
    mxh::net::Message list_msg;
    list_msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    list_msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListSyn);
    // Reconstruct the MSG_DWORD2 payload with user_id.
    list_msg.payload.resize(8, 0);
    std::memcpy(list_msg.payload.data(), &user_id, 4);

    handle_legacy_character_list(id, list_msg);
}

// ============================================================================
// handle_legacy_character_select - proto=16
//
// Client sends MSG_WORD after selecting a character:
//   MSGBASE(8B) [object_id = character ID]
//   + wData (2B) = channel number
//
// We validate the character belongs to this user, find the map number,
// and respond with MSG_BYTE:
//   MSGBASE(8B) [object_id = character ID]
//   + bData (1B) = map number
// ============================================================================

void AgentHandler::handle_legacy_character_select(
    mxh::net::ConnectionId id, const mxh::net::Message& msg) {

    std::uint32_t user_id = get_user_id(id);
    if (user_id == 0) {
        std::cout << "[Agent] CHARACTERSELECT_SYN no user_id\n";
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterSelectNack);
        reply_(id, m);
        return;
    }

    std::uint32_t character_id = msg.header.object_id;
    std::uint16_t channel = 0;
    if (msg.payload.size() >= 2) {
        std::memcpy(&channel, msg.payload.data(), 2);
    }

    std::cout << "[Agent] CHARACTERSELECT_SYN charid=" << character_id
              << " ch=" << channel << " uid=" << user_id << "\n";

    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> params = {
        mxh::db::bind(std::to_string(character_id)),
        mxh::db::bind(std::to_string(user_id))
    };
    auto result = db_.query(
        "SELECT chrid, map_num, charname FROM character_info "
        "WHERE chrid = ? AND userid = ?",
        params, rs);
    if (!result.ok() || rs.empty()) {
        std::cout << "[Agent] CHARACTERSELECT_NACK char not found"
                  << " result.ok=" << result.ok()
                  << " rs.empty=" << rs.empty()
                  << " err='" << result.error_message << "'"
                  << " chrid=" << character_id
                  << " uid=" << user_id << "\n";
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        m.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterSelectNack);
        m.header.object_id = character_id;
        reply_(id, m);
        return;
    }

    std::uint16_t map_num = static_cast<std::uint16_t>(
        get_int(rs, 0, "map_num", kDefaultMapNum));

    std::cout << "[Agent] CHARACTERSELECT_ACK chrid=" << character_id
              << " map=" << map_num << " name='"
              << get_str(rs, 0, "charname") << "'\n";

    {
        std::lock_guard<std::mutex> lk(user_mu_);
        conn_char_ids_[id.value] = character_id;
        conn_map_nums_[id.value] = map_num;
    }
    // Phase 9: register char_id → client routing for MapServer responses.
    {
        std::lock_guard<std::mutex> lk(map_route_mu_);
        char_to_client_[character_id] = id.value;
        std::cout << "[Agent] registered char_to_client_[" << character_id
                  << "] = conn=" << id.value << "\n";
    }

    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterSelectAck);
    m.header.object_id = character_id;
    put_u8(m.payload, static_cast<std::uint8_t>(map_num));
    std::cout << "[Agent] reply_ CHARACTERSELECT_ACK to conn=" << id.value
              << " payload_size=" << m.payload.size() << "\n";
    reply_(id, m);
}

// ============================================================================
// handle_legacy_gamein_syn - proto=28
//
// Client sends MSG_DWORD2 after loading map:
//   MSGBASE(8B) [object_id = character ID]
//   + dwData1 (4B) = channel number
//   + dwData2 (4B) = client data
//
// Original flow: Agent wraps into MSG_DWORD4 and forwards to MapServer.
// MapServer processes and sends GameInAck back.
//
// Stub: no MapServer yet, so we respond with minimal GameInAck (proto=29)
// containing simplified SEND_HERO_TOTALINFO.
// ============================================================================

void AgentHandler::handle_legacy_gamein_syn(
    mxh::net::ConnectionId id, const mxh::net::Message& msg) {

    std::uint32_t user_id = get_user_id(id);
    std::uint32_t char_id = get_char_id(id);
    std::uint16_t map_num = 0;
    {
        std::lock_guard<std::mutex> lk(user_mu_);
        auto it = conn_map_nums_.find(id.value);
        if (it != conn_map_nums_.end()) map_num = it->second;
    }

    std::uint32_t channel = 0;
    if (msg.payload.size() >= 4) {
        std::memcpy(&channel, msg.payload.data(), 4);
    }

    std::cout << "[Agent] GAMEIN_SYN charid=" << char_id
              << " ch=" << channel << " map=" << map_num << "\n";

    if (char_id == 0) {
        std::cout << "[Agent] GAMEIN_SYN no character selected\n";
        return;
    }

    // Phase 9: Forward GameInSyn to MapServer if connected.
    // Snapshot map_client_ under lock to avoid race with set_map_server().
    mxh::net::TcpClient* mc = nullptr;
    {
        std::lock_guard<std::mutex> lk(map_route_mu_);
        mc = map_client_;
    }
    if (mc && mc->is_connected()) {
        std::cout << "[Agent] forwarding GAMEIN_SYN to MapServer\n";

        // Build MSG_DWORD4 for MapServer (matching original AgentNetworkMsgParser.cpp:853-887).
        // Category=UserConn, Protocol=GameInSyn, ObjectID=char_id
        // dwData1=UniqueConnectIdx, dwData2=channel, dwData3=UserLevel, dwData4=returnMapNum
        mxh::net::Message fwd;
        fwd.header.category = msg.header.category;
        fwd.header.protocol = msg.header.protocol;
        fwd.header.object_id = char_id;
        fwd.payload.resize(16, 0);
        std::memcpy(fwd.payload.data() + 0, &user_id, 4);   // dwData1 = UniqueConnectIdx
        std::memcpy(fwd.payload.data() + 4, &channel, 4);    // dwData2 = channel
        std::uint32_t user_level = 0;
        std::memcpy(fwd.payload.data() + 8, &user_level, 4); // dwData3 = UserLevel
        std::memcpy(fwd.payload.data() + 12, &map_num, 4);   // dwData4 = returnMapNum

        auto err = mc->send(fwd);
        if (err != mxh::net::NetError::Ok) {
            std::cerr << "[Agent] failed to forward GAMEIN_SYN to MapServer: "
                      << mxh::net::to_string(err) << "\n";
        }
        // Response will come back via forward_from_map() when MapServer replies.
        return;
    }

    // Fallback: no MapServer connected, return stub GameInAck.
    std::cout << "[Agent] no MapServer, returning stub GameInAck\n";
    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    ack.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInAck);
    ack.header.object_id = char_id;

    put_u32(ack.payload, user_id);  // UniqueIDinAgent

    // BaseObjectInfo (35 bytes)
    put_u32(ack.payload, char_id);  // dwObjectID
    put_u32(ack.payload, user_id);  // dwUserID
    {
        mxh::db::ResultSet nr;
        std::vector<mxh::db::Bind> np = { mxh::db::bind(std::to_string(char_id)) };
        auto r = db_.query("SELECT charname FROM character_info WHERE chrid = ?",
                           np, nr);
        std::string nm = (r.ok() && !nr.empty()) ? get_str(nr, 0, "charname") : "Unknown";
        put_str(ack.payload, nm.c_str(), kMaxNameLength + 1);
    }
    put_u32(ack.payload, 0);   // BattleID
    put_u8(ack.payload, 0);    // BattleTeam
    put_u8(ack.payload, 0);    // ObjectState
    put_zeros(ack.payload, 4); // SingleSpecialState[4]

    // ChrTotalInfo (140 bytes)
    put_u32(ack.payload, 100); // Life
    put_u32(ack.payload, 100); // MaxLife
    put_u32(ack.payload, 0);   // Shield
    put_u32(ack.payload, 0);   // MaxShield
    {
        mxh::db::ResultSet ci;
        std::vector<mxh::db::Bind> cp = { mxh::db::bind(std::to_string(char_id)) };
        auto cr = db_.query(
            "SELECT sex_type,face_type,hair_type,height,width,level,map_num "
            "FROM character_info WHERE chrid = ?", cp, ci);
        if (cr.ok() && !ci.empty()) {
            put_u8(ack.payload, static_cast<std::uint8_t>(get_int(ci,0,"sex_type")));
            put_u8(ack.payload, static_cast<std::uint8_t>(get_int(ci,0,"face_type")));
            put_u8(ack.payload, static_cast<std::uint8_t>(get_int(ci,0,"hair_type")));
            put_zeros(ack.payload, 20); // WearedItemIdx[10]
            put_u8(ack.payload, 0);     // Stage
            put_u16(ack.payload, static_cast<std::uint16_t>(get_int(ci,0,"level",1)));
            std::uint16_t m = static_cast<std::uint16_t>(get_int(ci,0,"map_num",kDefaultMapNum));
            put_u16(ack.payload, m); put_u16(ack.payload, m); // Cur+Login
            put_u8(ack.payload, 0); put_u16(ack.payload, 0); put_u16(ack.payload, 0);
            put_u32(ack.payload, 0); put_u8(ack.payload, 0); put_u32(ack.payload, 0);
            put_u8(ack.payload, 1); put_u8(ack.payload, 0);
            put_i32(ack.payload, 0); put_i32(ack.payload, 0);
            put_f32(ack.payload, static_cast<float>(get_double(ci,0,"height",1.0)));
            put_f32(ack.payload, static_cast<float>(get_double(ci,0,"width",1.0)));
        } else {
            put_u8(ack.payload, 0); put_u8(ack.payload, 0); put_u8(ack.payload, 0);
            put_zeros(ack.payload, 20); put_u8(ack.payload, 0);
            put_u16(ack.payload, 1); put_u16(ack.payload, kDefaultMapNum);
            put_u16(ack.payload, kDefaultMapNum);
            put_u8(ack.payload, 0); put_u16(ack.payload, 0); put_u16(ack.payload, 0);
            put_u32(ack.payload, 0); put_u8(ack.payload, 0); put_u32(ack.payload, 0);
            put_u8(ack.payload, 1); put_u8(ack.payload, 0);
            put_i32(ack.payload, 0); put_i32(ack.payload, 0);
            put_f32(ack.payload, 1.0f); put_f32(ack.payload, 1.0f);
        }
    }
    put_zeros(ack.payload, 17 + 17); // NickName + GuildName
    put_u32(ack.payload, 0);         // dwGuildUnionIdx
    put_zeros(ack.payload, 17);      // sGuildUnionName
    put_u32(ack.payload, 0);         // dwGuildUnionMarkIdx
    put_u8(ack.payload, 0); put_u8(ack.payload, 0); put_u8(ack.payload, 0);

    std::cout << "[Agent] GAMEIN_ACK stub (" << ack.payload.size()
              << "B) chrid=" << char_id << "\n";
    reply_(id, ack);
}

}  // namespace mxh::server
