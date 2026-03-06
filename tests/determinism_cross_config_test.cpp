#include <engine/projectiles.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>

int main() {
    engine::ProjectileSystem ps;
    ps.initialize(20000, 420.0F, 32, 18);
    ps.spawnRadialBurst(5000, 90.0F, 3.0F, 42);

    for (int i = 0; i < 300; ++i) {
        ps.beginTick();
        ps.update(1.0F / 60.0F, {0.0F, 0.0F}, 12.0F);
    }

    const std::uint64_t hash = ps.debugStateHash();
    constexpr std::uint64_t expected = 10082966582318243511ULL;
    if (hash != expected) {
        std::cerr << "determinism_cross_config_test hash mismatch expected=" << expected << " actual=" << hash << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "determinism_cross_config_test passed\n";
    return EXIT_SUCCESS;
}
