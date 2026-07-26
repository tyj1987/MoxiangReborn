// tactic_ability_info.hpp - Phase D6 TaticAbilityInfo 1:1 port.
//
// Source-of-truth: legacy [Server]Map/TaticAbilityInfo.h + .cpp.
// Mirrors legacy CTaticAbilityInfo's level-indexed arrays:
//   m_Info.wTypeAttack[wLevel-1]
//   m_Info.wTypeRecover[wLevel-1]
//   m_Info.fTypeBuffRate[wLevel-1]
//   m_Info.wTypeBuff[wLevel-1]
// Bounds: wLevel <= 0 -> 0; wLevel >= MAX -> clamped to MAX.
//
// Note: the legacy file/class name "Tatic" (not "Tactic") is
// preserved in field names and filename via the modern namespace
// alias.

#pragma once

#include <array>
#include <cstdint>

namespace mxh::server {

// Legacy MAX_TATIC_ABILITY_NUM cap on the level index.
inline constexpr std::uint16_t MAX_TATIC_ABILITY_NUM = 12u;

// Mirror of legacy TATIC_ABILITY_INFO POD.
struct TaticAbilityInfoData {
    std::array<std::uint16_t, MAX_TATIC_ABILITY_NUM> wTypeAttack{};
    std::array<std::uint16_t, MAX_TATIC_ABILITY_NUM> wTypeRecover{};
    std::array<float,          MAX_TATIC_ABILITY_NUM> fTypeBuffRate{};
    std::array<std::uint16_t, MAX_TATIC_ABILITY_NUM> wTypeBuff{};
};

// Init from raw bytes (legacy memcpy from TATIC_ABILITY_INFO pointer).
void tactic_ability_info_copy(TaticAbilityInfoData& dst, const void* src,
                              std::size_t src_size);

// Pure legacy formula: clamp wLevel into [1, MAX], then return tabulated value.
std::uint16_t tactic_ability_get_attack(const TaticAbilityInfoData& info, std::uint16_t wLevel);
std::uint16_t tactic_ability_get_recover(const TaticAbilityInfoData& info, std::uint16_t wLevel);
float         tactic_ability_get_buff_rate(const TaticAbilityInfoData& info, std::uint16_t wLevel);
std::uint16_t tactic_ability_get_buff(const TaticAbilityInfoData& info, std::uint16_t wLevel);

}  // namespace mxh::server
