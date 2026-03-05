#include <engine/config.h>
#include <engine/crash_handler.h>
#include <engine/diagnostics.h>
#include <engine/logging.h>
#include <engine/runtime.h>
#include <engine/version.h>

#include <filesystem>
#include <exception>
#include <string>

int main(int argc, char** argv) {
    engine::Logger::instance().setLevel(engine::LogLevel::Info);

    engine::EngineConfig config = engine::loadConfigFromFile("engine_config.json");
    engine::applyCommandLineOverrides(config, argc, argv);

    const std::string buildStamp = std::string(engine::kEngineBuildStamp);
    engine::installCrashHandlers(buildStamp);
    engine::logInfo("Engine build stamp: " + buildStamp);
    if (!config.windowTitle.empty()) {
        config.windowTitle += " [" + buildStamp + "]";
    }

    try {
        if (config.replayVerifyMode) {
            const std::string tempReplayPath = "replay_verify_temp.json";

            engine::EngineConfig firstRun = config;
            firstRun.headless = true;
            if (firstRun.targetTicks <= 0) firstRun.targetTicks = 1200;
            firstRun.replayRecordPath = tempReplayPath;
            firstRun.replayPlaybackPath.clear();

            engine::logInfo("Replay verify: recording baseline run...");
            engine::Runtime baseline(firstRun);
            const int rcBaseline = baseline.run();
            if (rcBaseline != 0) {
                engine::logError("Replay verify failed during baseline run.");
                return rcBaseline;
            }

            engine::EngineConfig secondRun = config;
            secondRun.headless = true;
            secondRun.targetTicks = firstRun.targetTicks;
            secondRun.replayRecordPath.clear();
            secondRun.replayPlaybackPath = tempReplayPath;

            engine::logInfo("Replay verify: running playback verification...");
            engine::Runtime playback(secondRun);
            const int rcPlayback = playback.run();
            std::filesystem::remove(tempReplayPath);

            if (rcPlayback != 0) {
                engine::logError("Replay verify divergence detected.");
                return rcPlayback;
            }

            engine::logInfo("Replay verify passed with no divergence.");
            return 0;
        }

        engine::Runtime runtime(config);
        return runtime.run();
    } catch (const std::exception& ex) {
        engine::ErrorReport err = engine::makeError(engine::ErrorCode::RuntimeInitFailed, std::string("Fatal runtime error: ") + ex.what());
        engine::pushStack(err, "main");
        engine::logErrorReport(err);
        return 1;
    }
}
