#pragma once

#include <engine/render2d.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct Mix_Chunk;

namespace engine {

class AudioSystem {
  public:
    using SoundId = std::uint16_t;

    bool initialize();
    void shutdown();

    [[nodiscard]] SoundId loadSound(const std::string& path);
    void playSound(SoundId id, float volume = 1.0F);
    void playSoundPositional(SoundId id, Vec2 position, Vec2 listener, float volume = 1.0F);
    void update();

    [[nodiscard]] bool enabled() const { return initialized_; }

  private:
    struct PendingPlay {
        SoundId id {0};
        float volume {1.0F};
    };

    static constexpr SoundId kInvalidSound = 0;

    bool initialized_ {false};
    SoundId nextSoundId_ {1};
    std::unordered_map<SoundId, Mix_Chunk*> sounds_;
    std::vector<PendingPlay> pending_;
};

} // namespace engine
