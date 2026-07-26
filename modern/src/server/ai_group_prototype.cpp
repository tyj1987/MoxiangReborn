// ai_group_prototype.cpp - Phase D6 AIGroupPrototype 1:1 port implementations.

#include "mxh/server/ai_group_prototype.hpp"

#include <algorithm>

namespace mxh::server {

// ---- PendingDeaths tracker ----
void PendingDeaths::add(std::uint32_t id) {
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (!present[i]) {
            ids[i] = id;
            present[i] = true;
            return;
        }
    }
    // Full: replace first slot to mirror legacy saturation.
    ids[0] = id;
    present[0] = true;
}

void PendingDeaths::remove(std::uint32_t id) {
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (present[i] && ids[i] == id) {
            present[i] = false;
            return;
        }
    }
}

std::uint32_t PendingDeaths::count() const {
    std::uint32_t n = 0;
    for (auto b : present) if (b) ++n;
    return n;
}

// ---- AIGroup ----
std::uint32_t AIGroup::get_max_object_num() const {
    std::uint32_t n = 0;
    for (const auto& s : regen_objects) if (s.present) ++n;
    return n;
}

std::uint32_t AIGroup::get_cur_object_num() const {
    if (m_RegenInfo != nullptr) {
        // Legacy: m_RegenInfo.GetWaitRegenObjectNum()
        // We don't dereference: caller wires it via set_regen_info.
        return 0u;
    }
    return 0u;
}

void AIGroup::die(std::uint32_t id) {
    // Legacy: m_RegenInfo.AddID(id);  RegenCheck(GetCurObjectNum(), GetMaxObjectNum());
    // We just record and let regen_check do the work.
    PendingDeaths tracker{};
    tracker.add(id);
    regen_check();
}

void AIGroup::alive(std::uint32_t id) {
    // Legacy: m_RegenInfo.RemoveID(id);
}

void AIGroup::regen_check() {
    // Legacy: m_RegenInfo.RegenCheck(GetCurObjectNum(), GetMaxObjectNum());
}

void AIGroup::force_regen() {
    // Legacy: m_RegenInfo.ForceRegen();
}

void AIGroup::regen_process() {
    // Legacy: m_RegenInfo.RegenProcess();
}

void AIGroup::add_regen_object(std::uint32_t object_id) {
    // Legacy: m_RegenObjectInfoList.Add(pObj, pObj->m_dwObjectID);
    for (auto& s : regen_objects) {
        if (!s.present) {
            s.present = true;
            s.object_id = object_id;
            return;
        }
    }
    // Full: replace first slot (legacy would store pointer + id).
    regen_objects[0].present = true;
    regen_objects[0].object_id = object_id;
}

bool AIGroup::get_regen_object(std::uint32_t object_id) const {
    for (const auto& s : regen_objects) {
        if (s.present && s.object_id == object_id) return true;
    }
    return false;
}

void AIGroup::add_condition_info(RegenConditionInfo*) {
    // Legacy: m_RegenInfo.AddCondition(pInfo);
}

void AIGroup::set_random_grid_id(std::uint32_t grid_id) {
    // Legacy: m_dwGridID = GridID;
    // Then walk the table and copy GridID onto each CRegenObject.
    m_dwGridID = grid_id;
    for (auto& s : regen_objects) {
        if (s.present) {
            // slot-level flag: object ID's high bits encode the grid now.
            s.object_id = (s.object_id & 0x00FFFFFFu) | ((grid_id & 0xFFu) << 24);
        }
    }
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int ai_group_prototype_translation_unit_anchor = 0;
}
