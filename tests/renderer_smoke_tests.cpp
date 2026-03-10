#include <engine/config.h>
#include <engine/runtime.h>

#include <SDL.h>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <memory>

namespace {
void setVideoDriverDummy() {
#ifdef _WIN32
    _putenv_s("SDL_VIDEODRIVER", "dummy");
#else
    setenv("SDL_VIDEODRIVER", "dummy", 1);
#endif
}
}

int main() {
    setVideoDriverDummy();
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    engine::EngineConfig config;
    config.headless = false;
    config.rendererSmokeTest = true;
    config.windowWidth = 640;
    config.windowHeight = 360;
    config.windowTitle = "RendererSmoke";

    auto runtime = std::make_unique<engine::Runtime>(config);
    const int rc = runtime->run();
    if (rc != 0) {
        std::cerr << "renderer_smoke_tests failed: runtime returned " << rc << '\n';
        std::fflush(stderr);
        std::_Exit(1);
    }

    std::cout << "renderer_smoke_tests passed\n";
    std::fflush(stdout);
    std::_Exit(0);
}
