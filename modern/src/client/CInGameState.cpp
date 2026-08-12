// mxh/client/CInGameState.cpp
// Phase B.2.3 â€” in-game state implementation.

#include "CInGameState.hpp"
#include "CEngine.hpp"
#include "CMainGame.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
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

inline void put_u16(std::vector<std::uint8_t>& dst, std::size_t off,
                    std::uint16_t v) {
    dst[off + 0] = static_cast<std::uint8_t>(v & 0xFFu);
    dst[off + 1] = static_cast<std::uint8_t>((v >> 8) & 0xFFu);
}

inline void put_u32(std::vector<std::uint8_t>& dst, std::size_t off,
                    std::uint32_t v) {
    dst[off + 0] = static_cast<std::uint8_t>(v & 0xFFu);
    dst[off + 1] = static_cast<std::uint8_t>((v >> 8) & 0xFFu);
    dst[off + 2] = static_cast<std::uint8_t>((v >> 16) & 0xFFu);
    dst[off + 3] = static_cast<std::uint8_t>((v >> 24) & 0xFFu);
}

std::uint64_t steady_now_ms() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<
        std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}
} // namespace

// -------------------------------------------------------------------------
// In-game input + gameplay wire helpers.
// -------------------------------------------------------------------------

std::uint32_t key_mask_for_vk(std::uint32_t vk) noexcept {
    switch (vk) {
        case kVkW:
        case kVkUp:
            return static_cast<std::uint32_t>(MoveKey::Forward);
        case kVkS:
        case kVkDown:
            return static_cast<std::uint32_t>(MoveKey::Back);
        case kVkQ:
            return static_cast<std::uint32_t>(MoveKey::StrafeLeft);
        case kVkE:
            return static_cast<std::uint32_t>(MoveKey::StrafeRight);
        case kVkA:
        case kVkLeft:
            return static_cast<std::uint32_t>(MoveKey::RotateLeft);
        case kVkD:
        case kVkRight:
            return static_cast<std::uint32_t>(MoveKey::RotateRight);
        default:
            return 0;
    }
}

MoveResult step_movement(std::uint32_t keyMask, float yaw,
                         float x, float z, float dt) noexcept {
    MoveResult result;
    result.x = x;
    result.z = z;
    result.yaw = yaw;
    if (dt <= 0.0f) return result;

    // Camera-relative basis: at yaw=0 the legacy camera faces +Z.
    const float fwdX = std::sin(yaw);
    const float fwdZ = std::cos(yaw);
    const float rightX = std::cos(yaw);
    const float rightZ = -std::sin(yaw);

    float dx = 0.0f;
    float dz = 0.0f;
    if (keyMask & static_cast<std::uint32_t>(MoveKey::Forward)) {
        dx += fwdX; dz += fwdZ;
    }
    if (keyMask & static_cast<std::uint32_t>(MoveKey::Back)) {
        dx -= fwdX; dz -= fwdZ;
    }
    if (keyMask & static_cast<std::uint32_t>(MoveKey::StrafeLeft)) {
        dx -= rightX; dz -= rightZ;
    }
    if (keyMask & static_cast<std::uint32_t>(MoveKey::StrafeRight)) {
        dx += rightX; dz += rightZ;
    }

    const float len = std::sqrt(dx * dx + dz * dz);
    if (len > 0.0001f) {
        dx /= len;
        dz /= len;
        result.x = std::clamp(x + dx * kMoveSpeed * dt, 0.0f, kWorldLimit);
        result.z = std::clamp(z + dz * kMoveSpeed * dt, 0.0f, kWorldLimit);
        result.moving = true;
    }
    if (keyMask & static_cast<std::uint32_t>(MoveKey::RotateLeft)) {
        result.yaw -= kRotateSpeed * dt;
    }
    if (keyMask & static_cast<std::uint32_t>(MoveKey::RotateRight)) {
        result.yaw += kRotateSpeed * dt;
    }
    return result;
}

std::optional<std::uint32_t>
pick_attack_target(const std::vector<MonsterAddInfo>& monsters,
                   float px, float pz, float range) noexcept {
    float bestSq = range * range;
    std::optional<std::uint32_t> best;
    for (const auto& monster : monsters) {
        if (monster.current_life == 0) continue;
        const float dx = static_cast<float>(monster.position_x) - px;
        const float dz = static_cast<float>(monster.position_z) - pz;
        const float d2 = dx * dx + dz * dz;
        if (d2 <= bestSq) {
            bestSq = d2;
            best = monster.object_id;
        }
    }
    return best;
}

mxh::net::Message make_move_message(std::uint32_t player_id,
                                    mxh::proto::MoveProtocol proto,
                                    std::uint16_t x, std::uint16_t z) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Move);
    message.header.protocol = static_cast<std::uint8_t>(proto);
    message.header.object_id = player_id;
    message.payload.resize(4);
    put_u16(message.payload, 0, x);
    put_u16(message.payload, 2, z);
    return message;
}

mxh::net::Message make_attack_message(std::uint32_t player_id,
                                      std::uint32_t skill_idx,
                                      std::uint32_t main_target,
                                      float target_x, float target_z) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Skill);
    message.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::SkillProtocol::StartSyn);
    message.header.object_id = player_id;
    message.payload.resize(16);
    put_u32(message.payload, 0, skill_idx);
    put_u32(message.payload, 4, main_target);
    std::memcpy(message.payload.data() + 8, &target_x, sizeof(target_x));
    std::memcpy(message.payload.data() + 12, &target_z, sizeof(target_z));
    return message;
}

mxh::net::Message make_quest_message(std::uint32_t player_id,
                                     mxh::proto::QuestProtocol protocol,
                                     std::uint16_t quest_id) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    message.header.protocol = static_cast<std::uint8_t>(protocol);
    message.header.object_id = player_id;
    message.payload.resize(2);
    put_u16(message.payload, 0, quest_id);
    return message;
}

std::optional<std::pair<std::uint16_t, std::uint16_t>>
parse_move_payload(std::span<const std::uint8_t> payload) {
    if (payload.size() < 4) return std::nullopt;
    const std::uint16_t x = static_cast<std::uint16_t>(
        payload[0] | (static_cast<std::uint16_t>(payload[1]) << 8));
    const std::uint16_t z = static_cast<std::uint16_t>(
        payload[2] | (static_cast<std::uint16_t>(payload[3]) << 8));
    return std::make_pair(x, z);
}

std::optional<std::pair<std::uint32_t, std::uint32_t>>
parse_monster_life_payload(std::span<const std::uint8_t> payload) {
    if (payload.size() < 8) return std::nullopt;
    const auto life = get_u32(payload.data() + 0);
    const auto shield = get_u32(payload.data() + 4);
    return std::make_pair(life, shield);
}

mxh::net::Message make_chat_message(std::uint32_t player_id,
                                    const std::string& text) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Chat);
    message.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::ChatProtocol::All);
    message.header.object_id = player_id;
    message.payload.assign(text.begin(), text.end());
    return message;
}

std::string parse_chat_payload(std::span<const std::uint8_t> payload) {
    std::string out;
    out.reserve(payload.size());
    for (const auto b : payload) {
        if (b == 0) break;  // null-terminated legacy chat string
        out.push_back(static_cast<char>(b));
    }
    return out;
}

std::vector<ShopItem> parse_shop_list(std::span<const std::uint8_t> payload) {
    std::vector<ShopItem> out;
    if (payload.size() < 6) return out;
    const auto count = static_cast<std::uint16_t>(
        payload[4] | (static_cast<std::uint16_t>(payload[5]) << 8));
    out.reserve(count);
    std::size_t off = 6;
    for (std::uint16_t i = 0; i < count; ++i) {
        if (off + 6 > payload.size()) break;
        ShopItem item;
        item.item_id = static_cast<std::uint16_t>(
            payload[off] | (static_cast<std::uint16_t>(payload[off + 1]) << 8));
        item.price = get_u32(payload.data() + off + 2);
        out.push_back(item);
        off += 6;
    }
    return out;
}

mxh::net::Message make_buy_message(std::uint32_t player_id,
                                   std::uint16_t item_id,
                                   std::uint16_t qty) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Item);
    message.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::ItemProtocol::BuySyn);
    message.header.object_id = player_id;
    message.payload.resize(4);
    put_u16(message.payload, 0, item_id);
    put_u16(message.payload, 2, qty);
    return message;
}

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
    info.position_x = get_u16(payload.data() + 49);
    info.position_z = get_u16(payload.data() + 51);
    return info;
}

std::optional<NpcInfo> parse_legacy_npc_add(
    std::span<const std::uint8_t> payload) {
    if (payload.size() < 64) return std::nullopt;
    NpcInfo info;
    info.npc_id = get_u32(payload.data() + 0);
    for (std::size_t i = 0; i < 17; ++i) {
        info.name[i] = static_cast<char>(payload[8 + i]);
    }
    info.npc_kind = get_u16(payload.data() + 35);
    info.position_x = get_u16(payload.data() + 45);
    info.position_z = get_u16(payload.data() + 47);
    return info;
}

bool project_npc_to_screen(float player_x, float player_z, float yaw,
                           float npc_x, float npc_z,
                           float& screen_x, float& screen_y) noexcept {
    // Camera-relative components: forward = (sin yaw, cos yaw),
    // right = (cos yaw, -sin yaw). World scale 0.001, ~800px visible
    // over ~1000 world units at the follow-camera distance.
    const float dx = npc_x - player_x;
    const float dz = npc_z - player_z;
    const float forward = dx * std::sin(yaw) + dz * std::cos(yaw);
    const float right = dx * std::cos(yaw) - dz * std::sin(yaw);
    if (forward < 0.0f) return false;  // behind the camera
    constexpr float kPixelsPerUnit = 0.8f;
    screen_x = 400.0f + right * kPixelsPerUnit;
    screen_y = 300.0f - forward * kPixelsPerUnit;
    return true;
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
    info.face_type = payload[35 + 17];
    info.hair_type = payload[35 + 18];
    for (std::size_t slot = 0; slot < info.weared_item_idx.size(); ++slot)
        info.weared_item_idx[slot] = get_u16(payload.data() + 35 + 19 + slot * 2);
    info.level    = get_u16(payload.data() + 35 + 40);
    info.map_num  = get_u16(payload.data() + 35 + 42);

    // HERO_TOTALINFO [147..206): naeryuk(+8/+12), exp(+20), money(+30).
    info.mp      = get_u32(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 8);
    info.max_mp  = get_u32(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 12);
    info.exp     = get_u32(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 20);
    info.money   = get_u32(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 30);

    info.position_x = get_u16(payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET);
    info.position_z = get_u16(payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET + 2);

    // SYSTEMTIME ServerTime at HERO_TOTAL_SERVER_TIME_OFFSET â€” 5 little-endian u16s:
    //   +0 year, +2 month, +4 wday, +6 day, +8 hour
    info.server_year  = get_u16(payload.data() + mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET + 0);
    info.server_month = get_u16(payload.data() + mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET + 2);
    // The next two bytes after month contain wday; we do not store it.
    info.server_day   = get_u16(payload.data() + mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET + 6);
    info.server_hour  = get_u16(payload.data() + mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET + 8);

    info.mugong = parse_legacy_mugong_total(payload);
    info.items  = parse_legacy_item_total(payload);

    return info;
}

std::array<MugongInfo, kMugongSlotCount>
parse_legacy_mugong_total(std::span<const std::uint8_t> payload) {
    std::array<MugongInfo, kMugongSlotCount> out{};
    constexpr std::size_t kSlotBytes = 18;
    if (payload.size() < mxh::game::HERO_TOTAL_MUGONG_OFFSET +
                             kMugongSlotCount * kSlotBytes) {
        return out;
    }
    const auto* p = payload.data() + mxh::game::HERO_TOTAL_MUGONG_OFFSET;
    for (std::size_t i = 0; i < kMugongSlotCount; ++i) {
        const std::size_t off = i * kSlotBytes;
        out[i].db_idx       = get_u32(p + off + 0);
        out[i].icon_idx     = get_u16(p + off + 4);
        out[i].position     = get_u16(p + off + 6);
        out[i].exp          = get_u32(p + off + 8);
        out[i].sung         = p[off + 12];
        out[i].wear         = p[off + 13];
        out[i].quick_position = get_u16(p + off + 14);
        out[i].option_idx   = get_u16(p + off + 16);
    }
    return out;
}

mxh::game::ItemTotalInfo
parse_legacy_item_total(std::span<const std::uint8_t> payload) {
    mxh::game::ItemTotalInfo out{};
    if (payload.size() <
        mxh::game::HERO_TOTAL_ITEM_OFFSET + sizeof(out)) {
        return out;
    }
    std::memcpy(&out, payload.data() + mxh::game::HERO_TOTAL_ITEM_OFFSET,
                sizeof(out));
    return out;
}

std::uint32_t quick_skill_for_slot(const GameInInfo& info,
                                   std::size_t slot) noexcept {
    if (slot >= kQuickSlotCount) return 0;
    if (info.mugong[slot].icon_idx != 0) {
        return info.mugong[slot].icon_idx;
    }
    // Level-1 starter set until the server sends real per-character skills.
    static constexpr std::uint32_t kStarter[kQuickSlotCount] =
        {1, 2, 3, 10, 0, 0, 0, 0};
    return kStarter[slot];
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
    m_releasing = true;
    if (m_client) {
        if (m_client->is_connected()) m_client->disconnect();
        m_client.reset();
    }
    m_info = GameInInfo{};
    m_inGame   = false;
    m_started  = false;
    m_sentGameInSyn = false;
    m_failed   = false;
    m_failureReason.clear();
    setInitialized(false);
    m_releasing = false;
}

void CInGameState::Process() {
    tick();
    update_movement(steady_now_ms());
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
                         std::uint32_t player_id, std::uint16_t map_num,
                         bool use_hsel) {
    m_pEngine = engine;
    m_host    = std::move(host);
    m_port    = port;
    m_playerId = player_id;
    m_mapNum   = map_num;
    m_useHsel = use_hsel;
    if (m_useHsel) {
        m_hsel = std::make_unique<mxh::crypto::HselStreamCipher>();
    }
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
    cfg.use_encryption     = m_useHsel;  // Phase R-1: HSEL map session
    cfg.connect_timeout    = std::chrono::milliseconds(3000);
    auto e = m_client->connect(cfg);
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("TcpClient::connect to MapServer failed: ") +
                  mxh::net::to_string(e));
    }
}

mxh::net::IEncryptor* CInGameState::encryptor_for(
    mxh::net::ConnectionId) {
    return m_hsel ? m_hsel.get() : nullptr;
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
    const auto cat   = static_cast<Category>(msg.header.category);
    const auto proto = msg.header.protocol;
    MLOG_DEBUG("CInGameState::on_message id=%llu cat=%s proto=%d obj=%u payload=%zu",
               static_cast<unsigned long long>(id.value),
               mxh::proto::category_name(cat),
               static_cast<int>(proto),
               static_cast<unsigned>(msg.header.object_id),
               msg.payload.size());

    switch (cat) {
        case Category::UserConn:
            handle_userconn_message(msg);
            break;
        case Category::Move:
            handle_move_broadcast(msg);
            break;
        case Category::Monster:
            handle_monster_broadcast(msg);
            break;
        case Category::Skill:
            handle_skill_broadcast(msg);
            break;
        case Category::Chat:
            handle_chat_broadcast(msg);
            break;
        case Category::Item:
            handle_item_broadcast(msg);
            break;
        case Category::Quest:
            handle_quest_broadcast(msg);
            break;
        default:
            // Phase 10b: MapServer may also push ITEM_TOTALINFO_LOCAL
            // (Category::Item, ItemProtocol::TotalInfoLocal) after the
            // GameInAck.  Logged and ignored until the inventory UI lands.
            MLOG_DEBUG("CInGameState: ignoring category=%s proto=%d",
                       mxh::proto::category_name(cat),
                       static_cast<int>(msg.header.protocol));
            break;
    }
}

void CInGameState::on_disconnect(mxh::net::ConnectionId id,
                                  mxh::net::NetError reason) {
    MLOG_INFO("CInGameState::on_disconnect id=%llu reason=%s",
              static_cast<unsigned long long>(id.value),
              mxh::net::to_string(reason));
    if (!m_releasing && m_inGame && !m_failed) {
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
    m_localX = static_cast<float>(info.position_x);
    m_localZ = static_cast<float>(info.position_z);
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

void CInGameState::handle_userconn_message(const mxh::net::Message& msg) {
    using mxh::proto::UserConnProtocol;
    const auto proto = static_cast<UserConnProtocol>(msg.header.protocol);
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
            MLOG_DEBUG("CInGameState: MonsterAdd object_id=%u kind=%u life=%u pos=(%u,%u) name=%.16s",
                       static_cast<unsigned>(info->object_id),
                       static_cast<unsigned>(info->monster_kind),
                       static_cast<unsigned>(info->current_life),
                       static_cast<unsigned>(info->position_x),
                       static_cast<unsigned>(info->position_z), info->name);
            break;
        }
        case UserConnProtocol::NpcAdd: {
            auto info = parse_legacy_npc_add(msg.payload);
            if (!info) {
                MLOG_WARN("CInGameState: NpcAdd payload too short (%zu bytes)",
                          msg.payload.size());
                break;
            }
            m_npcs.push_back(*info);
            MLOG_INFO("CInGameState: NpcAdd id=%u kind=%u pos=(%u,%u) name=%.16s",
                      static_cast<unsigned>(info->npc_id),
                      static_cast<unsigned>(info->npc_kind),
                      static_cast<unsigned>(info->position_x),
                      static_cast<unsigned>(info->position_z), info->name);
            break;
        }
        case UserConnProtocol::ObjectRemove: {
            if (msg.payload.size() < 4) break;
            const auto removed = get_u32(msg.payload.data());
            monsters_.erase(std::remove_if(monsters_.begin(), monsters_.end(),
                [removed](const MonsterAddInfo& m) {
                    return m.object_id == removed;
                }),
                monsters_.end());
            m_remotePlayers.erase(removed);
            MLOG_INFO("CInGameState: ObjectRemove id=%u", removed);
            break;
        }
        case UserConnProtocol::ConnectionCheckOk: {
            // Phase 10d keep-alive.  Server pushes this every ~10s
            // once you're in game; we just log.
            MLOG_DEBUG("CInGameState: ConnectionCheckOk (keep-alive)");
            break;
        }
        default:
            if (proto == static_cast<UserConnProtocol>(
                             mxh::proto::kModernHselKey)) {
                if (msg.payload.size() < sizeof(mxh::crypto::HselInit)) {
                    fail_with("HselKey payload too short");
                    break;
                }
                mxh::crypto::HselInit init{};
                std::memcpy(&init, msg.payload.data(), sizeof(init));
                if (!m_hsel || !m_hsel->import_init(init)) {
                    fail_with("HselKey import failed");
                    break;
                }
                MLOG_INFO("CInGameState: HSEL map session key imported");
                break;
            }
            MLOG_WARN("CInGameState: unhandled userconn proto=%d",
                      static_cast<int>(proto));
            break;
    }
}

void CInGameState::handle_move_broadcast(const mxh::net::Message& msg) {
    const auto pos = parse_move_payload(msg.payload);
    if (!pos) return;
    const auto object_id = msg.header.object_id;
    if (object_id == m_playerId) return;  // own echo (broadcast excludes sender)
    for (auto& monster : monsters_) {
        if (monster.object_id == object_id) {
            monster.position_x = pos->first;
            monster.position_z = pos->second;
            MLOG_DEBUG("CInGameState: monster move id=%u pos=(%u,%u)",
                       object_id, pos->first, pos->second);
            return;
        }
    }
    m_remotePlayers[object_id] = *pos;
    MLOG_DEBUG("CInGameState: remote player move id=%u pos=(%u,%u)",
               object_id, pos->first, pos->second);
}

void CInGameState::handle_monster_broadcast(const mxh::net::Message& msg) {
    const auto proto = static_cast<mxh::proto::MonsterProtocol>(
        msg.header.protocol);
    if (proto != mxh::proto::MonsterProtocol::LifeNotify) return;
    const auto life = parse_monster_life_payload(msg.payload);
    if (!life) return;
    for (auto& monster : monsters_) {
        if (monster.object_id == msg.header.object_id) {
            monster.current_life = life->first;
            monster.current_shield = life->second;
            MLOG_DEBUG("CInGameState: monster life id=%u life=%u shield=%u",
                       msg.header.object_id, life->first, life->second);
            return;
        }
    }
}

void CInGameState::handle_skill_broadcast(const mxh::net::Message& msg) {
    using mxh::proto::SkillProtocol;
    const auto proto = static_cast<SkillProtocol>(msg.header.protocol);
    switch (proto) {
        case SkillProtocol::StartAck: {
            if (msg.payload.size() >= 8) {
                const auto skill_idx = get_u32(msg.payload.data());
                const auto skill_object = get_u32(msg.payload.data() + 4);
                MLOG_INFO("CInGameState: SkillStartAck skill=%u object=%u",
                          skill_idx, skill_object);
            }
            break;
        }
        case SkillProtocol::StartNack: {
            const std::uint8_t err =
                msg.payload.empty() ? 0xFFu : msg.payload[0];
            MLOG_WARN("CInGameState: SkillStartNack error=%u",
                      static_cast<unsigned>(err));
            break;
        }
        case SkillProtocol::SingleResult: {
            // Payload: [target_id:u32][damage:i32][hit_result:u8].
            if (msg.payload.size() >= 9) {
                const auto target = get_u32(msg.payload.data());
                std::int32_t damage = 0;
                std::memcpy(&damage, msg.payload.data() + 4, sizeof(damage));
                const auto hit = msg.payload[8];
                MLOG_INFO("CInGameState: SkillSingleResult target=%u damage=%d hit=%u",
                          target, damage, static_cast<unsigned>(hit));
            }
            break;
        }
        default:
            MLOG_DEBUG("CInGameState: skill broadcast proto=%d",
                       static_cast<int>(proto));
            break;
    }
}

void CInGameState::handle_chat_broadcast(const mxh::net::Message& msg) {
    const auto text = parse_chat_payload(msg.payload);
    if (text.empty()) return;
    if (msg.header.object_id == m_playerId) return;  // own echo
    m_chatLines.push_back(text);
    if (m_chatLines.size() > 50) {
        m_chatLines.erase(m_chatLines.begin(),
                          m_chatLines.begin() +
                          static_cast<std::ptrdiff_t>(m_chatLines.size() - 50));
    }
    MLOG_INFO("CInGameState: chat from=%u: %s",
              msg.header.object_id, text.c_str());
}

void CInGameState::handle_item_broadcast(const mxh::net::Message& msg) {
    const auto proto = msg.header.protocol;
    if (proto == mxh::proto::kModernShopList) {
        m_shopItems = parse_shop_list(msg.payload);
        m_shopNpcId = 0;
        m_shopOpen = true;
        MLOG_INFO("CInGameState: shop list %zu items",
                  m_shopItems.size());
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::TotalInfoLocal)) {
        if (msg.payload.size() >= sizeof(mxh::game::ItemTotalInfo)) {
            std::memcpy(&m_info.items, msg.payload.data(),
                        sizeof(m_info.items));
            MLOG_INFO("CInGameState: inventory refreshed");
        }
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::BuyAck)) {
        m_shopOpen = false;
        MLOG_INFO("CInGameState: BuyAck (shop closed)");
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::BuyNack)) {
        MLOG_WARN("CInGameState: BuyNack");
    }
}

void CInGameState::OnKeyEvent(bool pressed, std::uint32_t vk) {
    if (vk == kVkReturn) {
        if (pressed) {
            if (m_chatOpen) {
                send_chat();
            } else {
                m_chatOpen = true;
            }
        }
        return;
    }
    if (vk == kVkEscape && pressed && m_chatOpen) {
        m_chatOpen = false;
        m_chatBuffer.clear();
        return;
    }
    if (vk == kVkEscape && pressed && m_shopOpen) {
        m_shopOpen = false;
        return;
    }
    if (vk == kVkBack && pressed && m_chatOpen) {
        if (!m_chatBuffer.empty()) m_chatBuffer.pop_back();
        return;
    }
    if (m_chatOpen) return;  // typing: consume everything else

    if (pressed) {
        if (vk >= 0x70 && vk <= 0x77) {  // F1..F8 quick slots
            use_quick_slot(static_cast<std::size_t>(vk - 0x70));
            return;
        }
        if (vk == 0x49) {  // 'I' toggles the inventory panel
            toggle_inventory();
            return;
        }
        if (vk == 0x42) {  // 'B' opens the NPC shop (npc 0 = first catalog)
            open_shop(0);
            return;
        }
        if (vk == 0x51) {  // 'Q' toggles the quest panel
            m_questOpen = !m_questOpen;
            return;
        }
        if (vk == 0x4A && m_questOpen) {  // 'J' accepts the selected quest
            send_quest(mxh::proto::QuestProtocol::StartSyn);
            return;
        }
        if (vk == 0x4B && m_questOpen) {  // 'K' claims the selected quest
            send_quest(mxh::proto::QuestProtocol::EndSyn);
            return;
        }
    }

    const std::uint32_t mask = key_mask_for_vk(vk);
    if (mask == 0) return;
    if (pressed) {
        m_keyMask |= mask;
    } else {
        m_keyMask &= ~mask;
    }
}

void CInGameState::send_quest(mxh::proto::QuestProtocol protocol) {
    if (!m_inGame || !m_client || !m_client->is_connected()) return;
    const auto result = m_client->send(make_quest_message(m_playerId, protocol, m_questId));
    if (result == mxh::net::NetError::Ok) {
        m_questStatus = protocol == mxh::proto::QuestProtocol::StartSyn
                          ? "Accepting..." : "Claiming...";
    }
}

void CInGameState::handle_quest_broadcast(const mxh::net::Message& msg) {
    if (msg.payload.size() >= 2) {
        m_questId = static_cast<std::uint16_t>(msg.payload[0] |
                    (static_cast<std::uint16_t>(msg.payload[1]) << 8));
    }
    const auto protocol = static_cast<mxh::proto::QuestProtocol>(msg.header.protocol);
    switch (protocol) {
        case mxh::proto::QuestProtocol::StartAck: m_questStatus = "Active - hunt monsters"; break;
        case mxh::proto::QuestProtocol::StartNack: m_questStatus = "Cannot accept"; break;
        case mxh::proto::QuestProtocol::EndAck: m_questStatus = "Reward claimed"; break;
        case mxh::proto::QuestProtocol::EndNack: m_questStatus = "Not complete"; break;
        default: break;
    }
    MLOG_INFO("CInGameState: quest id=%u status=%s", m_questId, m_questStatus.c_str());
}

void CInGameState::OnChar(std::uint32_t ch) {
    if (!m_chatOpen) return;
    if (ch < 0x20 || ch == 0x7F) return;
    if (m_chatBuffer.size() >= 200) return;
    m_chatBuffer.push_back(static_cast<char>(ch & 0xFFu));
}

void CInGameState::OnMouseButton(bool left, bool down,
                                 std::int32_t x, std::int32_t y) {
    m_lastMouseX = x;
    m_lastMouseY = y;
    if (left && down && m_shopOpen) {
        const float fx = static_cast<float>(x);
        const float fy = static_cast<float>(y);
        if (fx >= kShopPanelX && fx <= kShopPanelX + kShopPanelW &&
            fy >= kShopPanelY) {
            const std::size_t row = static_cast<std::size_t>(
                (fy - kShopPanelY) / kShopRowH);
            if (row < m_shopItems.size()) {
                buy_shop_item(row);
                return;
            }
        }
    }
    if (left && down && !m_shopOpen) {
        // Click an NPC marker before falling back to attack.
        const float fx = static_cast<float>(x);
        const float fy = static_cast<float>(y);
        const std::uint32_t npc = pick_npc_at_screen(fx, fy);
        if (npc != 0) {
            open_shop(npc);
            return;
        }
    }
    if (left && down) {
        try_attack();
        return;
    }
    if (!left) m_cameraDrag = down;
}

void CInGameState::OnMouseMove(std::int32_t x, std::int32_t y) {
    if (m_cameraDrag) {
        m_cameraYaw += static_cast<float>(x - m_lastMouseX) * 0.01f;
    }
    m_lastMouseX = x;
    m_lastMouseY = y;
}

void CInGameState::update_movement(std::uint64_t now_ms) {
    if (!m_inGame) return;
    float dt = 0.016f;
    if (m_lastTickMs != 0) {
        dt = std::min(0.05f,
                      static_cast<float>(now_ms - m_lastTickMs) / 1000.0f);
    }
    m_lastTickMs = now_ms;

    const auto step = step_movement(m_keyMask, m_cameraYaw,
                                    m_localX, m_localZ, dt);
    m_cameraYaw = step.yaw;
    m_localX = step.x;
    m_localZ = step.z;

    if (step.moving) {
        m_info.position_x = static_cast<std::uint16_t>(m_localX);
        m_info.position_z = static_cast<std::uint16_t>(m_localZ);
        m_moving = true;
        if (now_ms - m_lastMoveSendMs >=
            static_cast<std::uint64_t>(kMoveReportEveryMs)) {
            send_move(static_cast<std::uint16_t>(m_localX),
                      static_cast<std::uint16_t>(m_localZ),
                      mxh::proto::MoveProtocol::OneTarget);
            m_lastMoveSendMs = now_ms;
        }
    } else if (m_moving) {
        m_moving = false;
        send_move(static_cast<std::uint16_t>(m_localX),
                  static_cast<std::uint16_t>(m_localZ),
                  mxh::proto::MoveProtocol::Stop);
    }
}

void CInGameState::send_move(std::uint16_t x, std::uint16_t z,
                             mxh::proto::MoveProtocol proto) {
    if (!m_client || !m_client->is_connected()) return;
    const auto e = m_client->send(
        make_move_message(m_playerId, proto, x, z));
    if (e != mxh::net::NetError::Ok) {
        MLOG_DEBUG("CInGameState: send_move proto=%d failed: %s",
                   static_cast<int>(proto), mxh::net::to_string(e));
    } else {
        MLOG_DEBUG("CInGameState: send_move proto=%d pos=(%u,%u)",
                   static_cast<int>(proto), x, z);
    }
}

void CInGameState::try_attack() {
    const auto now = steady_now_ms();
    if (!m_inGame || !m_client || !m_client->is_connected()) return;
    if (now - m_lastAttackMs <
        static_cast<std::uint64_t>(kAttackCooldownMs)) {
        return;
    }
    const auto target = pick_attack_target(
        monsters_, m_localX, m_localZ, kAttackRange);
    if (!target) return;

    float target_x = 0;
    float target_z = 0;
    for (const auto& monster : monsters_) {
        if (monster.object_id == *target) {
            target_x = static_cast<float>(monster.position_x);
            target_z = static_cast<float>(monster.position_z);
            break;
        }
    }
    const auto e = m_client->send(
        make_attack_message(m_playerId, 1u, *target, target_x, target_z));
    if (e == mxh::net::NetError::Ok) {
        m_lastAttackMs = now;
        MLOG_INFO("CInGameState: attack target=%u pos=(%.0f,%.0f)",
                  *target, target_x, target_z);
    }
}

std::uint32_t CInGameState::pick_npc_at_screen(float sx, float sy) const {
    std::uint32_t best = 0;
    float best_d2 = 18.0f * 18.0f;
    for (const auto& npc : m_npcs) {
        float px = 0;
        float py = 0;
        if (!project_npc_to_screen(
                m_localX, m_localZ, m_cameraYaw,
                static_cast<float>(npc.position_x),
                static_cast<float>(npc.position_z), px, py)) {
            continue;
        }
        const float dx = px - sx;
        const float dy = py - sy;
        const float d2 = dx * dx + dy * dy;
        if (d2 <= best_d2) {
            best_d2 = d2;
            best = npc.npc_id;
        }
    }
    return best;
}

void CInGameState::use_quick_slot(std::size_t slot) {
    if (!m_inGame || !m_client || !m_client->is_connected()) return;
    const auto skill = quick_skill_for_slot(m_info, slot);
    if (skill == 0) return;
    const auto now = steady_now_ms();
    if (now - m_lastAttackMs <
        static_cast<std::uint64_t>(kAttackCooldownMs)) {
        return;
    }

    std::uint32_t target = 0;
    float target_x = 0;
    float target_z = 0;
    if (skill == 3u) {  // Heal: self-cast
        target = m_playerId;
        target_x = m_localX;
        target_z = m_localZ;
    } else {
        const auto t = pick_attack_target(
            monsters_, m_localX, m_localZ, kAttackRange);
        if (!t) return;
        target = *t;
        for (const auto& monster : monsters_) {
            if (monster.object_id == *t) {
                target_x = static_cast<float>(monster.position_x);
                target_z = static_cast<float>(monster.position_z);
                break;
            }
        }
    }

    const auto e = m_client->send(
        make_attack_message(m_playerId, skill, target, target_x, target_z));
    if (e == mxh::net::NetError::Ok) {
        m_lastAttackMs = now;
        MLOG_INFO("CInGameState: quick slot %zu skill=%u target=%u",
                  slot, skill, target);
    }
}

void CInGameState::open_shop(std::uint32_t npc_id) {
    if (!m_inGame || !m_client || !m_client->is_connected()) return;
    m_shopOpen = false;
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Npc);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::NpcProtocol::SpeechSyn);
    msg.header.object_id = m_playerId;
    msg.payload.resize(4);
    put_u32(msg.payload, 0, npc_id);
    const auto e = m_client->send(msg);
    if (e == mxh::net::NetError::Ok) {
        MLOG_INFO("CInGameState: open_shop npc=%u", npc_id);
    }
}

void CInGameState::buy_shop_item(std::size_t index) {
    if (!m_inGame || !m_client || !m_client->is_connected()) return;
    if (index >= m_shopItems.size()) return;
    const auto& item = m_shopItems[index];
    const auto e = m_client->send(
        make_buy_message(m_playerId, item.item_id, 1u));
    if (e == mxh::net::NetError::Ok) {
        MLOG_INFO("CInGameState: buy item=%u price=%u",
                  item.item_id, item.price);
    }
}

void CInGameState::send_chat() {
    const auto text = m_chatBuffer;
    m_chatBuffer.clear();
    m_chatOpen = false;
    if (text.empty()) return;
    if (!m_client || !m_client->is_connected()) return;
    const auto e = m_client->send(make_chat_message(m_playerId, text));
    if (e != mxh::net::NetError::Ok) {
        MLOG_WARN("CInGameState: send_chat failed: %s",
                  mxh::net::to_string(e));
        return;
    }
    m_chatLines.push_back(text);
    if (m_chatLines.size() > 50) {
        m_chatLines.erase(m_chatLines.begin(),
                          m_chatLines.begin() +
                          static_cast<std::ptrdiff_t>(m_chatLines.size() - 50));
    }
    MLOG_INFO("CInGameState: chat sent: %s", text.c_str());
}

void CInGameState::fail_with(const std::string& reason) {
    if (m_failed) return;
    m_failed = true;
    m_failureReason = reason;
    MLOG_ERROR("CInGameState: %s", reason.c_str());
}

} // namespace mxh::client
