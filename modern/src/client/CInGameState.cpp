// mxh/client/CInGameState.cpp
// Phase B.2.3 â€” in-game state implementation.

#include "CInGameState.hpp"
#include "CEngine.hpp"
#include "CMainGame.hpp"

#include <cstring>
#include <utility>

#include "mxh/log/mlog.hpp"
#include "mxh/game/hero_total_layout.hpp"
#include "mxh/proto/protocol.hpp"

namespace mxh::client {

// -------------------------------------------------------------------------
// Wire-format helpers (pure functions, unit-tested independently).
//
// Offsets match map_handler.cpp:
//   kPayloadBaseObjOff   = 0
//   kPayloadCharTotalOff = 35
//   HERO_TOTAL_SERVER_TIME_OFFSET = 3757
// -------------------------------------------------------------------------

namespace {
// Match map_handler.cpp's put_u32 (LE) layout.
inline std::uint32_t get_u32(const std::uint8_t* p) {
    return  static_cast<std::uint32_t>(p[0])
         | (static_cast<std::uint32_t>(p[1]) << 8)
         | (static_cast<std::uint32_t>(p[2]) << 16)
         | (static_cast<std::uint32_t>(p[3]) << 24);
}
inline std::uint16_t get_u16(const std::uint8_t* p) {
    return  static_cast<std::uint16_t>(p[0])
         | (static_cast<std::uint16_t>(p[1]) << 8);
}
} // namespace

std::optional<MonsterAddInfo>
parse_legacy_monster_add(std::span<const std::uint8_t> payload) {
    if (payload.size() < 64) return std::nullopt;
    MonsterAddInfo info;
    info.object_id = get_u32(payload.data() + 0);
    info.user_id = get_u32(payload.data() + 4);
    for (std::size_t i = 0; i < 17; ++i) info.name[i] = static_cast<char>(payload[8 + i]);
    info.current_life = get_u32(payload.data() + 35);
    info.current_shield = get_u32(payload.data() + 39);
    info.monster_kind = get_u16(payload.data() + 43);
    info.group = get_u16(payload.data() + 45);
    info.map_num = get_u16(payload.data() + 47);
    return info;
}

std::optional<GameInInfo>
parse_legacy_gamein_ack(std::span<const std::uint8_t> payload) {
    // Need at least the trailing ServerTime block (current fixed payload).
    if (payload.size() < mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE) return std::nullopt;

    GameInInfo info;
    // BASEOBJECT_INFO [0..35)
    info.player_id = get_u32(payload.data() + 0);
    info.user_id   = get_u32(payload.data() + 4);
    // name is char[17] null-padded at offset 8; stop at the first NUL
    // (or at the 17-byte boundary).
    constexpr std::size_t kNameOffset = 8;
    constexpr std::size_t kNameMax    = 17;
    std::size_t name_end = kNameMax;
    for (std::size_t i = 0; i < kNameMax; ++i) {
        if (payload[kNameOffset + i] == 0) { name_end = i; break; }
    }
    info.name.assign(reinterpret_cast<const char*>(
                        payload.data() + kNameOffset),
                    name_end);

    // CHARACTER_TOTALINFO [35..147)
    info.life     = static_cast<std::uint16_t>(
                        get_u32(payload.data() + 35 + 0)  & 0xFFFFu);
    info.max_life = static_cast<std::uint16_t>(
                        get_u32(payload.data() + 35 + 4)  & 0xFFFFu);
    info.gender   = payload[35 + 16];
    info.level    = get_u16(payload.data() + 35 + 40);
    info.map_num  = get_u16(payload.data() + 35 + 42);

    // SYSTEMTIME ServerTime at HERO_TOTAL_SERVER_TIME_OFFSET â€” 5 little-endian u16s:
    //   +0 year, +2 month, +4 wday, +6 day, +8 hour
    info.server_year  = get_u16(payload.data() + mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET + 0);
    info.server_month = get_u16(payload.data() + mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET + 2);
    // The next two bytes after month contain wday; we do not store it.
    info.server_day   = get_u16(payload.data() + mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET + 6);
    info.server_hour  = get_u16(payload.data() + mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET + 8);

    return info;
}

// -------------------------------------------------------------------------
// CInGameState
// -------------------------------------------------------------------------

CInGameState::CInGameState() = default;

CInGameState::~CInGameState() {
    if (m_client && m_client->is_connected()) m_client->disconnect();
}

void CInGameState::Init(void* /*pInitParam*/) {
    MLOG_DEBUG("CInGameState::Init (waiting for Start() from host)");
    setInitialized(true);
}

void CInGameState::Release() {
    MLOG_DEBUG("CInGameState::Release");
    if (m_client) {
        if (m_client->is_connected()) m_client->disconnect();
        m_client.reset();
    }
    m_info = GameInInfo{};
    m_inGame   = false;
    m_started  = false;
    m_failed   = false;
    m_failureReason.clear();
    setInitialized(false);
}

void CInGameState::Process() {
    tick();
    // MapServer doesn't push DistConnectSuccess (unlike Distribute /
    // Agent), so we have to send GameInSyn from the host.  We try
    // in on_connect first (most paths); if that fails or hasn't
    // happened yet, Process() retries every tick once a connect is
    // detected.  m_sentGameInSyn is set once the send succeeded.
    if (m_client && m_client->is_connected() && !m_sentGameInSyn) {
        send_gamein_syn();
    }
}

void CInGameState::Start(CEngine* engine, std::string host,
                          std::uint16_t port,
                          std::uint32_t player_id, std::uint16_t map_num) {
    m_pEngine = engine;
    m_host    = std::move(host);
    m_port    = port;
    m_playerId = player_id;
    m_mapNum   = map_num;
    if (m_started) return;
    m_started = true;
    MLOG_INFO("CInGameState connecting to %s:%u (player_id=%u, map=%u)",
              m_host.c_str(), static_cast<unsigned>(m_port),
              static_cast<unsigned>(m_playerId),
              static_cast<unsigned>(m_mapNum));
    m_client = std::make_unique<mxh::net::TcpClient>(*this);
    mxh::net::ClientConfig cfg;
    cfg.remote_address     = m_host;
    cfg.port               = m_port;
    cfg.use_legacy_framing = true;
    cfg.use_encryption     = false;
    cfg.connect_timeout    = std::chrono::milliseconds(3000);
    auto e = m_client->connect(cfg);
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("TcpClient::connect to MapServer failed: ") +
                  mxh::net::to_string(e));
    }
}

bool CInGameState::is_connected() const noexcept {
    return m_client && m_client->is_connected();
}

bool CInGameState::on_connect(mxh::net::ConnectionId id,
                              const std::string& remote_addr) {
    MLOG_INFO("CInGameState::on_connect id=%llu from %s",
              static_cast<unsigned long long>(id.value),
              remote_addr.c_str());
    (void)id;
    (void)remote_addr;
    // MapServer's on_connect doesn't push DistConnectSuccess; send
    // GameInSyn right after TCP accept completes.
    send_gamein_syn();
    return true;
}

void CInGameState::on_message(mxh::net::ConnectionId id,
                               const mxh::net::Message& msg) {
    using mxh::proto::Category;
    using mxh::proto::UserConnProtocol;
    const auto cat   = static_cast<Category>(msg.header.category);
    const auto proto = static_cast<UserConnProtocol>(msg.header.protocol);
    MLOG_DEBUG("CInGameState::on_message id=%llu cat=%s proto=%d obj=%u payload=%zu",
               static_cast<unsigned long long>(id.value),
               mxh::proto::category_name(cat),
               static_cast<int>(proto),
               static_cast<unsigned>(msg.header.object_id),
               msg.payload.size());
    if (cat != Category::UserConn) {
        // Phase 10b: MapServer may also push ITEM_TOTALINFO_LOCAL
        // (Category::Item, ItemProtocol::TotalInfoLocal) after the
        // GameInAck.  Phase B.2.3 logs and ignores it; Phase C+
        // will plug it into the inventory UI.
        MLOG_DEBUG("CInGameState: ignoring non-UserConn message (cat=%s, proto=%d)",
                   mxh::proto::category_name(cat),
                   static_cast<int>(proto));
        return;
    }
    switch (proto) {
        case UserConnProtocol::GameInAck: {
            auto info = parse_legacy_gamein_ack(msg.payload);
            if (!info) {
                fail_with("GameInAck payload too short for SEND_HERO_TOTALINFO");
                return;
            }
            dispatch_gamein_ack(*info);
            break;
        }
        case UserConnProtocol::GameOutAck: {
            MLOG_INFO("CInGameState: GameOutAck (server confirmed disconnect)");
            break;
        }
        case UserConnProtocol::MonsterAdd: {
            auto info = parse_legacy_monster_add(msg.payload);
            if (!info) {
                MLOG_WARN("CInGameState: MonsterAdd payload too short (%zu bytes)",
                          msg.payload.size());
                break;
            }
            monsters_.push_back(*info);
            MLOG_DEBUG("CInGameState: MonsterAdd object_id=%u kind=%u life=%u name=%.16s",
                       static_cast<unsigned>(info->object_id),
                       static_cast<unsigned>(info->monster_kind),
                       static_cast<unsigned>(info->current_life), info->name);
            break;
        }
        case UserConnProtocol::ConnectionCheckOk: {
            // Phase 10d keep-alive.  Server pushes this every ~10s
            // once you're in game; we just log.
            MLOG_DEBUG("CInGameState: ConnectionCheckOk (keep-alive)");
            break;
        }
        default:
            MLOG_WARN("CInGameState: unhandled userconn proto=%d",
                      static_cast<int>(proto));
            break;
    }
}

void CInGameState::on_disconnect(mxh::net::ConnectionId id,
                                  mxh::net::NetError reason) {
    MLOG_INFO("CInGameState::on_disconnect id=%llu reason=%s",
              static_cast<unsigned long long>(id.value),
              mxh::net::to_string(reason));
    if (m_inGame && !m_failed) {
        MLOG_WARN("CInGameState: disconnected after entering game (in_game=%d)",
                  m_inGame ? 1 : 0);
    }
}

void CInGameState::send_gamein_syn() {
    if (m_sentGameInSyn) return;
    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    out.header.object_id = m_playerId;   // chrid in MSGBASE
    out.payload          = {};           // empty payload
    const auto e = m_client->send(out);
    if (e != mxh::net::NetError::Ok) {
        // Don't fail; Process() will retry on the next tick once the
        // connection is fully up.  TcpClient::send() can transiently
        // return Disconnected during the connect handshake.
        MLOG_DEBUG("CInGameState: send GameInSyn transiently failed: %s",
                   mxh::net::to_string(e));
        return;
    }
    m_sentGameInSyn = true;
    MLOG_INFO("CInGameState: sent GameInSyn player_id=%u (empty payload)",
              static_cast<unsigned>(m_playerId));
}

void CInGameState::dispatch_gamein_ack(const GameInInfo& info) {
    m_info   = info;
    m_inGame = true;
    MLOG_INFO("CInGameState: GameInAck player_id=%u user_id=%u name='%s' "
              "level=%u map=%u life=%u/%u gender=%u "
              "server_time=%u-%u-%u %u:00",
              static_cast<unsigned>(info.player_id),
              static_cast<unsigned>(info.user_id),
              info.name.c_str(),
              static_cast<unsigned>(info.level),
              static_cast<unsigned>(info.map_num),
              static_cast<unsigned>(info.life),
              static_cast<unsigned>(info.max_life),
              static_cast<unsigned>(info.gender),
              static_cast<unsigned>(info.server_year),
              static_cast<unsigned>(info.server_month),
              static_cast<unsigned>(info.server_day),
              static_cast<unsigned>(info.server_hour));
    // B.2.3 doesn't switch state â€” the in-game loop is the terminal
    // happy state.  Future Phase D will hook chat/movement/combat
    // handlers here.
}

void CInGameState::fail_with(const std::string& reason) {
    if (m_failed) return;
    m_failed = true;
    m_failureReason = reason;
    MLOG_ERROR("CInGameState: %s", reason.c_str());
}

} // namespace mxh::client
