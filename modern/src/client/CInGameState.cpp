// mxh/client/CInGameState.cpp
// Phase B.2.3 â€” in-game state implementation.

#include "CInGameState.hpp"
#include "CEngine.hpp"
#include "CMainGame.hpp"
#include "cinventoryexdialog.hpp"

#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <utility>
#include <optional>

#include "mxh/log/mlog.hpp"
#include "mxh/game/hero_total_layout.hpp"
#include "mxh/game/npc_role.hpp"
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
constexpr std::array<std::string_view, 13> kChinaGameInUiScripts{
    "15.bin", "51.bin", "24.bin", "10.bin", "11.bin", "23.bin",
    "19.bin", "22.bin", "31.bin", "14.bin", "17.bin",
    "QuestTotal.bin", "ItemShop.bin"};
constexpr std::string_view kInventoryDialogId = "IN_INVENTORYDLG";
constexpr std::string_view kQuestDialogId = "QUE_TOTALDLG";
constexpr std::string_view kItemShopDialogId = "ITMALL_BASEDLG";
constexpr std::array<std::string_view, 4> kDefaultHudDialogIds{
    "MI_MAINDLG", "QI_QUICKDLG", "MNM_DIALOG", "CG_GUAGEDLG"};

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
                         float x, float z, float dt,
                         float max_x, float max_z) noexcept {
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
        result.x = std::clamp(x + dx * kMoveSpeed * dt, 0.0f,
                              std::max(0.0f, max_x));
        result.z = std::clamp(z + dz * kMoveSpeed * dt, 0.0f,
                              std::max(0.0f, max_z));
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

mxh::net::Message make_pickup_message(std::uint32_t player_id,
                                      std::uint32_t drop_object_id) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Item);
    message.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::ItemProtocol::PickupSyn);
    message.header.object_id = player_id;
    message.payload.resize(4);
    put_u32(message.payload, 0, drop_object_id);
    return message;
}

std::optional<GroundDropInfo>
parse_legacy_ground_drop(std::span<const std::uint8_t> payload) {
    if (payload.size() < 20) return std::nullopt;
    GroundDropInfo drop;
    drop.object_id = get_u32(payload.data());
    drop.source_monster_id = get_u32(payload.data() + 4);
    drop.item_id = get_u16(payload.data() + 8);
    drop.count = get_u16(payload.data() + 10);
    std::memcpy(&drop.position_x, payload.data() + 12, sizeof(float));
    std::memcpy(&drop.position_z, payload.data() + 16, sizeof(float));
    if (drop.object_id == 0 || drop.item_id == 0) return std::nullopt;
    return drop;
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

std::optional<RemotePlayerInfo>
parse_legacy_character_add(std::span<const std::uint8_t> payload) {
    constexpr std::size_t kBaseObjectSize = 35;
    constexpr std::size_t kCharacterTotalSize = 112;
    constexpr std::size_t kMoveInfoSize = 14;
    constexpr std::size_t kRequiredSize =
        kBaseObjectSize + kCharacterTotalSize + kMoveInfoSize;
    if (payload.size() < kRequiredSize) return std::nullopt;

    RemotePlayerInfo info;
    info.object_id = get_u32(payload.data());
    info.user_id = get_u32(payload.data() + 4);
    const auto* name = reinterpret_cast<const char*>(payload.data() + 8);
    const auto nameLength = std::find(name, name + 17, '\0') - name;
    info.name.assign(name, static_cast<std::size_t>(nameLength));

    const auto characterOffset = kBaseObjectSize;
    info.life = get_u32(payload.data() + characterOffset);
    info.max_life = get_u32(payload.data() + characterOffset + 4);
    info.shield = get_u32(payload.data() + characterOffset + 8);
    info.max_shield = get_u32(payload.data() + characterOffset + 12);
    info.gender = payload[characterOffset + 16];
    info.face_type = payload[characterOffset + 17];
    info.hair_type = payload[characterOffset + 18];
    for (std::size_t slot = 0; slot < info.weared_item_idx.size(); ++slot) {
        info.weared_item_idx[slot] =
            get_u16(payload.data() + characterOffset + 19 + slot * 2);
    }
    info.level = get_u16(payload.data() + characterOffset + 40);
    info.map_num = get_u16(payload.data() + characterOffset + 42);
    info.visible = payload[characterOffset + 60] != 0;
    std::memcpy(&info.height, payload.data() + characterOffset + 70,
                sizeof(info.height));
    std::memcpy(&info.width, payload.data() + characterOffset + 74,
                sizeof(info.width));

    const auto moveOffset = kBaseObjectSize + kCharacterTotalSize;
    info.position_x = get_u16(payload.data() + moveOffset);
    info.position_z = get_u16(payload.data() + moveOffset + 2);
    info.appearance_known = true;
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

CInGameState::CInGameState()
    : m_inventoryService(std::make_unique<mxh::services::InventoryServiceImpl>(m_info.items)) {}

CInGameState::~CInGameState() = default;

void CInGameState::Init(void* pInitParam) {
    MLOG_DEBUG("CInGameState::Init (waiting for Start() from host)");
    setInitialized(true);

    // Load the GameIn InterfaceScript tree on Init so the state always
    // owns its dialog tree, even before the host calls Start(). The
    // pInitParam may carry the engine (production path) or be nullptr
    // (unit tests). Fall back to MXH_PLAYDH_ROOT so headless tests
    // don't need a fully wired CEngine.
    std::optional<std::filesystem::path> playdh;
    if (auto* engine = static_cast<CEngine*>(pInitParam)) {
        playdh = engine->playdh_root();
    }
    if (!playdh.has_value()) {
        if (const char* env = std::getenv("MXH_PLAYDH_ROOT")) {
            playdh = std::filesystem::path(env);
        }
    }
    if (!playdh.has_value()) {
        for (const auto& candidate : {
                 std::filesystem::path("modern/data/PlayDH"),
                 std::filesystem::path("data/PlayDH"),
                 std::filesystem::path("../data/PlayDH"),
                 std::filesystem::path("../../data/PlayDH"),
                 std::filesystem::path("../../../../data/PlayDH"),
                 std::filesystem::path("C:/moxiang/modern/data/PlayDH"),
                 std::filesystem::path("C:/moxiang/墨香【源码配套资源】/PlayDH")}) {
            if (std::filesystem::exists(candidate / "Image" / "InterfaceScript" /
                                          "15.bin")) {
                playdh = std::filesystem::absolute(candidate);
                break;
            }
        }
    }
    if (playdh.has_value() && m_uiRuntime.empty()) {
        std::string ui_error;
        if (!m_uiRuntime.loadMany(*playdh, kChinaGameInUiScripts,
                                  m_pEngine
                                      ? m_pEngine->ui_resolution_mode()
                                      : mxh::ui::ResolutionMode::Low800x600,
                                  &ui_error)) {
            MLOG_WARN("CInGameState::Init UI load failed: %s",
                      ui_error.c_str());
        } else {
            m_uiRuntime.applyActiveSet(kDefaultHudDialogIds);
        }
    }
}

void CInGameState::Release() {
    MLOG_DEBUG("CInGameState::Release");
    m_releasing = true;
    m_info = GameInInfo{};
    m_inGame   = false;
    m_started  = false;
    m_sentGameInSyn = false;
    m_failed   = false;
    m_failureReason.clear();
    m_uiRuntime.clear();
    m_keyMask = 0;
    m_moving = false;
    m_cameraDrag = false;
    m_chatOpen = false;
    m_chatBuffer.clear();
    m_effectEvents.clear();
    set_inventory_open(false);
    set_shop_open(false);
    set_quest_open(false);
    setInitialized(false);
    m_releasing = false;
}

void CInGameState::Process() {
    tick();
    if (m_pEngine) {
        for (auto& event : m_pEngine->agent_session().events().drain()) {
            switch (event.kind) {
                case ClientRuntimeEventKind::Connected:
                    on_connect(event.connection, event.detail);
                    break;
                case ClientRuntimeEventKind::Message:
                    on_message(event.connection, event.message);
                    break;
                case ClientRuntimeEventKind::Disconnected:
                    on_disconnect(event.connection, event.network_error);
                    break;
                case ClientRuntimeEventKind::Error:
                    fail_with(event.detail);
                    break;
            }
        }
    }
    update_movement(steady_now_ms());
    if (is_connected() && !m_sentGameInSyn) {
        send_gamein_syn();
    }
}

void CInGameState::Start(CEngine* engine, std::string host,
                         std::uint16_t port,
                         std::uint32_t player_id, std::uint16_t map_num,
                         bool use_hsel) {
    (void)host;
    (void)port;
    (void)use_hsel;
    Start(engine, player_id, map_num);
}

void CInGameState::Start(CEngine* engine, std::uint32_t player_id,
                          std::uint16_t map_num) {
    m_pEngine = engine;
    m_playerId = player_id;
    m_mapNum = map_num;
    if (m_started) return;
    m_started = true;
    if (!m_pEngine || !m_pEngine->playdh_root().has_value()) {
        fail_with("GameIn requires an explicit PlayDH resource root");
        return;
    }
    if (m_uiRuntime.empty()) {
        std::string ui_error;
        if (!m_uiRuntime.loadMany(*m_pEngine->playdh_root(),
                                  kChinaGameInUiScripts,
                                  m_pEngine->ui_resolution_mode(),
                                  &ui_error)) {
            fail_with("GameIn UI load failed: " + ui_error);
            return;
        }
    }
    m_uiRuntime.applyActiveSet(kDefaultHudDialogIds);
    MLOG_INFO("CInGameState using persistent AgentSession (player_id=%u, map=%u)",
              static_cast<unsigned>(m_playerId),
              static_cast<unsigned>(m_mapNum));
    if (!m_pEngine || !m_pEngine->agent_session().is_connected()) {
        fail_with("GameIn requires a connected AgentSession");
        return;
    }
    send_gamein_syn();
}

mxh::net::IEncryptor* CInGameState::encryptor_for(
    mxh::net::ConnectionId) {
    return m_hsel ? m_hsel.get() : nullptr;
}

bool CInGameState::is_connected() const noexcept {
    return m_pEngine && m_pEngine->agent_session().is_connected();
}

void CInGameState::set_quest_catalog(mxh::compat::QuestStringCatalog catalog) {
    m_questCatalog = std::move(catalog);
    m_mainQuests = m_questCatalog.main_quests();
    m_questSelection = 0;
    if (!m_mainQuests.empty()) m_questId = m_mainQuests.front()->quest_id;
}

bool CInGameState::on_connect(mxh::net::ConnectionId id,
                              const std::string& remote_addr) {
    MLOG_INFO("CInGameState::on_connect id=%llu from %s",
              static_cast<unsigned long long>(id.value),
              remote_addr.c_str());
    (void)id;
    (void)remote_addr;
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
    const auto e = m_pEngine->agent_session().send(out);
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
        case UserConnProtocol::CharacterAdd: {
            auto info = parse_legacy_character_add(msg.payload);
            if (!info) {
                MLOG_WARN("CInGameState: CharacterAdd payload too short (%zu bytes)",
                          msg.payload.size());
                break;
            }
            if (info->object_id == m_playerId) break;
            const auto objectId = info->object_id;
            m_remotePlayers.insert_or_assign(objectId, std::move(*info));
            const auto& player = m_remotePlayers.at(objectId);
            MLOG_INFO("CInGameState: CharacterAdd id=%u gender=%u pos=(%u,%u) name=%s",
                      static_cast<unsigned>(player.object_id),
                      static_cast<unsigned>(player.gender),
                      static_cast<unsigned>(player.position_x),
                      static_cast<unsigned>(player.position_z),
                      player.name.c_str());
            break;
        }
        case UserConnProtocol::MonsterAdd: {
            auto info = parse_legacy_monster_add(msg.payload);
            if (!info) {
                MLOG_WARN("CInGameState: MonsterAdd payload too short (%zu bytes)",
                          msg.payload.size());
                break;
            }
            const auto existing = std::find_if(
                monsters_.begin(), monsters_.end(),
                [&](const MonsterAddInfo& monster) {
                    return monster.object_id == info->object_id;
                });
            if (existing == monsters_.end()) monsters_.push_back(*info);
            else *existing = *info;
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
            const auto existing = std::find_if(
                m_npcs.begin(), m_npcs.end(),
                [&](const NpcInfo& npc) {
                    return npc.npc_id == info->npc_id;
                });
            if (existing == m_npcs.end()) m_npcs.push_back(*info);
            else *existing = *info;
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
            m_npcs.erase(std::remove_if(m_npcs.begin(), m_npcs.end(),
                [removed](const NpcInfo& npc) {
                    return npc.npc_id == removed;
                }),
                m_npcs.end());
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
        case UserConnProtocol::ChangeMapAck:
            if (msg.payload.size() >= sizeof(std::uint16_t) && m_pEngine) {
                const auto target_map = get_u16(msg.payload.data());
                if (target_map != 0u && target_map != m_mapNum) {
                    m_pEngine->SetPendingTransfer(
                        GameEntryRequest{m_playerId, target_map});
                    m_pEngine->RequestStateChange(
                        static_cast<int>(GameStateId::GameLoading));
                    MLOG_INFO("CInGameState: ChangeMapAck map=%u -> GameLoading",
                              static_cast<unsigned>(target_map));
                }
            }
            break;
        case UserConnProtocol::ChangeMapNack:
            MLOG_WARN("CInGameState: ChangeMapNack; current scene remains active");
            break;
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
    const bool moving = msg.header.protocol != static_cast<std::uint8_t>(
        mxh::proto::MoveProtocol::Stop);
    if (object_id == m_playerId) return;  // own echo (broadcast excludes sender)
    for (auto& monster : monsters_) {
        if (monster.object_id == object_id) {
            const float dx = static_cast<float>(pos->first) - monster.position_x;
            const float dz = static_cast<float>(pos->second) - monster.position_z;
            if (dx != 0.0f || dz != 0.0f) {
                monster.facing_yaw = std::atan2(dx, dz);
            }
            monster.position_x = pos->first;
            monster.position_z = pos->second;
            monster.moving = moving;
            MLOG_DEBUG("CInGameState: monster move id=%u pos=(%u,%u)",
                       object_id, pos->first, pos->second);
            return;
        }
    }
    auto [it, inserted] = m_remotePlayers.try_emplace(object_id);
    auto& player = it->second;
    if (inserted) player.object_id = object_id;
    const float dx = static_cast<float>(pos->first) - player.position_x;
    const float dz = static_cast<float>(pos->second) - player.position_z;
    if (dx != 0.0f || dz != 0.0f) {
        player.facing_yaw = std::atan2(dx, dz);
    }
    player.position_x = pos->first;
    player.position_z = pos->second;
    player.moving = moving;
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
            if (monster.current_life == 0) {
                monster.moving = false;
                push_effect_event(EffectEvent{
                    EffectEventKind::Death, m_lastTickMs, m_playerId,
                    monster.object_id, 0, 0, 0, 0, 0});
            }
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
                push_effect_event(EffectEvent{
                    EffectEventKind::CastRelease, m_lastTickMs, m_playerId,
                    skill_object, skill_idx, skill_idx, 0, 0, 0});
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
                push_effect_event(EffectEvent{
                    EffectEventKind::Hit, m_lastTickMs, m_playerId, target,
                    0, 0, 0, damage, hit});
                push_effect_event(EffectEvent{
                    EffectEventKind::End, m_lastTickMs, m_playerId, target,
                    0, 0, 0, damage, hit});
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

void CInGameState::push_effect_event(EffectEvent event) noexcept {
    if (event.timestamp_ms == 0) event.timestamp_ms = m_lastTickMs;
    m_effectEvents.push_back(event);
    constexpr std::size_t kMaxEffectEvents = 256;
    if (m_effectEvents.size() > kMaxEffectEvents) {
        m_effectEvents.erase(m_effectEvents.begin(),
                             m_effectEvents.begin() +
                                 (m_effectEvents.size() - kMaxEffectEvents));
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

void CInGameState::try_pickup() {
    if (!m_inGame) return;
    const auto drop_id = pick_nearest_drop();
    if (drop_id == 0) return;
    if (!is_connected()) return;
    const auto e = m_pEngine->agent_session().send(
        make_pickup_message(m_playerId, drop_id));
    if (e == mxh::net::NetError::Ok) {
        MLOG_INFO("CInGameState: pickup drop=%u", drop_id);
    }
}

std::uint32_t CInGameState::pick_nearest_drop() const noexcept {
    std::uint32_t best = 0;
    float best_d2 = kPickupRange * kPickupRange;
    for (const auto& drop : m_groundDrops) {
        const float dx = drop.position_x - m_localX;
        const float dz = drop.position_z - m_localZ;
        const float d2 = dx * dx + dz * dz;
        if (d2 <= best_d2) {
            best_d2 = d2;
            best = drop.object_id;
        }
    }
    return best;
}

std::uint32_t CInGameState::pick_drop_at_screen(float sx, float sy) const {
    std::uint32_t best = 0;
    float best_d2 = 18.0f * 18.0f;
    for (const auto& drop : m_groundDrops) {
        float px = 0;
        float py = 0;
        if (!project_npc_to_screen(
                m_localX, m_localZ, m_cameraYaw,
                drop.position_x, drop.position_z, px, py)) {
            continue;
        }
        const float dx = px - sx;
        const float dy = py - sy;
        const float d2 = dx * dx + dy * dy;
        if (d2 <= best_d2) {
            best_d2 = d2;
            best = drop.object_id;
        }
    }
    return best;
}

bool CInGameState::apply_pickup_to_inventory(std::uint32_t drop_id,
                                             std::uint16_t item_id,
                                             std::uint16_t count) noexcept {
    if (drop_id == 0u || item_id == 0u || count == 0u) return false;
    for (std::uint16_t slot = 0;
         slot < mxh::game::SLOT_INVENTORY_NUM; ++slot) {
        auto& item = m_info.items.Inventory[slot];
        if (!mxh::game::is_empty_slot(item)) continue;
        item = mxh::game::make_item(drop_id, item_id, slot, 100u, count);
        return true;
    }
    return false;
}

void CInGameState::handle_item_broadcast(const mxh::net::Message& msg) {
    const auto proto = msg.header.protocol;
    if (proto == static_cast<std::uint8_t>(
            mxh::proto::ItemProtocol::MonsterObtainNotify)) {
        auto drop = parse_legacy_ground_drop(msg.payload);
        if (!drop) {
            MLOG_WARN("CInGameState: MonsterObtainNotify payload too short (%zu)",
                      msg.payload.size());
            return;
        }
        const auto existing = std::find_if(
            m_groundDrops.begin(), m_groundDrops.end(),
            [&](const GroundDropInfo& candidate) {
                return candidate.object_id == drop->object_id;
            });
        if (existing == m_groundDrops.end()) m_groundDrops.push_back(*drop);
        else *existing = *drop;
        MLOG_INFO("CInGameState: ground drop id=%u item=%u pos=(%.0f,%.0f)",
                  drop->object_id, drop->item_id,
                  drop->position_x, drop->position_z);
        return;
    }
    if (proto == static_cast<std::uint8_t>(
            mxh::proto::ItemProtocol::PickupAck)) {
        if (msg.payload.size() >= 4) {
            const auto drop_id = get_u32(msg.payload.data());
            m_groundDrops.erase(
                std::remove_if(m_groundDrops.begin(), m_groundDrops.end(),
                    [drop_id](const GroundDropInfo& drop) {
                        return drop.object_id == drop_id;
                    }),
                m_groundDrops.end());
            const auto item_id = msg.payload.size() >= 6
                ? get_u16(msg.payload.data() + 4) : 0u;
            const auto count = msg.payload.size() >= 8
                ? get_u16(msg.payload.data() + 6) : 0u;
            const bool inventory_updated =
                apply_pickup_to_inventory(drop_id, item_id, count);
            MLOG_INFO("CInGameState: picked up drop=%u item=%u count=%u inventory=%s",
                      drop_id, item_id, count,
                      inventory_updated ? "updated" : "full-or-invalid");
            if (inventory_updated && m_pEngine) {
                m_pEngine->EmitAudio(CEngine::AudioCue::Pickup);
            }
        }
        return;
    }
    if (proto == static_cast<std::uint8_t>(
            mxh::proto::ItemProtocol::PickupNack)) {
        MLOG_WARN("CInGameState: PickupNack");
        return;
    }
    if (proto == mxh::proto::kModernShopList) {
        m_shopItems = parse_shop_list(msg.payload);
        m_shopNpcId = 0;
        if (msg.payload.size() >= 4) {
            m_shopNpcId = get_u32(msg.payload.data());
        }
        set_shop_open(true);
        MLOG_INFO("CInGameState: shop list %zu items npc=%u",
                  m_shopItems.size(), m_shopNpcId);
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::Money)) {
        if (msg.payload.size() < sizeof(std::uint32_t)) return;
        m_info.money = get_u32(msg.payload.data());
        if (m_inventoryOpen) set_inventory_open(true);
        MLOG_INFO("CInGameState: money updated=%u", m_info.money);
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::UseAck)) {
        // UseAck: [pos:u16][icon:u16][hp_delta:i32][mp_delta:i32]
        //          [current_hp:u32][current_mp:u32].
        if (msg.payload.size() < 20) return;
        const auto pos = get_u16(msg.payload.data());
        const auto current_hp = get_u32(msg.payload.data() + 12);
        const auto current_mp = get_u32(msg.payload.data() + 16);
        if (pos < mxh::game::SLOT_INVENTORY_NUM) {
            m_info.items.Inventory[pos] = mxh::game::make_empty_item();
            m_info.items.Inventory[pos].Position = pos;
        }
        m_info.life = static_cast<std::uint16_t>(
            std::min<std::uint32_t>(current_hp, 0xffffu));
        m_info.mp = current_mp;
        if (m_inventoryOpen) set_inventory_open(true);
        MLOG_INFO("CInGameState: item used pos=%u hp=%u mp=%u",
                  pos, current_hp, current_mp);
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::DiscardAck)) {
        if (msg.payload.size() < sizeof(std::uint16_t)) return;
        const auto pos = get_u16(msg.payload.data());
        if (pos >= mxh::game::SLOT_INVENTORY_NUM) return;
        m_info.items.Inventory[pos] = mxh::game::make_empty_item();
        m_info.items.Inventory[pos].Position = pos;
        if (m_inventoryOpen) set_inventory_open(true);
        MLOG_INFO("CInGameState: item discarded pos=%u", pos);
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::MoveAck)) {
        // MoveAck echoes ITEMBASE(22B) + target position(u16).
        if (msg.payload.size() < 24) return;
        const auto db_idx = get_u32(msg.payload.data());
        const auto target = get_u16(msg.payload.data() + 22);
        if (db_idx == 0u || target >= mxh::game::SLOT_INVENTORY_NUM) return;
        std::size_t source = mxh::game::SLOT_INVENTORY_NUM;
        for (std::size_t i = 0; i < mxh::game::SLOT_INVENTORY_NUM; ++i) {
            if (m_info.items.Inventory[i].dwDBIdx == db_idx) {
                source = i;
                break;
            }
        }
        if (source == mxh::game::SLOT_INVENTORY_NUM) return;
        std::swap(m_info.items.Inventory[source], m_info.items.Inventory[target]);
        m_info.items.Inventory[source].Position = static_cast<std::uint16_t>(source);
        m_info.items.Inventory[target].Position = target;
        if (m_inventoryOpen) set_inventory_open(true);
        MLOG_INFO("CInGameState: item moved db=%u source=%zu target=%u",
                  db_idx, source, target);
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::SellAck)) {
        // SellAck echoes [target_pos][item_idx][item_num][dealer_idx].
        if (msg.payload.size() < 8) return;
        const auto pos = get_u16(msg.payload.data());
        const auto item_idx = get_u16(msg.payload.data() + 2);
        const auto quantity = get_u16(msg.payload.data() + 4);
        if (pos >= mxh::game::SLOT_INVENTORY_NUM || quantity == 0u) return;
        auto& item = m_info.items.Inventory[pos];
        if (item.wIconIdx != item_idx || item.ItemParam < quantity) return;
        if (item.ItemParam == quantity) {
            item = mxh::game::make_empty_item();
            item.Position = pos;
        } else {
            item.ItemParam -= quantity;
        }
        if (m_inventoryOpen) set_inventory_open(true);
        MLOG_INFO("CInGameState: item sold pos=%u item=%u qty=%u",
                  pos, item_idx, quantity);
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::TotalInfoLocal)) {
        if (msg.payload.size() >= sizeof(mxh::game::ItemTotalInfo)) {
            std::memcpy(&m_info.items, msg.payload.data(),
                        sizeof(m_info.items));
            MLOG_INFO("CInGameState: inventory refreshed");
        }
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::BuyAck)) {
        set_shop_open(false);
        MLOG_INFO("CInGameState: BuyAck (shop closed)");
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::BuyNack)) {
        MLOG_WARN("CInGameState: BuyNack");
    }
}

void CInGameState::set_inventory_open(bool open) noexcept {
    m_inventoryOpen = open;
    m_uiRuntime.setDialogActive(kInventoryDialogId, open);
    if (!open || !m_inventoryService) return;
    for (auto& dialog : m_uiRuntime.dialogsMutable()) {
        if (!dialog || dialog->legacyId() != kInventoryDialogId) continue;
        auto* inventory = dynamic_cast<mxh::ui::cInventoryExDialog*>(dialog.get());
        if (!inventory) continue;
        inventory->SetInventoryService(m_inventoryService.get());
        inventory->SetMoney(m_info.money);
        inventory->RefreshFromInventoryService();
        break;
    }
}

void CInGameState::toggle_inventory() noexcept {
    set_inventory_open(!m_inventoryOpen);
}

void CInGameState::set_shop_open(bool open) noexcept {
    m_shopOpen = open;
    m_uiRuntime.setDialogActive(kItemShopDialogId, open);
}

void CInGameState::set_quest_open(bool open) noexcept {
    m_questOpen = open;
    m_uiRuntime.setDialogActive(kQuestDialogId, open);
}

bool CInGameState::handle_ui_activation(
    const ClientUiActivation& activation) {
    if (activation.legacy_id != "CMI_CLOSEBTN") return false;
    if (activation.dialog_legacy_id == kInventoryDialogId) {
        set_inventory_open(false);
        return true;
    }
    if (activation.dialog_legacy_id == kQuestDialogId) {
        set_quest_open(false);
        return true;
    }
    if (activation.dialog_legacy_id == kItemShopDialogId) {
        set_shop_open(false);
        return true;
    }
    return false;
}

void CInGameState::OnKeyEvent(bool pressed, std::uint32_t vk) {
    const std::uint32_t move_mask = key_mask_for_vk(vk);
    if (move_mask != 0 && !m_chatOpen) {
        // HUD widgets must not swallow WASD/QE. Original Q/E is strafe.
        if (pressed) m_keyMask |= move_mask;
        else m_keyMask &= ~move_mask;
        return;
    }
    if (m_uiRuntime.onKey(pressed, static_cast<std::int32_t>(vk))) return;
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
        set_shop_open(false);
        return;
    }
    if (vk == kVkEscape && pressed && m_inventoryOpen) {
        set_inventory_open(false);
        return;
    }
    if (vk == kVkEscape && pressed && m_questOpen) {
        set_quest_open(false);
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
        if (vk == 0x42) {  // 'B' opens the nearest NPC's shop
            if (m_shopOpen) {
                set_shop_open(false);
                return;
            }
            const auto nearest = pick_nearest_npc();
            if (nearest != 0) interact_with_npc(nearest);
            return;
        }
        if (vk == kVkL) {  // 'L' toggles the quest log (Q is strafe)
            set_quest_open(!m_questOpen);
            return;
        }
        if (vk == kVkF) {
            try_pickup();
            return;
        }
        if (m_questOpen && !m_mainQuests.empty() && (vk == 0x26 || vk == 0x28)) {
            if (vk == 0x26) {
                m_questSelection = m_questSelection == 0
                    ? m_mainQuests.size() - 1 : m_questSelection - 1;
            } else {
                m_questSelection = (m_questSelection + 1) % m_mainQuests.size();
            }
            m_questId = m_mainQuests[m_questSelection]->quest_id;
            m_questStatus = "Not accepted";
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
    if (!m_inGame || !is_connected()) return;
    const auto result = m_pEngine->agent_session().send(make_quest_message(m_playerId, protocol, m_questId));
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
    if (m_uiRuntime.onChar(static_cast<std::int32_t>(ch))) return;
    if (!m_chatOpen) return;
    if (ch < 0x20 || ch == 0x7F) return;
    if (m_chatBuffer.size() >= 200) return;
    m_chatBuffer.push_back(static_cast<char>(ch & 0xFFu));
}

void CInGameState::OnMouseButton(bool left, bool down,
                                  std::int32_t x, std::int32_t y) {
    m_lastMouseX = x;
    m_lastMouseY = y;
    const auto ui = m_uiRuntime.onMouseButton(left, down, x, y);
    if (ui.activation) handle_ui_activation(*ui.activation);
    if (ui.consumed) return;
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
        const float fx = static_cast<float>(x);
        const float fy = static_cast<float>(y);
        const std::uint32_t drop = pick_drop_at_screen(fx, fy);
        if (drop != 0) {
            if (is_connected()) {
                (void)m_pEngine->agent_session().send(
                    make_pickup_message(m_playerId, drop));
            }
            return;
        }
        const std::uint32_t npc = pick_npc_at_screen(fx, fy);
        if (npc != 0) {
            interact_with_npc(npc);
            return;
        }
        const std::uint32_t monster = pick_monster_at_screen(fx, fy);
        if (monster != 0) {
            m_pendingAttackTarget = monster;
            try_attack();
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
    if (m_uiRuntime.onMouseMove(x, y)) {
        m_lastMouseX = x;
        m_lastMouseY = y;
        return;
    }
    if (m_cameraDrag) {
        m_cameraYaw += static_cast<float>(x - m_lastMouseX) * 0.01f;
    }
    m_lastMouseX = x;
    m_lastMouseY = y;
}

void CInGameState::OnMouseWheel(std::int32_t delta) {
    if (!m_inGame || delta == 0) return;
    constexpr float kWheelStep = 0.75f;
    const float direction = delta > 0 ? -1.0f : 1.0f;
    m_cameraDistance = std::clamp(
        m_cameraDistance + direction * kWheelStep, 3.0f, 12.0f);
}

void CInGameState::set_world_bounds(float max_x, float max_z) noexcept {
    m_worldLimitX = std::max(1.0f, max_x);
    m_worldLimitZ = std::max(1.0f, max_z);
    m_localX = std::clamp(m_localX, 0.0f, m_worldLimitX);
    m_localZ = std::clamp(m_localZ, 0.0f, m_worldLimitZ);
}

void CInGameState::set_collision_query(
    std::function<bool(float, float, float)> query) {
    m_collisionQuery = std::move(query);
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
                                    m_localX, m_localZ, dt,
                                    m_worldLimitX, m_worldLimitZ);
    if (step.moving && m_collisionQuery &&
        m_collisionQuery(step.x, step.z, 24.0f)) {
        // Keep the last valid position and let the next tick retry.  The
        // server remains authoritative; this only prevents the local avatar
        // from visibly tunnelling through loaded static geometry.
        return;
    }
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
    if (!is_connected()) return;
    const auto e = m_pEngine->agent_session().send(
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
    if (!m_inGame) return;
    if (now - m_lastAttackMs <
        static_cast<std::uint64_t>(kAttackCooldownMs)) {
        return;
    }
    std::optional<std::uint32_t> target;
    if (m_pendingAttackTarget != 0) {
        const auto it = std::find_if(
            monsters_.begin(), monsters_.end(),
            [this](const MonsterAddInfo& monster) {
                return monster.object_id == m_pendingAttackTarget &&
                       monster.current_life != 0;
            });
        if (it != monsters_.end()) target = it->object_id;
    }
    m_pendingAttackTarget = 0;
    if (!target) {
        target = pick_attack_target(
            monsters_, m_localX, m_localZ, kAttackRange);
    }
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
    m_lastAttackTarget = *target;
    m_lastAttackMs = now;
    m_attackFlashMs = now;
    push_effect_event(EffectEvent{
        EffectEventKind::CastStart, now, m_playerId, *target,
        1u, 1u, 0, 0, 0});
    if (!is_connected()) {
        MLOG_INFO("CInGameState: attack target=%u (offline)", *target);
        if (m_pEngine) m_pEngine->EmitAudio(CEngine::AudioCue::Attack);
        return;
    }
    const auto e = m_pEngine->agent_session().send(
        make_attack_message(m_playerId, 1u, *target, target_x, target_z));
    if (e != mxh::net::NetError::Ok) {
        MLOG_DEBUG("CInGameState: attack send failed: %s",
                   mxh::net::to_string(e));
    } else {
        MLOG_INFO("CInGameState: attack target=%u pos=(%.0f,%.0f)",
                  *target, target_x, target_z);
    }
    if (m_pEngine) {
        const float dx = target_x - m_localX;
        const float dz = target_z - m_localZ;
        m_pEngine->EmitAudioAt(CEngine::AudioCue::Attack,
                               std::sqrt(dx * dx + dz * dz));
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

std::uint32_t CInGameState::pick_monster_at_screen(float sx, float sy) const {
    std::uint32_t best = 0;
    float best_d2 = 24.0f * 24.0f;
    for (const auto& monster : monsters_) {
        if (monster.current_life == 0) continue;
        float px = 0;
        float py = 0;
        if (!project_npc_to_screen(
                m_localX, m_localZ, m_cameraYaw,
                static_cast<float>(monster.position_x),
                static_cast<float>(monster.position_z), px, py)) {
            continue;
        }
        const float dx = px - sx;
        const float dy = py - sy;
        const float d2 = dx * dx + dy * dy;
        if (d2 <= best_d2) {
            best_d2 = d2;
            best = monster.object_id;
        }
    }
    return best;
}

std::uint32_t CInGameState::pick_nearest_npc() const noexcept {
    if (m_npcs.empty()) return 0;
    std::uint32_t bestId = 0;
    float bestD2 = 500.0f * 500.0f;  // max interaction range 500 world units
    for (const auto& npc : m_npcs) {
        const float dx = static_cast<float>(npc.position_x) - m_localX;
        const float dz = static_cast<float>(npc.position_z) - m_localZ;
        const float d2 = dx * dx + dz * dz;
        if (d2 < bestD2) {
            bestD2 = d2;
            bestId = npc.npc_id;
        }
    }
    return bestId;
}

std::uint64_t CInGameState::attack_flash_age_ms() const noexcept {
    if (m_attackFlashMs == 0) return 0;
    const auto now = steady_now_ms();
    return now >= m_attackFlashMs ? (now - m_attackFlashMs) : 0;
}

void CInGameState::use_quick_slot(std::size_t slot) {
    if (!m_inGame || !is_connected()) return;
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

    const auto e = m_pEngine->agent_session().send(
        make_attack_message(m_playerId, skill, target, target_x, target_z));
    if (e == mxh::net::NetError::Ok) {
        m_lastAttackMs = now;
        push_effect_event(EffectEvent{
            EffectEventKind::CastStart, now, m_playerId, target,
            skill, skill, 0, 0, 0});
        MLOG_INFO("CInGameState: quick slot %zu skill=%u target=%u",
                  slot, skill, target);
        if (m_pEngine) {
            const float dx = target_x - m_localX;
            const float dz = target_z - m_localZ;
            m_pEngine->EmitAudioAt(CEngine::AudioCue::Skill,
                                   std::sqrt(dx * dx + dz * dz));
        }
    }
}

void CInGameState::open_shop(std::uint32_t npc_id) {
    if (!m_inGame || npc_id == 0) return;
    m_shopNpcId = npc_id;
    if (!is_connected()) {
        MLOG_INFO("CInGameState: open_shop npc=%u (offline)", npc_id);
        return;
    }
    set_shop_open(false);
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Npc);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::NpcProtocol::SpeechSyn);
    msg.header.object_id = m_playerId;
    msg.payload.resize(4);
    put_u32(msg.payload, 0, npc_id);
    const auto e = m_pEngine->agent_session().send(msg);
    if (e == mxh::net::NetError::Ok) {
        MLOG_INFO("CInGameState: open_shop npc=%u", npc_id);
    }
}

void CInGameState::interact_with_npc(std::uint32_t npc_id) {
    if (!m_inGame || npc_id == 0) return;
    const auto it = std::find_if(m_npcs.begin(), m_npcs.end(),
        [npc_id](const NpcInfo& npc) { return npc.npc_id == npc_id; });
    if (it == m_npcs.end()) return;

    const auto role = mxh::game::role_from_wire(it->npc_kind);
    if (role == mxh::game::NpcRole::Dealer ||
        role == mxh::game::NpcRole::Bobusang) {
        open_shop(npc_id);
        return;
    }
    if (!is_connected()) {
        MLOG_INFO("CInGameState: NPC interaction npc=%u role=%u (offline)",
                  npc_id, static_cast<unsigned>(it->npc_kind));
        return;
    }

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Npc);
    msg.header.protocol = role == mxh::game::NpcRole::MapChange
        ? static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ChangeMapSyn)
        : static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechSyn);
    msg.header.object_id = m_playerId;
    if (role == mxh::game::NpcRole::MapChange) {
        // Legacy MapChangeRole selects the configured destination (the
        // original baseline NPC sends the Jang Ahn map, 12). The server
        // validates the target route and may reject it; no local scene is
        // discarded until that handoff is acknowledged.
        msg.payload.resize(4, 0);
        const std::uint16_t target_map = 12u;
        put_u16(msg.payload, 0, target_map);
    } else {
        msg.payload.resize(sizeof(npc_id));
        put_u32(msg.payload, 0, npc_id);
    }
    if (m_pEngine->agent_session().send(msg) == mxh::net::NetError::Ok) {
        MLOG_INFO("CInGameState: NPC interaction npc=%u role=%u protocol=%u",
                  npc_id, static_cast<unsigned>(it->npc_kind),
                  static_cast<unsigned>(msg.header.protocol));
    }
}

void CInGameState::buy_shop_item(std::size_t index) {
    if (!m_inGame) return;
    if (index >= m_shopItems.size()) return;
    const auto& item = m_shopItems[index];
    m_lastBuyItemId = item.item_id;
    if (!is_connected()) {
        MLOG_INFO("CInGameState: buy item=%u (offline)", item.item_id);
        return;
    }
    const auto e = m_pEngine->agent_session().send(
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
    if (!is_connected()) return;
    const auto e = m_pEngine->agent_session().send(make_chat_message(m_playerId, text));
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
