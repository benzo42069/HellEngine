#include <engine/audio_system.h>

#include <engine/logging.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace engine {

namespace {

float clampVolume(const float volume) {
    return std::clamp(volume, 0.0F, 1.0F);
}

std::size_t busIndex(const AudioBus bus) {
    return static_cast<std::size_t>(bus);
}

float busGain(const AudioBus bus, const float master, const float music, const float sfx) {
    if (bus == AudioBus::Music) return master * music;
    if (bus == AudioBus::Sfx) return master * sfx;
    return master;
}

bool decodeClip(const std::string& path, const SDL_AudioSpec& targetSpec, std::vector<float>& outSamples, std::uint32_t& outFrames) {
    SDL_AudioSpec wavSpec {};
    Uint8* wavBuffer = nullptr;
    Uint32 wavLength = 0;
    if (!SDL_LoadWAV(path.c_str(), &wavSpec, &wavBuffer, &wavLength)) {
        return false;
    }

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, wavSpec.format, wavSpec.channels, wavSpec.freq, targetSpec.format, targetSpec.channels, targetSpec.freq) < 0) {
        SDL_FreeWAV(wavBuffer);
        return false;
    }

    cvt.len = static_cast<int>(wavLength);
    cvt.buf = static_cast<Uint8*>(SDL_malloc(static_cast<std::size_t>(cvt.len) * static_cast<std::size_t>(cvt.len_mult)));
    if (!cvt.buf) {
        SDL_FreeWAV(wavBuffer);
        return false;
    }
    std::memcpy(cvt.buf, wavBuffer, wavLength);
    SDL_FreeWAV(wavBuffer);

    if (SDL_ConvertAudio(&cvt) != 0) {
        SDL_free(cvt.buf);
        return false;
    }

    const std::size_t sampleCount = static_cast<std::size_t>(cvt.len_cvt) / sizeof(float);
    outSamples.resize(sampleCount);
    std::memcpy(outSamples.data(), cvt.buf, static_cast<std::size_t>(cvt.len_cvt));
    SDL_free(cvt.buf);

    outFrames = static_cast<std::uint32_t>(sampleCount / static_cast<std::size_t>(targetSpec.channels));
    return outFrames > 0;
}

} // namespace

AudioSystem::~AudioSystem() { shutdown(); }

bool AudioSystem::initialize(const bool enableDevice) {
    if (initialized_) return true;

    initialized_ = true;
    outputEnabled_ = enableDevice;
    queuedEvents_.reserve(64);

    if (!enableDevice) return true;

    SDL_AudioSpec desired {};
    desired.freq = 48000;
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = &AudioSystem::audioCallback;
    desired.userdata = this;

    device_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained_, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (device_ == 0) {
        outputEnabled_ = false;
        logWarn("Audio device unavailable; running silent audio runtime: " + std::string(SDL_GetError()));
        return true;
    }

    SDL_PauseAudioDevice(device_, 0);
    return true;
}

void AudioSystem::shutdown() {
    if (!initialized_) return;

    if (device_ != 0) {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }

    initialized_ = false;
    outputEnabled_ = false;
    clips_.clear();
    routes_.clear();
    activeVoices_.clear();
    queuedEvents_.clear();
}

bool AudioSystem::loadContent(const AudioContentDatabase& content, const std::string& contentRoot) {
    clips_.clear();
    routes_.clear();
    activeVoices_.clear();
    musicClipIndex_ = static_cast<std::size_t>(-1);

    if (!initialized_) return false;

    for (const AudioClipRecord& record : content.clips) {
        ClipData clip;
        clip.id = record.id;
        clip.bus = record.bus;
        clip.loop = record.loop;
        clip.baseGain = std::max(0.0F, record.baseGain);

        bool decoded = false;
        if (outputEnabled_ && device_ != 0 && !record.path.empty()) {
            const std::filesystem::path fullPath = std::filesystem::path(contentRoot) / record.path;
            decoded = decodeClip(fullPath.string(), obtained_, clip.samples, clip.frameCount);
        }
        if (!decoded) {
            clip.samples = makeFallbackTone(record.bus);
            clip.frameCount = static_cast<std::uint32_t>(clip.samples.size() / 2U);
        }
        clips_.push_back(std::move(clip));
    }

    for (const AudioEventBinding& binding : content.events) {
        for (std::size_t i = 0; i < clips_.size(); ++i) {
            if (clips_[i].id == binding.clipId) {
                routes_.push_back(EventRoute {.event = binding.event, .clipIndex = i, .gain = std::max(0.0F, binding.gain)});
                break;
            }
        }
    }

    for (std::size_t i = 0; i < clips_.size(); ++i) {
        if (clips_[i].id == content.musicClipId) {
            musicClipIndex_ = i;
            break;
        }
    }

    return true;
}

void AudioSystem::queueEvent(const AudioEventId event) {
    queuedEvents_.push_back(event);
}

void AudioSystem::flushQueuedEvents() {
    if (device_ != 0) SDL_LockAudioDevice(device_);
    for (const AudioEventId event : queuedEvents_) {
        for (const EventRoute& route : routes_) {
            if (route.event != event) continue;
            activeVoices_.push_back(Voice {.clipIndex = route.clipIndex, .frameCursor = 0, .gain = route.gain, .loop = clips_[route.clipIndex].loop});
        }
    }
    queuedEvents_.clear();
    if (device_ != 0) SDL_UnlockAudioDevice(device_);
}

void AudioSystem::setBusVolume(const AudioBus bus, const float volume) {
    const float clamped = clampVolume(volume);
    if (bus == AudioBus::Master) masterVolume_ = clamped;
    if (bus == AudioBus::Music) musicVolume_ = clamped;
    if (bus == AudioBus::Sfx) sfxVolume_ = clamped;
}

float AudioSystem::busVolume(const AudioBus bus) const {
    if (bus == AudioBus::Master) return masterVolume_;
    if (bus == AudioBus::Music) return musicVolume_;
    if (bus == AudioBus::Sfx) return sfxVolume_;
    return masterVolume_;
}

void AudioSystem::playMusic() {
    if (musicClipIndex_ == static_cast<std::size_t>(-1) || musicClipIndex_ >= clips_.size()) return;
    if (device_ != 0) SDL_LockAudioDevice(device_);
    activeVoices_.push_back(Voice {.clipIndex = musicClipIndex_, .frameCursor = 0, .gain = 1.0F, .loop = true});
    if (device_ != 0) SDL_UnlockAudioDevice(device_);
}

void AudioSystem::stopMusic() {
    if (device_ != 0) SDL_LockAudioDevice(device_);
    activeVoices_.erase(
        std::remove_if(activeVoices_.begin(), activeVoices_.end(), [this](const Voice& v) { return v.clipIndex == musicClipIndex_; }),
        activeVoices_.end()
    );
    if (device_ != 0) SDL_UnlockAudioDevice(device_);
}

void AudioSystem::audioCallback(void* userdata, Uint8* stream, const int len) {
    auto* self = static_cast<AudioSystem*>(userdata);
    if (!self) {
        std::memset(stream, 0, static_cast<std::size_t>(len));
        return;
    }
    const int sampleCount = len / static_cast<int>(sizeof(float));
    auto* out = reinterpret_cast<float*>(stream);
    const int frames = sampleCount / 2;
    self->mixAudio(out, frames);
}

void AudioSystem::mixAudio(float* out, const int frameCount) {
    std::fill(out, out + frameCount * 2, 0.0F);
    if (!outputEnabled_) return;

    for (std::size_t voiceIndex = 0; voiceIndex < activeVoices_.size();) {
        Voice& voice = activeVoices_[voiceIndex];
        if (voice.clipIndex >= clips_.size()) {
            activeVoices_.erase(activeVoices_.begin() + static_cast<long>(voiceIndex));
            continue;
        }

        const ClipData& clip = clips_[voice.clipIndex];
        const float gain = voice.gain * clip.baseGain * busGain(clip.bus, masterVolume_, musicVolume_, sfxVolume_);
        for (int frame = 0; frame < frameCount; ++frame) {
            if (voice.frameCursor >= clip.frameCount) {
                if (voice.loop) {
                    voice.frameCursor = 0;
                } else {
                    break;
                }
            }
            if (voice.frameCursor >= clip.frameCount) break;

            const std::size_t sampleBase = static_cast<std::size_t>(voice.frameCursor) * 2U;
            out[static_cast<std::size_t>(frame) * 2U] += clip.samples[sampleBase] * gain;
            out[static_cast<std::size_t>(frame) * 2U + 1U] += clip.samples[sampleBase + 1U] * gain;
            ++voice.frameCursor;
        }

        if (!voice.loop && voice.frameCursor >= clip.frameCount) {
            activeVoices_.erase(activeVoices_.begin() + static_cast<long>(voiceIndex));
            continue;
        }
        ++voiceIndex;
    }

    for (int i = 0; i < frameCount * 2; ++i) {
        out[i] = std::clamp(out[i], -1.0F, 1.0F);
    }
}

std::vector<float> AudioSystem::makeFallbackTone(const AudioBus bus) {
    constexpr int kSampleRate = 48000;
    constexpr int kDurationMs = 90;
    const int frameCount = (kSampleRate * kDurationMs) / 1000;
    const float freq = bus == AudioBus::Music ? 220.0F : 880.0F;
    std::vector<float> samples(static_cast<std::size_t>(frameCount) * 2U, 0.0F);
    for (int i = 0; i < frameCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = std::exp(-12.0F * t);
        const float v = std::sin(2.0F * 3.14159265F * freq * t) * env * 0.2F;
        samples[static_cast<std::size_t>(i) * 2U] = v;
        samples[static_cast<std::size_t>(i) * 2U + 1U] = v;
    }
    return samples;
}

} // namespace engine
