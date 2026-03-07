#include <engine/projectiles.h>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace {
std::uint64_t runSimHash(const std::uint64_t seed) {
    engine::ProjectileSystem system;
    system.initialize(4096, 420.0F, 32, 18);
    system.spawnRadialBurst(1024, 140.0F, 2.5F, seed);

    constexpr int kTicks = 240;
    for (int i = 0; i < kTicks; ++i) {
        system.beginTick();
        system.update(1.0F / 60.0F, {0.0F, 0.0F}, 12.0F);
    }

    return system.debugStateHash();
}
} // namespace

TEST_CASE("Same seed produces same hash", "[determinism][property]") {
    const std::uint64_t a = runSimHash(123456789ULL);
    const std::uint64_t b = runSimHash(123456789ULL);
    REQUIRE(a == b);
}

TEST_CASE("Different seeds produce different hashes", "[determinism][property]") {
    const std::uint64_t a = runSimHash(1111ULL);
    const std::uint64_t b = runSimHash(2222ULL);
    REQUIRE(a != b);
}
