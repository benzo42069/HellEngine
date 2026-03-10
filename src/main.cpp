#ifdef _WIN32
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <windows.h>
#include <SDL.h>

extern int __argc;
extern char** __argv;
#include <SDL.h>
#endif

#include <engine/config.h>
#include <engine/crash_handler.h>
#include <engine/diagnostics.h>
#include <engine/logging.h>
#include <engine/runtime.h>
#include <engine/version.h>

#include <filesystem>
#include <atomic>
#include <exception>
#include <filesystem>
#include <string>

namespace {

int runApplication(int argc, char** argv) {
#ifdef _WIN32
    static std::atomic_flag mainEntered = ATOMIC_FLAG_INIT;
    if (mainEntered.test_and_set()) {
        engine::logError("Detected recursive entry into main on Windows; aborting to prevent stack overflow.");
        return 2;
    }

    SDL_SetMainReady();
#endif
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

} // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SDL_SetMainReady();
#endif
    return runApplication(argc, argv);
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    SDL_SetMainReady();
    return runApplication(__argc, __argv);
}
#endif
