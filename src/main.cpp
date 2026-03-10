#ifdef _WIN32
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <windows.h>
#include <shellapi.h>
#include <SDL.h>
#endif

#include <engine/config.h>
#include <engine/crash_handler.h>
#include <engine/diagnostics.h>
#include <engine/logging.h>
#include <engine/runtime.h>
#include <engine/version.h>

#include <atomic>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

namespace {

#ifdef _WIN32
void enableProcessDpiAwareness() {
    using SetDpiContextFn = BOOL(WINAPI*)(HANDLE);
    if (HMODULE user32 = LoadLibraryW(L"user32.dll")) {
        if (auto setContext = reinterpret_cast<SetDpiContextFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"))) {
            (void)setContext(reinterpret_cast<HANDLE>(-4)); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
            FreeLibrary(user32);
            return;
        }
        FreeLibrary(user32);
    }
    (void)SetProcessDPIAware();
}

std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    const int needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (needed <= 0) return {};
    std::string out(static_cast<std::size_t>(needed), '\0');
    (void)WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), out.data(), needed, nullptr, nullptr);
    return out;
}
#endif

int runApplication(int argc, char** argv) {
#ifdef _WIN32
    static std::atomic_flag entryInProgress = ATOMIC_FLAG_INIT;
    if (entryInProgress.test_and_set()) {
        engine::logError("Detected recursive process entry on Windows; aborting to prevent stack overflow.");
        return 2;
    }

    enableProcessDpiAwareness();
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
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    int argc = 0;
    LPWSTR* argvWide = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argvWide || argc <= 0) {
        return runApplication(0, nullptr);
    }

    std::vector<std::string> utf8Storage;
    utf8Storage.reserve(static_cast<std::size_t>(argc));
    std::vector<char*> argv;
    argv.reserve(static_cast<std::size_t>(argc));

    for (int i = 0; i < argc; ++i) {
        utf8Storage.push_back(wideToUtf8(argvWide[i] ? argvWide[i] : L""));
    }
    for (std::string& arg : utf8Storage) {
        argv.push_back(arg.data());
    }

    LocalFree(argvWide);
    return runApplication(argc, argv.data());
}
#endif
