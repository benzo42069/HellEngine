#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace engine {

enum class PatternStylePreset {
    Balanced,
    SpiralDance,
    BurstFan,
    SniperLanes,
};

struct PatternGeneratorInput {
    PatternStylePreset style {PatternStylePreset::Balanced};
    std::uint64_t seed {1337};
    float density {0.5F};
    float speed {0.5F};
    float symmetry {0.5F};
    float chaos {0.3F};
    float fairness {0.6F};
};

struct PatternGeneratorMetrics {
    float estimatedDifficultyScore {0.0F};
    float averageDodgeGapEstimate {0.0F};
    float estimatedBulletsPerSecond {0.0F};
    std::array<float, 64> densityHeatmap {};
};

struct PatternGeneratorResult {
    std::string json;
    PatternGeneratorMetrics metrics {};
};

PatternGeneratorResult generatePatternGraphJson(const PatternGeneratorInput& input);
PatternGeneratorResult mutatePatternGraphJson(const PatternGeneratorInput& input);
PatternGeneratorResult remixPatternGraphJson(const PatternGeneratorInput& a, const PatternGeneratorInput& b);

} // namespace engine
