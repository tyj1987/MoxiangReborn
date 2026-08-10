#include "mxh/audio/bgm_player.hpp"

#include "mxh/log/mlog.hpp"

#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

namespace mxh::audio {
namespace {
constexpr std::uint16_t kNoSound = 0xffffu;

void setError(std::string* error, const std::string& message) {
    if (error) *error = message;
}

#ifdef _WIN32
bool sendMci(const std::wstring& command, std::string* error) {
    wchar_t detail[256]{};
    const MCIERROR result = mciSendStringW(command.c_str(), nullptr, 0, nullptr);
    if (result == 0) return true;
    mciGetErrorStringW(result, detail, static_cast<UINT>(std::size(detail)));
    setError(error, "MCI error " + std::to_string(result));
    MLOG_ERROR("[audio] MCI command failed (%u): %ls", result, detail);
    return false;
}
#endif
} // namespace

BgmPlayer::~BgmPlayer() { stop(); }

bool BgmPlayer::initialize(const std::filesystem::path& sound_root, std::string* error) {
    stop();
    sound_root_ = std::filesystem::weakly_canonical(sound_root);
    if (!mxh::compat::load_sound_list(sound_root_ / "SoundList.bin", manifest_, error)) {
        ready_ = false;
        return false;
    }
    ready_ = true;
    return true;
}

std::filesystem::path BgmPlayer::resolve(std::uint16_t sound_id) const {
    if (!ready_ || sound_id >= manifest_.entries.size()) return {};
    const auto& entry = manifest_.entries[sound_id];
    if (!entry.available || !entry.streaming) return {};
    const auto candidate = std::filesystem::weakly_canonical(sound_root_ / entry.file_name);
    return std::filesystem::is_regular_file(candidate) ? candidate : std::filesystem::path{};
}

bool BgmPlayer::play(std::uint16_t sound_id, std::string* error) {
    const auto path = resolve(sound_id);
    if (path.empty()) {
        setError(error, "BGM sound ID is missing or is not a streaming entry");
        return false;
    }
    stop();
#ifdef _WIN32
    if (path.native().find(L'"') != std::wstring::npos) {
        setError(error, "BGM path contains an unsupported quote");
        return false;
    }
    if (!sendMci(L"open \"" + path.native() + L"\" type mpegvideo alias mxh_bgm", error)) return false;
    const auto mciVolume = static_cast<unsigned>(std::clamp(volume_, 0.0f, 1.0f) * 1000.0f);
    sendMci(L"setaudio mxh_bgm volume to " + std::to_wstring(mciVolume), nullptr);
    if (!sendMci(L"play mxh_bgm repeat", error)) {
        sendMci(L"close mxh_bgm", nullptr);
        return false;
    }
    current_id_ = sound_id;
    MLOG_INFO("[audio] playing original BGM id=%u", sound_id);
    return true;
#else
    setError(error, "BGM playback is only supported on Windows");
    return false;
#endif
}

void BgmPlayer::stop() noexcept {
#ifdef _WIN32
    if (current_id_ != kNoSound) {
        sendMci(L"stop mxh_bgm", nullptr);
        sendMci(L"close mxh_bgm", nullptr);
    }
#endif
    current_id_ = kNoSound;
}

void BgmPlayer::setVolume(float normalized) noexcept {
    volume_ = std::clamp(normalized, 0.0f, 1.0f);
#ifdef _WIN32
    if (current_id_ != kNoSound) {
        const auto value = static_cast<unsigned>(volume_ * 1000.0f);
        sendMci(L"setaudio mxh_bgm volume to " + std::to_wstring(value), nullptr);
    }
#endif
}

} // namespace mxh::audio
