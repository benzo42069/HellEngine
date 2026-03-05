#include <engine/defensive_special.h>
#include <engine/projectiles.h>

#include <cstdlib>
#include <iostream>

int main() {
    engine::DefensiveSpecialConfig cfg;
    cfg.baseCapacity = 1;
    cfg.maxCapacity = 3;
    cfg.cooldownPerChargeSeconds = 12.0F;
    cfg.minCooldownSeconds = 6.0F;
    cfg.grazeCooldownTicks = 10;

    engine::DefensiveSpecialSystem system;
    system.initialize(cfg, engine::presetFromLabel("standard"));

    if (system.capacity() != 1 || system.currentCharges() != 1) {
        std::cerr << "initial charge state invalid\n";
        return EXIT_FAILURE;
    }
    if (!system.tryActivate() || system.currentCharges() != 0 || !system.active()) {
        std::cerr << "activation consume rule failed\n";
        return EXIT_FAILURE;
    }

    engine::TraitModifiers mods;
    mods.defensiveSpecialCapacityAdd = 2;
    mods.defensiveSpecialCooldownReduction = 0.4F;

    for (int i = 0; i < 600; ++i) {
        system.update(1.0F / 60.0F, mods);
    }
    if (system.capacity() != 3) {
        std::cerr << "capacity upgrade clamp failed\n";
        return EXIT_FAILURE;
    }

    system.addDamagePoints(50.0F);
    system.update(0.0F, mods);

    // deterministic time dilation through projectile sim
    engine::ProjectileSystem p1;
    engine::ProjectileSystem p2;
    p1.initialize(64, 420.0F, 16, 9);
    p2.initialize(64, 420.0F, 16, 9);
    for (int i = 0; i < 20; ++i) {
        const float x = -200.0F + static_cast<float>(i) * 8.0F;
        engine::ProjectileSpawn s;
        s.pos = {x, -20.0F};
        s.vel = {120.0F, 0.0F};
        s.radius = 3.0F;
        s.allegiance = (i % 2 == 0) ? engine::ProjectileAllegiance::Enemy : engine::ProjectileAllegiance::Player;
        p1.spawn(s);
        p2.spawn(s);
    }

    const engine::TimeDilationPreset preset = engine::presetFromLabel("standard");
    for (int t = 0; t < 180; ++t) {
        p1.beginTick();
        p2.beginTick();
        p1.update(1.0F / 60.0F, {0.0F, 0.0F}, 10.0F, preset.enemyScale, preset.playerProjectileScale);
        p2.update(1.0F / 60.0F, {0.0F, 0.0F}, 10.0F, preset.enemyScale, preset.playerProjectileScale);
    }
    if (p1.debugStateHash() != p2.debugStateHash()) {
        std::cerr << "time dilation determinism hash mismatch\n";
        return EXIT_FAILURE;
    }

    // graze accounting stability
    engine::ProjectileSystem graze;
    graze.initialize(8, 420.0F, 4, 4);
    engine::ProjectileSpawn g;
    g.pos = {18.0F, 0.0F};
    g.vel = {0.0F, 0.0F};
    g.radius = 2.0F;
    g.allegiance = engine::ProjectileAllegiance::Enemy;
    graze.spawn(g);

    std::uint32_t points0 = graze.collectGrazePoints({0.0F, 0.0F}, 8.0F, 2.0F, 24.0F, 10, 10);
    std::uint32_t points1 = graze.collectGrazePoints({0.0F, 0.0F}, 8.0F, 2.0F, 24.0F, 15, 10);
    std::uint32_t points2 = graze.collectGrazePoints({0.0F, 0.0F}, 8.0F, 2.0F, 24.0F, 21, 10);
    if (points0 != 1 || points1 != 0 || points2 != 1) {
        std::cerr << "graze cooldown accounting failed\n";
        return EXIT_FAILURE;
    }

    // upgrade effects clamp
    engine::TraitModifiers clampMods;
    clampMods.defensiveSpecialCapacityAdd = 8;
    clampMods.defensiveSpecialCooldownReduction = 0.9F;
    system.update(0.0F, clampMods);
    if (system.capacity() != 3) {
        std::cerr << "capacity clamp failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "defensive_special_tests passed\n";
    return EXIT_SUCCESS;
}
