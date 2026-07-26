#pragma once

#include <cstdint>

namespace mxh::game {

enum class AttackCalcLocale : std::uint8_t {
    KoreaChina,
    Japan,
};

struct CriticalInput {
    std::int32_t attacker_stat = 0;
    std::int32_t attacker_level = 1;
    std::int32_t target_level = 1;
    float seed_rate = 0.0f;
    std::int32_t unique_rate = 0;
};

struct PhysicalAttackInput {
    std::uint32_t min_value = 0;
    std::uint32_t max_value = 0;
    float attack_rate = 1.0f;
    float stat_option = 0.0f;
    bool critical = false;
    AttackCalcLocale locale = AttackCalcLocale::KoreaChina;
    std::int32_t random_value = 0;
};

struct PlayerAttributeAttackInput {
    std::int32_t level = 1;
    std::uint16_t sim_mek = 0;
    std::uint32_t attack_min = 0;
    std::uint32_t attack_max = 0;
    float attack_rate = 0.0f;
    float attribute_plus = 0.0f;
    std::int32_t random_value = 0;
};

struct DefenceInput {
    double physical_defence = 0.0;
    std::int32_t attacker_level = 1;
    bool attacker_is_player = false;
    std::int32_t enemy_defence_percent = 0;
    bool target_is_player = false;
    double target_regist_phys_percent = 0.0;
    double target_skill_phy_def = 0.0;
    double party_defence_rate = 1.0;
    bool target_in_titan = false;
    std::uint16_t titan_defence = 0;
    AttackCalcLocale locale = AttackCalcLocale::KoreaChina;
};

std::uint16_t legacy_get_percent(float seed_rate,
                                 std::int32_t operator_level,
                                 std::int32_t target_level);
std::uint16_t legacy_critical_percent(const CriticalInput& input);
bool legacy_roll_percent(std::uint16_t percent, std::int32_t random_value);

float legacy_japan_critical_rate(const CriticalInput& input);
bool legacy_roll_japan_rate(float rate, std::int32_t random_value);

double legacy_player_physical_attack(const PhysicalAttackInput& input);
double legacy_monster_physical_attack(const PhysicalAttackInput& input);
double legacy_titan_physical_attack(const PhysicalAttackInput& input);

double legacy_player_attribute_attack(const PlayerAttributeAttackInput& input);
double legacy_player_attribute_attack_japan(const PlayerAttributeAttackInput& input);
double legacy_monster_attribute_attack(std::uint32_t attack_min,
                                       std::uint32_t attack_max,
                                       std::int32_t random_value);
double legacy_titan_attribute_attack(std::uint32_t titan_attribute,
                                     std::uint16_t owner_sim_mek,
                                     float attack_rate,
                                     std::int32_t random_value);

double legacy_titan_player_physical_attack(const PhysicalAttackInput& player,
                                           const PhysicalAttackInput& titan);
double legacy_titan_player_attribute_attack(
    const PlayerAttributeAttackInput& player,
    std::uint32_t titan_attribute,
    std::uint16_t owner_sim_mek,
    float titan_attack_rate,
    std::int32_t titan_random_value);

double legacy_phy_defence_level(const DefenceInput& input);

}  // namespace mxh::game
