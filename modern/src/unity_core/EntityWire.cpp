#include "../client/CInGameState.hpp"
#include <cstring>

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
}
