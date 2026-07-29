// c_monster_speech_manager.hpp - per-server speech cue registry.
//
// 1:1 port of legacy [Server]Map/cMonsterSpeechManager.h.
// The legacy class loads MonsterSpeechInfoList.bin and lets the boss
// trigger a chat line by SpeechIdx. Modern port loads into memory at
// startup and exposes typed accessors.

#pragma once

#include "mxh/game/monster_types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace mxh::server {

inline constexpr std::uint8_t MAX_SPEECH_PER_MONSTER = 8;

// One speech cue (legacy cMonsterSpeechManager::SpeechInfo).
struct MonsterSpeech final {
    std::uint32_t speech_id  = 0;
    std::uint32_t monster_kind = 0;
    std::uint8_t  trigger    = 0;     // 0=on-spawn, 1=on-HP-50%, 2=on-die
    std::uint8_t  reserved0  = 0;
    std::uint16_t reserved1  = 0;
    char          text[mxh::game::MAX_MONSTER_SPEECH_LEN] = {};
};

// In-memory speech registry (1:1 with legacy cMonsterSpeechManager).
class cMonsterSpeechManager final {
public:
    // Register a speech cue. Used by the resource loader; tests construct
    // cues directly.
    void register_speech(const MonsterSpeech& s) noexcept;

    // Query the cue for a given MonsterKind + trigger. Returns nullptr if
    // not found.
    const MonsterSpeech* find(std::uint32_t monster_kind, std::uint8_t trigger) const noexcept;

    // Convenience: pick the speech whose trigger == on-die (2).
    const MonsterSpeech* death_speech(std::uint32_t monster_kind) const noexcept;

    std::size_t size() const noexcept { return speeches_.size(); }

private:
    std::vector<MonsterSpeech> speeches_;
};

}  // namespace mxh::server