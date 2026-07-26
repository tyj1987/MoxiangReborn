#include "mxh/game/attack_calc.hpp"

#include <algorithm>
#include <cstdint>

namespace mxh::game {
namespace {

std::uint32_t inclusive_roll(std::uint32_t minimum,
                             std::uint32_t maximum,
                             std::int32_t random_value) {
    if (maximum <= minimum) return minimum;
    const auto gap = static_cast<std::uint64_t>(maximum) - minimum + 1u;
    const auto sample = random_value < 0
        ? 0u
        : static_cast<std::uint32_t>(random_value);
    return minimum + static_cast<std::uint32_t>(sample % gap);
}

std::uint32_t sim_mek_cap(std::uint16_t sim_mek) {
    if (sim_mek <= 12u) return 0u;
    return std::min<std::uint32_t>(sim_mek - 12u, 25u);
}

std::uint32_t level_value(std::int32_t level) {
    return level < 0 ? 0u : static_cast<std::uint32_t>(level);
}

}  // namespace

std::uint16_t legacy_get_percent(float seed_rate,
                                 std::int32_t operator_level,
                                 std::int32_t target_level) {
    const auto level_gap = operator_level - target_level;
    const float rate = seed_rate + static_cast<float>(level_gap) * 0.025f;
    if (rate <= 0.0f) return 0;
    const auto percent = static_cast<std::uint32_t>(rate * 100.0f);
    return static_cast<std::uint16_t>(percent & 0xffffu);
}

std::uint16_t legacy_critical_percent(const CriticalInput& input) {
    const float denominator = static_cast<float>(input.target_level * 20 + 300);
    if (denominator <= 0.0f) return 0;
    float critical_rate =
        static_cast<float>(input.attacker_stat + 20) / denominator;
    if (critical_rate > 0.2f) critical_rate = 0.2f;

    auto percent = static_cast<std::uint32_t>(critical_rate * 100.0f);
    if (input.seed_rate != 0.0f) {
        percent += legacy_get_percent(input.seed_rate,
                                      input.attacker_level,
                                      input.target_level);
    }

    const auto with_unique = static_cast<std::int64_t>(percent) +
                             input.unique_rate;
    if (with_unique < 0) return 0;
    return static_cast<std::uint16_t>(
        static_cast<std::uint64_t>(with_unique) & 0xffffu);
}

bool legacy_roll_percent(std::uint16_t percent, std::int32_t random_value) {
    const auto sample = random_value < 0 ? 0 : random_value % 100;
    return sample < static_cast<std::int32_t>(percent);
}

float legacy_japan_critical_rate(const CriticalInput& input) {
    const float denominator = static_cast<float>(input.target_level * 5 + 100);
    if (denominator <= 0.0f) return 0.0f;
    float critical_rate =
        static_cast<float>(input.attacker_stat + 20) / denominator;
    if (critical_rate > 0.15f) critical_rate = 0.15f;

    if (input.attacker_level < input.target_level) {
        critical_rate += input.seed_rate -
            static_cast<float>(input.target_level - input.attacker_level) * 0.02f;
        if (critical_rate < 0.0f) critical_rate = 0.0f;
    } else {
        critical_rate += input.seed_rate +
            static_cast<float>(input.attacker_level - input.target_level) * 0.004f;
    }
    return critical_rate;
}

bool legacy_roll_japan_rate(float rate, std::int32_t random_value) {
    const auto sample = random_value < 0 ? 0 : random_value % 100;
    const float random_rate = static_cast<float>(sample) / 100.0f;
    return rate >= random_rate;
}

double legacy_player_physical_attack(const PhysicalAttackInput& input) {
    float base_rate = 1.0f + input.stat_option;
    if (base_rate < 0.0f) base_rate = 0.0f;
    const auto minimum = static_cast<std::uint32_t>(
        static_cast<float>(input.min_value) * base_rate + 0.5f);
    const auto maximum = static_cast<std::uint32_t>(
        static_cast<float>(input.max_value) * base_rate + 0.5f);
    const auto rolled = inclusive_roll(minimum, maximum, input.random_value);
    double result = static_cast<double>(rolled) * input.attack_rate;
    if (input.critical) {
        result *= input.locale == AttackCalcLocale::Japan ? 2.25 : 1.5;
    }
    return result;
}

double legacy_monster_physical_attack(const PhysicalAttackInput& input) {
    return inclusive_roll(input.min_value, input.max_value, input.random_value);
}

double legacy_titan_physical_attack(const PhysicalAttackInput& input) {
    const auto rolled = inclusive_roll(input.min_value, input.max_value,
                                       input.random_value);
    double result = static_cast<double>(rolled) * input.attack_rate;
    if (input.critical) result *= 1.5;
    return result;
}

double legacy_player_attribute_attack(const PlayerAttributeAttackInput& input) {
    std::uint32_t minimum = 0;
    std::uint32_t maximum = 0;
    if (input.attack_rate > 0.0f) {
        const float midterm =
            (static_cast<float>(input.sim_mek) + 200.0f) / 100.0f;
        const auto minimum_level = level_value(input.level);
        const auto maximum_level = minimum_level + 10u;
        const auto fixed = static_cast<std::uint32_t>(input.sim_mek / 5u) +
                           sim_mek_cap(input.sim_mek);
        minimum = static_cast<std::uint32_t>(
            static_cast<float>(minimum_level) * input.attack_rate * midterm +
            fixed);
        maximum = static_cast<std::uint32_t>(
            static_cast<float>(maximum_level) * input.attack_rate * midterm +
            fixed);
    }

    minimum += input.attack_min;
    maximum += input.attack_max;
    const float attribute_rate = 1.0f + input.attribute_plus;
    minimum = static_cast<std::uint32_t>(
        static_cast<float>(minimum) * attribute_rate);
    maximum = static_cast<std::uint32_t>(
        static_cast<float>(maximum) * attribute_rate);
    return inclusive_roll(minimum, maximum, input.random_value);
}

double legacy_player_attribute_attack_japan(
    const PlayerAttributeAttackInput& input) {
    std::uint32_t minimum = 0;
    std::uint32_t maximum = 0;
    if (input.attack_rate > 0.0f || input.attack_max > 0u) {
        const auto minimum_level = level_value(input.level);
        const auto maximum_level = minimum_level + 10u;
        const float sim_half = static_cast<float>(input.sim_mek / 2u);
        minimum = static_cast<std::uint32_t>(
            (static_cast<float>(minimum_level) + input.attribute_plus + sim_half) *
                input.attack_rate + input.attack_min);
        maximum = static_cast<std::uint32_t>(
            (static_cast<float>(maximum_level) + input.attribute_plus + sim_half) *
                input.attack_rate + input.attack_max);
    }
    return inclusive_roll(minimum, maximum, input.random_value);
}

double legacy_monster_attribute_attack(std::uint32_t attack_min,
                                       std::uint32_t attack_max,
                                       std::int32_t random_value) {
    return inclusive_roll(attack_min, attack_max, random_value);
}

double legacy_titan_attribute_attack(std::uint32_t titan_attribute,
                                     std::uint16_t owner_sim_mek,
                                     float attack_rate,
                                     std::int32_t random_value) {
    const auto base = static_cast<std::uint64_t>(titan_attribute) *
                          (owner_sim_mek + 100u) / 400u +
                      owner_sim_mek / 5u;
    const auto power = static_cast<std::uint32_t>(
        static_cast<float>(base) * 0.74f);
    const auto rolled = inclusive_roll(power, power, random_value);
    return static_cast<double>(rolled) * attack_rate;
}

double legacy_titan_player_physical_attack(const PhysicalAttackInput& player,
                                           const PhysicalAttackInput& titan) {
    return legacy_titan_physical_attack(titan) +
           legacy_player_physical_attack(player) * 0.6;
}

double legacy_titan_player_attribute_attack(
    const PlayerAttributeAttackInput& player,
    std::uint32_t titan_attribute,
    std::uint16_t owner_sim_mek,
    float titan_attack_rate,
    std::int32_t titan_random_value) {
    return legacy_titan_attribute_attack(titan_attribute, owner_sim_mek,
                                         titan_attack_rate, titan_random_value) +
           legacy_player_attribute_attack(player) * 0.6;
}

double legacy_phy_defence_level(const DefenceInput& input) {
    double physical_defence = input.physical_defence;
    if (input.attacker_is_player) {
        physical_defence *=
            1.0 - static_cast<double>(input.enemy_defence_percent) * 0.01;
    }
    if (input.target_is_player) {
        physical_defence += physical_defence *
            (input.target_regist_phys_percent / 100.0);
        double skill_rate = 1.0 + input.target_skill_phy_def;
        if (skill_rate < 0.0) skill_rate = 0.0;
        physical_defence *= skill_rate;
        physical_defence *= input.party_defence_rate;
    }

    const auto attacker_level = std::max(input.attacker_level, 1);
    double result = 0.0;
    if (input.locale == AttackCalcLocale::Japan) {
        result = (physical_defence * 2.0) /
                 (static_cast<double>(attacker_level) * 50.0);
        result = std::clamp(result, 0.0, 0.99);
    } else {
        result = (physical_defence * 2.0 + 50.0) /
                 (static_cast<double>(attacker_level) * 20.0 + 150.0);
        result = std::clamp(result, 0.0, 0.9);
    }

    if (input.target_is_player && input.target_in_titan) {
        result = input.titan_defence == 0u ? 0.0 : 0.8;
    }
    return result;
}

}  // namespace mxh::game
