#include <engine/config.h>

#include <engine/logging.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <string>

namespace engine {

EngineConfig loadConfigFromFile(const std::string& filePath) {
    EngineConfig config;

    std::ifstream input(filePath);
    if (!input.good()) {
        logWarn("Config file not found, using defaults: " + filePath);
        return config;
    }

    nlohmann::json json;
    input >> json;

    if (json.contains("windowWidth")) config.windowWidth = json["windowWidth"].get<int>();
    if (json.contains("windowHeight")) config.windowHeight = json["windowHeight"].get<int>();
    if (json.contains("windowTitle")) config.windowTitle = json["windowTitle"].get<std::string>();
    if (json.contains("fixedDeltaSeconds")) config.fixedDeltaSeconds = json["fixedDeltaSeconds"].get<double>();
    if (json.contains("maxFrameSteps")) config.maxFrameSteps = json["maxFrameSteps"].get<std::uint32_t>();
    if (json.contains("simulationSeed")) config.simulationSeed = json["simulationSeed"].get<std::uint64_t>();
    if (json.contains("projectileCapacity")) config.projectileCapacity = json["projectileCapacity"].get<std::uint32_t>();
    if (json.contains("projectileSpawnBurst")) config.projectileSpawnBurst = json["projectileSpawnBurst"].get<std::uint32_t>();
    if (json.contains("contentPackPath")) config.contentPackPath = json["contentPackPath"].get<std::string>();
    if (json.contains("replayRecordPath")) config.replayRecordPath = json["replayRecordPath"].get<std::string>();
    if (json.contains("replayPlaybackPath")) config.replayPlaybackPath = json["replayPlaybackPath"].get<std::string>();
    if (json.contains("replayVerifyMode")) config.replayVerifyMode = json["replayVerifyMode"].get<bool>();
    if (json.contains("replayHashPeriodTicks")) config.replayHashPeriodTicks = json["replayHashPeriodTicks"].get<std::uint32_t>();
    if (json.contains("difficultyProfile")) config.difficultyProfile = json["difficultyProfile"].get<std::string>();

    return config;
}

void applyCommandLineOverrides(EngineConfig& config, const int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--headless") {
            config.headless = true;
            continue;
        }

        if (arg == "--renderer-smoke-test") {
            config.rendererSmokeTest = true;
            continue;
        }

        if (arg == "--ticks" && i + 1 < argc) {
            config.targetTicks = std::stoi(argv[++i]);
            continue;
        }

        if (arg == "--seed" && i + 1 < argc) {
            config.simulationSeed = static_cast<std::uint64_t>(std::stoull(argv[++i]));
            continue;
        }

        if (arg == "--dt" && i + 1 < argc) {
            config.fixedDeltaSeconds = std::stod(argv[++i]);
            continue;
        }

        if (arg == "--projectiles" && i + 1 < argc) {
            config.projectileCapacity = static_cast<std::uint32_t>(std::stoul(argv[++i]));
            continue;
        }

        if (arg == "--spawn-burst" && i + 1 < argc) {
            config.projectileSpawnBurst = static_cast<std::uint32_t>(std::stoul(argv[++i]));
            continue;
        }

        if (arg == "--content-pack" && i + 1 < argc) {
            config.contentPackPath = argv[++i];
            continue;
        }

        if (arg == "--replay-record" && i + 1 < argc) {
            config.replayRecordPath = argv[++i];
            continue;
        }

        if (arg == "--replay-playback" && i + 1 < argc) {
            config.replayPlaybackPath = argv[++i];
            continue;
        }

        if (arg == "--replay-verify") {
            config.replayVerifyMode = true;
            continue;
        }

        if (arg == "--replay-hash-period" && i + 1 < argc) {
            config.replayHashPeriodTicks = static_cast<std::uint32_t>(std::stoul(argv[++i]));
            continue;
        }

        if (arg == "--difficulty" && i + 1 < argc) {
            config.difficultyProfile = argv[++i];
            continue;
        }

        if (arg == "--stress-10k") {
            config.stress10k = true;
            config.projectileCapacity = std::max(config.projectileCapacity, 10000U);
            config.projectileSpawnBurst = 10000;
            continue;
        }
    }
}

} // namespace engine
