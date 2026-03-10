#include <engine/entities.h>
#include <engine/patterns.h>
#include <engine/projectiles.h>

#include <catch2/catch_test_macros.hpp>

#include <array>

TEST_CASE("Entity system spawns enemies and emits projectile/resource stats", "[entity]") {
    engine::EntityDatabase db;
    const bool entitiesLoaded = db.loadFromFile("data/entities.json") || db.loadFromFile("../data/entities.json");
    REQUIRE(entitiesLoaded);

    engine::PatternBank pb;
    const bool patternsLoaded = pb.loadFromFile("assets/patterns/sandbox_patterns.json")
        || pb.loadFromFile("../assets/patterns/sandbox_patterns.json");
    REQUIRE(patternsLoaded);

    engine::ProjectileSystem proj;
    proj.initialize(20000, 420.0F, 32, 18);

    engine::EntitySystem sys;
    sys.setTemplates(&db.templates());
    sys.setPatternBank(&pb);
    sys.setRunSeed(77);
    sys.reset();

    const std::array<engine::CollisionTarget, 1> playerTarget {
        engine::CollisionTarget {.pos = {0.0F, 0.0F}, .radius = 12.0F, .id = 0U, .team = 0U},
    };
    std::array<engine::CollisionEvent, 2048> events {};

    for (int i = 0; i < 420; ++i) {
        proj.beginTick();
        sys.update(1.0F / 60.0F, proj, {0.0F, 0.0F});
        proj.updateMotion(1.0F / 60.0F);
        proj.buildGrid();
        std::uint32_t eventCount = 0;
        proj.resolveCollisions(playerTarget, events, eventCount);
    }

    const auto& stats = sys.stats();
    REQUIRE(stats.aliveEnemies > 0);
    REQUIRE(proj.stats().activeCount > 0);
    REQUIRE(stats.harvestedNodes > 0);
    REQUIRE(stats.upgradeCurrency > 0.0F);
    REQUIRE(stats.healthRecoveryAccum > 0.0F);
}
