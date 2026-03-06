#include <engine/audio_system.h>

#include <engine/logging.h>

#include <SDL_mixer.h>

#include <algorithm>
#include <cmath>

namespace engine {

bool AudioSystem::initialize() {
    pending_.clear();
    if ((Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG) == 0) {
        logWarn("Audio disabled: SDL_mixer OGG init failed");
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {
        logWarn(std::string("Audio disabled: Mix_OpenAudio failed: ") + Mix_GetError());
        Mix_Quit();
        return false;
    }
    Mix_AllocateChannels(32);
    initialized_ = true;
    return true;
}

void AudioSystem::shutdown() {
    for (auto& [id, chunk] : sounds_) {
        (void)id;
        if (chunk) Mix_FreeChunk(chunk);
    }
    sounds_.clear();
    pending_.clear();
    nextSoundId_ = 1;

    if (initialized_) {
        Mix_CloseAudio();
        Mix_Quit();
    }
    initialized_ = false;
}

AudioSystem::SoundId AudioSystem::loadSound(const std::string& path) {
    if (!initialized_) return kInvalidSound;
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        logWarn("Audio file missing/failed to load: " + path);
        return kInvalidSound;
    }
    const SoundId id = nextSoundId_++;
    sounds_[id] = chunk;
    return id;
}

void AudioSystem::playSound(const SoundId id, const float volume) {
    if (!initialized_ || id == kInvalidSound) return;
    pending_.push_back(PendingPlay {.id = id, .volume = std::clamp(volume, 0.0F, 1.0F)});
}

void AudioSystem::playSoundPositional(const SoundId id, const Vec2 position, const Vec2 listener, const float volume) {
    const float dx = position.x - listener.x;
    const float dy = position.y - listener.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float attenuation = std::clamp(1.0F - (distance / 600.0F), 0.2F, 1.0F);
    playSound(id, volume * attenuation);
}

void AudioSystem::update() {
    if (!initialized_) return;
    for (const PendingPlay& p : pending_) {
        auto it = sounds_.find(p.id);
        if (it == sounds_.end() || !it->second) continue;
        Mix_VolumeChunk(it->second, static_cast<int>(p.volume * static_cast<float>(MIX_MAX_VOLUME)));
        (void)Mix_PlayChannel(-1, it->second, 0);
    }
    pending_.clear();
}

} // namespace engine
