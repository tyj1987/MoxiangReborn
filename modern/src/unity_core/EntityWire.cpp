#include "../client/CInGameState.hpp"
#include <cstring>
#include <cmath>

namespace mxh::client {
namespace {
std::uint16_t u16(const std::uint8_t* p) { return static_cast<std::uint16_t>(p[0] | (p[1] << 8)); }
std::uint32_t u32(const std::uint8_t* p) { return static_cast<std::uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24)); }
}

std::optional<MonsterAddInfo> parse_legacy_monster_add(std::span<const std::uint8_t> payload) {
    if (payload.size() < 64) return std::nullopt;
    MonsterAddInfo info;
    info.object_id = u32(payload.data()); info.user_id = u32(payload.data() + 4);
    std::memcpy(info.name, payload.data() + 8, 17);
    info.current_life = u32(payload.data() + 35); info.current_shield = u32(payload.data() + 39);
    info.monster_kind = u16(payload.data() + 43); info.group = u16(payload.data() + 45);
    info.map_num = u16(payload.data() + 47); info.position_x = u16(payload.data() + 49);
    info.position_z = u16(payload.data() + 51);
    return info;
}

std::optional<NpcInfo> parse_legacy_npc_add(std::span<const std::uint8_t> payload) {
    if (payload.size() < 64) return std::nullopt;
    NpcInfo info; info.npc_id = u32(payload.data());
    std::memcpy(info.name, payload.data() + 8, 17);
    info.npc_kind = u16(payload.data() + 35); info.position_x = u16(payload.data() + 45);
    info.position_z = u16(payload.data() + 47);
    return info;
}

std::optional<std::pair<std::uint32_t, std::uint32_t>>
parse_monster_life_payload(std::span<const std::uint8_t> payload) {
    if (payload.size() < 8) return std::nullopt;
    return std::make_pair(u32(payload.data()), u32(payload.data() + 4));
}

std::optional<GroundDropInfo> parse_legacy_ground_drop(std::span<const std::uint8_t> payload) {
    if (payload.size() < 20) return std::nullopt;
    GroundDropInfo drop;
    drop.object_id = u32(payload.data());
    drop.source_monster_id = u32(payload.data() + 4);
    drop.item_id = u16(payload.data() + 8);
    drop.count = u16(payload.data() + 10);
    std::memcpy(&drop.position_x, payload.data() + 12, sizeof(float));
    std::memcpy(&drop.position_z, payload.data() + 16, sizeof(float));
    if (drop.object_id == 0 || drop.item_id == 0 || drop.count == 0 || !std::isfinite(drop.position_x) || !std::isfinite(drop.position_z)) return std::nullopt;
    return drop;
}

mxh::net::Message make_pickup_message(std::uint32_t player_id,
                                      std::uint32_t drop_object_id) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupSyn);
    message.header.object_id = player_id;
    message.payload.resize(4);
    message.payload[0] = static_cast<std::uint8_t>(drop_object_id);
    message.payload[1] = static_cast<std::uint8_t>(drop_object_id >> 8);
    message.payload[2] = static_cast<std::uint8_t>(drop_object_id >> 16);
    message.payload[3] = static_cast<std::uint8_t>(drop_object_id >> 24);
    return message;
}

mxh::net::Message make_quest_message(std::uint32_t player_id,
                                     mxh::proto::QuestProtocol protocol,
                                     std::uint16_t quest_id) {
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    message.header.protocol = static_cast<std::uint8_t>(protocol);
    message.header.object_id = player_id;
    message.payload = {static_cast<std::uint8_t>(quest_id), static_cast<std::uint8_t>(quest_id >> 8)};
    return message;
}

std::optional<PickupAckInfo> parse_pickup_ack_payload(std::span<const std::uint8_t> payload) {
    if (payload.size() < 8) return std::nullopt;
    PickupAckInfo ack;
    ack.drop_id = u32(payload.data());
    ack.item_id = u16(payload.data() + 4);
    ack.count = u16(payload.data() + 6);
    if (ack.drop_id == 0 || ack.item_id == 0 || ack.count == 0) return std::nullopt;
    return ack;
}
}
