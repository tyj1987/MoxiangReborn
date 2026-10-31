#pragma once

// Portable audio decisions used by the Windows playback path.
// No window, device, or decoder headers. These functions copy the rules
// already implemented by BgmPlayer, SfxPlayer, and the mxh_client call
// sites. They do not invent a fade: the client audio path has none.
// Contract tests feed synthetic numbers only. Passing these tests does
// not play audio and does not prove a 1:1 listen against the original.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace mxh::audio::contract {

// BgmPlayer and SfxPlayer use 0xffff as "no current sound" (stop()).
inline constexpr std::uint16_t kNoSound = 0xffffu;

// mxh_client plays this id for the login theme before any map loads.
// main.cpp: "1667 is the original login theme in SoundList.bin."
inline constexpr std::uint16_t kLoginBgmSoundId = 1667u;

// SfxPlayer::MediaState keeps a fixed round-robin pool so overlapping
// effects replace the oldest voice instead of growing without a bound.
inline constexpr std::size_t kSfxVoiceCount = 16;

enum class Channel { Bgm, Sfx };

enum class BgmAction { Keep, Stop, Play };

// Map and login playback: a missing file fails the load unless the debug
// UI-bounds switch is on, in which case the client logs and continues.
enum class MissingBgm { FailLoad, WarnAndContinue };

enum class CueReject {
    Playable,
    OutOfRange,
    WrongEntry,
    EscapesRoot,
    MissingFile,
};

// Mirrors CEngine::AudioCue plus an unknown value. The client still picks
// the sound id; this only answers whether that cue is allowed to play.
enum class EngineCueKind { Attack, Skill, UiClick, Pickup, Other };

struct BgmCommand {
    BgmAction action = BgmAction::Keep;
    std::uint16_t sound_id = kNoSound;
};

// BgmPlayer::setVolume: a non-finite bus becomes silence, then clamp [0, 1].
inline float bgm_bus_volume(float normalized) noexcept {
    if (!std::isfinite(normalized)) normalized = 0.0f;
    return std::clamp(normalized, 0.0f, 1.0f);
}

// BgmPlayer::play: a non-finite or non-positive SoundList volume becomes 1.
// Values above 1 stay raw; bgm_output_gain clamps the product.
inline float bgm_entry_volume(float entry) noexcept {
    return std::isfinite(entry) && entry > 0.0f ? entry : 1.0f;
}

inline float bgm_output_gain(float bus_volume, float entry_volume) noexcept {
    return std::clamp(bus_volume * entry_volume, 0.0f, 1.0f);
}

// SfxPlayer::setVolume: a non-finite runtime value falls back to 1, not 0.
// That differs from BgmPlayer on purpose.
inline float sfx_bus_volume(float value) noexcept {
    return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : 1.0f;
}

// SfxPlayer::playAtOnBus: only `entry > 0` is kept. NaN and negatives
// become 1. Positive infinity is kept and clamped in the product.
inline float sfx_entry_volume(float entry) noexcept {
    return entry > 0.0f ? entry : 1.0f;
}

// A non-finite per-call bus gain fails the play before any metadata is stored.
inline bool sfx_bus_gain_finite(float bus_gain) noexcept {
    return std::isfinite(bus_gain);
}

inline float sfx_clamped_bus_gain(float bus_gain) noexcept {
    return std::clamp(bus_gain, 0.0f, 1.0f);
}

// SfxPlayer::distanceGain. Linear falloff between the SoundList distances.
// Non-finite inputs are silence. A non-positive span is full volume.
inline float distance_gain(float distance, float min_distance,
                           float max_distance) noexcept {
    if (!std::isfinite(distance) || !std::isfinite(min_distance) ||
        !std::isfinite(max_distance)) return 0.0f;
    if (distance <= min_distance || max_distance <= min_distance) return 1.0f;
    if (distance >= max_distance) return 0.0f;
    return std::clamp(1.0f - (distance - min_distance) /
                          (max_distance - min_distance),
                      0.0f, 1.0f);
}

// Stored on the voice so a later setVolume can rescale without the entry.
inline float sfx_voice_mix(float bus_gain, float entry,
                           float distance) noexcept {
    return std::clamp(bus_gain * entry * distance, 0.0f, 1.0f);
}

inline float sfx_output_gain(float bus_volume, float bus_gain, float entry,
                             float distance) noexcept {
    return std::clamp(bus_volume * bus_gain * entry * distance, 0.0f, 1.0f);
}

// setVolume multiplies the already-stored voice mix by the new bus volume.
inline float scale_stored_mix(float bus_volume, float stored_mix) noexcept {
    return std::clamp(bus_volume * stored_mix, 0.0f, 1.0f);
}

// next_voice is post-incremented by the caller. Slot 16 wraps to 0.
inline std::size_t sfx_voice_slot(std::size_t issued) noexcept {
    return issued % kSfxVoiceCount;
}

// BGM resolves only available streaming rows. SFX resolves only available
// non-streaming rows. NULL.WAV rows are already `available == false`.
inline bool channel_accepts_entry(Channel channel, bool available,
                                  bool streaming) noexcept {
    if (!available) return false;
    return channel == Channel::Bgm ? streaming : !streaming;
}

// After weakly_canonical, the relative path must stay inside the Sound root.
// Empty, absolute, or a leading ".." is rejected.
inline bool relative_stays_in_root(bool relative_empty, bool relative_absolute,
                                   bool first_is_dotdot) noexcept {
    if (relative_empty || relative_absolute) return false;
    if (first_is_dotdot) return false;
    return true;
}

// A resolved path plays only when it stayed inside the root and names a file.
// There is no substitute sound.
inline bool cue_file_present(bool inside_root, bool regular_file) noexcept {
    return inside_root && regular_file;
}

inline CueReject classify_cue(bool in_range, Channel channel, bool available,
                              bool streaming, bool relative_empty,
                              bool relative_absolute, bool first_is_dotdot,
                              bool regular_file) noexcept {
    if (!in_range) return CueReject::OutOfRange;
    if (!channel_accepts_entry(channel, available, streaming)) {
        return CueReject::WrongEntry;
    }
    if (!relative_stays_in_root(relative_empty, relative_absolute,
                                first_is_dotdot)) {
        return CueReject::EscapesRoot;
    }
    if (!regular_file) return CueReject::MissingFile;
    return CueReject::Playable;
}

inline BgmCommand login_bgm() noexcept {
    return BgmCommand{BgmAction::Play, kLoginBgmSoundId};
}

// Map load plays BMHM *BGM as a raw id, including 0. It does not substitute.
inline BgmCommand map_bgm(std::uint16_t bgm_sound_num) noexcept {
    return BgmCommand{BgmAction::Play, bgm_sound_num};
}

// Map rollback: same id keeps the current track, 0xffff stops, otherwise
// the previous id is played again.
inline BgmCommand restore_previous_bgm(std::uint16_t current,
                                       std::uint16_t previous) noexcept {
    if (current == previous) return BgmCommand{BgmAction::Keep, current};
    if (previous == kNoSound) return BgmCommand{BgmAction::Stop, kNoSound};
    return BgmCommand{BgmAction::Play, previous};
}

inline MissingBgm missing_bgm_policy(bool debug_ui_bounds) noexcept {
    return debug_ui_bounds ? MissingBgm::WarnAndContinue : MissingBgm::FailLoad;
}

// WM_ACTIVATEAPP and the option apply path: unfocused is silence.
inline float focused_volume(float stored, bool focused) noexcept {
    return focused ? stored : 0.0f;
}

// Option checkbox is on when the stored volume is strictly positive.
inline bool checkbox_from_positive_volume(float volume) noexcept {
    return volume > 0.0f;
}

// Option gauge is lround(volume * 100). The apply path divides by 100
// and clamps. The environment slider writes ambient volume, not the SFX bus.
inline int volume_to_percent(float volume) noexcept {
    return static_cast<int>(std::lround(volume * 100.0f));
}

inline float percent_to_volume(int percent) noexcept {
    return std::clamp(static_cast<float>(percent) / 100.0f, 0.0f, 1.0f);
}

inline float muted_or_volume(bool enabled, float normalized) noexcept {
    return enabled ? normalized : 0.0f;
}

// BEFF/effect events ignore id 0 and anything that does not fit in 16 bits.
// 65535 is still passed through; the cue path treats that value as no sound
// only when the selected attack/pickup slot itself is kNoSound.
inline bool effect_sound_id_accepted(std::uint32_t sound_id) noexcept {
    return sound_id != 0u && sound_id <= 0xffffu;
}

// SoundList loop flag. True repeats until stop; false plays once.
inline bool entry_loops(bool loop_flag) noexcept {
    return loop_flag;
}

// Generic engine cues drop UI click and skill. Skill is also dropped on
// the spatial path. UI click is not dropped there.
inline bool generic_engine_cue_plays(EngineCueKind cue) noexcept {
    return cue != EngineCueKind::UiClick && cue != EngineCueKind::Skill;
}

inline bool spatial_engine_cue_plays(EngineCueKind cue) noexcept {
    return cue != EngineCueKind::Skill;
}

}  // namespace mxh::audio::contract
