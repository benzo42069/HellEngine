#include <engine/projectiles.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Projectile system deterministic burst simulation", "[projectile]") {
    engine::ProjectileSystem systemA;
    engine::ProjectileSystem systemB;

    systemA.initialize(12000, 420.0F, 32, 18);
    systemB.initialize(12000, 420.0F, 32, 18);

    systemA.spawnRadialBurst(10000, 90.0F, 3.0F, 42);
    systemB.spawnRadialBurst(10000, 90.0F, 3.0F, 42);

    for (int i = 0; i < 180; ++i) {
        systemA.update(1.0F / 60.0F, {0.0F, 0.0F}, 12.0F);
        systemB.update(1.0F / 60.0F, {0.0F, 0.0F}, 12.0F);
    }

    const auto& a = systemA.stats();
    const auto& b = systemB.stats();

    REQUIRE(a.activeCount == b.activeCount);
    REQUIRE(a.totalCollisions == b.totalCollisions);
    REQUIRE(a.activeCount != 0);
}
