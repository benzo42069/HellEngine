#include <engine/projectiles.h>

#include <cstdint>

#include <array>
#include <cstdlib>
#include <iostream>

namespace {

std::uint64_t runScenario() {
    engine::ProjectileSystem ps;
    ps.initialize(20000, 420.0F, 32, 18);
    ps.spawnRadialBurst(5000, 90.0F, 3.0F, 42);

    for (int i = 0; i < 300; ++i) {
        ps.beginTick();
        ps.updateMotion(1.0F / 60.0F);
        ps.buildGrid();
        const engine::CollisionTarget playerTarget {.pos = {0.0F, 0.0F}, .radius = 12.0F, .id = 0U, .team = 0U};
        std::array<engine::CollisionEvent, 20000> events {};
        std::uint32_t eventCount = 0;
        ps.resolveCollisions(std::span<const engine::CollisionTarget>(&playerTarget, 1), events, eventCount);
    }

    return ps.debugStateHash();
}

constexpr std::uint64_t expectedHashForToolchain() {
#if defined(_MSC_VER)
    return 2655185603234195783ULL;
#elif defined(__clang__)
    return 12663723333501355397ULL;
#elif defined(__GNUC__)
    return 12663723333501355397ULL;
#else
    return 0ULL;
#endif
}

} // namespace

int main() {
    const std::uint64_t hashA = runScenario();
    const std::uint64_t hashB = runScenario();
    if (hashA != hashB) {
        std::cerr << "determinism_cross_config_test instability hashA=" << hashA << " hashB=" << hashB << "\n";
        return EXIT_FAILURE;
    }

    const std::uint64_t expected = expectedHashForToolchain();
    if (expected != 0ULL && hashA != expected) {
        std::cerr << "determinism_cross_config_test hash mismatch expected=" << expected << " actual=" << hashA << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "determinism_cross_config_test passed\n";
    return EXIT_SUCCESS;
}
