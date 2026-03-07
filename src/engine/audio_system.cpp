#include <engine/audio_system.h>

#include <engine/logging.h>

#include <algorithm>
#include <cmath>

namespace engine {

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

void AudioSystem::shutdown() {
    for (Mix_Chunk*& chunk : sounds_) {
        if (chunk) {
            Mix_FreeChunk(chunk);
            chunk = nullptr;
        }
    }
    sounds_.clear();

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

    const float clamped = std::clamp(volume, 0.0F, 1.0F);
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

void AudioSystem::update() {}

bool AudioSystem::available() const { return initialized_; }

} // namespace engine
