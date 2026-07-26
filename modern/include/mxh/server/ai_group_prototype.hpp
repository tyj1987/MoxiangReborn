// ai_group_prototype.hpp - Phase D6 AIGroupPrototype 1:1 port.
//
// Source-of-truth: legacy [Server]Map/AIGroupPrototype.h + .cpp.
// Mirrors legacy CAIGroup base class.  The legacy class owns a
// CYHHashTable<CRegenObject> and a CGroupRegenInfo; modern exposes
// the per-group state and a tracker for alive object IDs.
//
// AddRegenObject / GetRegenObject are exposed via the
// RegenObjectSlot entry list which models the hash-table pointer
// table (capacity 10 in the legacy default initializer).

#pragma once

#include <array>
#include <cstdint>

namespace mxh::server {

// Legacy default capacity for m_RegenObjectInfoList.Initialize.
inline constexpr std::uint32_t AIGROUP_REGEN_HASH_CAPACITY = 10u;

struct GroupRegenInfo;
struct RegenObject;
struct RegenConditionInfo;

// ---- Per-slot entry in the regen object table ----
struct RegenObjectSlot {
    bool present  = false;
    std::uint32_t object_id = 0;
};

// ---- AI Group state (mirror CAIGroup) ----
struct AIGroup {
    std::uint32_t m_dwGroupID = 0;
    std::uint32_t m_dwGridID  = 0;
    std::array<RegenObjectSlot, AIGROUP_REGEN_HASH_CAPACITY> regen_objects{};
    GroupRegenInfo* m_RegenInfo = nullptr;  // owned by caller

    // GetMaxObjectNum returns count of registered slots.
    std::uint32_t get_max_object_num() const;

    // GetCurObjectNum returns count of pending (dead/awaiting regen) IDs.
    std::uint32_t get_cur_object_num() const;

    // Die / Alive lifecycle: legacy m_RegenInfo.AddID / RemoveID.
    void die(std::uint32_t id);
    void alive(std::uint32_t id);

    // Regen flow: legacy m_RegenInfo.{RegenCheck, ForceRegen, RegenProcess}.
    // Modern dispatches via the registered m_RegenInfo.
    void regen_check();
    void force_regen();
    void regen_process();

    // AddRegenObject / GetRegenObject map the legacy hash table.
    void add_regen_object(std::uint32_t object_id);
    bool get_regen_object(std::uint32_t object_id) const;

    // AddConditionInfo / SetRandomGridID.
    void add_condition_info(RegenConditionInfo* info);
    void set_random_grid_id(std::uint32_t grid_id);
};

// ---- Pending death tracker (mirror legacy m_RegenInfo AddID/RemoveID) ----
// AIGroup owns its own death-ID tracker when m_RegenInfo is nullptr.
struct PendingDeaths {
    std::array<std::uint32_t, AIGROUP_REGEN_HASH_CAPACITY> ids{};
    std::array<bool, AIGROUP_REGEN_HASH_CAPACITY> present{};
    void add(std::uint32_t id);
    void remove(std::uint32_t id);
    std::uint32_t count() const;
};

}  // namespace mxh::server
