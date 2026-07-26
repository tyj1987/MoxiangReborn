// fame_manager.hpp - Phase D6 FameManager 1:1 port.
//
// Source-of-truth: legacy [Server]Map/FameManager.h + .cpp.
// Mirrors legacy CFameManager singleton's fame delta table for
// guild membership transitions and the BadFame enum (PK mode +
// kill + bail).  The actual player-store and DB write hooks are
// left to integration; we expose the per-transition delta
// arithmetic so the test suite can lock it.

#pragma once

#include <cstdint>

namespace mxh::server {

// ---- Mirror of legacy MUNPA_* rank constants ----
inline constexpr std::uint8_t MUNPA_MEMBER       = 1;
inline constexpr std::uint8_t MUNPA_VICE_MASTER  = 2;
inline constexpr std::uint8_t MUNPA_SENIOR       = 3;
inline constexpr std::uint8_t MUNPA_MASTER       = 4;

// ---- Fame_Case enum (mirror legacy) ----
// 1:1 ordinal + compose rule eFame_Breakup+X for breakup entries.
enum class FameCase : std::uint8_t {
    BeMaster                = 0,
    BeMember                = 1,
    BeMembertoSenior        = 2,
    BeMembertoViceMaster    = 3,
    BeSeniortoViceMaster    = 4,
    BeSeniortoMember        = 5,
    BeViceMastertoSenior    = 6,
    BeViceMastertoMember    = 7,
    BeMembertoNotmember     = 8,
    BeSeniortoNotmember     = 9,
    BeVicemastertoNotmember = 10,
    Breakup                 = 50,
    BreakupMaster           = 50 + MUNPA_MASTER,
    BreakupViceMaster       = 50 + MUNPA_VICE_MASTER,
    BreakupSenior           = 50 + MUNPA_SENIOR,
    BreakupMember           = 50 + MUNPA_MEMBER,
};

// ---- BadFame kind (mirror legacy eBADFAME_KIND) ----
enum class BadFameKind : std::int32_t {
    PkModeOn  = 1,
    Attack    = 5,
    Kill      = 5,
    Bail      = -500,
};

// ---- Fame delta arithmetic ----
// Pure function: applies the legacy CFameManager::ChangePlayerFame
// numeric to current_fame and returns the new value.  Returns
// current_fame unchanged if fame_case is not in the table.
std::uint32_t apply_fame_delta(std::uint32_t current_fame, FameCase fame_case);
std::int32_t  apply_bad_fame_delta(std::int32_t current_bad_fame, BadFameKind kind);

// ---- Tick / gate ----
struct FameUpdateClock {
    std::uint16_t start_update_hour = 0;     // m_StartUpdateTime (hour 0-23)
    std::uint8_t  updated_day        = 0;    // m_UpdatedDate (1-31)
    bool          is_updated         = false;
};

// IsTimetoFameUpdate: legacy drives off local clock hour/day.  We
// accept the now day + now hour as inputs.
bool is_time_to_fame_update(FameUpdateClock& clock,
                            std::uint8_t now_day,
                            std::uint8_t now_hour);

}  // namespace mxh::server
