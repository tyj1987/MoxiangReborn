// ai_param.cpp - Phase D6 AIParam 1:1 port implementations.

#include "mxh/server/ai_param.hpp"

namespace mxh::server {

void ai_param_init(AIPARAM& p, std::uint32_t cur_time_ms) {
    // Legacy AIParam.cpp::CAIParam::Init body.
    p.AttackStartTime            = 0u;
    p.CurAttackKind              = 0u;
    p.CurAttackPatternNum        = 0u;
    p.CurAttackPatternIndex      = 0u;
    p.SearchLastTime             = cur_time_ms + AIPARAM_SEARCH_PERIOD_MS;
    p.CollSearchLastTime         = cur_time_ms + AIPARAM_SEARCH_PERIOD_MS;
    p.PursuitForgiveStartTime    = cur_time_ms;
    p.RunawayTypeVal             = RunawayType::None;
    p.pTarget                    = nullptr;
    p.pHelperMonster             = nullptr;
    p.prePursuitForgiveTime      = 0u;
}

std::uint32_t ai_param_get_cur_attack_kind(const AIPARAM& p) {
    return p.CurAttackKind;
}

void ai_param_set_current_attack_pattern(AIPARAM& p, std::uint16_t w) {
    p.CurAttackPatternNum   = w;
    p.CurAttackPatternIndex = 0;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int ai_param_translation_unit_anchor = 0;
}
