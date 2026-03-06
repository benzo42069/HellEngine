#include <engine/replay.h>
#include <engine/deterministic_rng.h>

#include <filesystem>
#include <cstdlib>
#include <iostream>

int main() {
    engine::ReplayRecorder rec;
    rec.begin(777, engine::buildContentVersionTag("content.pak"), engine::buildContentHashTag("content.pak"), 30);
    rec.recordTickInput(engine::InputMoveLeft | engine::InputMoveUp);
    rec.recordTickInput(engine::InputMoveRight);

    engine::ReplayStateSample s0;
    s0.tick = 0;
    s0.bulletsHash = 11;
    s0.entitiesHash = 22;
    s0.runStateHash = 33;
    s0.totalHash = engine::computeReplayStateHash(0, s0.bulletsHash, s0.entitiesHash, s0.runStateHash);
    rec.recordStateSample(s0);

    engine::ReplayStateSample s1;
    s1.tick = 30;
    s1.bulletsHash = 111;
    s1.entitiesHash = 222;
    s1.runStateHash = 333;
    s1.totalHash = engine::computeReplayStateHash(30, s1.bulletsHash, s1.entitiesHash, s1.runStateHash);
    rec.recordStateSample(s1);

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

    if (!player.validFor(777, engine::buildContentVersionTag("content.pak"), engine::buildContentHashTag("content.pak"))) {
        std::cerr << "replay header validation failed\n";
        return EXIT_FAILURE;
    }

    if (player.hashPeriodTicks() != 30) {
        std::cerr << "hash period mismatch\n";
        return EXIT_FAILURE;
    }

    if (player.inputForTick(0) != (engine::InputMoveLeft | engine::InputMoveUp)) {
        std::cerr << "tick0 input mismatch\n";
        return EXIT_FAILURE;
    }

    engine::ReplayMismatch mismatch;
    if (!player.verifyStateSample(s1, &mismatch)) {
        std::cerr << "state sample should match\n";
        return EXIT_FAILURE;
    }

    engine::ReplayStateSample bad = s1;
    bad.bulletsHash += 1;
    bad.totalHash = engine::computeReplayStateHash(bad.tick, bad.bulletsHash, bad.entitiesHash, bad.runStateHash);
    if (player.verifyStateSample(bad, &mismatch) || mismatch.subsystem != engine::ReplaySubsystem::Bullets) {
        std::cerr << "state mismatch classification failed\n";
        return EXIT_FAILURE;
    }


    if (engine::stableHash64("sim") != 0x82489E195CECBF94ULL || engine::stableHash64("gpu-bullets") != 0xD206C4205F96CCB1ULL) {
        std::cerr << "stableHash64 regression\n";
        return EXIT_FAILURE;
    }

    const auto h1 = engine::computeReplayStateHash(12, 9, 3, 145);
    const auto h2 = engine::computeReplayStateHash(12, 9, 3, 145);
    if (h1 != h2) {
        std::cerr << "hash function not deterministic\n";
        return EXIT_FAILURE;
    }

    std::filesystem::remove(path);
    std::cout << "replay_tests passed\n";
    return EXIT_SUCCESS;
}
