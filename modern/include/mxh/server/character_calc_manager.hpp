// character_calc_manager.hpp - 1:1 numeric port of legacy CharacterCalcManager.
//
// Source: \u58a8\u9999\u3010\u6e90\u7801\u3011\\[Server]Map\\CharacterCalcManager.cpp
// All formulas reproduced 1:1 from the legacy KR/CN branch (the !_JAPAN_LOCAL_
// path). Each legacy manager function is split into a pure function over a
// POD input struct + the legacy side-effect handled by the caller.
//
// 1:1 quirks preserved:
//   * max-life/shield/naeryuk clamp to >= 1 on UniqueItem negative overflow.
//   * general recovery is 5 s tick (KR/CN) with 1 % base + mussang bonus
//     (1.5 % / 2.0 %) for life/shield/naeryuk symmetrically.
//   * Ungi (running meditation) recovery: 5 s tick + 10 flat + max*3 % +
//     UngiPlusRate + ability UngiUpVal + ShopItemStats multiplier.

#pragma once

#include <cstdint>

namespace mxh::server {

struct CalcBaseStats final {
    std::uint16_t level      = 1;
    std::uint16_t gengol     = 0;
    std::uint16_t simmek     = 0;
    std::uint16_t minchub    = 0;
    std::uint16_t cheryuk    = 0;
};

struct CalcEquipBonuses final {
    std::int32_t item_max_life      = 0;
    std::int32_t item_max_shield    = 0;
    std::int32_t item_max_naeryuk   = 0;
    std::int32_t set_dw_life        = 0;
    std::int32_t set_dw_shield      = 0;
    std::int32_t set_dw_naeryuk     = 0;
    std::int32_t ability_life_up    = 0;
    std::int32_t ability_shield_up  = 0;
    std::int32_t ability_naeryuk_up = 0;
    std::int32_t shop_life          = 0;
    std::int32_t shop_shield        = 0;
    std::int32_t shop_naeryuk       = 0;
    std::int32_t avatar_life        = 0;
    std::int32_t avatar_shield      = 0;
    std::int32_t avatar_naeryuk     = 0;
    std::int32_t skill_life         = 0;
    std::int32_t skill_shield       = 0;
    std::int32_t skill_naeryuk      = 0;
    std::int32_t unique_n_hp        = 0;
    std::int32_t unique_n_shield    = 0;
    std::int32_t unique_n_mp        = 0;
};

std::uint32_t compute_max_life(const CalcBaseStats& s,
                                const CalcEquipBonuses& b);
std::uint32_t compute_max_shield(const CalcBaseStats& s,
                                 const CalcEquipBonuses& b);
std::uint32_t compute_max_naeryuk(const CalcBaseStats& s,
                                  const CalcEquipBonuses& b);

enum class MussangStage : std::uint8_t {
    Normal = 0, Hwa = 1, Geuk = 2, Hyun = 3, Tal = 4,
};

struct RecoveryResult final {
    std::uint32_t new_value      = 0;
    bool          updated        = false;
    std::uint32_t new_check_time = 0;
};

RecoveryResult tick_life_recovery(std::uint32_t cur_time,
                                  std::uint32_t last_check_time,
                                  std::uint32_t life, std::uint32_t max_life,
                                  std::uint32_t up_life,
                                  bool is_mussang_mode,
                                  MussangStage stage,
                                  std::uint32_t recover_rate_bp);

RecoveryResult tick_shield_recovery(std::uint32_t cur_time,
                                    std::uint32_t last_check_time,
                                    std::uint32_t shield,
                                    std::uint32_t max_shield,
                                    std::uint32_t up_shield,
                                    bool is_mussang_mode,
                                    MussangStage stage,
                                    std::uint32_t recover_rate_bp);

RecoveryResult tick_naeryuk_recovery(std::uint32_t cur_time,
                                     std::uint32_t last_check_time,
                                     std::uint32_t naeryuk,
                                     std::uint32_t max_naeryuk,
                                     std::uint32_t up_naeryuk,
                                     bool is_mussang_mode,
                                     MussangStage stage,
                                     std::uint32_t recover_rate_bp);

RecoveryResult tick_life_ungi(std::uint32_t cur_time,
                              std::uint32_t last_check_time,
                              std::uint32_t life, std::uint32_t max_life,
                              std::uint32_t ungi_up_val,
                              float ungi_plus_rate,
                              float ungispeed,
                              bool is_snow_weather);

RecoveryResult tick_shield_ungi(std::uint32_t cur_time,
                                std::uint32_t last_check_time,
                                std::uint32_t shield,
                                std::uint32_t max_shield,
                                std::uint32_t ungi_up_val,
                                float ungi_plus_rate,
                                float ungispeed,
                                bool is_snow_weather);

RecoveryResult tick_naeryuk_ungi(std::uint32_t cur_time,
                                 std::uint32_t last_check_time,
                                 std::uint32_t naeryuk,
                                 std::uint32_t max_naeryuk,
                                 std::uint32_t ungi_up_val,
                                 float ungi_plus_rate,
                                 float ungispeed,
                                 bool is_snow_weather,
                                 bool is_mussang_mode,
                                 MussangStage stage);

}  // namespace mxh::server
