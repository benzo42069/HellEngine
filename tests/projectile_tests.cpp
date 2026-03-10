#include <engine/projectiles.h>

#include <catch2/catch_test_macros.hpp>

#include <array>

TEST_CASE("Projectile system deterministic burst simulation", "[projectile]") {
    engine::ProjectileSystem systemA;
    engine::ProjectileSystem systemB;

    systemA.initialize(12000, 420.0F, 32, 18);
    systemB.initialize(12000, 420.0F, 32, 18);

    systemA.spawnRadialBurst(10000, 90.0F, 3.0F, 42);
    systemB.spawnRadialBurst(10000, 90.0F, 3.0F, 42);

    const std::array<engine::CollisionTarget, 1> playerTarget {
        engine::CollisionTarget {.pos = {0.0F, 0.0F}, .radius = 12.0F, .id = 0U, .team = 0U},
    };
    std::array<engine::CollisionEvent, 1024> eventsA {};
    std::array<engine::CollisionEvent, 1024> eventsB {};

    for (int i = 0; i < 180; ++i) {
        systemA.beginTick();
        systemA.updateMotion(1.0F / 60.0F);
        systemA.buildGrid();
        std::uint32_t eventCountA = 0;
        systemA.resolveCollisions(playerTarget, eventsA, eventCountA);

        systemB.beginTick();
        systemB.updateMotion(1.0F / 60.0F);
        systemB.buildGrid();
        std::uint32_t eventCountB = 0;
        systemB.resolveCollisions(playerTarget, eventsB, eventCountB);
    }

    const auto& a = systemA.stats();
    const auto& b = systemB.stats();

    REQUIRE(a.activeCount == b.activeCount);
    REQUIRE(a.totalCollisions == b.totalCollisions);
    REQUIRE(a.activeCount != 0);
}
