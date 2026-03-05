#pragma once

#include <engine/public/versioning.h>

#include <cstdint>
#include <memory>
#include <string>

namespace engine::public_api {

struct EngineLaunchOptions {
    bool headless {false};
    int ticks {0};
    std::uint64_t seed {1337};
    std::string contentPackPath {"content.pak"};
};

class EngineHost {
  public:
    EngineHost();
    ~EngineHost();

    EngineHost(const EngineHost&) = delete;
    EngineHost& operator=(const EngineHost&) = delete;

    int run(const EngineLaunchOptions& options);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

ApiVersion publicApiVersion();

} // namespace engine::public_api
