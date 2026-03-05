#pragma once

#include <engine/run_structure.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace engine {

enum class DifficultyProfile {
    Normal,
    Hard,
    Nightmare,
    Endless,
};

struct DifficultyScalars {
    float overall {1.0F};
    float patternSpeed {1.0F};
    float spawnRate {1.0F};
    float enemyHp {1.0F};
};

struct DeterministicPerformanceMetrics {
    float playerHealth01 {1.0F};
    float collisionsPerMinute {0.0F};
    std::uint32_t defeatedBosses {0};
    std::uint64_t tick {0};
};

class DifficultyModel {
  public:
    void initializeDefaults();
    bool loadProfilesFromFile(const std::string& path);
    void setProfile(DifficultyProfile profile);
    [[nodiscard]] DifficultyProfile profile() const;
    [[nodiscard]] std::string profileLabel() const;

    void setStageZone(std::size_t stageIndex, ZoneType zoneType);
    void update(float dt, const DeterministicPerformanceMetrics& metrics);

    [[nodiscard]] float currentDifficulty() const;
    [[nodiscard]] const DifficultyScalars& scalars() const;

  private:
    struct ProfileConfig {
        float baseDifficulty {1.0F};
        float stageRamp {0.1F};
        std::array<float, 4> zoneAdd {{0.0F, 0.1F, -0.05F, 0.25F}};
        bool performanceEnabled {true};
        float perfHealthWeight {0.2F};
        float perfCollisionWeight {0.03F};
        float perfBossWeight {0.1F};
        float perfMin {-0.2F};
        float perfMax {0.2F};
        float endlessGrowthPerMinute {0.0F};
        float difficultyMin {0.5F};
        float difficultyMax {4.0F};
        float patternSpeedFactor {0.30F};
        float spawnRateFactor {0.40F};
        float enemyHpFactor {0.55F};
    };

    std::array<ProfileConfig, 4> profiles_ {};
    DifficultyProfile activeProfile_ {DifficultyProfile::Normal};
    std::size_t stageIndex_ {0};
    ZoneType zoneType_ {ZoneType::Combat};
    float elapsedSeconds_ {0.0F};
    float currentDifficulty_ {1.0F};
    DifficultyScalars scalars_ {};
};

std::string toString(DifficultyProfile profile);
DifficultyProfile difficultyProfileFromString(const std::string& s);

} // namespace engine
