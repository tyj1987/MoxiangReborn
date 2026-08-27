#include "mxh/audio/sfx_player.hpp"
#include "mxh/log/mlog.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#ifdef _WIN32
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <xaudio2.h>
#include <wrl/client.h>
#include <windows.h>
#endif

namespace mxh::audio {
namespace { void error(std::string* out, const std::string& value) { if (out) *out = value; } }

// SoundList is data, not a trusted path source.  Keep every decoded file
// inside the profile's Sound directory even when an entry contains `..` or
// an absolute path.  This also makes the release profile boundary explicit.
static std::filesystem::path resolveWithinRoot(const std::filesystem::path& root,
                                                const std::string& file_name) {
    std::error_code ec;
    const auto canonical_root = std::filesystem::weakly_canonical(root, ec);
    if (ec || canonical_root.empty()) return {};
    const auto candidate = std::filesystem::weakly_canonical(root / file_name, ec);
    if (ec || candidate.empty()) return {};
    const auto relative = candidate.lexically_relative(canonical_root);
    if (relative.empty() || relative.is_absolute()) return {};
    const auto first = relative.begin();
    if (first == relative.end() || *first == std::filesystem::path("..")) return {};
    return candidate;
}

#ifdef _WIN32
struct SfxPlayer::MediaState {
    Microsoft::WRL::ComPtr<IXAudio2> engine;
    IXAudio2MasteringVoice* mastering = nullptr;
    struct Voice {
        IXAudio2SourceVoice* source = nullptr;
        std::vector<std::uint8_t> pcm;
        WAVEFORMATEX format{};
    };
    // Effects must overlap: a sword hit cannot cut off the footstep or the
    // previous skill cue.  A bounded round-robin pool keeps this deterministic
    // under a burst of BEFF/SFX events while avoiding unbounded voice growth.
    static constexpr std::size_t kVoiceCount = 16;
    std::array<Voice, kVoiceCount> voices{};
    std::size_t next_voice = 0;
    bool mf_started = false;
    bool com_owned = false;
    ~MediaState() {
        for (auto& voice : voices) {
            if (voice.source) {
                voice.source->Stop(0);
                voice.source->FlushSourceBuffers();
                voice.source->DestroyVoice();
            }
        }
        if (mastering) mastering->DestroyVoice();
        if (mf_started) MFShutdown();
        if (com_owned) CoUninitialize();
    }
};

static bool ensure_media(SfxPlayer::MediaState& m, std::string* out) {
    if (m.engine) return true;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(com)) m.com_owned = true;
    else if (com != RPC_E_CHANGED_MODE && com != S_FALSE) { error(out, "SFX COM initialization failed"); return false; }
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_FULL))) { error(out, "SFX Media Foundation initialization failed"); return false; }
    m.mf_started = true;
    if (FAILED(XAudio2Create(&m.engine, 0, XAUDIO2_DEFAULT_PROCESSOR)) ||
        FAILED(m.engine->CreateMasteringVoice(&m.mastering))) { error(out, "SFX XAudio2 initialization failed"); return false; }
    return true;
}

static void stop_media(SfxPlayer::MediaState& m) noexcept {
    for (auto& voice : m.voices) {
        if (voice.source) {
            voice.source->Stop(0);
            voice.source->FlushSourceBuffers();
            voice.source->DestroyVoice();
            voice.source = nullptr;
        }
        voice.pcm.clear();
        voice.format = {};
    }
}
#endif

SfxPlayer::SfxPlayer() = default;
SfxPlayer::~SfxPlayer() { stop(); }

bool SfxPlayer::initialize(const std::filesystem::path& root, std::string* out) {
    stop();
    sound_root_ = std::filesystem::weakly_canonical(root);
    ready_ = mxh::compat::load_sound_list(sound_root_ / "SoundList.bin", manifest_, out);
    return ready_;
}

std::filesystem::path SfxPlayer::resolve(std::uint16_t id) const {
    if (!ready_ || id >= manifest_.entries.size()) return {};
    const auto& entry = manifest_.entries[id];
    if (!entry.available || entry.streaming) return {};
    const auto path = resolveWithinRoot(sound_root_, entry.file_name);
    return !path.empty() && std::filesystem::is_regular_file(path)
        ? path : std::filesystem::path{};
}

float SfxPlayer::distanceGain(float distance, float min_distance,
                              float max_distance) noexcept {
    if (!std::isfinite(distance) || !std::isfinite(min_distance) ||
        !std::isfinite(max_distance)) return 0.0f;
    if (distance <= min_distance || max_distance <= min_distance) return 1.0f;
    if (distance >= max_distance) return 0.0f;
    return std::clamp(1.0f - (distance - min_distance) /
        (max_distance - min_distance), 0.0f, 1.0f);
}

bool SfxPlayer::play(std::uint16_t id, std::string* out) {
    return playAtOnBus(id, 0.0f, 1.0f, out);
}

bool SfxPlayer::playAt(std::uint16_t id, float distance, std::string* out) {
    return playAtOnBus(id, distance, 1.0f, out);
}

bool SfxPlayer::playOnBus(std::uint16_t id, float bus_gain, std::string* out) {
    return playAtOnBus(id, 0.0f, bus_gain, out);
}

bool SfxPlayer::playAtOnBus(std::uint16_t id, float distance, float bus_gain,
                            std::string* out) {
    const auto path = resolve(id);
    if (path.empty()) { error(out, "SFX sound ID is missing or not a WAV entry"); return false; }
    if (!std::isfinite(bus_gain)) {
        error(out, "SFX bus gain is not finite");
        return false;
    }
    const auto& entry = manifest_.entries[id];
    current_entry_volume_ = entry.volume > 0.0f ? entry.volume : 1.0f;
    current_bus_gain_ = std::clamp(bus_gain, 0.0f, 1.0f);
    current_distance_gain_ = distanceGain(
        distance, entry.min_distance, entry.max_distance);
#ifdef _WIN32
    if (!media_) media_ = std::make_unique<MediaState>();
    if (!ensure_media(*media_, out)) return false;
    auto& voice = media_->voices[media_->next_voice++ %
                                 SfxPlayer::MediaState::kVoiceCount];
    if (voice.source) {
        voice.source->Stop(0);
        voice.source->FlushSourceBuffers();
        voice.source->DestroyVoice();
        voice.source = nullptr;
    }
    voice.pcm.clear();
    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
    HRESULT hr = MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader);
    Microsoft::WRL::ComPtr<IMFMediaType> requested;
    if (SUCCEEDED(hr)) hr = MFCreateMediaType(&requested);
    if (SUCCEEDED(hr)) hr = requested->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(hr)) hr = requested->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    if (SUCCEEDED(hr)) hr = reader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, requested.Get());
    Microsoft::WRL::ComPtr<IMFMediaType> actual;
    if (SUCCEEDED(hr)) hr = reader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &actual);
    UINT32 channels = 0, rate = 0, bits = 0;
    if (SUCCEEDED(hr)) hr = actual->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
    if (SUCCEEDED(hr)) hr = actual->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
    if (SUCCEEDED(hr)) hr = actual->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits);
    if (FAILED(hr) || channels == 0 || rate == 0 || bits == 0 || bits > 32) { error(out, "SFX WAV format is invalid"); return false; }
    DWORD flags = 0;
    while (true) {
        Microsoft::WRL::ComPtr<IMFSample> sample; LONGLONG timestamp = 0;
        hr = reader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, nullptr, &flags, &timestamp, &sample);
        if (FAILED(hr)) { error(out, "SFX WAV decode failed"); return false; }
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
        if (!sample) continue;
        Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
        if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) continue;
        BYTE* data = nullptr; DWORD max = 0, current = 0;
        if (SUCCEEDED(buffer->Lock(&data, &max, &current))) { voice.pcm.insert(voice.pcm.end(), data, data + current); buffer->Unlock(); }
    }
    voice.format = {}; voice.format.wFormatTag = WAVE_FORMAT_PCM; voice.format.nChannels = static_cast<WORD>(channels);
    voice.format.nSamplesPerSec = rate; voice.format.wBitsPerSample = static_cast<WORD>(bits);
    voice.format.nBlockAlign = static_cast<WORD>(channels * bits / 8); voice.format.nAvgBytesPerSec = rate * voice.format.nBlockAlign;
    if (voice.pcm.empty() || FAILED(media_->engine->CreateSourceVoice(&voice.source, &voice.format))) { error(out, "SFX source voice creation failed"); return false; }
    XAUDIO2_BUFFER buffer{}; buffer.AudioBytes = static_cast<UINT32>(voice.pcm.size()); buffer.pAudioData = voice.pcm.data();
    if (entry.loop) buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    else buffer.Flags = XAUDIO2_END_OF_STREAM;
    const float gain = std::clamp(volume_ * current_bus_gain_ *
        current_entry_volume_ * current_distance_gain_, 0.0f, 1.0f);
    hr = voice.source->SubmitSourceBuffer(&buffer); if (SUCCEEDED(hr)) hr = voice.source->SetVolume(gain); if (SUCCEEDED(hr)) hr = voice.source->Start(0);
    if (FAILED(hr)) {
        voice.source->Stop(0); voice.source->FlushSourceBuffers();
        voice.source->DestroyVoice(); voice.source = nullptr; voice.pcm.clear();
        error(out, "SFX playback failed"); return false;
    }
    current_id_ = id; MLOG_DEBUG("[audio] playing SFX id=%u", id); return true;
#else
    (void)id; error(out, "SFX playback is only supported on Windows"); return false;
#endif
}

void SfxPlayer::stop() noexcept {
    current_id_ = 0xffffu;
    current_entry_volume_ = 1.0f;
    current_bus_gain_ = 1.0f;
    current_distance_gain_ = 1.0f;
#ifdef _WIN32
    if (media_) stop_media(*media_);
#endif
}
void SfxPlayer::setVolume(float value) noexcept {
    // Runtime callers (including focus/device-recovery paths) can bypass the
    // JSON settings validator.  Never propagate a non-finite gain into
    // XAudio2 or retain it for the next sound.
    volume_ = std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : 1.0f;
#ifdef _WIN32
    if (media_) {
        const auto gain = std::clamp(volume_ * current_bus_gain_ *
                                     current_entry_volume_ * current_distance_gain_,
                                     0.0f, 1.0f);
        for (auto& voice : media_->voices) {
            if (voice.source) voice.source->SetVolume(gain);
        }
    }
#endif
}

} // namespace mxh::audio
