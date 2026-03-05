#include <engine/config.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

int main() {
    const char* path = "test_engine_config.json";
    {
        std::ofstream out(path);
        out << R"({
  "windowWidth": 1920,
  "windowHeight": 1080,
  "windowTitle": "ConfigTest",
  "fixedDeltaSeconds": 0.0083333333,
  "maxFrameSteps": 16,
  "simulationSeed": 42,
  "projectileCapacity": 12345,
  "projectileSpawnBurst": 64,
  "contentPackPath": "my_content.pak",
  "replayRecordPath": "record.json",
  "replayPlaybackPath": "playback.json"
})";
    }

    engine::EngineConfig config = engine::loadConfigFromFile(path);
    std::remove(path);

    if (config.windowWidth != 1920 || config.windowHeight != 1080) {
        std::cerr << "Window dimension parse failed\n";
        return EXIT_FAILURE;
    }

    if (config.windowTitle != "ConfigTest") {
        std::cerr << "Window title parse failed\n";
        return EXIT_FAILURE;
    }

    if (config.maxFrameSteps != 16 || config.simulationSeed != 42) {
        std::cerr << "Numeric parse failed\n";
        return EXIT_FAILURE;
    }

    if (config.projectileCapacity != 12345 || config.projectileSpawnBurst != 64 || config.contentPackPath != "my_content.pak"
        || config.replayRecordPath != "record.json" || config.replayPlaybackPath != "playback.json") {
        std::cerr << "Projectile config parse failed\n";
        return EXIT_FAILURE;
    }

    char appName[] = "EngineDemo";
    char headless[] = "--headless";
    char ticks[] = "--ticks";
    char ticksVal[] = "240";
    char seed[] = "--seed";
    char seedVal[] = "999";
    char dt[] = "--dt";
    char dtVal[] = "0.0166667";
    char projectiles[] = "--projectiles";
    char projectilesVal[] = "10000";
    char contentPack[] = "--content-pack";
    char contentPackVal[] = "override.pak";
    char stress[] = "--stress-10k";
    char replayRecord[] = "--replay-record";
    char replayRecordVal[] = "session_record.json";
    char replayPlayback[] = "--replay-playback";
    char replayPlaybackVal[] = "session_playback.json";
    char rendererSmoke[] = "--renderer-smoke-test";
    char replayVerify[] = "--replay-verify";
    char replayHashPeriod[] = "--replay-hash-period";
    char replayHashPeriodVal[] = "120";

    char* argv[] = {appName, headless, ticks, ticksVal, seed, seedVal, dt, dtVal, projectiles, projectilesVal, contentPack, contentPackVal, replayRecord, replayRecordVal, replayPlayback, replayPlaybackVal, stress, rendererSmoke, replayVerify, replayHashPeriod, replayHashPeriodVal};
    engine::applyCommandLineOverrides(config, 21, argv);

    if (!config.headless || config.targetTicks != 240 || config.simulationSeed != 999) {
        std::cerr << "Core command line override parse failed\n";
        return EXIT_FAILURE;
    }

    if (!config.stress10k || !config.rendererSmokeTest || !config.replayVerifyMode || config.replayHashPeriodTicks != 120 || config.projectileCapacity < 10000 || config.projectileSpawnBurst != 10000 || config.contentPackPath != "override.pak"
        || config.replayRecordPath != "session_record.json" || config.replayPlaybackPath != "session_playback.json") {
        std::cerr << "Stress/projectile command line override parse failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "config_tests passed\n";
    return EXIT_SUCCESS;
}
