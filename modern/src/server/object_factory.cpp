// object_factory.cpp - Phase D6 ObjectFactory 1:1 port implementations.

#include "mxh/server/object_factory.hpp"

#include <cstddef>

namespace mxh::server {

std::uint32_t ObjectFactoryState::total_active() const {
    std::uint32_t sum = 0;
    for (auto v : active) sum += v;
    return sum;
}

std::uint32_t ObjectFactoryState::total_cap() const {
    std::uint32_t sum = 0;
    for (auto v : cap) sum += v;
    return sum;
}

namespace {
constexpr std::size_t kCount = static_cast<std::size_t>(ObjectKind::Count);
const std::array<std::uint32_t, kCount> kDefaultCaps = {
    MAX_TOTAL_PLAYER_NUM,
    MAX_TOTAL_PET_NUM,
    MAX_TOTAL_MONSTER_NUM,
    MAX_TOTAL_NPC_NUM,
    MAX_TOTAL_TACTIC_NUM,
    MAX_TOTAL_BOSSMONSTER_NUM,
    MAX_MAPOBJECT_NUM,
    MAX_TOTAL_TITAN_NUM,
    MAX_TITANINFO_NUM,
    MAX_TOTAL_BOSSMONSTER_NUM,
    MAX_TOTAL_BOSSMONSTER_NUM * 10u,
};
}  // namespace

void object_factory_init(ObjectFactoryState& s) {
    s.active.fill(0);
    for (std::size_t i = 0; i < kCount; ++i) {
        s.cap[i] = kDefaultCaps[i];
    }
    s.initialized = true;
}

void object_factory_release(ObjectFactoryState& s) {
    s.active.fill(0);
    s.cap.fill(0);
    s.initialized = false;
}

bool object_factory_can_make(const ObjectFactoryState& s, ObjectKind kind) {
    const auto idx = static_cast<std::size_t>(kind);
    if (idx >= kCount) return false;
    return s.active[idx] < s.cap[idx];
}

bool object_factory_make_new(ObjectFactoryState& s, ObjectKind kind) {
    const auto idx = static_cast<std::size_t>(kind);
    if (idx >= kCount) return false;
    if (s.active[idx] >= s.cap[idx]) return false;
    ++s.active[idx];
    return true;
}

void object_factory_release_object(ObjectFactoryState& s, ObjectKind kind) {
    const auto idx = static_cast<std::size_t>(kind);
    if (idx >= kCount) return;
    if (s.active[idx] > 0) --s.active[idx];
}

MakeObjectResult object_factory_make_new_ex(ObjectFactoryState& s,
                                            std::uint8_t kind_byte) {
    if (!s.initialized) return MakeObjectResult::NotInit;
    // Legacy kinds: map incoming byte (EObjectKind) to ObjectKind.
    // Approximate mapping used by legacy MakeNewObject dispatch.
    ObjectKind kind = ObjectKind::Monster;
    switch (kind_byte) {
        case 1:  kind = ObjectKind::Player; break;
        case 2:  kind = ObjectKind::Npc; break;
        case 8:  kind = ObjectKind::Tactic; break;
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 40: kind = ObjectKind::Monster; break;
        case 41: kind = ObjectKind::Monster; break;  // TitanMonster uses monster pool
        case 64:
        case 65: kind = ObjectKind::MapObject; break;
        case 128: kind = ObjectKind::Pet; break;
        case 129: kind = ObjectKind::Titan; break;
        default: return MakeObjectResult::UnknownKind;
    }
    if (!object_factory_can_make(s, kind)) return MakeObjectResult::PoolFull;
    object_factory_make_new(s, kind);
    return MakeObjectResult::Created;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int object_factory_translation_unit_anchor = 0;
}
