#include <engine/entities.h>
#include <engine/patterns.h>
#include <engine/projectiles.h>

#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
    engine::PatternBank pb;
    if (!pb.loadFromFile("assets/patterns/sandbox_patterns.json") && !pb.loadFromFile("../assets/patterns/sandbox_patterns.json")) {
        pb.loadFallbackDefaults();
    }

    engine::EntityTemplate boss;
    boss.name = "Test Boss";
    boss.type = engine::EntityType::Boss;
    boss.movement = engine::MovementBehavior::Linear;
    boss.spawnPosition = {0.0F, -50.0F};
    boss.baseVelocity = {0.0F, 10.0F};
    boss.attacksEnabled = true;
    boss.attackPatternName = "Wave Weave";
    boss.attackIntervalSeconds = 0.4F;
    boss.boss.enabled = true;
    boss.boss.introDurationSeconds = 1.0F;
    boss.boss.phases = {
        engine::BossPhase {.attackPatternName = "Wave Weave", .patternSequence = {"Wave Weave", "Composed Helix"}, .movement = engine::MovementBehavior::Linear, .durationSeconds = 1.0F, .difficultyScale = 1.0F, .patternCadenceSeconds = 0.2F},
        engine::BossPhase {.attackPatternName = "Composed Helix", .patternSequence = {"Composed Helix", "Wave Weave"}, .movement = engine::MovementBehavior::Chase, .durationSeconds = 1.0F, .difficultyScale = 1.5F, .patternCadenceSeconds = 0.2F},
    };
    boss.boss.rewardDrop = engine::ResourceYield {.upgradeCurrency = 60.0F, .healthRecovery = 10.0F, .buffDurationSeconds = 3.0F};
    boss.spawnRule = engine::SpawnRule {.enabled = false};

    std::vector<engine::EntityTemplate> templates {boss};

    engine::EntitySystem sys;
    sys.setTemplates(&templates);
    sys.setPatternBank(&pb);
    sys.setRunSeed(1234);
    sys.reset();

    engine::ProjectileSystem proj;
    proj.initialize(4096, 420.0F, 32, 18);

    for (int i = 0; i < 30; ++i) {
        sys.update(1.0F / 60.0F, proj, {0.0F, 0.0F});
    }
    if (proj.stats().activeCount != 0) {
        std::cerr << "boss fired during intro sequence\n";
        return EXIT_FAILURE;
    }

    bool sawTelegraph = false;
    bool sawHazardSync = false;
    bool sawBossDefeatedEvent = false;
    for (int i = 0; i < 220; ++i) {
        sys.update(1.0F / 60.0F, proj, {0.0F, 0.0F});
        for (const engine::EntityRuntimeEvent& evt : sys.runtimeEvents()) {
            if (evt.type == engine::EntityRuntimeEventType::Telegraph) sawTelegraph = true;
            if (evt.type == engine::EntityRuntimeEventType::HazardSync) sawHazardSync = true;
            if (evt.type == engine::EntityRuntimeEventType::BossDefeated) sawBossDefeatedEvent = true;
        }
        proj.update(1.0F / 60.0F, {0.0F, 0.0F}, 12.0F);
    }

    const auto& stats = sys.stats();
    if (stats.bossPhaseTransitions == 0) {
        std::cerr << "boss phase transition did not trigger\n";
        return EXIT_FAILURE;
    }
    if (stats.defeatedBosses == 0) {
        std::cerr << "boss was not defeated after final phase\n";
        return EXIT_FAILURE;
    }
    if (stats.upgradeCurrency < 60.0F || stats.healthRecoveryAccum < 10.0F || stats.buffTimeRemaining <= 0.0F) {
        std::cerr << "boss reward drops missing\n";
        return EXIT_FAILURE;
    }
    if (!sawTelegraph || !sawHazardSync || !sawBossDefeatedEvent) {
        std::cerr << "boss runtime events missing\n";
        return EXIT_FAILURE;
    }
    if (stats.telegraphEvents == 0 || stats.hazardSyncEvents == 0) {
        std::cerr << "boss event counters missing\n";
        return EXIT_FAILURE;
    }

    std::cout << "boss_phase_tests passed\n";
    return EXIT_SUCCESS;
}
