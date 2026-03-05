#include <engine/difficulty_scaling.h>

#include <cstdlib>
#include <iostream>

int main() {
    engine::DifficultyModel a;
    engine::DifficultyModel b;
    a.initializeDefaults();
    b.initializeDefaults();

    if (!a.loadProfilesFromFile("../data/difficulty_profiles.json") || !b.loadProfilesFromFile("../data/difficulty_profiles.json")) {
        std::cerr << "failed to load difficulty profiles\n";
        return EXIT_FAILURE;
    }

    a.setProfile(engine::DifficultyProfile::Hard);
    b.setProfile(engine::DifficultyProfile::Hard);

    for (int i = 0; i < 600; ++i) {
        engine::DeterministicPerformanceMetrics m;
        m.playerHealth01 = 0.8F;
        m.collisionsPerMinute = 2.0F;
        m.defeatedBosses = static_cast<std::uint32_t>(i / 300);
        m.tick = static_cast<std::uint64_t>(i);
        a.setStageZone(1, i < 400 ? engine::ZoneType::Elite : engine::ZoneType::Boss);
        b.setStageZone(1, i < 400 ? engine::ZoneType::Elite : engine::ZoneType::Boss);
        a.update(1.0F / 60.0F, m);
        b.update(1.0F / 60.0F, m);
    }

    if (a.currentDifficulty() != b.currentDifficulty()) {
        std::cerr << "determinism mismatch\n";
        return EXIT_FAILURE;
    }

    const engine::DifficultyScalars s = a.scalars();
    if (s.patternSpeed < 0.5F || s.spawnRate < 0.5F || s.enemyHp < 0.5F) {
        std::cerr << "invalid scalar bounds\n";
        return EXIT_FAILURE;
    }

    std::cout << "difficulty_scaling_tests passed\n";
    return EXIT_SUCCESS;
}
