#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace engine {

enum class RenderLayer : std::uint8_t {
    Background = 0,
    Enemies = 1,
    Bosses = 2,
    PlayerProjectiles = 3,
    EnemyProjectiles = 4,
    Player = 5,
};

enum class SpiralSpeedPreset : std::uint8_t {
    Slow,
    Standard,
    Fast,
};

enum class DifficultyGapPreset : std::uint8_t {
    Easy,
    Normal,
    Hard,
    Lunatic,
};

struct EngineStandards {
    int playfieldWidth {1080};
    int playfieldHeight {1440};
    int renderTargetWidth {1440};
    int renderTargetHeight {1920};
    float uiPanelWidthFraction {0.20F};

    int projectileMicroMin {4};
    int projectileMicroMax {6};
    int projectileSmallMin {6};
    int projectileSmallMax {10};
    int projectileMediumMin {12};
    int projectileMediumMax {20};
    int projectileLargeMin {24};
    int projectileLargeMax {40};
    int projectileBossMin {40};
    int projectileBossMax {64};

    int playerHitboxStandardMin {4};
    int playerHitboxStandardMax {6};
    int playerHitboxPrecisionMin {3};
    int playerHitboxPrecisionMax {4};

    int beamThinMin {8};
    int beamThinMax {12};
    int beamStandardMin {16};
    int beamStandardMax {32};
    int beamBossMin {40};
    int beamBossMax {80};

    int streamGapEasyMin {40};
    int streamGapEasyMax {60};
    int streamGapNormalMin {24};
    int streamGapNormalMax {40};
    int streamGapHardMin {12};
    int streamGapHardMax {24};
    int streamGapLunaticMin {6};
    int streamGapLunaticMax {12};

    int wallGapEasy {80};
    int wallGapNormal {48};
    int wallGapHard {32};
    int wallGapLunatic {16};

    int densityNormal {2000};
    int densityBoss {5000};
    int densityExtreme {10000};

    float backgroundBrightnessMin {0.10F};
    float backgroundBrightnessMax {0.35F};
    float backgroundSaturationMax {0.40F};

    float cameraMaxScrollUnitsPerSec {260.0F};
    float cameraShakeMaxAmplitude {8.0F};
    float cameraZoomMin {0.85F};
    float cameraZoomMax {1.15F};

    std::array<RenderLayer, 6> renderOrder {
        RenderLayer::Player,
        RenderLayer::EnemyProjectiles,
        RenderLayer::PlayerProjectiles,
        RenderLayer::Bosses,
        RenderLayer::Enemies,
        RenderLayer::Background,
    };
};

EngineStandards specDefaultStandards();
void resetEngineStandardsToSpec();
const EngineStandards& engineStandards();
EngineStandards& mutableEngineStandards();
void clampStandards(EngineStandards& standards);

float getRadialAngleStep(int bulletCount);
float getSpiralSpeedDegPerTick(SpiralSpeedPreset preset);
int getStreamGapPreset(DifficultyGapPreset preset);
int getWallGapPreset(DifficultyGapPreset preset);

const char* toString(RenderLayer layer);

} // namespace engine
