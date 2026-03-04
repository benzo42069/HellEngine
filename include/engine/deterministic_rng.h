#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace engine {

class DeterministicRng {
  public:
    explicit DeterministicRng(std::uint64_t seed = 0);

    std::uint64_t nextU64();
    float nextFloat01();

  private:
    std::uint64_t state_;
};

class RngStreams {
  public:
    explicit RngStreams(std::uint64_t runSeed);

    DeterministicRng& stream(const std::string& name);

  private:
    std::uint64_t runSeed_;
    std::unordered_map<std::string, DeterministicRng> streams_;
};

} // namespace engine
