#include <engine/config.h>
#include <engine/gameplay_session.h>
#include <engine/gameplay_session_subsystems.h>
#include <engine/run_structure.h>

#include <iostream>

int main() {
    engine::EngineConfig config;
    config.simulationSeed = 1337U;

    engine::GameplaySession session(config);

    engine::RngStreams expectedStreams(config.simulationSeed);
    const std::uint64_t expectedValue = expectedStreams.stream("session_test").nextU64();
    const std::uint64_t actualValue = session.simulation_.rngStreams.stream("session_test").nextU64();
    if (expectedValue != actualValue) {
        std::cerr << "simulation seed stream mismatch\n";
        return 1;
    }

    if (session.playerPos().x != session.playerState_.playerPos.x || session.playerPos().y != session.playerState_.playerPos.y) {
        std::cerr << "player getter mismatch\n";
        return 1;
    }

    if (session.upgradeScreenOpen() != session.progression_.upgradeScreenOpen) {
        std::cerr << "progression getter mismatch\n";
        return 1;
    }

    if (session.perfHudOpen() != session.debugTools_.perfHudOpen) {
        std::cerr << "debug/perf getter mismatch\n";
        return 1;
    }

    session.setDangerFieldEnabled(true);
    if (!session.presentation_.dangerFieldEnabled || !session.dangerFieldEnabled()) {
        std::cerr << "danger field state mismatch\n";
        return 1;
    }

    session.toggleDangerField();
    if (session.presentation_.dangerFieldEnabled || session.dangerFieldEnabled()) {
        std::cerr << "danger field toggle mismatch\n";
        return 1;
    }



    engine::SessionOrchestrationSubsystem orchestration;
    bool upgradeOpen = false;
    orchestration.updateUpgradeCadence(300, false, upgradeOpen, []() { return true; });
    if (!upgradeOpen) {
        std::cerr << "upgrade cadence mismatch\n";
        return 1;
    }

    bool perfHudOpen = false;
    bool dangerFieldEnabled = false;
    bool rarityForced = false;
    engine::UpgradeDebugOptions debugOptions;
    debugOptions.showPerfHud = true;
    debugOptions.showDangerField = true;
    debugOptions.spawnUpgradeScreen = false;
    debugOptions.forcedRarity = static_cast<int>(engine::TraitRarity::Rare);
    orchestration.applyUpgradeDebugOptions(
        debugOptions,
        true,
        perfHudOpen,
        dangerFieldEnabled,
        upgradeOpen,
        []() { return false; },
        [&](const engine::TraitRarity) { rarityForced = true; }
    );
    if (!perfHudOpen || !dangerFieldEnabled || !rarityForced) {
        std::cerr << "upgrade debug options mismatch\n";
        return 1;
    }

    engine::EncounterSimulationSubsystem encounterSubsystem;
    std::vector<engine::ShakeParams> shakes;
    std::vector<engine::AudioEvent> audio;
    const engine::ZoneDefinition combatZone {.type = engine::ZoneType::Combat, .durationSeconds = 1.0F};
    const engine::ZoneDefinition bossZone {.type = engine::ZoneType::Boss, .durationSeconds = 1.0F};
    encounterSubsystem.emitZoneTransitionFeedback(&combatZone, &bossZone, session.playerPos(), shakes, audio);
    if (shakes.empty() || audio.empty() || audio.back().type != engine::AudioEventType::BossPhaseTransition) {
        std::cerr << "zone transition feedback mismatch\n";
        return 1;
    }

    const std::size_t shakeCountAfterBossTransition = shakes.size();
    encounterSubsystem.emitAmbientZoneFeedback(&bossZone, shakes);
    if (shakes.size() != shakeCountAfterBossTransition + 1) {
        std::cerr << "ambient zone feedback mismatch\n";
        return 1;
    }

    return 0;
}
