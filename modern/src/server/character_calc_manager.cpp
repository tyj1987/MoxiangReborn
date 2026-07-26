// character_calc_manager.cpp - 1:1 numeric port of legacy CharacterCalcManager.
//
// Source: legacy [Server]Map/CharacterCalcManager.cpp
// All branches selected are the legacy KR/CN path (!_JAPAN_LOCAL_).

#include "mxh/server/character_calc_manager.hpp"
#include <algorithm>

namespace mxh::server {

namespace {

constexpr std::uint32_t kTickPeriodMs = 5000;
constexpr std::uint32_t kUngiBaseMs   = 5000;

float mussang_life_multiplier(bool is_mussang, MussangStage stage) {
    if (!is_mussang) return 1.0f;
    switch (stage) {
        case MussangStage::Hyun:
        case MussangStage::Tal:  return 2.0f;
        default:                 return 1.5f;
    }
}

float mussang_shield_multiplier(bool is_mussang, MussangStage stage) {
    if (!is_mussang) return 1.0f;
    switch (stage) {
        case MussangStage::Hyun:
        case MussangStage::Tal:  return 2.0f;
        default:                 return 1.5f;
    }
}

float mussang_naeryuk_multiplier(bool is_mussang, MussangStage stage) {
    if (!is_mussang) return 1.0f;
    switch (stage) {
        case MussangStage::Normal: return 1.5f;
        case MussangStage::Hwa:
        case MussangStage::Geuk:   return 1.5f;
        case MussangStage::Hyun:
        case MussangStage::Tal:    return 2.0f;
        default:                   return 1.5f;
    }
}

std::int32_t apply_unique_item_clamp(std::int32_t base, std::int32_t unique) {
    std::int64_t sum = static_cast<std::int64_t>(base) + static_cast<std::int64_t>(unique);
    if (sum > 0) return static_cast<std::int32_t>(sum);
    return 1;
}

}  // namespace

std::uint32_t compute_max_life(const CalcBaseStats& s, const CalcEquipBonuses& b) {
    std::int64_t base = static_cast<std::int64_t>(s.level) * 5
                      + static_cast<std::int64_t>(s.cheryuk) * 10;
    std::int64_t sum = base
        + b.item_max_life + b.set_dw_life + b.ability_life_up
        + b.shop_life + b.avatar_life + b.skill_life;
    return static_cast<std::uint32_t>(apply_unique_item_clamp(
        static_cast<std::int32_t>(sum), b.unique_n_hp));
}

std::uint32_t compute_max_shield(const CalcBaseStats& s, const CalcEquipBonuses& b) {
    std::int64_t base = static_cast<std::int64_t>(s.level) * 5
                      + static_cast<std::int64_t>(s.simmek) * 10
                      + static_cast<std::int64_t>(s.gengol)  * 5
                      + static_cast<std::int64_t>(s.minchub) * 5;
    std::int64_t sum = base
        + b.item_max_shield + b.set_dw_shield + b.ability_shield_up
        + b.shop_shield + b.avatar_shield + b.skill_shield;
    return static_cast<std::uint32_t>(apply_unique_item_clamp(
        static_cast<std::int32_t>(sum), b.unique_n_shield));
}

std::uint32_t compute_max_naeryuk(const CalcBaseStats& s, const CalcEquipBonuses& b) {
    std::int64_t base = static_cast<std::int64_t>(s.level) * 5
                      + static_cast<std::int64_t>(s.simmek) * 10;
    std::int64_t sum = base
        + b.item_max_naeryuk + b.set_dw_naeryuk + b.ability_naeryuk_up
        + b.shop_naeryuk + b.avatar_naeryuk + b.skill_naeryuk;
    return static_cast<std::uint32_t>(apply_unique_item_clamp(
        static_cast<std::int32_t>(sum), b.unique_n_mp));
}

RecoveryResult tick_life_recovery(std::uint32_t cur_time,
                                  std::uint32_t last_check_time,
                                  std::uint32_t life, std::uint32_t max_life,
                                  std::uint32_t up_life,
                                  bool is_mussang_mode,
                                  MussangStage stage,
                                  std::uint32_t recover_rate_bp) {
    RecoveryResult r;
    r.new_value = life;
    r.new_check_time = last_check_time;
    if (cur_time <= last_check_time + kTickPeriodMs) return r;
    if (life >= max_life) {
        return r;
    }
    float mult = mussang_life_multiplier(is_mussang_mode, stage);
    float base_pct = 0.01f * mult;
    float new_val_f = static_cast<float>(life)
                    + static_cast<float>(max_life) * base_pct
                    + static_cast<float>(up_life);
    if (recover_rate_bp > 0) {
        new_val_f *= (static_cast<float>(recover_rate_bp) * 0.01f);
    }
    if (new_val_f > static_cast<float>(max_life)) new_val_f = static_cast<float>(max_life);
    r.new_value = static_cast<std::uint32_t>(new_val_f);
    r.new_check_time = cur_time;
    r.updated = true;
    return r;
}

RecoveryResult tick_shield_recovery(std::uint32_t cur_time,
                                    std::uint32_t last_check_time,
                                    std::uint32_t shield,
                                    std::uint32_t max_shield,
                                    std::uint32_t up_shield,
                                    bool is_mussang_mode,
                                    MussangStage stage,
                                    std::uint32_t recover_rate_bp) {
    RecoveryResult r;
    r.new_value = shield;
    r.new_check_time = last_check_time;
    if (cur_time <= last_check_time + kTickPeriodMs) return r;
    if (shield >= max_shield) {
        return r;
    }
    float mult = mussang_shield_multiplier(is_mussang_mode, stage);
    float base_pct = 0.01f * mult;
    float new_val_f = static_cast<float>(shield)
                    + static_cast<float>(max_shield) * base_pct
                    + static_cast<float>(up_shield);
    if (recover_rate_bp > 0) {
        new_val_f *= (static_cast<float>(recover_rate_bp) * 0.01f);
    }
    if (new_val_f > static_cast<float>(max_shield)) new_val_f = static_cast<float>(max_shield);
    r.new_value = static_cast<std::uint32_t>(new_val_f);
    r.new_check_time = cur_time;
    r.updated = true;
    return r;
}

RecoveryResult tick_naeryuk_recovery(std::uint32_t cur_time,
                                     std::uint32_t last_check_time,
                                     std::uint32_t naeryuk,
                                     std::uint32_t max_naeryuk,
                                     std::uint32_t up_naeryuk,
                                     bool is_mussang_mode,
                                     MussangStage stage,
                                     std::uint32_t recover_rate_bp) {
    RecoveryResult r;
    r.new_value = naeryuk;
    r.new_check_time = last_check_time;
    if (cur_time <= last_check_time + kTickPeriodMs) return r;
    if (naeryuk >= max_naeryuk) {
        return r;
    }
    float mult = mussang_naeryuk_multiplier(is_mussang_mode, stage);
    float base_pct = 0.01f * mult;
    float new_val_f = static_cast<float>(naeryuk)
                    + static_cast<float>(max_naeryuk) * base_pct
                    + static_cast<float>(up_naeryuk);
    if (recover_rate_bp > 0) {
        new_val_f *= (static_cast<float>(recover_rate_bp) * 0.01f);
    }
    if (new_val_f > static_cast<float>(max_naeryuk)) new_val_f = static_cast<float>(max_naeryuk);
    r.new_value = static_cast<std::uint32_t>(new_val_f);
    r.new_check_time = cur_time;
    r.updated = true;
    return r;
}

static std::uint32_t ungi_period_ms(float ungispeed, bool is_snow) {
    float p = static_cast<float>(kUngiBaseMs);
    if (ungispeed > 0.0f) p *= (1.0f / ungispeed);
    if (is_snow) p *= 0.5f;
    return static_cast<std::uint32_t>(p);
}

RecoveryResult tick_life_ungi(std::uint32_t cur_time,
                              std::uint32_t last_check_time,
                              std::uint32_t life, std::uint32_t max_life,
                              std::uint32_t ungi_up_val,
                              float ungi_plus_rate,
                              float ungispeed,
                              bool is_snow_weather) {
    RecoveryResult r;
    r.new_value = life;
    r.new_check_time = last_check_time;
    std::uint32_t period = ungi_period_ms(ungispeed, is_snow_weather);
    if (cur_time <= last_check_time + period) return r;
    if (life >= max_life) {
        r.new_value = life;
        return r;
    }
    float baseplus = 10.0f + static_cast<float>(max_life) * 0.03f
                   + static_cast<float>(ungi_up_val);
    float plus = static_cast<float>(max_life) * ungi_plus_rate;
    float new_val_f = static_cast<float>(life) + baseplus + plus;
    if (new_val_f > static_cast<float>(max_life)) new_val_f = static_cast<float>(max_life);
    r.new_value = static_cast<std::uint32_t>(new_val_f);
    r.new_check_time = cur_time;
    r.updated = true;
    return r;
}

RecoveryResult tick_shield_ungi(std::uint32_t cur_time,
                                std::uint32_t last_check_time,
                                std::uint32_t shield,
                                std::uint32_t max_shield,
                                std::uint32_t ungi_up_val,
                                float ungi_plus_rate,
                                float ungispeed,
                                bool is_snow_weather) {
    RecoveryResult r;
    r.new_value = shield;
    r.new_check_time = last_check_time;
    std::uint32_t period = ungi_period_ms(ungispeed, is_snow_weather);
    if (cur_time <= last_check_time + period) return r;
    if (shield >= max_shield) {
        r.new_value = shield;
        return r;
    }
    float baseplus = 10.0f + static_cast<float>(max_shield) * 0.03f
                   + static_cast<float>(ungi_up_val);
    float plus = static_cast<float>(max_shield) * ungi_plus_rate;
    float new_val_f = static_cast<float>(shield) + baseplus + plus;
    if (new_val_f > static_cast<float>(max_shield)) new_val_f = static_cast<float>(max_shield);
    r.new_value = static_cast<std::uint32_t>(new_val_f);
    r.new_check_time = cur_time;
    r.updated = true;
    return r;
}

RecoveryResult tick_naeryuk_ungi(std::uint32_t cur_time,
                                 std::uint32_t last_check_time,
                                 std::uint32_t naeryuk,
                                 std::uint32_t max_naeryuk,
                                 std::uint32_t ungi_up_val,
                                 float ungi_plus_rate,
                                 float ungispeed,
                                 bool is_snow_weather,
                                 bool is_mussang_mode,
                                 MussangStage stage) {
    RecoveryResult r;
    r.new_value = naeryuk;
    r.new_check_time = last_check_time;
    std::uint32_t period = ungi_period_ms(ungispeed, is_snow_weather);
    if (cur_time <= last_check_time + period) return r;
    if (naeryuk >= max_naeryuk) {
        r.new_value = naeryuk;
        return r;
    }
    float recover = 10.0f + static_cast<float>(max_naeryuk) * 0.03f
                  + static_cast<float>(ungi_up_val);
    recover += static_cast<float>(max_naeryuk) * ungi_plus_rate;
    if (is_mussang_mode) {
        recover *= mussang_naeryuk_multiplier(is_mussang_mode, stage);
    }
    float new_val_f = static_cast<float>(naeryuk) + recover;
    if (new_val_f > static_cast<float>(max_naeryuk)) new_val_f = static_cast<float>(max_naeryuk);
    r.new_value = static_cast<std::uint32_t>(new_val_f);
    r.new_check_time = cur_time;
    r.updated = true;
    return r;
}

}  // namespace mxh::server
