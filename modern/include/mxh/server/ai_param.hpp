// ai_param.hpp - Phase D6 AIParam 1:1 port.
//
// Source-of-truth: legacy [Server]Map/AIParam.h + .cpp.
// Mirrors legacy AIPARAM POD + CAIParam Init/Get/Set behavior.
// pTarget/pHelperMonster are kept as opaque void* since the
// concrete types (CPlayer, CMonster) are owned by other modules.

#pragma once

#include <cstdint>

namespace mxh::server {

// ---- Runaway type (mirror legacy eRunawayType_*) ----
enum class RunawayType : std::uint16_t {
    None = 0,
};

// ---- Mirror of legacy AIPARAM POD ----
struct AIPARAM {
    std::uint32_t AttackStartTime               = 0;
    std::uint32_t SearchLastTime                = 0;
    std::uint32_t CollSearchLastTime            = 0;
    std::uint32_t CurAttackKind                 = 0;
    std::uint16_t CurAttackPatternNum           = 0;
    std::uint16_t CurAttackPatternIndex         = 0;
    std::uint32_t PursuitForgiveStartTime       = 0;
    std::uint32_t prePursuitForgiveTime         = 0;
    RunawayType   RunawayTypeVal                 = RunawayType::None;
    void*         pTarget                        = nullptr;
    void*         pHelperMonster                 = nullptr;
};

// Search/collision search periodic offset (legacy gCurTime + 5000 ms).
inline constexpr std::uint32_t AIPARAM_SEARCH_PERIOD_MS = 5000u;

// ---- AIParam ops (mirror CAIParam) ----
// Init initializes the AIPARAM with the legacy offsets applied to
// the current game-time clock.  Caller passes cur_time_ms; modern
// has no global gCurTime.
void ai_param_init(AIPARAM& p, std::uint32_t cur_time_ms);

std::uint32_t ai_param_get_cur_attack_kind(const AIPARAM& p);

// SetCurrentAttackPattern (legacy inline):
//   CurAttackPatternNum = w; CurAttackPatternIndex = 0;
void ai_param_set_current_attack_pattern(AIPARAM& p, std::uint16_t w);

}  // namespace mxh::server
