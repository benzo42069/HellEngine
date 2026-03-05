#include <engine/replay.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

int main() {
    const std::uint64_t h0 = engine::computeReplayStateHash(0, 1, 2, 3);
    const std::uint64_t h1 = engine::computeReplayStateHash(60, 123456, 789012, 345678);
    const std::uint64_t h2 = engine::computeReplayStateHash(120, 11259375, 1193046, 10066329);

    if (h0 != 12411622248166496467ULL || h1 != 12411622248642669239ULL || h2 != 12411622360687519014ULL) {
        std::cerr << "golden replay hashes changed unexpectedly\n";
        return EXIT_FAILURE;
    }

    engine::ReplayRecorder rec;
    rec.begin(7, "pack:golden", "deadbeef", 60);
    rec.recordTickInput(engine::InputMoveLeft);
    rec.recordStateSample(engine::ReplayStateSample {.tick = 0, .totalHash = h0, .bulletsHash = 1, .entitiesHash = 2, .runStateHash = 3});
    if (!rec.save("golden_replay_sample.json")) {
        std::cerr << "failed to save golden replay\n";
        return EXIT_FAILURE;
    }

    engine::ReplayPlayer player;
    if (!player.load("golden_replay_sample.json") || !player.validFor(7, "pack:golden", "deadbeef")) {
        std::cerr << "failed to load/validate golden replay\n";
        return EXIT_FAILURE;
    }

    engine::ReplayMismatch mm;
    if (!player.verifyStateSample(engine::ReplayStateSample {.tick = 0, .totalHash = h0, .bulletsHash = 1, .entitiesHash = 2, .runStateHash = 3}, &mm)) {
        std::cerr << "golden replay verification unexpectedly failed\n";
        return EXIT_FAILURE;
    }

    std::filesystem::remove("golden_replay_sample.json");
    std::cout << "golden_replay_tests passed\n";
    return EXIT_SUCCESS;
}
