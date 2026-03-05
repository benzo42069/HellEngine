#pragma once

#include <engine/deterministic_rng.h>
#include <engine/pattern_graph.h>
#include <engine/projectiles.h>
#include <engine/render2d.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace engine {

enum class PatternType {
    RadialBurst,
    Spiral,
    Spread,
    Wave,
    Aimed,
};

struct PatternModifiers {
    float rotationDeg {0.0F};
    float phaseOffsetDeg {0.0F};
    float speedCurveAmp {0.0F};
    float angleOscillationDeg {0.0F};
    float angleOscillationHz {1.0F};
    float deterministicJitterDeg {0.0F};
};

struct PatternTimelineEvent {
    float timeSeconds {0.0F};
    float extraRotationDeg {0.0F};
    float speedScale {1.0F};
};

struct PatternSubpatternRef {
    std::size_t layerIndex {0};
    float phaseOffsetDeg {0.0F};
};

struct PatternLayer {
    std::string name;
    PatternType type {PatternType::RadialBurst};
    std::uint32_t bulletCount {24};
    float bulletSpeed {120.0F};
    float projectileRadius {3.0F};
    float cooldownSeconds {0.15F};
    ProjectileBehavior projectileBehavior {};

    float angleStepDeg {8.0F};
    float spreadAngleDeg {35.0F};
    float waveAmplitudeDeg {20.0F};
    float waveFrequencyHz {1.5F};
    float aimJitterDeg {0.0F};

    PatternModifiers modifiers {};
    std::vector<PatternTimelineEvent> timeline;
    std::vector<PatternSubpatternRef> subpatterns;
};

struct PatternSequenceStep {
    float durationSeconds {0.5F};
    std::vector<std::size_t> activeLayers;
};

struct PatternDefinition {
    std::string guid;
    std::string name;
    std::uint64_t seedOffset {0};
    bool loopSequence {true};
    std::vector<PatternLayer> layers;
    std::vector<PatternSequenceStep> sequence;
};

class PatternBank {
  public:
    bool loadFromFile(const std::string& filePath);
    void loadFallbackDefaults();

    [[nodiscard]] const std::vector<PatternDefinition>& patterns() const;
    [[nodiscard]] const std::vector<CompiledPatternGraph>& compiledGraphs() const;
    [[nodiscard]] const std::vector<PatternGraphDiagnostic>& graphDiagnostics() const;
    [[nodiscard]] std::size_t findPatternIndexByGuidOrName(const std::string& ref) const;

  private:
    std::vector<PatternDefinition> patterns_;
    std::vector<CompiledPatternGraph> compiledGraphs_;
    std::vector<PatternGraphDiagnostic> graphDiagnostics_;
};

class PatternPlayer {
  public:
    void setBank(const PatternBank* bank);
    void setRunSeed(std::uint64_t runSeed);

    void setPatternIndex(std::size_t index);
    void cyclePattern(int direction);
    void resetCurrentPattern();

    void togglePause();
    void setPaused(bool paused);
    [[nodiscard]] bool paused() const;

    void setPlaybackSpeed(float speed);
    void adjustPlaybackSpeed(float delta);
    [[nodiscard]] float playbackSpeed() const;

    void setRuntimeModifiers(float cooldownScale, int extraBullets, float jitterAddDeg);

    void update(
        float fixedDt,
        Vec2 origin,
        Vec2 aimTarget,
        const std::function<void(const ProjectileSpawn&)>& emitProjectile
    );

    [[nodiscard]] std::size_t patternIndex() const;
    [[nodiscard]] const PatternDefinition* activePattern() const;
    [[nodiscard]] std::size_t activeSequenceStep() const;
    [[nodiscard]] std::size_t activeLayerCountForStep() const;

  private:
    struct LayerRuntimeState {
        float cooldownTimer {0.0F};
        float spiralAccumDeg {0.0F};
        std::size_t timelineCursor {0};
    };

    void emitLayer(
        const PatternDefinition& pattern,
        std::size_t layerIndex,
        float phaseOffsetDeg,
        Vec2 origin,
        Vec2 aimTarget,
        const std::function<void(const ProjectileSpawn&)>& emitProjectile
    );

    const PatternTimelineEvent* currentTimelineEvent(const PatternLayer& layer, LayerRuntimeState& state, float localTime) const;

    const PatternBank* bank_ {nullptr};
    std::size_t patternIndex_ {0};
    bool paused_ {false};
    float playbackSpeed_ {1.0F};
    float patternTime_ {0.0F};
    float sequenceTime_ {0.0F};
    std::size_t sequenceStepIndex_ {0};

    std::uint64_t runSeed_ {1337};
    DeterministicRng rng_ {1337};
    std::vector<LayerRuntimeState> layerStates_;

    float runtimeCooldownScale_ {1.0F};
    int runtimeExtraBullets_ {0};
    float runtimeJitterAddDeg_ {0.0F};
};

std::string toString(PatternType type);

} // namespace engine
