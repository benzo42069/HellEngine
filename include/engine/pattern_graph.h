#pragma once

#include <engine/deterministic_rng.h>
#include <engine/projectiles.h>

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine {

enum class PatternGraphNodeType {
    EmitRing,
    EmitSpread,
    EmitSpiral,
    EmitWave,
    EmitAimed,
    Wait,
    Loop,
    Branch,
    Subpattern,
    ModifierRotate,
    ModifierPhaseOffset,
    ModifierSymmetry,
    ModifierSpeedCurve,
    ModifierAngleCurve,
    DifficultyScalar,
    ParameterOverride,
    RandomRange,
};

struct PatternGraphNode {
    std::string id;
    PatternGraphNodeType type {PatternGraphNodeType::Wait};
    std::unordered_map<std::string, float> params;
    std::vector<std::string> outputs;
};

struct PatternGraphAsset {
    std::string id;
    std::vector<PatternGraphNode> nodes;
};

struct PatternGraphDiagnostic {
    std::string nodeId;
    std::string message;
    bool warning {false};
};

enum class PatternOpCode : std::uint8_t {
    Emit,
    Wait,
    SetRotate,
    SetPhaseOffset,
    SetSymmetry,
    SetDifficulty,
    RandomRange,
    JumpIfLoop,
    End,
};

struct PatternOp {
    PatternOpCode code {PatternOpCode::End};
    float a {0.0F};
    float b {0.0F};
    float c {0.0F};
    float d {0.0F};
    std::uint32_t u0 {0};
    std::uint32_t u1 {0};
};

struct CompiledPatternGraph {
    std::string id;
    std::vector<PatternOp> ops;
    std::vector<float> constantAnglesDeg;
    std::vector<PatternGraphDiagnostic> diagnostics;
    float staticSpawnRateEstimatePerSecond {0.0F};
};

class PatternGraphCompiler {
  public:
    bool compile(const PatternGraphAsset& asset, CompiledPatternGraph& out) const;
};

class PatternGraphVm {
  public:
    struct RuntimeState {
        std::size_t pc {0};
        float waitTimer {0.0F};
        float rotateDeg {0.0F};
        float phaseOffsetDeg {0.0F};
        float difficultyScalar {1.0F};
        std::array<std::uint16_t, 64> loopCounter {};
        std::array<DeterministicRng, 8> rngStreams {DeterministicRng {1}, DeterministicRng {2}, DeterministicRng {3}, DeterministicRng {4}, DeterministicRng {5}, DeterministicRng {6}, DeterministicRng {7}, DeterministicRng {8}};
    };

    void reset(RuntimeState& state, std::uint64_t seed) const;

    void execute(
        const CompiledPatternGraph& graph,
        RuntimeState& state,
        float dt,
        Vec2 origin,
        Vec2 aimTarget,
        const std::function<void(const ProjectileSpawn&)>& emitProjectile
    ) const;
};

bool loadPatternGraphsFromFile(const std::string& filePath, std::vector<PatternGraphAsset>& outAssets, std::vector<PatternGraphDiagnostic>& outDiagnostics);

} // namespace engine
