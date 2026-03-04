#include <engine/config.h>
#include <engine/logging.h>
#include <engine/runtime.h>

#include <exception>

int main(int argc, char** argv) {
    engine::Logger::instance().setLevel(engine::LogLevel::Info);

    engine::EngineConfig config = engine::loadConfigFromFile("engine_config.json");
    engine::applyCommandLineOverrides(config, argc, argv);

    try {
        engine::Runtime runtime(config);
        return runtime.run();
    } catch (const std::exception& ex) {
        engine::logError(std::string("Fatal runtime error: ") + ex.what());
        return 1;
    }
}
