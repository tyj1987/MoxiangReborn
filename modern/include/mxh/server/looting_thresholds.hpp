// looting_thresholds.hpp
//
// 1:1 port of the three legacy CLootingManager threshold functions
// from [Server]Map/LootingManager.cpp:
//   - GetLootingChance(DWORD dwBadFame) -> int
//   - GetLootingItemNum(DWORD dwBadFame) -> int
//   - GetWearItemLootingRatio(DWORD dwBadFame) -> int

// All three are pure threshold dispatchers with no global state.
// The legacy code wraps the first two in #ifdef _HK_LOCAL_ /
// _TW_LOCAL_ to select between three locale variants:
//   - HK non-TW: tighter thresholds (lower fame -> higher tier)
//   - TW:        loose thresholds (HK with extra padding)
//   - Default (KR/CN/JP):  same numeric values as TW.
// The third (GetWearItemLootingRatio) is locale-agnostic.

// 1:1 invariants:
//   - GetLootingChance returns 3..10 (always >= 3).
//   - GetLootingItemNum returns 0..5.
//   - GetWearItemLootingRatio returns 0..100 (0 for fame == 0).
//   - Boundary cases use strict-less-than (< N); the N itself
//     falls into the next bucket.
//   - Saturates at the highest bucket (else branch).

#pragma once

#include <cstdint>

namespace mxh::server {

// 1:1 with the legacy #ifdef _HK_LOCAL_ / _TW_LOCAL_ split.
// The legacy code uses two nested #ifdefs to select the
// threshold set: HK non-TW is the tight variant, TW and the
// default share the same loose thresholds.
enum class LootingLocale : std::uint8_t {
    Default = 0,  // legacy else branch (KR/CN/JP/TW share values)
    Hk      = 1,  // legacy HK_LOCAL without TW_LOCAL
};

// 1:1 with legacy CLootingManager::GetLootingChance.
// Returns 3..10 based on the bad-fame tier.
inline int looting_chance(std::uint32_t bad_fame, LootingLocale locale) noexcept {
    if (locale == LootingLocale::Hk) {
        if (bad_fame < 1000u)   return 3;
        if (bad_fame < 2000u)   return 4;
        if (bad_fame < 5000u)   return 5;
        if (bad_fame < 10000u)  return 6;
        if (bad_fame < 20000u)  return 7;
        if (bad_fame < 50000u)  return 8;
        if (bad_fame < 100000u) return 9;
        return 10;
    }
    // Default (KR/CN/JP/TW): same thresholds.
    if (bad_fame < 100000u)       return 3;
    if (bad_fame < 500000u)       return 4;
    if (bad_fame < 1000000u)      return 5;
    if (bad_fame < 5000000u)      return 6;
    if (bad_fame < 10000000u)     return 7;
    if (bad_fame < 50000000u)     return 8;
    if (bad_fame < 100000000u)    return 9;
    return 10;
}

// 1:1 with legacy CLootingManager::GetLootingItemNum.
// Returns 0..5 based on the bad-fame tier. The HK non-TW
// branch has 5 buckets (1..5) without the leading 0; the
// default / TW branches start at 1 too. The legacy KR/CN/JP
// path also has a leading bucket for bad_fame < 50 -> 0,
// which is the only path that returns 0.
inline int looting_item_num(std::uint32_t bad_fame, LootingLocale locale) noexcept {
    if (locale == LootingLocale::Hk) {
        if (bad_fame < 100000u)   return 1;
        if (bad_fame < 200000u)   return 2;
        if (bad_fame < 500000u)   return 3;
        if (bad_fame < 1000000u)  return 4;
        return 5;
    }
    // Default (KR/CN/JP/TW):
    // KR/CN/JP path has a leading bucket for bad_fame < 50 -> 0.
    // TW path goes straight to the 100000000 buckets. Both
    // share the upper 5 buckets.
    if (bad_fame < 50u)                return 0;
    if (bad_fame < 100000000u)         return 1;
    if (bad_fame < 400000000u)         return 2;
    if (bad_fame < 700000000u)         return 3;
    if (bad_fame < 1000000000u)        return 4;
    return 5;
}

// 1:1 with legacy CLootingManager::GetWearItemLootingRatio.
// Locale-agnostic. Returns 0..100 based on the bad-fame tier.
// Note: the legacy code has dwBadFame == 0 -> 0 BEFORE the
// dwBadFame < 50 bucket. The < 50 bucket then returns 1, so
// 0 is uniquely the zero-fame case.
inline int wear_item_looting_ratio(std::uint32_t bad_fame) noexcept {
    if (bad_fame == 0u)         return 0;
    if (bad_fame < 50u)         return 1;
    if (bad_fame < 4000u)       return 10;
    if (bad_fame < 20000u)      return 20;
    if (bad_fame < 80000u)      return 30;
    if (bad_fame < 400000u)     return 40;
    if (bad_fame < 1600000u)    return 50;
    if (bad_fame < 8000000u)    return 60;
    if (bad_fame < 32000000u)   return 70;
    if (bad_fame < 100000000u)  return 85;
    if (bad_fame < 500000000u)  return 100;
    return 100;
}

}  // namespace mxh::server