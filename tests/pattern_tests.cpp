#include <engine/patterns.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

TEST_CASE("PatternBank loads and PatternPlayer emission is deterministic", "[pattern]") {
    engine::PatternBank bank;
    const bool loaded = bank.loadFromFile("assets/patterns/sandbox_patterns.json")
        || bank.loadFromFile("../assets/patterns/sandbox_patterns.json");
    REQUIRE(loaded);

    REQUIRE(bank.patterns().size() >= 3);
    REQUIRE(bank.patterns()[0].layers.size() >= 2);
    REQUIRE_FALSE(bank.patterns()[0].sequence.empty());

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

    REQUIRE(outA.size() == outB.size());

    for (std::size_t i = 0; i < outA.size(); ++i) {
        REQUIRE(std::abs(outA[i].vel.x - outB[i].vel.x) <= 0.0001F);
        REQUIRE(std::abs(outA[i].vel.y - outB[i].vel.y) <= 0.0001F);
    }
}
