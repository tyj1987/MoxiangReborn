// c_monster_speech_manager.cpp

#include "mxh/server/c_monster_speech_manager.hpp"

namespace mxh::server {

void cMonsterSpeechManager::register_speech(const MonsterSpeech& s) noexcept {
    speeches_.push_back(s);
}

const MonsterSpeech* cMonsterSpeechManager::find(std::uint32_t monster_kind, std::uint8_t trigger) const noexcept {
    for (const auto& s : speeches_) {
        if (s.monster_kind == monster_kind && s.trigger == trigger) return &s;
    }
    return nullptr;
}

const MonsterSpeech* cMonsterSpeechManager::death_speech(std::uint32_t monster_kind) const noexcept {
    // Legacy "trigger == 2" is on-die.
    return find(monster_kind, 2);
}

}  // namespace mxh::server