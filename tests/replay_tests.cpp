#include <engine/replay.h>
#include <engine/deterministic_rng.h>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

TEST_CASE("Replay record load and verify", "[replay]") {
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
    REQUIRE(rec.save(path));

    engine::ReplayPlayer player;
    REQUIRE(player.load(path));

    REQUIRE(player.validFor(777, engine::buildContentVersionTag("content.pak"), engine::buildContentHashTag("content.pak")));
    REQUIRE(player.hashPeriodTicks() == 30);
    REQUIRE(player.inputForTick(0) == (engine::InputMoveLeft | engine::InputMoveUp));

    engine::ReplayMismatch mismatch;
    REQUIRE(player.verifyStateSample(s1, &mismatch));

    engine::ReplayStateSample bad = s1;
    bad.bulletsHash += 1;
    bad.totalHash = engine::computeReplayStateHash(bad.tick, bad.bulletsHash, bad.entitiesHash, bad.runStateHash);
    REQUIRE_FALSE(player.verifyStateSample(bad, &mismatch));
    REQUIRE(mismatch.subsystem == engine::ReplaySubsystem::Bullets);

    REQUIRE(engine::stableHash64("sim") == 0x82489E195CECBF94ULL);
    REQUIRE(engine::stableHash64("gpu-bullets") == 0xD206C4205F96CCB1ULL);

    const auto h1 = engine::computeReplayStateHash(12, 9, 3, 145);
    const auto h2 = engine::computeReplayStateHash(12, 9, 3, 145);
    REQUIRE(h1 == h2);

    std::filesystem::remove(path);
}
