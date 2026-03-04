#include <engine/patterns.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
    engine::PatternBank bank;
    const bool loaded = bank.loadFromFile("assets/patterns/sandbox_patterns.json")
        || bank.loadFromFile("../assets/patterns/sandbox_patterns.json");
    if (!loaded) {
        std::cerr << "failed to load pattern json\n";
        return EXIT_FAILURE;
    }

    if (bank.patterns().size() < 3) {
        std::cerr << "expected at least 3 composed patterns\n";
        return EXIT_FAILURE;
    }

    if (bank.patterns()[0].layers.size() < 2 || bank.patterns()[0].sequence.empty()) {
        std::cerr << "composition fields not loaded\n";
        return EXIT_FAILURE;
    }

    engine::PatternPlayer a;
    engine::PatternPlayer b;
    a.setBank(&bank);
    b.setBank(&bank);
    a.setRunSeed(1234);
    b.setRunSeed(1234);
    a.setPatternIndex(1);
    b.setPatternIndex(1);

    std::vector<engine::ProjectileSpawn> outA;
    std::vector<engine::ProjectileSpawn> outB;
    outA.reserve(2048);
    outB.reserve(2048);

    for (int i = 0; i < 60; ++i) {
        a.update(1.0F / 60.0F, {0.0F, 0.0F}, {50.0F, -10.0F}, [&](const engine::ProjectileSpawn& p) { outA.push_back(p); });
        b.update(1.0F / 60.0F, {0.0F, 0.0F}, {50.0F, -10.0F}, [&](const engine::ProjectileSpawn& p) { outB.push_back(p); });
    }

    if (outA.size() != outB.size()) {
        std::cerr << "deterministic emit count mismatch\n";
        return EXIT_FAILURE;
    }

    for (std::size_t i = 0; i < outA.size(); ++i) {
        if (std::abs(outA[i].vel.x - outB[i].vel.x) > 0.0001F || std::abs(outA[i].vel.y - outB[i].vel.y) > 0.0001F) {
            std::cerr << "deterministic velocity mismatch\n";
            return EXIT_FAILURE;
        }
    }

    std::cout << "pattern_tests passed\n";
    return EXIT_SUCCESS;
}
