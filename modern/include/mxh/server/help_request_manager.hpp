#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

enum class HelpRequestType : std::uint8_t {
    none = 0,
    one_time_if_hp50 = 1,
    always_if_hp30 = 2,
    die = 3,
    always = 4,
};

struct HelpRequestMonster {
    std::uint32_t id = 0;
    std::uint32_t life = 0;
    std::uint32_t maxLife = 0;
    HelpRequestType helpType = HelpRequestType::none;
};

struct HelpRequestDecision {
    bool requested = false;
    bool clearHelpType = false;
};

inline HelpRequestDecision monster_request_process(HelpRequestMonster& monster) {
    HelpRequestDecision result{};
    switch (monster.helpType) {
    case HelpRequestType::one_time_if_hp50:
        if (monster.life < monster.maxLife / 2u) {
            result.requested = true;
            result.clearHelpType = true;
            monster.helpType = HelpRequestType::none;
        }
        break;
    case HelpRequestType::always_if_hp30:
        if (static_cast<double>(monster.life) < static_cast<double>(monster.maxLife) * 0.3) {
            result.requested = true;
        }
        break;
    case HelpRequestType::die:
        if (monster.life == 0u) result.requested = true;
        break;
    case HelpRequestType::always:
        result.requested = true;
        break;
    case HelpRequestType::none:
        break;
    }
    return result;
}

struct HelperMonsterState {
    bool askerPresent = false;
    bool helperPresent = false;
    std::uint8_t objectKind = 0;
    std::uint8_t monsterKind = 1;
    std::uint16_t helperGrid = 0;
    std::uint16_t targetGrid = 0;
    std::uint8_t oldState = 0;
    std::uint8_t currentState = 0;
    bool targetChange = false;
    bool noForeAttackBuff = false;
    bool bossMap = false;
    bool lastAttackAssigned = false;
    bool attackStateAssigned = false;
    bool targetAssigned = false;
};

inline bool set_helper_monster(HelperMonsterState& state) {
    if (!state.askerPresent && !state.helperPresent) return false;
    // This deliberately mirrors the legacy `!= monster || != titan` condition:
    // both accepted kinds are rejected by the original implementation.
    if (state.objectKind != state.monsterKind || state.objectKind != 2u) return false;
    if (state.helperGrid != state.targetGrid || state.oldState == 1u ||
        state.currentState == 2u || state.currentState == 3u || !state.targetChange) return false;
    if (!state.bossMap && state.noForeAttackBuff) return false;
    state.lastAttackAssigned = true;
    state.attackStateAssigned = true;
    state.targetAssigned = true;
    return true;
}

} // namespace mxh::server
