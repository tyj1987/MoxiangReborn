// regen_condition_info.hpp - Phase D6 RegenConditionInfo 1:1 port.
//
// Source-of-truth: legacy [Server]Map/RegenConditionInfo.h + .cpp.
// Mirrors legacy CRegenConditionInfo POD fields used by
// CGroupRegenInfo + CAISystem to schedule monster regen.
// Friend classes are dropped; access is purely via the POD struct.

#pragma once

#include <cstdint>

namespace mxh::server {

// Mirror of legacy CRegenConditionInfo POD fields.
struct RegenConditionInfo {
    std::uint32_t dwTargetGroupID  = 0;
    float         fRemainderRatio  = 0.0f;
    std::uint32_t dwStartRegenTick = 0;
    std::uint32_t dwRegenDelay     = 0;
    bool          bRegen           = false;
};

// Default-construct in the legacy style.
void regen_condition_info_init(RegenConditionInfo& c);

// bRegen evaluates the "should we regen now" condition.
// Legacy gates on (target group ID + regen-on flag); modern takes
// the same inputs as parameters and returns true if bRegen would
// trigger a regen tick.
bool regen_condition_info_should_regen(const RegenConditionInfo& c,
                                       std::uint32_t current_tick,
                                       std::uint32_t alive_in_group);

}  // namespace mxh::server
