// tactic_ability_info.cpp - Phase D6 TaticAbilityInfo 1:1 port.

#include "mxh/server/tactic_ability_info.hpp"

#include <algorithm>
#include <cstring>

namespace mxh::server {

void tactic_ability_info_copy(TaticAbilityInfoData& dst, const void* src,
                              std::size_t src_size) {
    if (src == nullptr || src_size != sizeof(TaticAbilityInfoData)) {
        dst = TaticAbilityInfoData{};
        return;
    }
    std::memcpy(&dst, src, sizeof(TaticAbilityInfoData));
}

namespace {
std::uint16_t clamp_level(std::uint16_t wLevel) {
    if (wLevel == 0) return 0;
    if (wLevel > MAX_TATIC_ABILITY_NUM) return MAX_TATIC_ABILITY_NUM;
    return wLevel;
}
}  // namespace

std::uint16_t tactic_ability_get_attack(const TaticAbilityInfoData& info, std::uint16_t wLevel) {
    std::uint16_t lv = clamp_level(wLevel);
    if (lv == 0) return 0;
    return info.wTypeAttack[lv - 1];
}

std::uint16_t tactic_ability_get_recover(const TaticAbilityInfoData& info, std::uint16_t wLevel) {
    std::uint16_t lv = clamp_level(wLevel);
    if (lv == 0) return 0;
    return info.wTypeRecover[lv - 1];
}

float tactic_ability_get_buff_rate(const TaticAbilityInfoData& info, std::uint16_t wLevel) {
    std::uint16_t lv = clamp_level(wLevel);
    if (lv == 0) return 0.0f;
    return info.fTypeBuffRate[lv - 1];
}

std::uint16_t tactic_ability_get_buff(const TaticAbilityInfoData& info, std::uint16_t wLevel) {
    std::uint16_t lv = clamp_level(wLevel);
    if (lv == 0) return 0;
    return info.wTypeBuff[lv - 1];
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int tactic_ability_info_translation_unit_anchor = 0;
}
