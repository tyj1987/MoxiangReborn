// boss_monster_info.hpp - per-boss static data loaded from MonsterList.bin.
//
// 1:1 port of legacy [Server]Map/BossMonsterInfo.h CBossMonsterInfo.
// The legacy class is a per-MonsterKind static table with raid-wide
// parameters (announce msg, max-killers, reward IDs, time-window).
// Modern port keeps it as a POD struct loaded by BossMonsterManager.

#pragma once

#include <cstdint>
#include <array>

namespace mxh::server {

// Maximum simultaneous rewarders (legacy CBossMonsterInfo MAX_DROP).
inline constexpr std::uint8_t BOSS_INFO_MAX_DAMAGER = 30;

// Maximum unique drop items a boss can produce (legacy CBossMonsterInfo).
inline constexpr std::uint8_t BOSS_INFO_MAX_DROP = 10;

// Boss rewards (legacy REWARDINFO).
struct BossReward final {
    std::uint32_t item_id = 0;     // legacy ITEM_IDX
    std::uint32_t ratio  = 0;      // legacy drop ratio percent (1..100)
};

// Static per-boss info (1:1 port of legacy CBossMonsterInfo).
struct BossMonsterInfo final {
    std::uint32_t monster_kind         = 0;  // MonsterKind (idx into MonsterList.bin)
    std::uint8_t  is_field_boss        = 0;  // 0=map boss, 1=field boss
    std::uint8_t  reserved0            = 0;
    std::uint16_t reserved1            = 0;
    std::uint32_t time_limit_ms        = 0;  // legacy dwLimitTime
    std::uint32_t killer_limit         = 0;  // legacy dwKillerNum (max simultaneous rewarders)
    std::uint32_t announce_msg_id      = 0;  // legacy dwNoticeIndex
    std::array<BossReward, BOSS_INFO_MAX_DROP> rewards{};
    std::uint8_t  speech_count         = 0;  // legacy cMonsterSpeechManager hook
    std::uint8_t  reserved2            = 0;
    std::uint16_t reserved3            = 0;
    std::uint32_t speech_id_base       = 0;  // legacy dwSpeechBase (index into MonsterSpeechInfoList.bin)
};

}  // namespace mxh::server