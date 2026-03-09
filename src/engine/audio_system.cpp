#include <engine/audio_system.h>

#include <engine/logging.h>

#include <algorithm>
#include <cmath>

namespace engine {

namespace {
float clamp01(const float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

float clampDistance(const float value) {
    return std::max(1.0F, value);
}

std::string normalizeAssetRoot(const std::string& root) {
    if (root.empty()) return std::string();
    if (root.back() == '/' || root.back() == '\\') return root;
    return root + '/';
}
} // namespace

bool AudioSystem::initialize() {
    if (initialized_) return true;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        initialized_ = false;
        logWarn("Audio disabled: SDL_mixer init failed: " + std::string(Mix_GetError()));
        return false;
    }

    Mix_AllocateChannels(16);
    initialized_ = true;
    return true;
}

void AudioSystem::clearLoadedSounds() {
    for (Mix_Chunk*& chunk : sounds_) {
        if (chunk) {
            Mix_FreeChunk(chunk);
            chunk = nullptr;
        }
    }
    sounds_.clear();
}

void AudioSystem::shutdown() {
    clearLoadedSounds();
    loadedClips_.clear();
    eventRoutes_.clear();
    musicClipId_.clear();
    musicChannel_ = -1;

    if (initialized_) {
        Mix_CloseAudio();
    }
    initialized_ = false;
}

AudioSystem::SoundId AudioSystem::loadSound(const std::string& path) {
    if (!initialized_) return kInvalidSound;
    if (sounds_.size() >= static_cast<std::size_t>(kInvalidSound)) return kInvalidSound;

    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        logWarn("Audio clip missing or unreadable: " + path + " (" + Mix_GetError() + ")");
        return kInvalidSound;
    }

    sounds_.push_back(chunk);
    return static_cast<SoundId>(sounds_.size() - 1U);
}

void AudioSystem::playSound(const SoundId id, const float volume) {
    if (!initialized_) return;
    if (id == kInvalidSound) return;
    if (id >= sounds_.size()) return;

    Mix_Chunk* chunk = sounds_[id];
    if (!chunk) return;

    const float clamped = clamp01(volume);
    Mix_VolumeChunk(chunk, static_cast<int>(std::lround(clamped * static_cast<float>(MIX_MAX_VOLUME))));
    (void)Mix_PlayChannel(-1, chunk, 0);
}

void AudioSystem::playSoundPositional(const SoundId id, const Vec2 position, const Vec2 listener, const float maxDist) {
    const float dx = position.x - listener.x;
    const float dy = position.y - listener.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    if (distance > maxDist || maxDist <= 0.0F) return;

    const float attenuation = 1.0F - std::clamp(distance / maxDist, 0.0F, 1.0F);
    playSound(id, attenuation);
}

bool AudioSystem::configureFromContent(const AudioContentDatabase& content, const std::string& assetRoot) {
    if (!initialized_) return false;

    clearLoadedSounds();
    loadedClips_.clear();
    eventRoutes_.clear();
    musicClipId_.clear();
    if (musicChannel_ >= 0) {
        Mix_HaltChannel(musicChannel_);
        musicChannel_ = -1;
    }

    const std::string normalizedRoot = normalizeAssetRoot(assetRoot);
    for (const AudioClipRecord& clip : content.clips) {
        if (clip.id.empty()) continue;
        const SoundId soundId = loadSound(normalizedRoot + clip.path);
        if (soundId == kInvalidSound) continue;
        loadedClips_[clip.id] = LoadedClip {.soundId = soundId, .bus = clip.bus, .loop = clip.loop, .baseGain = clamp01(clip.baseGain)};
    }

    for (const AudioEventBinding& event : content.events) {
        if (loadedClips_.find(event.clipId) == loadedClips_.end()) {
            logWarn("Audio event binding dropped due to missing clip id: " + event.clipId);
            continue;
        }
        eventRoutes_[event.event] = EventRoute {.clipId = event.clipId, .gain = clamp01(event.gain)};
    }

    if (!content.musicClipId.empty()) {
        if (loadedClips_.find(content.musicClipId) != loadedClips_.end()) {
            musicClipId_ = content.musicClipId;
        } else {
            logWarn("Audio music clip id not found in loaded clips: " + content.musicClipId);
        }
    }

    startMusicIfConfigured();
    return !loadedClips_.empty();
}

void AudioSystem::setBusVolume(const AudioBus bus, const float normalized) {
    const float clamped = clamp01(normalized);
    switch (bus) {
    case AudioBus::Master:
        masterVolume_ = clamped;
        break;
    case AudioBus::Music:
        musicVolume_ = clamped;
        break;
    case AudioBus::Sfx:
        sfxVolume_ = clamped;
        break;
    }
}

void AudioSystem::setMasterVolume(const float normalized) {
    setBusVolume(AudioBus::Master, normalized);
}

void AudioSystem::setListenerPosition(const Vec2 listener) {
    listener_ = listener;
}

float AudioSystem::busGain(const AudioBus bus) const {
    switch (bus) {
    case AudioBus::Master:
        return masterVolume_;
    case AudioBus::Music:
        return musicVolume_;
    case AudioBus::Sfx:
        return sfxVolume_;
    }
    return 1.0F;
}

float AudioSystem::resolvePlaybackGain(const LoadedClip& clip, const float eventGain) const {
    return clamp01(clip.baseGain * clamp01(eventGain) * masterVolume_ * busGain(clip.bus));
}

void AudioSystem::startMusicIfConfigured() {
    if (!initialized_) return;
    if (musicClipId_.empty()) return;
    const auto clipIt = loadedClips_.find(musicClipId_);
    if (clipIt == loadedClips_.end()) return;

    const LoadedClip& clip = clipIt->second;
    if (!clip.loop) return;
    if (musicChannel_ >= 0 && Mix_Playing(musicChannel_)) return;
    if (clip.soundId >= sounds_.size() || !sounds_[clip.soundId]) return;

    Mix_Chunk* chunk = sounds_[clip.soundId];
    const float gain = resolvePlaybackGain(clip, 1.0F);
    Mix_VolumeChunk(chunk, static_cast<int>(std::lround(gain * static_cast<float>(MIX_MAX_VOLUME))));
    musicChannel_ = Mix_PlayChannel(-1, chunk, -1);
}

void AudioSystem::dispatchEvent(const AudioEventId event, const Vec2 position) {
    if (!initialized_) return;

    const auto routeIt = eventRoutes_.find(event);
    if (routeIt == eventRoutes_.end()) return;
    const auto clipIt = loadedClips_.find(routeIt->second.clipId);
    if (clipIt == loadedClips_.end()) return;

    const LoadedClip& clip = clipIt->second;
    const float gain = resolvePlaybackGain(clip, routeIt->second.gain);
    if (clip.loop) {
        if (event == AudioEventId::BossWarning || event == AudioEventId::BossPhaseShift) {
            if (musicChannel_ >= 0) {
                Mix_HaltChannel(musicChannel_);
                musicChannel_ = -1;
            }
        }
        startMusicIfConfigured();
        return;
    }

    if (clip.bus == AudioBus::Sfx) {
        const float dx = position.x - listener_.x;
        const float dy = position.y - listener_.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        const float attenuation = 1.0F - std::clamp(distance / clampDistance(400.0F), 0.0F, 1.0F);
        playSound(clip.soundId, gain * attenuation);
        return;
    }

    playSound(clip.soundId, gain);
}

void AudioSystem::update() {
    if (musicChannel_ < 0 && !musicClipId_.empty()) {
        startMusicIfConfigured();
    }
    if (musicChannel_ < 0 || musicClipId_.empty()) return;
    if (Mix_Playing(musicChannel_)) {
        const auto clipIt = loadedClips_.find(musicClipId_);
        if (clipIt == loadedClips_.end() || clipIt->second.soundId >= sounds_.size() || !sounds_[clipIt->second.soundId]) return;
        Mix_Chunk* chunk = sounds_[clipIt->second.soundId];
        const float gain = resolvePlaybackGain(clipIt->second, 1.0F);
        Mix_VolumeChunk(chunk, static_cast<int>(std::lround(gain * static_cast<float>(MIX_MAX_VOLUME))));
    } else {
        musicChannel_ = -1;
    }
}

bool AudioSystem::available() const { return initialized_; }

} // namespace engine
