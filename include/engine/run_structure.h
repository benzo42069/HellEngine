#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace engine {

enum class ZoneType {
    Combat,
    Elite,
    Event,
    Boss,
};

enum class RunState {
    InProgress,
    Completed,
    Failed,
};

struct ZoneDefinition {
    ZoneType type {ZoneType::Combat};
    float durationSeconds {8.0F};
};

struct StageDefinition {
    std::string name;
    std::vector<ZoneDefinition> zones;
};

class RunStructure {
  public:
    void initializeDefaults();
    void setStages(std::vector<StageDefinition> stages);
    void reset();
    void update(float dt, std::uint32_t defeatedBossesTotal, bool playerAlive);

    [[nodiscard]] RunState state() const;
    [[nodiscard]] std::size_t stageIndex() const;
    [[nodiscard]] std::size_t zoneIndex() const;
    [[nodiscard]] float zoneTimeRemaining() const;
    [[nodiscard]] std::size_t stageCount() const;
    [[nodiscard]] const std::vector<StageDefinition>& stages() const;
    [[nodiscard]] const StageDefinition* currentStage() const;
    [[nodiscard]] const ZoneDefinition* currentZone() const;

  private:
    void advanceZone(std::uint32_t defeatedBossesTotal);

    std::vector<StageDefinition> stages_;
    RunState state_ {RunState::InProgress};
    std::size_t stageIndex_ {0};
    std::size_t zoneIndex_ {0};
    float zoneTimeRemaining_ {0.0F};
    std::uint32_t bossDefeatsAtZoneStart_ {0};
};

std::string toString(ZoneType type);
std::string toString(RunState state);

} // namespace engine
