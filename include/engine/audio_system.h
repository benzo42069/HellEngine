#pragma once

#include <engine/render2d.h>

#include <SDL_mixer.h>

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
    using SoundId = std::uint16_t;
    static constexpr SoundId kInvalidSound = 0xFFFF;

    bool initialize();
    void shutdown();

    SoundId loadSound(const std::string& path);
    void playSound(SoundId id, float volume = 1.0F);
    void playSoundPositional(SoundId id, Vec2 position, Vec2 listener, float maxDist = 400.0F);

    void update();
    [[nodiscard]] bool available() const;

  private:
    bool initialized_ {false};
    std::vector<Mix_Chunk*> sounds_;
};

} // namespace engine
