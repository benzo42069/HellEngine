#include <engine/replay.h>

#include <filesystem>
#include <cstdlib>
#include <iostream>

int main() {
    engine::ReplayRecorder rec;
    rec.begin(777, engine::buildContentVersionTag("content.pak"));
    rec.recordTick(engine::InputMoveLeft | engine::InputMoveUp, 12345);
    rec.recordTick(engine::InputMoveRight, 54321);

    const std::string path = "replay_test.json";
    if (!rec.save(path)) {
        std::cerr << "failed to save replay\n";
        return EXIT_FAILURE;
    }

    engine::ReplayPlayer player;
    if (!player.load(path)) {
        std::cerr << "failed to load replay\n";
        return EXIT_FAILURE;
    }

    if (!player.validFor(777, engine::buildContentVersionTag("content.pak"))) {
        std::cerr << "replay header validation failed\n";
        return EXIT_FAILURE;
    }

    if (player.inputForTick(0) != (engine::InputMoveLeft | engine::InputMoveUp)) {
        std::cerr << "tick0 input mismatch\n";
        return EXIT_FAILURE;
    }

    if (!player.verifyTick(1, 54321) || player.verifyTick(1, 1)) {
        std::cerr << "state hash verification mismatch\n";
        return EXIT_FAILURE;
    }

    const auto h1 = engine::computeReplayStateHash(12, 9, 3, 1, 14.5F);
    const auto h2 = engine::computeReplayStateHash(12, 9, 3, 1, 14.5F);
    if (h1 != h2) {
        std::cerr << "hash function not deterministic\n";
        return EXIT_FAILURE;
    }

    std::filesystem::remove(path);
    std::cout << "replay_tests passed\n";
    return EXIT_SUCCESS;
}
