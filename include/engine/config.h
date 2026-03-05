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
    bool enableModernRenderer {false};
    std::string defensiveSpecialPreset {"standard"};
    float defensiveEnemyScaleOverride {0.0F};
    float defensivePlayerProjectileScaleOverride {0.0F};
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
    float defensiveDurationSeconds {2.25F};
    float defensiveCooldownSeconds {12.0F};
    int standardsPlayfieldWidth {1080};
    int standardsPlayfieldHeight {1440};
    int standardsRenderTargetWidth {1440};
    int standardsRenderTargetHeight {1920};
};

EngineConfig loadConfigFromFile(const std::string& filePath);
void applyCommandLineOverrides(EngineConfig& config, int argc, char** argv);

} // namespace engine
