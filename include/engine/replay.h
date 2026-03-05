#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace engine {

enum ReplayInputBits : std::uint32_t {
    InputMoveLeft = 1U << 0,
    InputMoveRight = 1U << 1,
    InputMoveUp = 1U << 2,
    InputMoveDown = 1U << 3,
    InputDefensiveSpecial = 1U << 4,
};

enum class ReplaySubsystem {
    None,
    Bullets,
    Entities,
    RunState,
};

struct ReplayHeader {
    std::uint64_t simulationSeed {0};
    std::string contentVersion;
    std::string contentHash;
    std::uint32_t hashPeriodTicks {60};
};

struct ReplayStateSample {
    std::uint64_t tick {0};
    std::uint64_t totalHash {0};
    std::uint64_t bulletsHash {0};
    std::uint64_t entitiesHash {0};
    std::uint64_t runStateHash {0};
};

struct ReplayMismatch {
    std::uint64_t tick {0};
    ReplaySubsystem subsystem {ReplaySubsystem::None};
    std::uint64_t expected {0};
    std::uint64_t actual {0};
};

struct ReplayData {
    ReplayHeader header;
    std::vector<std::uint32_t> inputsPerTick;
    std::vector<ReplayStateSample> stateSamples;
};

class ReplayRecorder {
  public:
    void begin(std::uint64_t simulationSeed, std::string contentVersion, std::string contentHash, std::uint32_t hashPeriodTicks);
    void recordTickInput(std::uint32_t inputMask);
    void recordStateSample(const ReplayStateSample& sample);
    [[nodiscard]] const ReplayData& data() const;
    bool save(const std::string& path) const;

  private:
    ReplayData data_;
};

class ReplayPlayer {
  public:
    bool load(const std::string& path);
    [[nodiscard]] bool validFor(std::uint64_t simulationSeed, const std::string& contentVersion, const std::string& contentHash) const;
    [[nodiscard]] std::uint32_t inputForTick(std::uint64_t tick) const;
    [[nodiscard]] bool verifyStateSample(const ReplayStateSample& sample, ReplayMismatch* mismatch) const;
    [[nodiscard]] std::uint32_t hashPeriodTicks() const;

  private:
    ReplayData data_;
};

std::string buildContentVersionTag(const std::string& contentPackPath);
std::string buildContentHashTag(const std::string& contentPackPath);
std::uint64_t computeReplayStateHash(std::uint64_t tick, std::uint64_t bulletsHash, std::uint64_t entitiesHash, std::uint64_t runStateHash);
const char* toString(ReplaySubsystem subsystem);

} // namespace engine
