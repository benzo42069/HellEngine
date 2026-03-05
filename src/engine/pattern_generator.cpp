#include <engine/pattern_generator.h>

#include <engine/deterministic_rng.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>

namespace engine {

namespace {
float clamp01(const float v) { return std::clamp(v, 0.0F, 1.0F); }

float styleBaseSpread(const PatternStylePreset style) {
    switch (style) {
        case PatternStylePreset::Balanced: return 50.0F;
        case PatternStylePreset::SpiralDance: return 80.0F;
        case PatternStylePreset::BurstFan: return 95.0F;
        case PatternStylePreset::SniperLanes: return 30.0F;
    }
    return 50.0F;
}

std::string styleName(const PatternStylePreset style) {
    switch (style) {
        case PatternStylePreset::Balanced: return "balanced";
        case PatternStylePreset::SpiralDance: return "spiral_dance";
        case PatternStylePreset::BurstFan: return "burst_fan";
        case PatternStylePreset::SniperLanes: return "sniper_lanes";
    }
    return "balanced";
}

PatternGeneratorResult build(const PatternGeneratorInput& in, const std::uint64_t salt) {
    PatternGeneratorInput p = in;
    p.density = clamp01(p.density);
    p.speed = clamp01(p.speed);
    p.symmetry = clamp01(p.symmetry);
    p.chaos = clamp01(p.chaos);
    p.fairness = clamp01(p.fairness);

    DeterministicRng rng(p.seed ^ salt);

    const int ringCount = 6 + static_cast<int>(p.density * 22.0F);
    const float bulletSpeed = 70.0F + p.speed * 180.0F;
    const float spread = styleBaseSpread(p.style) * (0.45F + p.symmetry * 0.8F);
    const float waitSeconds = std::max(0.03F, 0.24F - p.density * 0.16F + p.fairness * 0.06F);
    const int loops = std::max(1, static_cast<int>(2 + p.density * 6.0F - p.fairness * 2.0F));
    const float randomMin = -8.0F - p.chaos * 45.0F;
    const float randomMax = 8.0F + p.chaos * 45.0F;

    nlohmann::json root;
    root["graphs"] = nlohmann::json::array();
    auto& g = root["graphs"].emplace_back();
    g["id"] = std::string("gen_") + styleName(p.style) + "_" + std::to_string(p.seed);
    g["nodes"] = nlohmann::json::array();

    g["nodes"].push_back({{"id", "001-random"}, {"type", "random_range"}, {"params", {{"min", randomMin}, {"max", randomMax}, {"stream", 2.0F}}}, {"outputs", nlohmann::json::array()}});
    g["nodes"].push_back({{"id", "010-emit-ring"}, {"type", "emit_ring"}, {"params", {{"count", static_cast<float>(ringCount)}, {"speed", bulletSpeed}, {"radius", 2.5F}, {"angle", 0.0F}}}, {"outputs", nlohmann::json::array()}});
    g["nodes"].push_back({{"id", "020-emit-spread"}, {"type", "emit_spread"}, {"params", {{"count", static_cast<float>(std::max(3, ringCount / 2))}, {"speed", bulletSpeed * 0.85F}, {"radius", 2.7F}, {"angle", spread}}}, {"outputs", nlohmann::json::array()}});
    g["nodes"].push_back({{"id", "030-wait"}, {"type", "wait"}, {"params", {{"seconds", waitSeconds}}}, {"outputs", nlohmann::json::array()}});
    g["nodes"].push_back({{"id", "040-loop"}, {"type", "loop"}, {"params", {{"count", static_cast<float>(loops)}, {"target", 0.0F}}}, {"outputs", nlohmann::json::array()}});

    PatternGeneratorResult out;
    out.json = root.dump(2);

    out.metrics.estimatedBulletsPerSecond = static_cast<float>(ringCount + std::max(3, ringCount / 2)) / waitSeconds;
    const float chaosPenalty = p.chaos * 2.4F;
    const float fairnessBonus = p.fairness * 2.2F;
    out.metrics.estimatedDifficultyScore = out.metrics.estimatedBulletsPerSecond * 0.012F + p.speed * 3.2F + chaosPenalty - fairnessBonus;
    out.metrics.averageDodgeGapEstimate = std::max(2.0F, 26.0F - p.density * 14.0F - p.speed * 4.0F + p.fairness * 10.0F - p.chaos * 8.0F);

    for (std::size_t i = 0; i < out.metrics.densityHeatmap.size(); ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(out.metrics.densityHeatmap.size() - 1U);
        const float wave = std::sin((t * 6.2831853F) * (1.0F + p.chaos * 2.5F) + rng.nextFloat01() * 0.2F);
        const float val = std::clamp(p.density * 0.6F + 0.4F + wave * (0.2F + p.chaos * 0.3F) - p.fairness * 0.2F, 0.0F, 1.0F);
        out.metrics.densityHeatmap[i] = val;
    }

    return out;
}
} // namespace

PatternGeneratorResult generatePatternGraphJson(const PatternGeneratorInput& input) { return build(input, 0xA11CEULL); }

PatternGeneratorResult mutatePatternGraphJson(const PatternGeneratorInput& input) {
    PatternGeneratorInput m = input;
    DeterministicRng rng(input.seed ^ 0x51A7EULL);
    m.density = clamp01(m.density + (rng.nextFloat01() * 2.0F - 1.0F) * 0.15F);
    m.speed = clamp01(m.speed + (rng.nextFloat01() * 2.0F - 1.0F) * 0.15F);
    m.chaos = clamp01(m.chaos + (rng.nextFloat01() * 2.0F - 1.0F) * 0.2F);
    return build(m, 0x51A7EULL);
}

PatternGeneratorResult remixPatternGraphJson(const PatternGeneratorInput& a, const PatternGeneratorInput& b) {
    PatternGeneratorInput r;
    r.style = (a.seed & 1ULL) == 0ULL ? a.style : b.style;
    r.seed = a.seed ^ (b.seed << 1U);
    r.density = clamp01((a.density + b.density) * 0.5F);
    r.speed = clamp01((a.speed + b.speed) * 0.5F);
    r.symmetry = clamp01((a.symmetry + b.symmetry) * 0.5F);
    r.chaos = clamp01((a.chaos + b.chaos) * 0.5F);
    r.fairness = clamp01((a.fairness + b.fairness) * 0.5F);
    return build(r, 0xBEEF55ULL);
}

} // namespace engine
