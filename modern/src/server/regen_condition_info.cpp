// regen_condition_info.cpp - Phase D6 RegenConditionInfo 1:1 port.

#include "mxh/server/regen_condition_info.hpp"

namespace mxh::server {

void regen_condition_info_init(RegenConditionInfo& c) {
    c.dwTargetGroupID  = 0u;
    c.fRemainderRatio  = 0.0f;
    c.dwStartRegenTick = 0u;
    c.dwRegenDelay     = 0u;
    c.bRegen           = false;
}

bool regen_condition_info_should_regen(const RegenConditionInfo& c,
                                       std::uint32_t current_tick,
                                       std::uint32_t alive_in_group) {
    if (!c.bRegen) return false;
    if (alive_in_group > 0) return false;
    // dwStartRegenTick + dwRegenDelay <= current_tick triggers regen.
    if (c.dwRegenDelay == 0u) return true;
    const std::uint64_t trigger_tick =
        static_cast<std::uint64_t>(c.dwStartRegenTick) +
        static_cast<std::uint64_t>(c.dwRegenDelay);
    return trigger_tick <= static_cast<std::uint64_t>(current_tick);
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int regen_condition_info_translation_unit_anchor = 0;
}
