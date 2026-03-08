#pragma once

#include <engine/audio_system.h>
#include <engine/config.h>
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
    void dispatchAudioEvents();
    bool loadAudioContent(const std::string& packSearchPath);

    EngineConfig config_;
    bool running_ {true};
    SDL_Window* window_ {nullptr};

    InputSystem input_;
    AudioSystem audio_;
    GameplaySession session_;
    RenderPipeline renderPipeline_;
};

} // namespace engine
