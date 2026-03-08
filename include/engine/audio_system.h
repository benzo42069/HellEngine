#pragma once

#include <engine/audio_content.h>
#include <engine/render2d.h>

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

    void update();
    [[nodiscard]] bool available() const;

  private:
    bool initialized_ {false};
    std::vector<Mix_Chunk*> sounds_;
};

} // namespace engine
