#include <engine/projectiles.h>

#include <cstdlib>
#include <iostream>

int main() {
    engine::ProjectileSystem a;
    engine::ProjectileSystem b;
    a.initialize(512, 420.0F, 32, 18);
    b.initialize(512, 420.0F, 32, 18);

    engine::ProjectileBehavior h; h.homingTurnRateDegPerSec = 180.0F; h.homingMaxAngleStepDeg = 10.0F;
    engine::ProjectileBehavior c; c.curvedAngularVelocityDegPerSec = 90.0F;
    engine::ProjectileBehavior ad; ad.accelerationPerSec = 25.0F; ad.dragPerSec = 0.15F;
    engine::ProjectileBehavior bo; bo.maxBounces = 2;
    engine::ProjectileBehavior sp; sp.splitCount = 4; sp.splitAngleSpreadDeg = 60.0F; sp.splitDelaySeconds = 0.4F;
    engine::ProjectileBehavior ex; ex.explodeRadius = 24.0F; ex.explodeShards = 8;
    engine::ProjectileBehavior be; be.beamSegmentSamples = 6; be.beamDurationSeconds = 0.8F;

    const engine::ProjectileSpawn seeds[] = {
        {.pos = {0.0F, 0.0F}, .vel = {120.0F, 10.0F}, .radius = 3.0F, .behavior = h},
        {.pos = {0.0F, -60.0F}, .vel = {90.0F, 45.0F}, .radius = 3.0F, .behavior = c},
        {.pos = {-120.0F, 10.0F}, .vel = {85.0F, 0.0F}, .radius = 3.0F, .behavior = ad},
        {.pos = {380.0F, 0.0F}, .vel = {220.0F, 8.0F}, .radius = 3.0F, .behavior = bo},
        {.pos = {0.0F, 90.0F}, .vel = {60.0F, 0.0F}, .radius = 3.0F, .behavior = sp},
        {.pos = {390.0F, -20.0F}, .vel = {75.0F, 0.0F}, .radius = 3.0F, .behavior = ex},
        {.pos = {-40.0F, 0.0F}, .vel = {130.0F, 0.0F}, .radius = 2.0F, .behavior = be},
    };

    for (const auto& s : seeds) {
        (void)a.spawn(s);
        (void)b.spawn(s);
    }

    std::uint64_t hashA = 0;
    std::uint64_t hashB = 0;
    for (int i = 0; i < 180; ++i) {
        a.beginTick();
        b.beginTick();
        a.update(1.0F / 60.0F, {0.0F, 0.0F}, 12.0F);
        b.update(1.0F / 60.0F, {0.0F, 0.0F}, 12.0F);
        hashA ^= a.debugStateHash() + static_cast<std::uint64_t>(i);
        hashB ^= b.debugStateHash() + static_cast<std::uint64_t>(i);
    }

    if (hashA != hashB) {
        std::cerr << "projectile behavior determinism mismatch\n";
        return EXIT_FAILURE;
    }

    std::cout << "projectile_behavior_tests passed\n";
    return EXIT_SUCCESS;
}
