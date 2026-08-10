#pragma once

#include "mxh/compat/sound_list.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace mxh::audio {

// Plays the original streamed BGM entries from PlayDH/Sound/SoundList.bin.
// The legacy client has one active BGM channel, so replacing the current track
// is intentional and matches its state-driven playback model.
class BgmPlayer {
public:
    BgmPlayer() = default;
    ~BgmPlayer();
    BgmPlayer(const BgmPlayer&) = delete;
    BgmPlayer& operator=(const BgmPlayer&) = delete;

    [[nodiscard]] bool initialize(const std::filesystem::path& sound_root,
                                  std::string* error = nullptr);
    [[nodiscard]] bool play(std::uint16_t sound_id, std::string* error = nullptr);
    void stop() noexcept;
    void setVolume(float normalized) noexcept;

    [[nodiscard]] bool ready() const noexcept { return ready_; }
    [[nodiscard]] std::uint16_t currentSoundId() const noexcept { return current_id_; }
    [[nodiscard]] std::filesystem::path resolve(std::uint16_t sound_id) const;
    [[nodiscard]] const mxh::compat::SoundList& manifest() const noexcept { return manifest_; }

private:
    bool ready_ = false;
    float volume_ = 1.0f;
    std::uint16_t current_id_ = 0xffffu;
    std::filesystem::path sound_root_;
    mxh::compat::SoundList manifest_;
};

} // namespace mxh::audio
