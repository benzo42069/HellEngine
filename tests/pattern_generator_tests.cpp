#include <engine/pattern_generator.h>

#include <cmath>
#include <cstdlib>
#include <iostream>

int main() {
    engine::PatternGeneratorInput in;
    in.style = engine::PatternStylePreset::BurstFan;
    in.seed = 424242;
    in.density = 0.7F;
    in.speed = 0.6F;
    in.symmetry = 0.8F;
    in.chaos = 0.2F;
    in.fairness = 0.7F;

    const auto a = engine::generatePatternGraphJson(in);
    const auto b = engine::generatePatternGraphJson(in);

    if (a.json != b.json) {
        std::cerr << "generator output not deterministic\n";
        return EXIT_FAILURE;
    }
    if (std::abs(a.metrics.estimatedDifficultyScore - b.metrics.estimatedDifficultyScore) > 0.0001F) {
        std::cerr << "difficulty metric mismatch\n";
        return EXIT_FAILURE;
    }

    const auto m = engine::mutatePatternGraphJson(in);
    if (m.json == a.json) {
        std::cerr << "mutate should produce a different graph\n";
        return EXIT_FAILURE;
    }

    const auto r = engine::remixPatternGraphJson(in, engine::PatternGeneratorInput {.style = engine::PatternStylePreset::SniperLanes, .seed = 777, .density = 0.3F, .speed = 0.9F, .symmetry = 0.4F, .chaos = 0.6F, .fairness = 0.2F});
    if (r.metrics.estimatedBulletsPerSecond <= 0.0F) {
        std::cerr << "invalid remix metrics\n";
        return EXIT_FAILURE;
    }

    std::cout << "pattern_generator_tests passed\n";
    return EXIT_SUCCESS;
}
