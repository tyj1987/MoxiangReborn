// summon_monster.cpp - Phase D6 SummonMonster 1:1 port.

#include "mxh/server/summon_monster.hpp"

namespace mxh::server {

void summon_monster_init(SummonMonster& s) {
    s.m_SummmonerID = 0u;
    s.m_RegenTime   = 0u;
    s.m_DieTime     = 0u;
}

std::uint32_t summon_monster_age_ms(const SummonMonster& s,
                                    std::uint32_t cur_time_ms) {
    if (cur_time_ms < s.m_RegenTime) return 0u;
    return cur_time_ms - s.m_RegenTime;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int summon_monster_translation_unit_anchor = 0;
}
