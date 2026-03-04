#include <engine/projectiles.h>

#include <cstdlib>
#include <iostream>

int main() {
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

    if (a.activeCount != b.activeCount || a.totalCollisions != b.totalCollisions) {
        std::cerr << "determinism mismatch\n";
        return EXIT_FAILURE;
    }

    if (a.activeCount == 0) {
        std::cerr << "expected active projectiles after simulation\n";
        return EXIT_FAILURE;
    }

    std::cout << "projectile_tests passed\n";
    return EXIT_SUCCESS;
}
