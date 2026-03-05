#include <engine/public/api.h>

#include <engine/config.h>
#include <engine/runtime.h>

namespace engine::public_api {

class EngineHost::Impl {
  public:
    int run(const EngineLaunchOptions& options) {
        EngineConfig cfg = loadConfigFromFile("engine_config.json");
        cfg.headless = options.headless;
        cfg.targetTicks = options.ticks;
        cfg.simulationSeed = options.seed;
        cfg.contentPackPath = options.contentPackPath;
        Runtime runtime(cfg);
        return runtime.run();
    }
};

EngineHost::EngineHost() : impl_(std::make_unique<Impl>()) {}
EngineHost::~EngineHost() = default;

int EngineHost::run(const EngineLaunchOptions& options) {
    return impl_->run(options);
}

ApiVersion publicApiVersion() {
    return ApiVersion {};
}

} // namespace engine::public_api
