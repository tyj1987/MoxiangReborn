// mxh/client/CInGameState.cpp
// Phase B.2.3 â€” in-game state implementation.

#include "CInGameState.hpp"
#include "CEngine.hpp"
#include "CMainGame.hpp"
#include "cinventoryexdialog.hpp"
#include "mxh/ui/cEditBox.hpp"
#include "mxh/ui/ccharacterdialog.hpp"
#include "mxh/ui/cmpguagedialog.hpp"
#include "mxh/ui/cQuestDialog.hpp"

#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
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
constexpr std::array<std::string_view, 4> kInventoryTabButtonIds{
    "IN_TABBTN1", "IN_TABBTN2", "IN_TABBTN3", "IN_TABBTN4"};
constexpr std::array<std::string_view, 4> kInventoryTabDialogIds{
    "IN_TABDLG1", "IN_TABDLG2", "IN_TABDLG3", "IN_TABDLG4"};
constexpr std::string_view kQuestDialogId = "QUE_TOTALDLG";
constexpr std::string_view kItemShopDialogId = "ITMALL_BASEDLG";
constexpr std::string_view kCharacterDialogId = "CI_CHARDLG";
constexpr std::string_view kChatDialogId = "CTI_DLG";
constexpr std::array<std::string_view, 4> kDefaultHudDialogIds{
    "MI_MAINDLG", "QI_QUICKDLG", "MNM_DIALOG", "CG_GUAGEDLG"};
constexpr float kQuickSlotW = 44.0f;
constexpr float kQuickSlotH = 44.0f;
constexpr float kQuickSlotGap = 6.0f;
constexpr float kQuickSlotY = 470.0f;

std::optional<std::size_t> quick_slot_at_screen(float x, float y) {
    const float total = static_cast<float>(kQuickSlotCount) * kQuickSlotW +
        static_cast<float>(kQuickSlotCount - 1) * kQuickSlotGap;
    const float start = (800.0f - total) * 0.5f;
    if (y < kQuickSlotY || y >= kQuickSlotY + kQuickSlotH || x < start) {
        return std::nullopt;
    }
    const float stride = kQuickSlotW + kQuickSlotGap;
    const auto slot = static_cast<std::size_t>((x - start) / stride);
    if (slot >= kQuickSlotCount) return std::nullopt;
    const float local = x - (start + static_cast<float>(slot) * stride);
    if (local >= kQuickSlotW) return std::nullopt;
    return slot;
}

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

class GameInPlayerStatsService final : public mxh::services::IPlayerStatsService {
public:
    GameInPlayerStatsService(const GameInInfo* info,
                             const mxh::game::ExperienceCurve* curve) noexcept
        : m_info(info), m_curve(curve) {}
    std::uint16_t getStr() const noexcept override { return 0; }
    std::uint16_t getAgi() const noexcept override { return 0; }
    std::uint16_t getInt() const noexcept override { return 0; }
    std::uint16_t getWis() const noexcept override { return 0; }
    std::uint16_t getDex() const noexcept override { return 0; }
    std::uint16_t getGenGol() const noexcept override { return m_info ? m_info->gen_gol : 0; }
    std::uint16_t getMinChub() const noexcept override { return m_info ? m_info->min_chub : 0; }
    std::uint16_t getCheRyuk() const noexcept override { return m_info ? m_info->che_ryuk : 0; }
    std::uint16_t getSimMek() const noexcept override { return m_info ? m_info->sim_mek : 0; }
    std::uint16_t getLevel() const noexcept override { return m_info ? m_info->level : 0; }
    std::uint32_t getLevelExp() const noexcept override { return m_info ? m_info->exp : 0; }
    std::uint32_t getExpForNextLevel() const noexcept override {
        if (!m_info || !m_curve || m_info->level == 0 ||
            m_info->level >= m_curve->size()) return 0;
        const auto threshold = m_curve->max_exp_point(m_info->level);
        return threshold > std::numeric_limits<std::uint32_t>::max()
            ? std::numeric_limits<std::uint32_t>::max()
            : static_cast<std::uint32_t>(threshold);
    }
    std::uint32_t getCurrentHp() const noexcept override { return m_info ? m_info->life : 0; }
    std::uint32_t getMaxHp() const noexcept override { return m_info ? m_info->max_life : 0; }
    std::uint32_t getCurrentMp() const noexcept override { return m_info ? m_info->mp : 0; }
    std::uint32_t getMaxMp() const noexcept override { return m_info ? m_info->max_mp : 0; }
    float getHpFraction() const noexcept override {
        return !m_info || m_info->max_life == 0 ? 0.0f
            : static_cast<float>(m_info->life) / static_cast<float>(m_info->max_life);
    }
    float getMpFraction() const noexcept override {
        return !m_info || m_info->max_mp == 0 ? 0.0f
            : static_cast<float>(m_info->mp) / static_cast<float>(m_info->max_mp);
    }
private:
    const GameInInfo* m_info = nullptr;
    const mxh::game::ExperienceCurve* m_curve = nullptr;
};

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

mxh::net::Message make_party_create_message(std::uint32_t player_id,
                                             std::uint8_t option) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::CreateSyn);
    message.header.object_id = player_id;
    message.payload = {option};
    return message;
}

mxh::net::Message make_party_request_message(std::uint32_t player_id,
                                              mxh::proto::PartyProtocol protocol,
                                              std::span<const std::uint8_t> payload) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    message.header.protocol = static_cast<std::uint8_t>(protocol);
    message.header.object_id = player_id;
    message.payload.assign(payload.begin(), payload.end());
    return message;
}

mxh::net::Message make_guild_request_message(std::uint32_t player_id,
                                              mxh::proto::GuildProtocol protocol,
                                              std::span<const std::uint8_t> payload) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Guild);
    message.header.protocol = static_cast<std::uint8_t>(protocol);
    message.header.object_id = player_id;
    message.payload.assign(payload.begin(), payload.end());
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
    info.gen_gol  = get_u16(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 0);
    info.min_chub = get_u16(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 2);
    info.che_ryuk = get_u16(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 4);
    info.sim_mek  = get_u16(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 6);
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
                 std::filesystem::path("../../../../data/PlayDH")}) {
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
    if (m_inGame && !m_sentGameOutSyn && m_pEngine &&
        m_pEngine->agent_session().is_connected()) {
        send_gameout_syn();
    }
    m_info = GameInInfo{};
    m_inGame   = false;
    m_started  = false;
    m_sentGameInSyn = false;
    m_sentGameOutSyn = false;
    m_pendingSkillId = 0;
    m_failed   = false;
    m_failureReason.clear();
    m_uiRuntime.clear();
    m_keyMask = 0;
    m_moving = false;
    m_cameraDrag = false;
    m_inventoryDragSource.reset();
    m_inventoryTab = 0;
    m_chatOpen = false;
    m_chatBuffer.clear();
    m_effectEvents.clear();
    m_pendingSkillEffects.clear();
    m_effectRuntime.clear();
    m_playerStatsService.reset();
    if (m_effectCatalogLoad.valid()) {
        m_effectCatalogLoad.wait();
    }
    m_effectCatalogLoading = false;
    m_experienceCurve.reset();
    set_inventory_open(false);
    set_shop_open(false);
    set_quest_open(false);
    set_character_open(false);
    set_chat_open(false);
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
    refresh_live_ui_bindings();
    if (m_effectCatalogLoading && m_effectCatalogLoad.valid() &&
        m_effectCatalogLoad.wait_for(std::chrono::milliseconds(0)) ==
            std::future_status::ready) {
        auto result = m_effectCatalogLoad.get();
        m_effectCatalogLoading = false;
        if (result.second.empty()) {
            m_effectRuntime.adopt_catalog(std::move(result.first));
            MLOG_INFO("CInGameState effect catalog ready: assets=%zu beff=%zu befl=%zu packed=%zu loose=%zu",
                      m_effectRuntime.catalog().assets().size(),
                      m_effectRuntime.catalog().beff_count(),
                      m_effectRuntime.catalog().befl_count(),
                      m_effectRuntime.catalog().packed_count(),
                      m_effectRuntime.catalog().loose_count());
            // Network replies can arrive before the asynchronous effect
            // index is ready. Replay those authoritative skill events now;
            // dropping them would make the first attacks visibly silent.
            auto pending = std::move(m_pendingSkillEffects);
            m_pendingSkillEffects.clear();
            for (const auto& request : pending) {
                start_skill_effect(request.skill_id,
                                   request.target_object_id,
                                   request.timestamp_ms,
                                   request.source_object_id);
            }
        } else {
            MLOG_WARN("CInGameState effect catalog unavailable: %s", result.second.c_str());
        }
    }
    const auto now_ms = steady_now_ms();
    m_effectRuntime.advance(now_ms, [this](const RuntimeEffectEvent& event) {
        constexpr std::size_t kMaxRuntimeEvents = 256;
        if (m_runtimeEffectEvents.size() >= kMaxRuntimeEvents) {
            m_runtimeEffectEvents.erase(m_runtimeEffectEvents.begin());
        }
        m_runtimeEffectEvents.push_back(event);
    });
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
    {
        const auto effect_root = *m_pEngine->playdh_root();
        if (const char* smoke_exit = std::getenv("MXH_GUI_SMOKE_EXIT");
            smoke_exit && *smoke_exit == '1') {
            MLOG_INFO("CInGameState effect catalog deferred for GUI smoke exit");
        } else {
            m_effectCatalogLoading = true;
            m_effectCatalogLoad = std::async(std::launch::async, [effect_root] {
                mxh::game::EffectCatalog catalog;
                std::string error;
                (void)catalog.load(effect_root, &error);
                return std::make_pair(std::move(catalog), std::move(error));
            });
            MLOG_INFO("CInGameState effect catalog loading asynchronously");
        }
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
    const auto skillPath = *m_pEngine->playdh_root() / "Resource" / "SkillList.bin";
    try {
        std::uint32_t skillErrors = 0;
        m_skillManager.init_from_bin(skillPath.string(), &skillErrors);
        MLOG_INFO("CInGameState skill list loaded skills=%zu errors=%u",
                  m_skillManager.size(),
                  static_cast<unsigned>(skillErrors));
    } catch (const std::exception& ex) {
        MLOG_WARN("CInGameState skill list unavailable: %s", ex.what());
    }
    const auto experiencePath = *m_pEngine->playdh_root() / "Resource" /
        "CharacterExpPoint.bin";
    try {
        m_experienceCurve = std::make_unique<mxh::game::ExperienceCurve>(
            mxh::game::ExperienceCurve::load_from_bin(experiencePath));
        MLOG_INFO("CInGameState experience curve loaded levels=%zu",
                  m_experienceCurve->size());
    } catch (const std::exception& ex) {
        m_experienceCurve.reset();
        MLOG_ERROR("CInGameState experience curve unavailable: %s", ex.what());
        fail_with("GameIn experience curve unavailable: " + experiencePath.string());
        return;
    }
    m_uiRuntime.applyActiveSet(kDefaultHudDialogIds);
    m_playerStatsService = std::make_unique<GameInPlayerStatsService>(
        &m_info, m_experienceCurve.get());
    refresh_live_ui_bindings();
    MLOG_INFO("CInGameState using persistent AgentSession (player_id=%u, map=%u)",
              static_cast<unsigned>(m_playerId),
              static_cast<unsigned>(m_mapNum));
    if (!m_pEngine || !m_pEngine->agent_session().is_connected()) {
        fail_with("GameIn requires a connected AgentSession");
        return;
    }
    send_gamein_syn();
}

void CInGameState::refresh_live_ui_bindings() {
    if (!m_playerStatsService) return;
    for (auto& dialog : m_uiRuntime.dialogsMutable()) {
        if (!dialog) continue;
        if (auto* character = dynamic_cast<mxh::ui::cCharacterDialog*>(dialog.get())) {
            character->SetPlayerStatsService(m_playerStatsService.get());
            character->RefreshFromPlayerStats();
        }
        if (auto* mp = dynamic_cast<mxh::ui::cMPGuageDialog*>(dialog.get())) {
            mp->SetPlayerStatsService(m_playerStatsService.get());
            mp->RefreshFromPlayerStats();
        }
    }
}

mxh::net::IEncryptor* CInGameState::encryptor_for(
    mxh::net::ConnectionId) {
    return m_hsel ? m_hsel.get() : nullptr;
}

bool CInGameState::is_connected() const noexcept {
    return m_pEngine && m_pEngine->agent_session().is_connected();
}

std::optional<float> CInGameState::distance_to_object(
    std::uint32_t object_id) const noexcept {
    if (object_id == 0) return std::nullopt;
    if (object_id == m_playerId || object_id == m_info.player_id) return 0.0f;
    const auto distance = [this](float x, float z) {
        return std::hypot(x - m_info.position_x, z - m_info.position_z);
    };
    for (const auto& monster : monsters_) {
        if (monster.object_id == object_id) {
            return distance(static_cast<float>(monster.position_x),
                            static_cast<float>(monster.position_z));
        }
    }
    for (const auto& npc : m_npcs) {
        if (npc.npc_id == object_id) {
            return distance(static_cast<float>(npc.position_x),
                            static_cast<float>(npc.position_z));
        }
    }
    for (const auto& [id, player] : m_remotePlayers) {
        if (id == object_id) {
            return distance(static_cast<float>(player.position_x),
                            static_cast<float>(player.position_z));
        }
    }
    return std::nullopt;
}

void CInGameState::set_quest_catalog(mxh::compat::QuestStringCatalog catalog) {
    m_questCatalog = std::move(catalog);
    m_mainQuests = m_questCatalog.main_quests();
    m_questSelection = 0;
    if (!m_mainQuests.empty()) m_questId = m_mainQuests.front()->quest_id;
    if (auto* window = m_uiRuntime.findWindowByLegacyId(kQuestDialogId)) {
        if (auto* dialog = dynamic_cast<mxh::ui::cQuestDialog*>(window)) {
            for (const auto* quest : m_mainQuests) {
                if (!quest) continue;
                mxh::ui::QuestEntry entry;
                entry.id = quest->quest_id;
                entry.title = quest->title;
                entry.status = mxh::ui::QuestStatus::Available;
                dialog->AddQuest(std::move(entry));
            }
        }
    }
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
        case Category::Party:
            handle_party_message(msg);
            break;
        case Category::Guild:
            handle_guild_message(msg);
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

void CInGameState::send_gameout_syn() {
    if (m_sentGameOutSyn || !m_pEngine || m_playerId == 0) return;
    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameOutSyn);
    out.header.object_id = m_playerId;
    out.payload = {};
    const auto e = m_pEngine->agent_session().send(out);
    if (e != mxh::net::NetError::Ok) {
        MLOG_WARN("CInGameState: send GameOutSyn failed: %s",
                  mxh::net::to_string(e));
        return;
    }
    m_sentGameOutSyn = true;
    MLOG_INFO("CInGameState: sent GameOutSyn player_id=%u",
              static_cast<unsigned>(m_playerId));
}

void CInGameState::dispatch_gamein_ack(const GameInInfo& info) {
    // The server's GameInAck is authoritative for the character object ID.
    // Normally the host already assigned it before sending GameInSyn, but
    // accepting it here also keeps reconnect/map-change paths consistent.
    if (info.player_id != 0u) m_playerId = info.player_id;
    m_info   = info;
    m_inGame = true;
    if (info.map_num != 0) m_mapNum = info.map_num;
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
    // The GUI smoke harness requests a deterministic handoff marker.  Stop
    // only after the authoritative ack has been parsed; normal clients keep
    // running and proceed into the regular render/input loop.
    if (const char* smoke_exit = std::getenv("MXH_GUI_SMOKE_EXIT");
        smoke_exit && *smoke_exit == '1') {
        MLOG_INFO("mxh_client: GUI_SMOKE_PASS player_id=%u map=%u",
                  static_cast<unsigned>(m_playerId),
                  static_cast<unsigned>(m_mapNum));
        m_smokeExitRequested = true;
    }
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
            if (info->object_id == m_playerId) {
                m_info.life = static_cast<std::uint16_t>(
                    std::min(info->life, std::uint32_t{0xffffu}));
                m_info.max_life = static_cast<std::uint16_t>(
                    std::min(info->max_life, std::uint32_t{0xffffu}));
                m_info.gender = info->gender;
                m_info.face_type = info->face_type;
                m_info.hair_type = info->hair_type;
                m_info.weared_item_idx = info->weared_item_idx;
                refresh_live_ui_bindings();
                MLOG_INFO("CInGameState: self CharacterAdd refresh life=%u/%u",
                          static_cast<unsigned>(m_info.life),
                          static_cast<unsigned>(m_info.max_life));
                break;
            }
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
                        static_cast<int>(GameStateId::MapChange));
                    MLOG_INFO("CInGameState: ChangeMapAck map=%u -> MapChange",
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
                const auto source_object = msg.header.object_id != 0
                    ? msg.header.object_id : m_playerId;
                // The server acknowledgement is the presentation authority:
                // do not start a BEFF timeline merely because the client
                // write succeeded.  This also prevents a rejected or
                // duplicated request from producing a phantom effect.
                start_skill_effect(skill_idx, skill_object, m_lastTickMs,
                                   source_object);
                push_effect_event(EffectEvent{
                    EffectEventKind::CastRelease, m_lastTickMs, source_object,
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
                const auto source_object = msg.header.object_id != 0
                    ? msg.header.object_id : m_playerId;
                if (m_pendingSkillId != 0) {
                    start_skill_effect(m_pendingSkillId, target, m_lastTickMs,
                                       source_object);
                }
                push_effect_event(EffectEvent{
                    EffectEventKind::Hit, m_lastTickMs, source_object, target,
                    0, 0, 0, damage, hit});
                push_effect_event(EffectEvent{
                    EffectEventKind::End, m_lastTickMs, source_object, target,
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

std::vector<EffectEvent> CInGameState::drain_effect_events() noexcept {
    std::vector<EffectEvent> pending;
    pending.swap(m_effectEvents);
    return pending;
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

void CInGameState::handle_party_message(const mxh::net::Message& msg) {
    using mxh::proto::PartyProtocol;
    const auto proto = static_cast<PartyProtocol>(msg.header.protocol);
    if (proto == PartyProtocol::CreateAck && msg.payload.size() >= 5) {
        std::memcpy(&m_partyId, msg.payload.data(), sizeof(m_partyId));
        m_partyMemberCount = msg.payload[4];
        m_uiRuntime.showMessage(9101, "Party created.");
        return;
    }
    if (proto == PartyProtocol::AddInvite && msg.payload.size() >= 4) {
        std::memcpy(&m_pendingPartyInviteId, msg.payload.data(), sizeof(m_pendingPartyInviteId));
        m_uiRuntime.showMessage(9104, "You received a party invitation.");
        return;
    }
    if (proto == PartyProtocol::InviteAcceptAck && msg.payload.size() >= 5) {
        std::memcpy(&m_partyId, msg.payload.data(), sizeof(m_partyId));
        m_partyMemberCount = msg.payload[4];
        m_pendingPartyInviteId = 0;
        m_uiRuntime.showMessage(9105, "Joined party.");
        return;
    }
    if (proto == PartyProtocol::Info && msg.payload.size() >= 5) {
        std::memcpy(&m_partyId, msg.payload.data(), sizeof(m_partyId));
        m_partyMemberCount = msg.payload[4];
        return;
    }
    if (proto == PartyProtocol::BreakupAck && msg.payload.size() >= 4) {
        m_partyId = 0;
        m_partyMemberCount = 0;
        m_uiRuntime.showMessage(9102, "Party disbanded.");
        return;
    }
    if (proto == PartyProtocol::CreateNack || proto == PartyProtocol::AddNack ||
        proto == PartyProtocol::InviteAcceptNack || proto == PartyProtocol::BreakupNack) {
        m_uiRuntime.showMessage(9103, "Party action failed.");
    }
}

void CInGameState::handle_guild_message(const mxh::net::Message& msg) {
    using mxh::proto::GuildProtocol;
    const auto proto = static_cast<GuildProtocol>(msg.header.protocol);
    if ((proto == GuildProtocol::CreateAck || proto == GuildProtocol::Info) &&
        msg.payload.size() >= 5) {
        std::memcpy(&m_guildId, msg.payload.data(), sizeof(m_guildId));
        m_guildMemberCount = msg.payload[4];
        if (proto == GuildProtocol::CreateAck) {
            m_uiRuntime.showMessage(9110, "Guild created.");
        }
        return;
    }
    if (proto == GuildProtocol::AddMemberInvite && msg.payload.size() >= 4) {
        std::memcpy(&m_pendingGuildInviteId, msg.payload.data(),
                    sizeof(m_pendingGuildInviteId));
        m_uiRuntime.showMessage(9113, "You received a guild invitation.");
        return;
    }
    if (proto == GuildProtocol::InviteAccept && msg.payload.size() >= 5) {
        std::memcpy(&m_guildId, msg.payload.data(), sizeof(m_guildId));
        m_guildMemberCount = msg.payload[4];
        m_pendingGuildInviteId = 0;
        m_uiRuntime.showMessage(9114, "Joined guild.");
        return;
    }
    if (proto == GuildProtocol::BreakupAck) {
        m_guildId = 0;
        m_guildMemberCount = 0;
        m_uiRuntime.showMessage(9111, "Guild disbanded.");
        return;
    }
    if (proto == GuildProtocol::CreateNack || proto == GuildProtocol::AddMemberNack ||
        proto == GuildProtocol::InviteAcceptNack || proto == GuildProtocol::BreakupNack) {
        m_uiRuntime.showMessage(9112, "Guild action failed.");
    }
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
        if (db_idx == 0u || target >= mxh::game::TP_WEAREDITEM_END) return;
        const auto is_inventory = [](std::uint16_t position) {
            return position < mxh::game::SLOT_INVENTORY_NUM;
        };
        const auto is_equipment = [](std::uint16_t position) {
            return position >= mxh::game::TP_WEAREDITEM_START &&
                   position < mxh::game::TP_WEAREDITEM_END;
        };
        std::uint16_t source = mxh::game::TP_WEAREDITEM_END;
        for (std::size_t i = 0; i < mxh::game::SLOT_INVENTORY_NUM; ++i) {
            if (m_info.items.Inventory[i].dwDBIdx == db_idx) {
                source = static_cast<std::uint16_t>(i);
                break;
            }
        }
        if (source == mxh::game::TP_WEAREDITEM_END) {
            for (std::size_t i = 0; i < mxh::game::WEARED_ITEM_MAX; ++i) {
                if (m_info.items.WearedItem[i].dwDBIdx == db_idx) {
                    source = static_cast<std::uint16_t>(
                        mxh::game::TP_WEAREDITEM_START + i);
                    break;
                }
            }
        }
        if ((!is_inventory(source) && !is_equipment(source)) ||
            (!is_inventory(target) && !is_equipment(target))) return;
        auto& source_item = is_inventory(source)
            ? m_info.items.Inventory[source]
            : m_info.items.WearedItem[source - mxh::game::TP_WEAREDITEM_START];
        auto& target_item = is_inventory(target)
            ? m_info.items.Inventory[target]
            : m_info.items.WearedItem[target - mxh::game::TP_WEAREDITEM_START];
        std::swap(source_item, target_item);
        source_item.Position = source;
        target_item.Position = target;
        if (m_inventoryOpen) set_inventory_open(true);
        for (std::size_t slot = 0; slot < m_info.weared_item_idx.size(); ++slot) {
            m_info.weared_item_idx[slot] = m_info.items.WearedItem[slot].wIconIdx;
        }
        MLOG_INFO("CInGameState: item moved db=%u source=%u target=%u",
                  db_idx, source, target);
    } else if (proto == static_cast<std::uint8_t>(
                   mxh::proto::ItemProtocol::MoveNack)) {
        // Keep the authoritative rejection visible to the player.  The
        // server deliberately does not encode a new error protocol here;
        // the legacy client presents a modal item-move failure message.
        (void)m_uiRuntime.showMessage(9101, "Cannot equip or move this item.");
        MLOG_WARN("CInGameState: item move rejected by server");
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
            for (std::size_t slot = 0; slot < m_info.weared_item_idx.size(); ++slot) {
                m_info.weared_item_idx[slot] = m_info.items.WearedItem[slot].wIconIdx;
            }
            if (m_inventoryOpen) set_inventory_open(true);
            refresh_live_ui_bindings();
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
    if (open) (void)select_inventory_tab(m_inventoryTab);
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

bool CInGameState::select_inventory_tab(std::size_t tab) noexcept {
    if (tab >= kInventoryTabDialogIds.size()) return false;
    m_inventoryTab = tab;
    if (!m_inventoryOpen) return true;
    for (std::size_t i = 0; i < kInventoryTabDialogIds.size(); ++i) {
        m_uiRuntime.setDialogActive(kInventoryTabDialogIds[i], i == tab);
        if (auto* grid = m_uiRuntime.findWindowByLegacyId(kInventoryTabDialogIds[i])) {
            grid->SetDisable(false);
        }
    }
    m_inventoryDragSource.reset();
    return true;
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

void CInGameState::set_character_open(bool open) noexcept {
    m_characterOpen = open;
    m_uiRuntime.setDialogActive(kCharacterDialogId, open);
    if (open) refresh_live_ui_bindings();
}

void CInGameState::set_chat_open(bool open) noexcept {
    m_chatOpen = open;
    m_uiRuntime.setDialogActive(kChatDialogId, open);
    if (open) {
        if (auto* window = m_uiRuntime.findWindowByLegacyId("MI_CHATEDITBOX")) {
            if (auto* edit = dynamic_cast<mxh::ui::cEditBox*>(window)) {
                if (edit->maxBytes() == 0) edit->InitEditbox(420, 68);
                edit->SetEditText(m_chatBuffer);
            }
        }
    }
}

bool CInGameState::select_quest_index(std::size_t index) noexcept {
    if (index >= m_mainQuests.size() || m_mainQuests[index] == nullptr) {
        return false;
    }
    m_questSelection = index;
    m_questId = m_mainQuests[index]->quest_id;
    return true;
}

bool CInGameState::handle_ui_activation(
    const ClientUiActivation& activation) {
    // CI_BESTTIP is the original character-information button in the HUD.
    // Dispatch it by legacy ID so layout changes do not turn into a coordinate
    // based shortcut and the real stats dialog receives live state.
    if (activation.legacy_id == "CI_BESTTIP") {
        set_character_open(!m_characterOpen);
        return true;
    }
    for (std::size_t i = 0; i < kInventoryTabButtonIds.size(); ++i) {
        if (activation.legacy_id == kInventoryTabButtonIds[i]) {
            return select_inventory_tab(i);
        }
    }
    if (activation.legacy_id.rfind("QUE_PAGE", 0) == 0 &&
        activation.legacy_id.size() == 12 &&
        activation.legacy_id[8] >= '1' &&
        activation.legacy_id[8] <= '5') {
        const auto page = static_cast<std::size_t>(
            activation.legacy_id[8] - '1');
        return select_quest_index(page);
    }
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
    if (activation.dialog_legacy_id == kCharacterDialogId) {
        set_character_open(false);
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
    // The real chat editbox owns keyboard focus while the chat dialog is
    // active, but Enter/Escape are state-level submit/cancel actions. Handle
    // them before the generic UI dispatcher so focus cannot swallow them.
    if (m_chatOpen && pressed && vk == kVkReturn) {
        send_chat();
        return;
    }
    if (m_chatOpen && pressed && vk == kVkEscape) {
        set_chat_open(false);
        m_chatBuffer.clear();
        return;
    }
    if (m_chatOpen && pressed && vk == kVkBack) {
        if (!m_chatBuffer.empty()) m_chatBuffer.pop_back();
        if (auto* window = m_uiRuntime.findWindowByLegacyId("MI_CHATEDITBOX")) {
            if (auto* edit = dynamic_cast<mxh::ui::cEditBox*>(window)) {
                edit->SetEditText(m_chatBuffer);
            }
        }
        return;
    }
    if (m_uiRuntime.onKey(pressed, static_cast<std::int32_t>(vk))) {
        if (m_chatOpen) {
            if (auto* window = m_uiRuntime.findWindowByLegacyId("MI_CHATEDITBOX")) {
                if (auto* edit = dynamic_cast<mxh::ui::cEditBox*>(window)) {
                    if (edit->editText() != m_chatBuffer) {
                        m_chatBuffer = edit->editText();
                    } else if (vk == kVkBack && !m_chatBuffer.empty()) {
                        m_chatBuffer.pop_back();
                        edit->SetEditText(m_chatBuffer);
                    }
                }
            }
        }
        return;
    }
    if (vk == kVkReturn) {
        if (pressed) {
            if (m_chatOpen) {
                send_chat();
            } else {
                set_chat_open(true);
            }
        }
        return;
    }
    if (vk == kVkEscape && pressed && m_chatOpen) {
        set_chat_open(false);
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
    if (vk == kVkEscape && pressed && m_characterOpen) {
        set_character_open(false);
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
            (void)select_quest_index(m_questSelection);
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
    const auto protocol = static_cast<mxh::proto::QuestProtocol>(msg.header.protocol);
    if (protocol == mxh::proto::QuestProtocol::ChangeState && msg.payload.size() >= 8) {
        const auto read_u32 = [&msg](std::size_t offset) {
            return static_cast<std::uint32_t>(msg.payload[offset]) |
                   (static_cast<std::uint32_t>(msg.payload[offset + 1]) << 8) |
                   (static_cast<std::uint32_t>(msg.payload[offset + 2]) << 16) |
                   (static_cast<std::uint32_t>(msg.payload[offset + 3]) << 24);
        };
        m_questId = static_cast<std::uint16_t>(read_u32(0));
        const auto state = read_u32(4);
        mxh::ui::QuestStatus ui_status = mxh::ui::QuestStatus::Available;
        switch (state) {
            case 1u:
                m_questStatus = "Active - hunt monsters";
                ui_status = mxh::ui::QuestStatus::Active;
                break;
            case 2u:
                m_questStatus = "Ready to claim";
                ui_status = mxh::ui::QuestStatus::Completed;
                break;
            case 3u:
                m_questStatus = "Reward claimed";
                ui_status = mxh::ui::QuestStatus::Claimed;
                break;
            case 4u:
                m_questStatus = "Failed";
                break;
            default:
                m_questStatus = "Available";
                break;
        }
        if (auto* window = m_uiRuntime.findWindowByLegacyId(kQuestDialogId)) {
            if (auto* dialog = dynamic_cast<mxh::ui::cQuestDialog*>(window)) {
                (void)dialog->UpdateQuest(m_questId, ui_status);
            }
        }
        MLOG_INFO("CInGameState: quest id=%u state=%u status=%s",
                  m_questId, state, m_questStatus.c_str());
        return;
    }
    if (msg.payload.size() >= 2) {
        m_questId = static_cast<std::uint16_t>(msg.payload[0] |
                    (static_cast<std::uint16_t>(msg.payload[1]) << 8));
    }
    switch (protocol) {
        case mxh::proto::QuestProtocol::StartAck:
            m_questStatus = "Active - hunt monsters";
            break;
        case mxh::proto::QuestProtocol::StartNack:
            m_questStatus = "Cannot accept";
            break;
        case mxh::proto::QuestProtocol::EndAck:
            m_questStatus = "Reward claimed";
            break;
        case mxh::proto::QuestProtocol::EndNack:
            m_questStatus = "Not complete";
            break;
        default: break;
    }
    if (auto* window = m_uiRuntime.findWindowByLegacyId(kQuestDialogId)) {
        if (auto* dialog = dynamic_cast<mxh::ui::cQuestDialog*>(window)) {
            const auto status = protocol == mxh::proto::QuestProtocol::StartAck
                ? mxh::ui::QuestStatus::Active
                : protocol == mxh::proto::QuestProtocol::EndAck
                    ? mxh::ui::QuestStatus::Claimed
                    : protocol == mxh::proto::QuestProtocol::EndNack
                        ? mxh::ui::QuestStatus::Active
                        : mxh::ui::QuestStatus::Available;
            (void)dialog->UpdateQuest(m_questId, status);
        }
    }
    MLOG_INFO("CInGameState: quest id=%u status=%s", m_questId, m_questStatus.c_str());
}

void CInGameState::OnChar(std::uint32_t ch) {
    if (m_chatOpen) {
        if (ch < 0x20 || ch == 0x7F || m_chatBuffer.size() >= 200) return;
        m_chatBuffer.push_back(static_cast<char>(ch & 0xFFu));
        if (auto* window = m_uiRuntime.findWindowByLegacyId("MI_CHATEDITBOX")) {
            if (auto* edit = dynamic_cast<mxh::ui::cEditBox*>(window)) {
                edit->SetEditText(m_chatBuffer);
            }
        }
        return;
    }
    if (m_uiRuntime.onChar(static_cast<std::int32_t>(ch))) {
        if (m_chatOpen) {
            if (auto* window = m_uiRuntime.findWindowByLegacyId("MI_CHATEDITBOX")) {
                if (auto* edit = dynamic_cast<mxh::ui::cEditBox*>(window)) {
                    if (edit->editText() != m_chatBuffer) {
                        m_chatBuffer = edit->editText();
                    } else if (ch >= 0x20 && ch != 0x7F &&
                               m_chatBuffer.size() < 200) {
                        m_chatBuffer.push_back(static_cast<char>(ch & 0xFFu));
                        edit->SetEditText(m_chatBuffer);
                    }
                }
            }
        }
        return;
    }
    if (!m_chatOpen) return;
    if (ch < 0x20 || ch == 0x7F) return;
    if (m_chatBuffer.size() >= 200) return;
    m_chatBuffer.push_back(static_cast<char>(ch & 0xFFu));
    if (auto* window = m_uiRuntime.findWindowByLegacyId("MI_CHATEDITBOX")) {
        if (auto* edit = dynamic_cast<mxh::ui::cEditBox*>(window)) {
            edit->SetEditText(m_chatBuffer);
        }
    }
}

void CInGameState::OnMouseButton(bool left, bool down,
                                  std::int32_t x, std::int32_t y) {
    m_lastMouseX = x;
    m_lastMouseY = y;
    if (left && m_inventoryOpen) {
        auto inventory_slot_at = [&](std::int32_t px, std::int32_t py)
            -> std::optional<std::size_t> {
            const auto* grid = m_uiRuntime.findWindowByLegacyId(
                kInventoryTabDialogIds[m_inventoryTab]);
            if (!grid) return std::nullopt;
            const auto local_x = px - grid->absX();
            const auto local_y = py - grid->absY();
            constexpr std::int32_t cell = 40;
            constexpr std::int32_t gap = 5;
            const auto col = local_x / (cell + gap);
            const auto row = local_y / (cell + gap);
            const auto in_cell_x = local_x % (cell + gap);
            const auto in_cell_y = local_y % (cell + gap);
            if (col < 0 || col >= 5 || row < 0 || row >= 4 ||
                in_cell_x >= cell || in_cell_y >= cell) return std::nullopt;
            return m_inventoryTab * 20u + static_cast<std::size_t>(row * 5 + col);
        };
        auto equipment_slot_at = [&](std::int32_t px, std::int32_t py)
            -> std::optional<std::size_t> {
            const auto* wear = m_uiRuntime.findWindowByLegacyId("IN_WEAREDDLG");
            if (!wear) return std::nullopt;
            static constexpr std::array<std::array<std::int32_t, 2>, 10> cells{{
                {{53, 28}}, {{8, 73}}, {{98, 73}}, {{53, 118}},
                {{143, 28}}, {{188, 28}}, {{143, 73}}, {{188, 73}},
                {{143, 118}}, {{188, 118}}}};
            const auto local_x = px - wear->absX();
            const auto local_y = py - wear->absY();
            for (std::size_t i = 0; i < cells.size(); ++i) {
                if (local_x >= cells[i][0] && local_x < cells[i][0] + 42 &&
                    local_y >= cells[i][1] && local_y < cells[i][1] + 42) {
                    return mxh::game::TP_WEAREDITEM_START + i;
                }
            }
            return std::nullopt;
        };
        const auto inventory_slot = inventory_slot_at(x, y);
        const auto equipment_slot = equipment_slot_at(x, y);
        const auto hit_slot = inventory_slot ? inventory_slot : equipment_slot;
        if (down && hit_slot) {
            const auto* item = *hit_slot < mxh::game::SLOT_INVENTORY_NUM
                ? &m_info.items.Inventory[*hit_slot]
                : &m_info.items.WearedItem[*hit_slot - mxh::game::TP_WEAREDITEM_START];
            if (!mxh::game::is_empty_slot(*item)) m_inventoryDragSource = *hit_slot;
            return;
        }
        if (!down && m_inventoryDragSource && hit_slot) {
            const auto source = *m_inventoryDragSource;
            m_inventoryDragSource.reset();
            if (source != *hit_slot) (void)request_inventory_move(source, *hit_slot);
            return;
        }
        const auto grid_id = kInventoryTabDialogIds[m_inventoryTab];
        auto* grid = m_uiRuntime.findWindowByLegacyId(grid_id);
        if (grid) {
            const auto local_x = x - grid->absX();
            const auto local_y = y - grid->absY();
            constexpr std::int32_t cell = 40;
            constexpr std::int32_t gap = 5;
            const auto col = local_x / (cell + gap);
            const auto row = local_y / (cell + gap);
            const auto in_cell_x = local_x % (cell + gap);
            const auto in_cell_y = local_y % (cell + gap);
            if (col >= 0 && col < 5 && row >= 0 && row < 4 &&
                in_cell_x < cell && in_cell_y < cell) {
                const std::size_t slot = m_inventoryTab * 20u +
                    static_cast<std::size_t>(row * 5 + col);
                if (down) {
                    if (slot < mxh::game::SLOT_INVENTORY_NUM &&
                        !mxh::game::is_empty_slot(m_info.items.Inventory[slot])) {
                        m_inventoryDragSource = slot;
                    }
                    return;
                }
                if (m_inventoryDragSource) {
                    const auto source = *m_inventoryDragSource;
                    m_inventoryDragSource.reset();
                    if (source != slot) (void)request_inventory_move(source, slot);
                    return;
                }
            }
        }
    }
    const auto ui = m_uiRuntime.onMouseButton(left, down, x, y);
    if (ui.activation) handle_ui_activation(*ui.activation);
    if (ui.consumed) return;
    if (left && down) {
        if (const auto slot = quick_slot_at_screen(static_cast<float>(x),
                                                   static_cast<float>(y))) {
            use_quick_slot(*slot);
            return;
        }
    }
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

bool CInGameState::request_inventory_move(std::size_t source,
                                          std::size_t target) {
    if (!m_inGame || !is_connected() ||
        source >= mxh::game::TP_WEAREDITEM_END ||
        target >= mxh::game::TP_WEAREDITEM_END || source == target) {
        return false;
    }
    const auto& item = source < mxh::game::SLOT_INVENTORY_NUM
        ? m_info.items.Inventory[source]
        : m_info.items.WearedItem[source - mxh::game::TP_WEAREDITEM_START];
    if (mxh::game::is_empty_slot(item)) return false;
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    msg.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    msg.header.object_id = m_playerId;
    msg.payload.resize(sizeof(mxh::game::ItemBase) + sizeof(std::uint16_t));
    std::memcpy(msg.payload.data(), &item, sizeof(mxh::game::ItemBase));
    const auto target16 = static_cast<std::uint16_t>(target);
    std::memcpy(msg.payload.data() + sizeof(mxh::game::ItemBase), &target16,
                sizeof(target16));
    return m_pEngine->agent_session().send(msg) == mxh::net::NetError::Ok;
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

void CInGameState::set_map_change_target_resolver(
    MapChangeTargetResolver resolver) {
    m_mapChangeTargetResolver = std::move(resolver);
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
        // Keep the local presentation in sync with the accepted movement
        // request.  The server remains authoritative and subsequent move
        // notifications can correct this prediction, but without this
        // immediate update a click-to-move followed by an attack can never
        // enter the real range gate in a headless or high-latency session.
        m_localX = static_cast<float>(x);
        m_localZ = static_cast<float>(z);
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
        m_pendingSkillId = 1u;
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
        m_pendingSkillId = skill;
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

void CInGameState::start_skill_effect(std::uint32_t skill_id,
                                      std::uint32_t target_object_id,
                                      std::uint64_t now_ms,
                                      std::uint32_t source_object_id) {
    if (m_effectTickPerFrameMs == 0) return;
    if (source_object_id == 0) source_object_id = m_playerId;
    const auto refs = m_skillManager.effect_names(skill_id);
    if (!refs || refs->effect_use.empty()) return;
    if (m_effectCatalogLoading &&
        !m_effectRuntime.catalog().script(refs->effect_use)) {
        constexpr std::size_t kMaxPendingSkillEffects = 64;
        if (m_pendingSkillEffects.size() >= kMaxPendingSkillEffects) {
            m_pendingSkillEffects.erase(m_pendingSkillEffects.begin());
        }
        m_pendingSkillEffects.push_back(PendingSkillEffect{
            skill_id, target_object_id, now_ms, source_object_id});
        return;
    }
    if (!m_effectRuntime.start(refs->effect_use, source_object_id,
                               target_object_id, now_ms,
                               m_effectTickPerFrameMs)) {
        MLOG_WARN("CInGameState effect start rejected skill=%u effect=%s",
                  static_cast<unsigned>(skill_id), refs->effect_use.c_str());
    }
}

std::vector<RuntimeEffectEvent> CInGameState::drain_runtime_effect_events() noexcept {
    std::vector<RuntimeEffectEvent> events;
    events.swap(m_runtimeEffectEvents);
    return events;
}

void CInGameState::open_shop(std::uint32_t npc_id) {
    if (!m_inGame || (npc_id == 0 && !is_connected())) return;
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
        // The destination is route data, not a client-side constant.  Do not
        // send a fabricated map number when the selected profile has not
        // supplied the authoritative MapChange table.
        if (!m_mapChangeTargetResolver) {
            MLOG_WARN("CInGameState: MapChange NPC %u has no route resolver",
                      npc_id);
            return;
        }
        const auto target = m_mapChangeTargetResolver(npc_id, m_mapNum);
        if (!target || *target == 0 || *target == m_mapNum) {
            MLOG_WARN("CInGameState: MapChange NPC %u has no valid destination",
                      npc_id);
            return;
        }
        msg.payload.resize(4, 0);
        put_u16(msg.payload, 0, *target);
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
    set_chat_open(false);
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

bool CInGameState::request_party_create(std::uint8_t option) {
    if (!m_inGame || !is_connected() || m_playerId == 0) return false;
    return m_pEngine->agent_session().send(
               make_party_create_message(m_playerId, option)) == mxh::net::NetError::Ok;
}

bool CInGameState::request_party_invite(std::uint32_t target_player_id) {
    if (!m_inGame || !is_connected() || m_playerId == 0 || m_partyId == 0 ||
        target_player_id == 0) return false;
    std::array<std::uint8_t, 8> payload{};
    std::memcpy(payload.data(), &m_partyId, sizeof(m_partyId));
    std::memcpy(payload.data() + 4, &target_player_id, sizeof(target_player_id));
    return m_pEngine->agent_session().send(make_party_request_message(
               m_playerId, mxh::proto::PartyProtocol::AddSyn, payload)) ==
           mxh::net::NetError::Ok;
}

bool CInGameState::accept_party_invite() {
    if (!m_inGame || !is_connected() || m_playerId == 0 ||
        m_pendingPartyInviteId == 0) return false;
    std::array<std::uint8_t, 4> payload{};
    std::memcpy(payload.data(), &m_pendingPartyInviteId, sizeof(m_pendingPartyInviteId));
    return m_pEngine->agent_session().send(make_party_request_message(
               m_playerId, mxh::proto::PartyProtocol::InviteAcceptSyn, payload)) ==
           mxh::net::NetError::Ok;
}

bool CInGameState::request_guild_create(std::string_view name) {
    if (!m_inGame || !is_connected() || m_playerId == 0 || name.empty() || name.size() > 16) {
        return false;
    }
    std::vector<std::uint8_t> payload(name.begin(), name.end());
    return m_pEngine->agent_session().send(make_guild_request_message(
               m_playerId, mxh::proto::GuildProtocol::CreateSyn, payload)) ==
           mxh::net::NetError::Ok;
}

bool CInGameState::request_guild_invite(std::uint32_t target_player_id) {
    if (!m_inGame || !is_connected() || m_playerId == 0 || m_guildId == 0 ||
        target_player_id == 0) return false;
    std::array<std::uint8_t, 8> payload{};
    std::memcpy(payload.data(), &m_guildId, sizeof(m_guildId));
    std::memcpy(payload.data() + 4, &target_player_id, sizeof(target_player_id));
    return m_pEngine->agent_session().send(make_guild_request_message(
               m_playerId, mxh::proto::GuildProtocol::AddMemberSyn, payload)) ==
           mxh::net::NetError::Ok;
}

bool CInGameState::accept_guild_invite() {
    if (!m_inGame || !is_connected() || m_playerId == 0 ||
        m_pendingGuildInviteId == 0) return false;
    std::array<std::uint8_t, 4> payload{};
    std::memcpy(payload.data(), &m_pendingGuildInviteId,
                sizeof(m_pendingGuildInviteId));
    return m_pEngine->agent_session().send(make_guild_request_message(
               m_playerId, mxh::proto::GuildProtocol::InviteAccept, payload)) ==
           mxh::net::NetError::Ok;
}

void CInGameState::fail_with(const std::string& reason) {
    if (m_failed) return;
    m_failed = true;
    m_failureReason = reason;
    MLOG_ERROR("CInGameState: %s", reason.c_str());
}

} // namespace mxh::client
