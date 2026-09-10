#include "ClientWire.hpp"

#include "mxh/game/hero_total_layout.hpp"
#include "mxh/proto/protocol.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace mxh::client {
namespace {
std::uint16_t get_u16(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0]) |
           (static_cast<std::uint16_t>(p[1]) << 8);
}

std::uint32_t get_u32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

std::uint64_t get_u64(const std::uint8_t* p) noexcept {
    return static_cast<std::uint64_t>(get_u32(p)) |
           (static_cast<std::uint64_t>(get_u32(p + 4)) << 32);
}

void put_u16(std::vector<std::uint8_t>& out, std::size_t offset,
             std::uint16_t value) noexcept {
    out[offset] = static_cast<std::uint8_t>(value);
    out[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

void put_u32(std::vector<std::uint8_t>& out, std::size_t offset,
             std::uint32_t value) noexcept {
    for (std::size_t i = 0; i < 4; ++i)
        out[offset + i] = static_cast<std::uint8_t>(value >> (i * 8));
}

void put_f32(std::vector<std::uint8_t>& out, std::size_t offset,
             float value) noexcept {
    std::uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    put_u32(out, offset, bits);
}
}

std::vector<std::uint8_t> legacy_request_login_payload(
    std::uint32_t auth_key, const std::string& user_id,
    const std::string& password) {
    std::vector<std::uint8_t> out(38, 0);
    out[0] = static_cast<std::uint8_t>(auth_key);
    out[1] = static_cast<std::uint8_t>(auth_key >> 8);
    out[2] = static_cast<std::uint8_t>(auth_key >> 16);
    out[3] = static_cast<std::uint8_t>(auth_key >> 24);
    const auto copy_padded = [](std::uint8_t* dst, const std::string& value) {
        const auto count = std::min<std::size_t>(value.size(), 17);
        if (count != 0) std::memcpy(dst, value.data(), count);
    };
    copy_padded(out.data() + 4, user_id);
    copy_padded(out.data() + 21, password);
    return out;
}

std::optional<LegacyLoginAck> parse_legacy_login_ack(
    std::span<const std::uint8_t> payload) {
    if (payload.size() < 23) return std::nullopt;
    LegacyLoginAck ack;
    std::size_t end = 16;
    for (std::size_t i = 0; i < 16; ++i) {
        if (payload[i] == 0) { end = i; break; }
    }
    ack.agent_addr.assign(reinterpret_cast<const char*>(payload.data()), end);
    ack.agent_port = get_u16(payload.data() + 16);
    ack.user_idx = get_u32(payload.data() + 18);
    ack.user_level = payload[22];
    return ack;
}

bool is_listed_character(std::span<const CharacterSlot> slots,
                         std::uint32_t chrid) noexcept {
    return chrid != 0 && std::any_of(slots.begin(), slots.end(),
        [chrid](const CharacterSlot& slot) {
            return slot.valid && slot.chrid == chrid;
        });
}

std::vector<std::uint8_t> legacy_character_list_syn_payload(
    std::uint32_t user_id, std::uint32_t dist_auth_key) {
    std::vector<std::uint8_t> out(8);
    for (std::size_t i = 0; i < 4; ++i) {
        out[i] = static_cast<std::uint8_t>(user_id >> (i * 8));
        out[4 + i] = static_cast<std::uint8_t>(dist_auth_key >> (i * 8));
    }
    return out;
}

std::vector<std::uint8_t> legacy_character_select_syn_payload(
    std::uint16_t channel) {
    return {static_cast<std::uint8_t>(channel),
            static_cast<std::uint8_t>(channel >> 8)};
}

bool valid_legacy_character_name_bytes(std::string_view name) noexcept {
    if (name.size() < 4 || name.size() > 16) return false;
    for (const auto byte : name) {
        const auto value = static_cast<unsigned char>(byte);
        if (value < 0x20u || value == 0x7fu) return false;
    }
    return true;
}

std::optional<LegacyCharacterMakeParams> legacy_china_character_make_params(
    std::string wire_name, std::uint8_t sex_type, std::uint8_t hair_type,
    std::uint8_t face_type, std::uint8_t cloth_option,
    std::uint8_t boot_option, std::uint8_t weapon_option) {
    // Values are the exact CHINA CharMake_SelectOption.bin baseline already
    // locked by CharMakeOptions tests. Unity supplies indices, never item IDs.
    static constexpr std::array<std::uint16_t, 2> kCloth{23000, 23010};
    static constexpr std::array<std::uint16_t, 2> kBoot{27000, 27010};
    static constexpr std::array<std::uint16_t, 6> kWeapon{
        11000, 13000, 15000, 17000, 19000, 21000};
    if (!valid_legacy_character_name_bytes(wire_name) || sex_type > 1 ||
        hair_type > 4 || face_type > 4 || cloth_option >= kCloth.size() ||
        boot_option >= kBoot.size() || weapon_option >= kWeapon.size())
        return std::nullopt;
    LegacyCharacterMakeParams out;
    out.wire_name = std::move(wire_name);
    out.sex_type = sex_type;
    out.hair_type = hair_type;
    out.face_type = face_type;
    out.worn_item_index[1] = kWeapon[weapon_option];
    out.worn_item_index[2] = kCloth[cloth_option];
    out.worn_item_index[3] = kBoot[boot_option];
    return out;
}

std::optional<std::vector<std::uint8_t>> legacy_character_make_syn_payload(
    const LegacyCharacterMakeParams& params, std::uint32_t user_id) {
    if (!valid_legacy_character_name_bytes(params.wire_name) ||
        params.sex_type > 1 || params.hair_type > 4 || params.face_type > 4)
        return std::nullopt;
    std::vector<std::uint8_t> out(59, 0);
    const auto name_size = params.wire_name.size();
    if (name_size != 0)
        std::memcpy(out.data(), params.wire_name.data(), name_size);
    put_u32(out, 17, user_id);
    out[21] = params.sex_type;
    out[22] = params.body_type;
    out[23] = params.hair_type;
    out[24] = params.face_type;
    out[25] = params.start_area;
    for (std::size_t i = 0; i < params.worn_item_index.size(); ++i)
        put_u16(out, 30 + i * 2, params.worn_item_index[i]);
    out[50] = params.standing_array_num;
    put_f32(out, 51, params.height);
    put_f32(out, 55, params.width);
    return out;
}

mxh::net::Message make_move_message(std::uint32_t player_id,
    mxh::proto::MoveProtocol protocol, std::uint16_t x, std::uint16_t z) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    message.header.protocol = static_cast<std::uint8_t>(protocol);
    message.header.object_id = player_id;
    message.payload.resize(4);
    put_u16(message.payload, 0, x);
    put_u16(message.payload, 2, z);
    return message;
}

std::optional<std::pair<std::uint16_t, std::uint16_t>>
parse_move_payload(std::span<const std::uint8_t> payload) {
    if (payload.size() < 4) return std::nullopt;
    return std::make_pair(get_u16(payload.data()), get_u16(payload.data() + 2));
}

std::vector<std::uint8_t> legacy_character_remove_syn_payload(
    std::uint32_t character_id) {
    return {static_cast<std::uint8_t>(character_id),
            static_cast<std::uint8_t>(character_id >> 8),
            static_cast<std::uint8_t>(character_id >> 16),
            static_cast<std::uint8_t>(character_id >> 24)};
}

mxh::net::Message legacy_character_disconnect_syn_message() {
    mxh::net::Message out{};
    out.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::DisconnectSyn);
    return out;
}

std::optional<std::vector<CharacterSlot>> parse_legacy_character_list_ack(
    std::span<const std::uint8_t> payload) {
    constexpr std::size_t kMaxSlots = 5;
    constexpr std::size_t kBaseOff = 14;
    constexpr std::size_t kSlotSize = 35;
    constexpr std::size_t kNameOff = 8;
    constexpr std::size_t kNameSize = 17;
    constexpr std::size_t kTotalOff = 189;
    constexpr std::size_t kTotalSize = 140;
    constexpr std::size_t kWireSize = kTotalOff + kMaxSlots * kTotalSize;
    // The legacy ACK always serializes all five base and total-info slots,
    // even when CharNum is zero.  Accepting a prefix would turn a truncated
    // TCP/protocol frame into an apparently valid character list.
    if (payload.size() != kWireSize) return std::nullopt;
    std::int32_t advertised_count = 0;
    std::memcpy(&advertised_count, payload.data(), sizeof(advertised_count));
    if (advertised_count < 0 || advertised_count > static_cast<std::int32_t>(kMaxSlots))
        return std::nullopt;
    const auto count = static_cast<std::size_t>(advertised_count);
    std::array<std::uint32_t, kMaxSlots> character_ids{};
    for (std::size_t i = 0; i < kMaxSlots; ++i) {
        character_ids[i] = get_u32(payload.data() + kBaseOff + i * kSlotSize);
        if (i < count) {
            if (character_ids[i] == 0) return std::nullopt;
            for (std::size_t previous = 0; previous < i; ++previous) {
                if (character_ids[previous] == character_ids[i]) return std::nullopt;
            }
        } else if (character_ids[i] != 0) {
            return std::nullopt;
        }
    }
    std::vector<CharacterSlot> out(kMaxSlots);
    for (std::size_t i = 0; i < count; ++i) {
        const auto base = kBaseOff + i * kSlotSize;
        out[i].chrid = character_ids[i];
        out[i].valid = true;
        const auto* begin = payload.data() + base + kNameOff;
        const auto* end = std::find(begin, begin + kNameSize, std::uint8_t{0});
        out[i].name.assign(reinterpret_cast<const char*>(begin),
                           reinterpret_cast<const char*>(end));
        const auto total = kTotalOff + i * kTotalSize;
        out[i].gender = payload[total + 16];
        out[i].face_type = payload[total + 17];
        out[i].hair_type = payload[total + 18];
        for (std::size_t item = 0; item < out[i].weared_item_idx.size(); ++item)
            out[i].weared_item_idx[item] = get_u16(payload.data() + total + 19 + item * 2);
        out[i].level = get_u16(payload.data() + total + 40);
        out[i].map_num = get_u16(payload.data() + total + 42);
    }
    return out;
}

std::optional<std::uint16_t> parse_legacy_character_select_ack(
    std::span<const std::uint8_t> payload) {
    if (payload.empty()) return std::nullopt;
    return static_cast<std::uint16_t>(payload[0]);
}

std::optional<GameInInfo> parse_legacy_gamein_ack(
    std::span<const std::uint8_t> payload) {
    if (payload.size() < mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE) return std::nullopt;
    GameInInfo info;
    info.player_id = get_u32(payload.data());
    info.user_id = get_u32(payload.data() + 4);
    std::size_t name_end = 17;
    for (std::size_t i = 0; i < 17; ++i) {
        if (payload[8 + i] == 0) { name_end = i; break; }
    }
    info.name.assign(reinterpret_cast<const char*>(payload.data() + 8), name_end);
    info.life = get_u32(payload.data() + 35);
    info.max_life = get_u32(payload.data() + 39);
    info.gender = payload[51];
    info.face_type = payload[52];
    info.hair_type = payload[53];
    for (std::size_t i = 0; i < info.weared_item_idx.size(); ++i)
        info.weared_item_idx[i] = get_u16(payload.data() + 54 + i * 2);
    info.level = get_u16(payload.data() + 75);
    info.map_num = get_u16(payload.data() + 77);
    const auto hero = mxh::game::HERO_TOTAL_HERO_OFFSET;
    info.gen_gol = get_u16(payload.data() + hero);
    info.min_chub = get_u16(payload.data() + hero + 2);
    info.che_ryuk = get_u16(payload.data() + hero + 4);
    info.sim_mek = get_u16(payload.data() + hero + 6);
    info.mp = get_u32(payload.data() + hero + 8);
    info.max_mp = get_u32(payload.data() + hero + 12);
    // Packed legacy HERO_TOTALINFO: EXPTYPE starts at +22 (8 bytes) and
    // MONEYTYPE starts at +36.  These offsets match MapServer production.
    info.exp = get_u64(payload.data() + hero + 22);
    info.money = get_u32(payload.data() + hero + 36);
    info.position_x = get_u16(payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET);
    info.position_z = get_u16(payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET + 2);
    const auto time = mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET;
    info.server_year = get_u16(payload.data() + time);
    info.server_month = get_u16(payload.data() + time + 2);
    info.server_day = get_u16(payload.data() + time + 6);
    info.server_hour = get_u16(payload.data() + time + 8);
    info.mugong = parse_legacy_mugong_total(payload);
    info.items = parse_legacy_item_total(payload);
    return info;
}

std::array<MugongInfo, kMugongSlotCount> parse_legacy_mugong_total(
    std::span<const std::uint8_t> payload) {
    std::array<MugongInfo, kMugongSlotCount> out{};
    constexpr std::size_t kSlotBytes = 18;
    if (payload.size() < mxh::game::HERO_TOTAL_MUGONG_OFFSET + out.size() * kSlotBytes)
        return out;
    const auto* p = payload.data() + mxh::game::HERO_TOTAL_MUGONG_OFFSET;
    for (std::size_t i = 0; i < out.size(); ++i) {
        const auto off = i * kSlotBytes;
        out[i].db_idx = get_u32(p + off);
        out[i].icon_idx = get_u16(p + off + 4);
        out[i].position = get_u16(p + off + 6);
        out[i].exp = get_u32(p + off + 8);
        out[i].sung = p[off + 12];
        out[i].wear = p[off + 13];
        out[i].quick_position = get_u16(p + off + 14);
        out[i].option_idx = get_u16(p + off + 16);
    }
    return out;
}

mxh::game::ItemTotalInfo parse_legacy_item_total(
    std::span<const std::uint8_t> payload) {
    mxh::game::ItemTotalInfo out{};
    if (payload.size() >= mxh::game::HERO_TOTAL_ITEM_OFFSET + sizeof(out))
        std::memcpy(&out, payload.data() + mxh::game::HERO_TOTAL_ITEM_OFFSET, sizeof(out));
    return out;
}

mxh::net::Message make_legacy_gamein_syn_message(std::uint32_t player_id) {
    mxh::net::Message out{};
    out.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    out.header.object_id = player_id;
    return out;
}

mxh::net::Message make_legacy_gameout_syn_message(std::uint32_t player_id) {
    mxh::net::Message out{};
    out.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
    out.header.object_id = player_id;
    return out;
}

}  // namespace mxh::client
