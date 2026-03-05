#pragma once

#include <cstdint>
#include <string>

namespace engine {

struct EngineConfig {
    int windowWidth {1280};
    int windowHeight {720};
    std::string windowTitle {"EngineDemo"};
    double fixedDeltaSeconds {1.0 / 60.0};
    bool headless {false};
    bool rendererSmokeTest {false};
    std::uint32_t maxFrameSteps {8};
    std::uint64_t simulationSeed {1337};
    int targetTicks {0};

    std::uint32_t projectileCapacity {12000};
    std::uint32_t projectileSpawnBurst {128};
    bool stress10k {false};
    std::string contentPackPath {"content.pak"};
    std::string replayRecordPath;
    std::string replayPlaybackPath;
    bool replayVerifyMode {false};
    std::uint32_t replayHashPeriodTicks {60};
    std::string difficultyProfile {"normal"};
};

EngineConfig loadConfigFromFile(const std::string& filePath);
void applyCommandLineOverrides(EngineConfig& config, int argc, char** argv);

} // namespace engine
