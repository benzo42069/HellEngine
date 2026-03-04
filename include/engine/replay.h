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
};

struct ReplayHeader {
    std::uint64_t simulationSeed {0};
    std::string contentVersion;
};

struct ReplayData {
    ReplayHeader header;
    std::vector<std::uint32_t> inputsPerTick;
    std::vector<std::uint64_t> stateHashes;
};

class ReplayRecorder {
  public:
    void begin(std::uint64_t simulationSeed, std::string contentVersion);
    void recordTick(std::uint32_t inputMask, std::uint64_t stateHash);
    [[nodiscard]] const ReplayData& data() const;
    bool save(const std::string& path) const;

  private:
    ReplayData data_;
};

class ReplayPlayer {
  public:
    bool load(const std::string& path);
    [[nodiscard]] bool validFor(std::uint64_t simulationSeed, const std::string& contentVersion) const;
    [[nodiscard]] std::uint32_t inputForTick(std::uint64_t tick) const;
    [[nodiscard]] bool verifyTick(std::uint64_t tick, std::uint64_t stateHash) const;

  private:
    ReplayData data_;
};

std::string buildContentVersionTag(const std::string& contentPackPath);
std::uint64_t computeReplayStateHash(std::uint64_t tick, std::uint32_t projectiles, std::uint32_t entities, std::uint32_t collisions, float currency);

} // namespace engine
