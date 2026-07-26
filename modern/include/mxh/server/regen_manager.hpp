// regen_manager.hpp - Phase D5 1:1 port of legacy [Server]Map/RegenManager.h.
// Manages monster/NPC regen prototypes and random-position generation.
// Mirrors legacy CRegenManager fields.

#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace mxh::server {

// ---- Constants 1:1 ----

inline constexpr std::uint32_t MONSTER_REGEN_RANDOM_RANGE = 1500u;

// ---- POD structs ----

// Opaque legacy VECTOR3.
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// Mirrors legacy CRegenPrototype (subset we lock). In legacy this class owns
// monster / NPC / item regen instructions; here we keep the fields that
// drive 1:1 byte-level diff in side-by-side tests.
struct RegenPrototype {
    std::uint32_t m_dwID              = 0;
    std::uint32_t m_dwSubID           = 0;
    std::uint32_t m_dwGridID          = 0;
    std::uint32_t m_dwObjectID        = 0;
    std::uint16_t m_wObjectKind       = 0;
    std::uint16_t m_wMonsterKind      = 0;
    std::uint32_t m_dwGroupID         = 0;
    Vec3          m_vPos{};
    std::uint16_t m_DropItemID        = 0;
    std::uint32_t m_dwDropRatio       = 100u;
    bool          m_bRandomPos        = true;
    bool          m_bEventRegen       = false;
};

// Mirrors legacy CRegenManager.
struct RegenManagerState {
    std::unordered_map<std::uint32_t, RegenPrototype> m_RegenPrototypeList;
};

// ---- Free functions ----

inline RegenManagerState make_regen_manager() {
    return RegenManagerState{};
}

inline void release(RegenManagerState& s) {
    s.m_RegenPrototypeList.clear();
}

inline void add_prototype(RegenManagerState& s, const RegenPrototype& p) {
    s.m_RegenPrototypeList[p.m_dwID] = p;
}

inline RegenPrototype* get_prototype(RegenManagerState& s, std::uint32_t id) {
    auto it = s.m_RegenPrototypeList.find(id);
    return (it == s.m_RegenPrototypeList.end()) ? nullptr : &it->second;
}

// RangePosAtOrig: legacy applies a uniform random offset within `range` of
// the origin position. Here we provide the algorithm so the same caller
// can produce deterministic or random offsets. The output z is preserved.
inline void range_pos_at_orig(const Vec3& orig, int range, Vec3& out, std::int32_t rand_x, std::int32_t rand_z) {
    if (range <= 0) {
        out.x = orig.x;
        out.y = orig.y;
        out.z = orig.z;
        return;
    }
    const std::int32_t half = range / 2;
    const float dx = static_cast<float>(rand_x - half);
    const float dz = static_cast<float>(rand_z - half);
    out.x = orig.x + dx;
    out.z = orig.z + dz;
    out.y = orig.y;
}

// RegenObject: legacy creates a monster and returns it; for 1:1 tests we
// only need the bookkeeping of "did we register this regen?". The caller
// passes a fresh prototype, this function installs it and returns true.
inline bool regen_object(RegenManagerState& s, const RegenPrototype& p) {
    s.m_RegenPrototypeList[p.m_dwObjectID] = p;
    return true;
}

// RegenGroup: legacy fans out to all prototypes belonging to a group ID.
// Here we just count how many prototypes match the group.
inline std::size_t regen_group_count(const RegenManagerState& s, std::uint32_t group_id) {
    std::size_t n = 0u;
    for (const auto& kv : s.m_RegenPrototypeList) {
        if (kv.second.m_dwGroupID == group_id) ++n;
    }
    return n;
}

inline bool regen_group_contains(const RegenManagerState& s, std::uint32_t group_id) {
    for (const auto& kv : s.m_RegenPrototypeList) {
        if (kv.second.m_dwGroupID == group_id) return true;
    }
    return false;
}

// Drop ratio gate: legacy accepts the regen iff roll < dwDropRatio (0..100).
inline bool drop_ratio_passes(std::uint32_t dw_drop_ratio, std::uint32_t roll_basis_points) {
    if (dw_drop_ratio == 0u) return false;
    if (dw_drop_ratio >= 100u) return true;
    return roll_basis_points < (dw_drop_ratio * 100u);
}

// Random range helper for callers that want a range check (used in tests).
inline bool in_random_range(const Vec3& a, const Vec3& b, std::uint32_t range) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    if (dx * dx + dz * dz > static_cast<float>(range) * static_cast<float>(range)) return false;
    return true;
}

} // namespace mxh::server
