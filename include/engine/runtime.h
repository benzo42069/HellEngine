#pragma once

#include <engine/config.h>
#include <engine/audio_system.h>
#include <engine/gameplay_session.h>
#include <engine/input_system.h>
#include <engine/render_pipeline.h>

#include <SDL.h>

namespace engine {

class Runtime {
  public:
    explicit Runtime(EngineConfig config);
    ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    int run();

  private:
    void simTick(double dt);

    EngineConfig config_;
    bool running_ {true};
    SDL_Window* window_ {nullptr};

    InputSystem input_;
    GameplaySession session_;
    RenderPipeline renderPipeline_;
    AudioSystem audioSystem_;
    AudioSystem::SoundId hitSound_ {0};
    AudioSystem::SoundId grazeSound_ {0};
    AudioSystem::SoundId warningSound_ {0};
    AudioSystem::SoundId specialSound_ {0};
    ZoneType lastZoneType_ {ZoneType::Combat};
};

} // namespace engine
