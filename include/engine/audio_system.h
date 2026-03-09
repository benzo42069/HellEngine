#pragma once

#include <engine/audio_content.h>
#include <engine/render2d.h>

#include <map>

#include <SDL_mixer.h>

#include <cstdint>
#include <string>
#include <vector>

namespace engine {

class AudioSystem {
  public:
    using SoundId = std::uint16_t;
    static constexpr SoundId kInvalidSound = 0xFFFF;

    bool initialize();
    void shutdown();

    SoundId loadSound(const std::string& path);
    void playSound(SoundId id, float volume = 1.0F);
    void playSoundPositional(SoundId id, Vec2 position, Vec2 listener, float maxDist = 400.0F);

    bool configureFromContent(const AudioContentDatabase& content, const std::string& assetRoot);
    void setBusVolume(AudioBus bus, float normalized);
    void setMasterVolume(float normalized);
    void setListenerPosition(Vec2 listener);
    void dispatchEvent(AudioEventId event, Vec2 position);

    void update();
    [[nodiscard]] bool available() const;

  private:
    struct LoadedClip {
        SoundId soundId {kInvalidSound};
        AudioBus bus {AudioBus::Sfx};
        bool loop {false};
        float baseGain {1.0F};
    };

    struct EventRoute {
        std::string clipId;
        float gain {1.0F};
    };

    [[nodiscard]] float busGain(AudioBus bus) const;
    [[nodiscard]] float resolvePlaybackGain(const LoadedClip& clip, float eventGain) const;
    void clearLoadedSounds();
    void startMusicIfConfigured();

    bool initialized_ {false};
    std::vector<Mix_Chunk*> sounds_;
    std::map<std::string, LoadedClip> loadedClips_ {};
    std::map<AudioEventId, EventRoute> eventRoutes_ {};
    std::string musicClipId_ {};
    int musicChannel_ {-1};
    float masterVolume_ {1.0F};
    float musicVolume_ {1.0F};
    float sfxVolume_ {1.0F};
    Vec2 listener_ {0.0F, 0.0F};
};

} // namespace engine
