#include <engine/entities.h>
#include <engine/patterns.h>
#include <engine/projectiles.h>

#include <cstdlib>
#include <iostream>

int main() {
    engine::EntityDatabase db;
    if (!db.loadFromFile("data/entities.json") && !db.loadFromFile("../data/entities.json")) {
        std::cerr << "failed to load entities\n";
        return EXIT_FAILURE;
    }

    engine::PatternBank pb;
    if (!pb.loadFromFile("assets/patterns/sandbox_patterns.json") && !pb.loadFromFile("../assets/patterns/sandbox_patterns.json")) {
        std::cerr << "failed to load patterns\n";
        return EXIT_FAILURE;
    }

    engine::ProjectileSystem proj;
    proj.initialize(20000, 420.0F, 32, 18);

    engine::EntitySystem sys;
    sys.setTemplates(&db.templates());
    sys.setPatternBank(&pb);
    sys.setRunSeed(77);
    sys.reset();

    for (int i = 0; i < 420; ++i) {
        sys.update(1.0F / 60.0F, proj, {0.0F, 0.0F});
        proj.update(1.0F / 60.0F, {0.0F, 0.0F}, 12.0F);
    }

    const auto& stats = sys.stats();
    if (stats.aliveEnemies == 0) {
        std::cerr << "expected alive enemies from spawn rules\n";
        return EXIT_FAILURE;
    }

    if (proj.stats().activeCount == 0) {
        std::cerr << "expected projectile fire from enemy attack patterns\n";
        return EXIT_FAILURE;
    }

    if (stats.harvestedNodes == 0 || stats.upgradeCurrency <= 0.0F || stats.healthRecoveryAccum <= 0.0F) {
        std::cerr << "expected harvested resource yields (currency/health/buff)\n";
        return EXIT_FAILURE;
    }

    std::cout << "entity_tests passed\n";
    return EXIT_SUCCESS;
}
