#pragma once

#include <engine/deterministic_rng.h>
#include <engine/patterns.h>
#include <engine/projectiles.h>
#include <engine/render2d.h>

#include <cstdint>
#include <string>
#include <vector>

namespace engine {

enum class EntityType {
    Player,
    Enemy,
    Boss,
    ResourceNode,
    Hazard,
};

enum class MovementBehavior {
    Static,
    Linear,
    Sine,
    Chase,
};

struct SpawnRule {
    bool enabled {false};
    float initialDelaySeconds {0.0F};
    float intervalSeconds {1.0F};
    std::uint32_t maxAlive {1};
};

struct ResourceYield {
    float upgradeCurrency {0.0F};
    float healthRecovery {0.0F};
    float buffDurationSeconds {0.0F};
};

struct BossPhase {
    std::string attackPatternName;
    MovementBehavior movement {MovementBehavior::Static};
    float durationSeconds {4.0F};
    float difficultyScale {1.0F};
};

struct BossDefinition {
    bool enabled {false};
    float introDurationSeconds {1.0F};
    std::vector<BossPhase> phases;
    ResourceYield rewardDrop {};
};

struct EntityTemplate {
    std::string name;
    EntityType type {EntityType::Enemy};
    MovementBehavior movement {MovementBehavior::Static};

    Vec2 spawnPosition {0.0F, 0.0F};
    Vec2 baseVelocity {0.0F, 0.0F};
    float sineAmplitude {0.0F};
    float sineFrequencyHz {1.0F};

    float maxHealth {10.0F};
    float contactDamage {1.0F};

    bool attacksEnabled {false};
    std::string attackPatternName;
    float attackIntervalSeconds {0.5F};

    float interactionRadius {16.0F};
    ResourceYield resourceYield {};

    BossDefinition boss {};

    SpawnRule spawnRule {};
};

class EntityDatabase {
  public:
    bool loadFromFile(const std::string& path);
    void loadFallbackDefaults();

    [[nodiscard]] const std::vector<EntityTemplate>& templates() const;

  private:
    std::vector<EntityTemplate> templates_;
};

struct EntityRuntimeModifiers {
    float enemyFireRateScale {1.0F};
    float enemyProjectileSpeedScale {1.0F};
    float harvestYieldMultiplier {1.0F};
};

struct EntityStats {
    std::uint32_t aliveTotal {0};
    std::uint32_t aliveEnemies {0};
    std::uint32_t aliveBosses {0};
    std::uint32_t aliveResources {0};
    std::uint32_t aliveHazards {0};

    std::uint32_t harvestedNodes {0};
    std::uint32_t defeatedBosses {0};
    std::uint32_t bossPhaseTransitions {0};
    float upgradeCurrency {0.0F};
    float healthRecoveryAccum {0.0F};
    float buffTimeRemaining {0.0F};
};

class EntitySystem {
  public:
    void setTemplates(const std::vector<EntityTemplate>* templates);
    void setPatternBank(const PatternBank* bank);
    void setRunSeed(std::uint64_t seed);

    void reset();
    void update(float dt, ProjectileSystem& projectiles, Vec2 playerPos, const EntityRuntimeModifiers& runtimeMods = {});
    void debugDraw(DebugDraw& draw) const;

    [[nodiscard]] const EntityStats& stats() const;

  private:
    struct EntityInstance {
      std::size_t templateIndex {0};
      Vec2 position {0.0F, 0.0F};
      float ageSeconds {0.0F};
      float health {0.0F};
      float fireCooldown {0.0F};
      std::size_t activeBossPhase {0};
      float phaseTimeRemaining {0.0F};
      bool introPlaying {false};
      bool alive {true};
    };

    struct SpawnerState {
      float timer {0.0F};
      bool initialized {false};
    };

    void spawnEntity(std::size_t templateIndex);
    void applyHarvest(const EntityTemplate& t, const EntityRuntimeModifiers& runtimeMods);
    void applyRewardDrop(const ResourceYield& reward, const EntityRuntimeModifiers& runtimeMods);
    void emitPatternFromTemplate(const EntityTemplate& t, const std::string& patternOverride, Vec2 origin, Vec2 playerPos, ProjectileSystem& projectiles, const EntityRuntimeModifiers& runtimeMods, float difficultyScale);

    const std::vector<EntityTemplate>* templates_ {nullptr};
    const PatternBank* patternBank_ {nullptr};

    std::vector<EntityInstance> entities_;
    std::vector<SpawnerState> spawnerState_;

    DeterministicRng rng_ {1337};
    EntityStats stats_ {};
};

std::string toString(EntityType type);

} // namespace engine
