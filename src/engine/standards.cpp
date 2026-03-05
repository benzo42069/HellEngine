#include <engine/standards.h>

#include <algorithm>

namespace engine {

namespace {
EngineStandards g_standards = specDefaultStandards();
}

EngineStandards specDefaultStandards() {
    return {};
}

void clampStandards(EngineStandards& s) {
    s.playfieldWidth = std::clamp(s.playfieldWidth, 480, 4096);
    s.playfieldHeight = std::clamp(s.playfieldHeight, 640, 4096);
    s.renderTargetWidth = std::clamp(s.renderTargetWidth, 640, 8192);
    s.renderTargetHeight = std::clamp(s.renderTargetHeight, 640, 8192);
    s.uiPanelWidthFraction = std::clamp(s.uiPanelWidthFraction, 0.05F, 0.40F);

    s.densityNormal = std::clamp(s.densityNormal, 100, 30000);
    s.densityBoss = std::clamp(s.densityBoss, s.densityNormal, 50000);
    s.densityExtreme = std::clamp(s.densityExtreme, s.densityBoss, 100000);

    s.backgroundBrightnessMin = std::clamp(s.backgroundBrightnessMin, 0.0F, 1.0F);
    s.backgroundBrightnessMax = std::clamp(s.backgroundBrightnessMax, s.backgroundBrightnessMin, 1.0F);
    s.backgroundSaturationMax = std::clamp(s.backgroundSaturationMax, 0.0F, 1.0F);

    s.cameraMaxScrollUnitsPerSec = std::clamp(s.cameraMaxScrollUnitsPerSec, 10.0F, 2000.0F);
    s.cameraShakeMaxAmplitude = std::clamp(s.cameraShakeMaxAmplitude, 0.0F, 100.0F);
    s.cameraZoomMin = std::clamp(s.cameraZoomMin, 0.1F, 4.0F);
    s.cameraZoomMax = std::clamp(s.cameraZoomMax, s.cameraZoomMin, 4.0F);
}

void resetEngineStandardsToSpec() {
    g_standards = specDefaultStandards();
}

const EngineStandards& engineStandards() {
    return g_standards;
}

EngineStandards& mutableEngineStandards() {
    return g_standards;
}

float getRadialAngleStep(const int bulletCount) {
    if (bulletCount <= 0) return 360.0F;
    return 360.0F / static_cast<float>(bulletCount);
}

float getSpiralSpeedDegPerTick(const SpiralSpeedPreset preset) {
    constexpr float baseTick60Hz = 1.0F / 60.0F;
    switch (preset) {
        case SpiralSpeedPreset::Slow: return 7.5F / baseTick60Hz;
        case SpiralSpeedPreset::Fast: return 30.0F / baseTick60Hz;
        case SpiralSpeedPreset::Standard:
        default: return 15.0F / baseTick60Hz;
    }
}

int getStreamGapPreset(const DifficultyGapPreset preset) {
    const EngineStandards& s = engineStandards();
    switch (preset) {
        case DifficultyGapPreset::Easy: return (s.streamGapEasyMin + s.streamGapEasyMax) / 2;
        case DifficultyGapPreset::Hard: return (s.streamGapHardMin + s.streamGapHardMax) / 2;
        case DifficultyGapPreset::Lunatic: return (s.streamGapLunaticMin + s.streamGapLunaticMax) / 2;
        case DifficultyGapPreset::Normal:
        default: return (s.streamGapNormalMin + s.streamGapNormalMax) / 2;
    }
}

int getWallGapPreset(const DifficultyGapPreset preset) {
    const EngineStandards& s = engineStandards();
    switch (preset) {
        case DifficultyGapPreset::Easy: return s.wallGapEasy;
        case DifficultyGapPreset::Hard: return s.wallGapHard;
        case DifficultyGapPreset::Lunatic: return s.wallGapLunatic;
        case DifficultyGapPreset::Normal:
        default: return s.wallGapNormal;
    }
}

const char* toString(const RenderLayer layer) {
    switch (layer) {
        case RenderLayer::Player: return "Player";
        case RenderLayer::EnemyProjectiles: return "EnemyProjectiles";
        case RenderLayer::PlayerProjectiles: return "PlayerProjectiles";
        case RenderLayer::Bosses: return "Bosses";
        case RenderLayer::Enemies: return "Enemies";
        case RenderLayer::Background: return "Background";
    }
    return "Unknown";
}

} // namespace engine
