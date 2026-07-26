// fame_manager.cpp - Phase D6 FameManager 1:1 port implementations.

#include "mxh/server/fame_manager.hpp"

namespace mxh::server {

std::uint32_t apply_fame_delta(std::uint32_t current_fame, FameCase fame_case) {
    // Legacy CFameManager::ChangePlayerFame body (excerpted).
    switch (fame_case) {
        // +fame cases
        case FameCase::BeMaster:             return current_fame + 30u;
        case FameCase::BeMember:             return current_fame + 10u;
        case FameCase::BeMembertoSenior:     return current_fame + 5u;
        case FameCase::BeMembertoViceMaster: return current_fame + 15u;
        case FameCase::BeSeniortoViceMaster: return current_fame + 10u;
        // -fame cases
        case FameCase::BeSeniortoMember:        return current_fame - 10u;
        case FameCase::BeViceMastertoSenior:    return current_fame - 15u;
        case FameCase::BeViceMastertoMember:    return current_fame - 25u;
        case FameCase::BeVicemastertoNotmember: return current_fame - 25u;
        case FameCase::BeSeniortoNotmember:     return current_fame - 20u;
        case FameCase::BeMembertoNotmember:     return current_fame - 15u;
        // breakup
        case FameCase::BreakupMaster:     return current_fame - 70u;
        case FameCase::BreakupViceMaster: return current_fame - 30u;
        case FameCase::BreakupSenior:     return current_fame - 20u;
        case FameCase::BreakupMember:     return current_fame - 15u;
        // Breakup=50 itself is not a transition; bail.
        default: return current_fame;
    }
}

std::int32_t apply_bad_fame_delta(std::int32_t current_bad_fame, BadFameKind kind) {
    // Legacy eBADFAME_KIND table maps by integer value (Attack==Kill==5).
    const auto v = static_cast<std::int32_t>(kind);
    if (v == static_cast<std::int32_t>(BadFameKind::PkModeOn)) return current_bad_fame + 1;
    if (v == static_cast<std::int32_t>(BadFameKind::Attack))   return current_bad_fame + 5;
    if (v == static_cast<std::int32_t>(BadFameKind::Kill))     return current_bad_fame + 5;
    if (v == static_cast<std::int32_t>(BadFameKind::Bail))     return current_bad_fame - 500;
    return current_bad_fame;
}

bool is_time_to_fame_update(FameUpdateClock& clock,
                            std::uint8_t now_day,
                            std::uint8_t now_hour) {
    // Legacy CFameManager::IsTimetoFameUpdate body (excerpted):
    //   if (day != m_UpdatedDate && m_bIsUpdated) m_bIsUpdated = FALSE;
    //   if (!m_bIsUpdated && hour >= m_StartUpdateTime) {
    //     m_UpdatedDate = day; return TRUE;
    //   }
    //   return FALSE;
    if (now_day != clock.updated_day && clock.is_updated) {
        clock.is_updated = false;
    }
    if (!clock.is_updated && now_hour >= clock.start_update_hour) {
        clock.updated_day = now_day;
        clock.is_updated  = true;
        return true;
    }
    return false;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int fame_manager_translation_unit_anchor = 0;
}
