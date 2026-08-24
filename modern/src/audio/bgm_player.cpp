#include "mxh/audio/bgm_player.hpp"

#include "mxh/log/mlog.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <xaudio2.h>
#include <wrl/client.h>
#include <windows.h>
#endif

namespace mxh::audio {
namespace {
constexpr std::uint16_t kNoSound = 0xffffu;

void setError(std::string* error, const std::string& message) {
    if (error) *error = message;
}

} // namespace

#ifdef _WIN32
struct BgmPlayer::MediaState {
    Microsoft::WRL::ComPtr<IXAudio2> engine;
    IXAudio2MasteringVoice* mastering = nullptr;
    IXAudio2SourceVoice* source = nullptr;
    std::vector<std::uint8_t> pcm;
    WAVEFORMATEX format{};
    bool mf_started = false;
    bool com_owned = false;

    ~MediaState() {
        if (source) {
            source->Stop(0);
            source->FlushSourceBuffers();
            source->DestroyVoice();
            source = nullptr;
        }
        if (mastering) {
            mastering->DestroyVoice();
            mastering = nullptr;
        }
        if (mf_started) MFShutdown();
        if (com_owned) CoUninitialize();
    }
};

bool ensure_media(BgmPlayer::MediaState& media, std::string* error) {
    if (!media.engine) {
        const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(com)) media.com_owned = true;
        else if (com != RPC_E_CHANGED_MODE && com != S_FALSE) {
            setError(error, "COM initialization failed: " + std::to_string(com));
            return false;
        }
        const HRESULT mf = MFStartup(MF_VERSION, MFSTARTUP_FULL);
        if (FAILED(mf)) {
            setError(error, "Media Foundation initialization failed: " + std::to_string(mf));
            return false;
        }
        media.mf_started = true;
        const HRESULT xa = XAudio2Create(&media.engine, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(xa)) {
            setError(error, "XAudio2 initialization failed: " + std::to_string(xa));
            return false;
        }
        const HRESULT master = media.engine->CreateMasteringVoice(
            &media.mastering, XAUDIO2_DEFAULT_CHANNELS,
            XAUDIO2_DEFAULT_SAMPLERATE);
        if (FAILED(master)) {
            setError(error, "XAudio2 mastering voice failed: " + std::to_string(master));
            return false;
        }
    }
    return true;
}

void stop_media(BgmPlayer::MediaState& media) noexcept {
    if (!media.source) return;
    media.source->Stop(0);
    media.source->FlushSourceBuffers();
    media.source->DestroyVoice();
    media.source = nullptr;
    media.pcm.clear();
}
#endif

BgmPlayer::BgmPlayer() = default;
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
    const bool loop = manifest_.entries[sound_id].loop;
    stop();
#ifdef _WIN32
    if (!media_) media_ = std::make_unique<MediaState>();
    if (!ensure_media(*media_, error)) return false;

    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
    HRESULT hr = MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader);
    if (FAILED(hr)) {
        setError(error, "Media Foundation could not open BGM: " + std::to_string(hr));
        return false;
    }
    Microsoft::WRL::ComPtr<IMFMediaType> requested;
    hr = MFCreateMediaType(&requested);
    if (SUCCEEDED(hr)) hr = requested->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(hr)) hr = requested->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    if (SUCCEEDED(hr)) hr = reader->SetCurrentMediaType(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, requested.Get());
    if (FAILED(hr)) {
        setError(error, "Media Foundation could not configure PCM output: " + std::to_string(hr));
        return false;
    }
    Microsoft::WRL::ComPtr<IMFMediaType> actual;
    hr = reader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &actual);
    if (FAILED(hr)) {
        setError(error, "Media Foundation did not return an audio format: " + std::to_string(hr));
        return false;
    }
    UINT32 channels = 0, sample_rate = 0, bits = 0;
    if (FAILED(actual->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels)) ||
        FAILED(actual->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sample_rate)) ||
        FAILED(actual->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits)) ||
        channels == 0 || sample_rate == 0 || bits == 0 || bits > 32) {
        setError(error, "BGM has an invalid PCM format");
        return false;
    }
    media_->format = {};
    media_->format.wFormatTag = WAVE_FORMAT_PCM;
    media_->format.nChannels = static_cast<WORD>(channels);
    media_->format.nSamplesPerSec = sample_rate;
    media_->format.wBitsPerSample = static_cast<WORD>(bits);
    media_->format.nBlockAlign = static_cast<WORD>(channels * bits / 8);
    media_->format.nAvgBytesPerSec = sample_rate * media_->format.nBlockAlign;
    media_->format.cbSize = 0;

    DWORD flags = 0;
    while (true) {
        Microsoft::WRL::ComPtr<IMFSample> sample;
        LONGLONG timestamp = 0;
        hr = reader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0,
                                nullptr, &flags, &timestamp, &sample);
        if (FAILED(hr)) {
            setError(error, "BGM decode failed: " + std::to_string(hr));
            media_->pcm.clear();
            return false;
        }
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
        if (!sample) continue;
        Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
        if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) continue;
        BYTE* data = nullptr;
        DWORD current = 0, max = 0;
        if (FAILED(buffer->Lock(&data, &max, &current))) continue;
        media_->pcm.insert(media_->pcm.end(), data, data + current);
        buffer->Unlock();
    }
    if (media_->pcm.empty() || media_->format.nBlockAlign == 0 ||
        media_->pcm.size() % media_->format.nBlockAlign != 0) {
        setError(error, "BGM decoder returned no PCM data");
        return false;
    }
    hr = media_->engine->CreateSourceVoice(&media_->source, &media_->format);
    if (FAILED(hr)) {
        setError(error, "XAudio2 source voice failed: " + std::to_string(hr));
        media_->pcm.clear();
        return false;
    }
    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = static_cast<UINT32>(media_->pcm.size());
    buffer.pAudioData = media_->pcm.data();
    if (loop) {
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    } else {
        buffer.Flags = XAUDIO2_END_OF_STREAM;
    }
    hr = media_->source->SubmitSourceBuffer(&buffer);
    if (SUCCEEDED(hr)) hr = media_->source->SetVolume(std::clamp(volume_, 0.0f, 1.0f));
    if (SUCCEEDED(hr)) hr = media_->source->Start(0);
    if (FAILED(hr)) {
        stop_media(*media_);
        setError(error, "XAudio2 could not start BGM: " + std::to_string(hr));
        return false;
    }
    current_id_ = sound_id;
    MLOG_INFO("[audio] playing original BGM id=%u through Media Foundation/XAudio2", sound_id);
    return true;
#else
    setError(error, "BGM playback is only supported on Windows");
    return false;
#endif
}

void BgmPlayer::stop() noexcept {
#ifdef _WIN32
    if (media_) stop_media(*media_);
#endif
    current_id_ = kNoSound;
}

void BgmPlayer::setVolume(float normalized) noexcept {
    volume_ = std::clamp(normalized, 0.0f, 1.0f);
#ifdef _WIN32
    if (media_ && media_->source) media_->source->SetVolume(volume_);
#endif
}

} // namespace mxh::audio
