#pragma once

#include "mxh/compat/sound_list.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace mxh::audio {

// One-shot WAV channel backed by the original SoundList metadata.  This is
// intentionally separate from BgmPlayer so UI/gameplay effects never stop
// the active map or login music.
class SfxPlayer {
public:
#ifdef _WIN32
    struct MediaState;
#endif
    SfxPlayer();
    ~SfxPlayer();
    SfxPlayer(const SfxPlayer&) = delete;
    SfxPlayer& operator=(const SfxPlayer&) = delete;

    [[nodiscard]] bool initialize(const std::filesystem::path& sound_root,
                                  std::string* error = nullptr);
    [[nodiscard]] bool play(std::uint16_t sound_id, std::string* error = nullptr);
    void stop() noexcept;
    void setVolume(float normalized) noexcept;
    [[nodiscard]] bool ready() const noexcept { return ready_; }
    [[nodiscard]] std::filesystem::path resolve(std::uint16_t sound_id) const;
    [[nodiscard]] const mxh::compat::SoundList& manifest() const noexcept { return manifest_; }

private:
#ifdef _WIN32
    std::unique_ptr<MediaState> media_;
#endif
    bool ready_ = false;
    float volume_ = 1.0f;
    float current_entry_volume_ = 1.0f;
    std::uint16_t current_id_ = 0xffffu;
    std::filesystem::path sound_root_;
    mxh::compat::SoundList manifest_;
};

} // namespace mxh::audio
