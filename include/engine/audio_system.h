#pragma once

#include <SDL.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace engine {

enum class AudioBus : std::uint8_t {
    Master = 0,
    Music,
    Sfx,
};

enum class AudioEventId : std::uint8_t {
    Hit = 0,
    Graze,
    PlayerDamage,
    EnemyDeath,
    BossWarning,
    UiClick,
    UiConfirm,
};

struct AudioClipRecord {
    std::string id;
    std::string path;
    AudioBus bus {AudioBus::Sfx};
    bool loop {false};
    float baseGain {1.0F};
};

struct AudioEventBinding {
    AudioEventId event {AudioEventId::Hit};
    std::string clipId;
    float gain {1.0F};
};

struct AudioContentDatabase {
    std::vector<AudioClipRecord> clips;
    std::vector<AudioEventBinding> events;
    std::string musicClipId;
};

class AudioSystem {
  public:
    AudioSystem() = default;
    ~AudioSystem();

    bool initialize(bool enableDevice);
    void shutdown();

    bool loadContent(const AudioContentDatabase& content, const std::string& contentRoot);

    void queueEvent(AudioEventId event);
    void flushQueuedEvents();

    void setBusVolume(AudioBus bus, float volume);
    [[nodiscard]] float busVolume(AudioBus bus) const;

    void playMusic();
    void stopMusic();

    [[nodiscard]] bool initialized() const { return initialized_; }

  private:
    struct ClipData {
        std::string id;
        AudioBus bus {AudioBus::Sfx};
        bool loop {false};
        float baseGain {1.0F};
        std::vector<float> samples;
        std::uint32_t frameCount {0};
    };

    struct Voice {
        std::size_t clipIndex {0};
        std::uint32_t frameCursor {0};
        float gain {1.0F};
        bool loop {false};
    };

    struct EventRoute {
        AudioEventId event {AudioEventId::Hit};
        std::size_t clipIndex {0};
        float gain {1.0F};
    };

    static void audioCallback(void* userdata, Uint8* stream, int len);
    void mixAudio(float* out, int frameCount);
    static std::vector<float> makeFallbackTone(AudioBus bus);

    SDL_AudioDeviceID device_ {0};
    SDL_AudioSpec obtained_ {};
    bool initialized_ {false};
    bool outputEnabled_ {false};

    float masterVolume_ {1.0F};
    float musicVolume_ {0.75F};
    float sfxVolume_ {0.9F};

    std::vector<ClipData> clips_;
    std::vector<EventRoute> routes_;
    std::vector<Voice> activeVoices_;
    std::vector<AudioEventId> queuedEvents_;
    std::size_t musicClipIndex_ {static_cast<std::size_t>(-1)};
};

} // namespace engine
