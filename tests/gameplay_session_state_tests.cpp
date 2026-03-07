#include <engine/config.h>
#include <engine/gameplay_session.h>

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

    return 0;
}
