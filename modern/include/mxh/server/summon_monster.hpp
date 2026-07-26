// summon_monster.hpp - Phase D6 SummonMonster 1:1 port.
//
// Source-of-truth: legacy [Server]Map/SummonMonster.h.
// Mirrors legacy CSummonMonster subclass of CMonster with three
// unique fields: m_SummmonerID (note: legacy typo preserved),
// m_RegenTime, m_DieTime.

#pragma once

#include <cstdint>

namespace mxh::server {

// Mirror of legacy CSummonMonster POD (subset of CMonster fields).
// Modern uses struct composition rather than inheritance to keep the
// type a pure data carrier for the test suite.
struct SummonMonster {
    std::uint32_t m_SummmonerID = 0;   // legacy typo preserved 1:1
    std::uint32_t m_RegenTime   = 0;
    std::uint32_t m_DieTime     = 0;
};

void summon_monster_init(SummonMonster& s);

// IsExpired: legacy semantic -- a summon has died if its age exceeds
// (DieTime - RegenTime).  We expose age_in_ms for direct testing.
std::uint32_t summon_monster_age_ms(const SummonMonster& s,
                                    std::uint32_t cur_time_ms);

}  // namespace mxh::server
