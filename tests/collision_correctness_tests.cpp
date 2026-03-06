#include <engine/projectiles.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

std::vector<engine::CollisionEvent> bruteForce(
    const std::vector<engine::ProjectileSpawn>& bullets,
    const std::vector<engine::CollisionTarget>& targets
) {
    std::vector<engine::CollisionEvent> out;
    std::vector<std::uint8_t> hit(bullets.size(), 0);
    for (const engine::CollisionTarget& t : targets) {
        for (std::size_t i = 0; i < bullets.size(); ++i) {
            if (hit[i]) continue;
            if (t.team == 0U && bullets[i].allegiance == engine::ProjectileAllegiance::Player) continue;
            if (t.team == 1U && bullets[i].allegiance == engine::ProjectileAllegiance::Enemy) continue;
            const float dx = bullets[i].pos.x - t.pos.x;
            const float dy = bullets[i].pos.y - t.pos.y;
            const float rr = bullets[i].radius + t.radius;
            if (dx * dx + dy * dy <= rr * rr) {
                hit[i] = 1;
                out.push_back(engine::CollisionEvent {.bulletIndex = static_cast<std::uint32_t>(i), .targetId = t.id, .graze = false});
            }
        }
    }
    std::sort(out.begin(), out.end(), [](const engine::CollisionEvent& a, const engine::CollisionEvent& b) {
        if (a.targetId != b.targetId) return a.targetId < b.targetId;
        return a.bulletIndex < b.bulletIndex;
    });
    return out;
}

bool sameEvents(const std::vector<engine::CollisionEvent>& a, const std::vector<engine::CollisionEvent>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].bulletIndex != b[i].bulletIndex || a[i].targetId != b[i].targetId || a[i].graze != b[i].graze) return false;
    }
    return true;
}

} // namespace

int main() {
    {
        engine::ProjectileSystem sys;
        sys.initialize(64, 100.0F, 8, 8);
        sys.beginTick();
        sys.spawn({.pos = {10.0F, 12.0F}, .vel = {0, 0}, .radius = 3.0F});
        sys.updateMotion(0.0F);
        sys.buildGrid();
        std::vector<engine::CollisionTarget> targets {{.pos = {10.0F, 12.0F}, .radius = 4.0F, .id = 7, .team = 0}};
        std::vector<engine::CollisionEvent> events;
        events.reserve(64);
        sys.resolveCollisions(targets, events);
        if (events.size() != 1 || events[0].targetId != 7) return EXIT_FAILURE;
    }

    {
        engine::ProjectileSystem sys;
        sys.initialize(64, 100.0F, 8, 8);
        sys.beginTick();
        sys.spawn({.pos = {8.5F, 0.0F}, .vel = {0, 0}, .radius = 1.0F});
        if (sys.collectGrazePoints({0, 0}, 6.0F, 1.0F, 4.0F, 10, 1) != 1U) return EXIT_FAILURE;
    }

    {
        engine::ProjectileSystem sys;
        sys.initialize(64, 100.0F, 8, 8);
        sys.beginTick();
        sys.spawn({.pos = {0.0F, 0.0F}, .vel = {0, 0}, .radius = 2.0F, .behavior = {}, .allegiance = engine::ProjectileAllegiance::Player});
        sys.updateMotion(0.0F);
        sys.buildGrid();
        std::vector<engine::CollisionTarget> targets {{.pos = {0.0F, 0.0F}, .radius = 4.0F, .id = 0, .team = 0}};
        std::vector<engine::CollisionEvent> events;
        events.reserve(64);
        sys.resolveCollisions(targets, events);
        if (!events.empty()) return EXIT_FAILURE;
    }

    {
        engine::ProjectileSystem sys;
        sys.initialize(4096, 600.0F, 32, 18);
        std::vector<engine::ProjectileSpawn> bullets;
        bullets.reserve(1000);
        for (std::uint32_t i = 0; i < 1000; ++i) {
            const float x = static_cast<float>((i % 100) * 8) - 300.0F;
            const float y = static_cast<float>((i / 100) * 12) - 200.0F;
            bullets.push_back({.pos = {x, y}, .vel = {0, 0}, .radius = 1.5F});
            sys.spawn(bullets.back());
        }
        std::vector<engine::CollisionTarget> targets;
        targets.reserve(50);
        for (std::uint32_t i = 0; i < 50; ++i) {
            targets.push_back(engine::CollisionTarget {.pos = {static_cast<float>(i * 10) - 200.0F, static_cast<float>((i % 10) * 15) - 60.0F}, .radius = 3.0F, .id = i + 1, .team = 0});
        }

        sys.beginTick();
        sys.updateMotion(0.0F);
        sys.buildGrid();
        std::vector<engine::CollisionEvent> got;
        got.reserve(2000);
        sys.resolveCollisions(targets, got);
        if (!sameEvents(got, bruteForce(bullets, targets))) return EXIT_FAILURE;
    }

    {
        engine::ProjectileSystem a;
        engine::ProjectileSystem b;
        a.initialize(256, 120.0F, 12, 8);
        b.initialize(256, 120.0F, 12, 8);
        for (std::uint32_t i = 0; i < 100; ++i) {
            engine::ProjectileSpawn s {.pos = {static_cast<float>(i) - 30.0F, static_cast<float>((i * 7) % 40) - 20.0F}, .vel = {0, 0}, .radius = 2.0F};
            a.spawn(s);
            b.spawn(s);
        }
        std::vector<engine::CollisionTarget> targets {{.pos = {0, 0}, .radius = 40.0F, .id = 1, .team = 0}, {.pos = {15, 5}, .radius = 25.0F, .id = 2, .team = 0}};
        std::vector<engine::CollisionEvent> ea;
        std::vector<engine::CollisionEvent> eb;
        ea.reserve(256);
        eb.reserve(256);
        a.beginTick();
        b.beginTick();
        a.updateMotion(0.0F);
        b.updateMotion(0.0F);
        a.buildGrid();
        b.buildGrid();
        a.resolveCollisions(targets, ea);
        b.resolveCollisions(targets, eb);
        if (!sameEvents(ea, eb)) return EXIT_FAILURE;
    }

    std::cout << "collision_correctness_tests passed\n";
    return EXIT_SUCCESS;
}
