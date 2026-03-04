#pragma once

#include <SDL.h>

#include <cstdint>
#include <deque>
#include <string>

namespace engine {

struct ToolRuntimeSnapshot {
    std::uint64_t tick {0};
    int fps {0};
    float frameMs {0.0F};
    std::uint32_t projectileCount {0};
    std::uint32_t entityCount {0};
    std::uint32_t collisionsTotal {0};
};

bool generateDemoContent(const std::string& outputDir, std::string* error = nullptr);

class ControlCenterToolSuite {
  public:
    void initialize(SDL_Window* window, SDL_Renderer* renderer);
    void shutdown();
    void processEvent(const SDL_Event& event);

    void beginFrame();
    void drawControlCenter(const ToolRuntimeSnapshot& snapshot);
    void endFrame();

  private:
    bool initialized_ {false};
    bool open_ {true};

    bool showContentBrowser_ {true};
    bool showPatternEditor_ {true};
    bool showEntityEditor_ {true};
    bool showTraitEditor_ {true};
    bool showWaveEditor_ {true};
    bool showValidator_ {true};
    bool showProfiler_ {true};

    std::string statusMessage_ {"Ready"};
    std::deque<float> frameHistoryMs_;
    SDL_Renderer* renderer_ {nullptr};
};

} // namespace engine
