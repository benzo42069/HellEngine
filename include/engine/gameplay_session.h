#pragma once

#include <engine/archetypes.h>
#include <engine/config.h>
#include <engine/defensive_special.h>
#include <engine/deterministic_rng.h>
#include <engine/difficulty_scaling.h>
#include <engine/editor_tools.h>
#include <engine/entities.h>
#include <engine/gpu_bullets.h>
#include <engine/input_system.h>
#include <engine/job_system.h>
#include <engine/memory.h>
#include <engine/meta_progression.h>
#include <engine/patterns.h>
#include <engine/perf_profiler.h>
#include <engine/projectiles.h>
#include <engine/replay.h>
#include <engine/run_structure.h>
#include <engine/traits.h>
#include <engine/upgrade_ui_model.h>

#include <SDL.h>

#include <array>
#include <cstdint>
#include <string>

namespace engine {

class GameplaySession {
  public:
    struct UpgradeCardAnimState {
        float appearT {0.0F};
        float hoverT {0.0F};
        float confirmT {0.0F};
    };

    explicit GameplaySession(EngineConfig& config);

    void initializeContent(PatternBank& patternBank, PatternPlayer& patternPlayer);
    void onUpgradeNavigation(UpgradeNavAction action);
    void updateGameplay(double dt, std::uint32_t inputMask);
    void drawUpgradeSelectionUi(double frameDelta);

    [[nodiscard]] Vec2 playerPos() const { return playerPos_; }
    [[nodiscard]] Vec2 aimTarget() const { return aimTarget_; }
    [[nodiscard]] float playerRadius() const { return playerRadius_; }
    [[nodiscard]] float playerHealth() const { return playerHealth_; }
    [[nodiscard]] bool upgradeScreenOpen() const { return upgradeScreenOpen_; }
    [[nodiscard]] bool perfHudOpen() const { return perfHudOpen_; }
    [[nodiscard]] bool debugHudOpen() const { return debugHudOpen_; }
    [[nodiscard]] bool archetypeSelectionOpen() const { return archetypeSelectionOpen_; }

    FrameAllocator frameAllocator_;
    JobSystem jobSystem_;
    RngStreams rngStreams_;
    std::uint64_t tickIndex_ {0};
    double simClock_ {0.0};

    ProjectileSystem projectiles_;
    PatternBank patternBank_;
    PatternPlayer patternPlayer_;
    EntityDatabase entityDatabase_;
    EntitySystem entitySystem_;
    TraitSystem traitSystem_;
    ArchetypeSystem archetypeSystem_;
    RunStructure runStructure_;
    MetaProgression metaProgression_;
    ReplayRecorder replayRecorder_;
    ReplayPlayer replayPlayer_;
    bool replayPlaybackMode_ {false};
    bool replayVerificationFailed_ {false};
    std::string replayContentVersion_;
    std::string replayContentHash_;
    bool useCompiledPatternGraph_ {false};
    BulletSimulationMode bulletSimMode_ {BulletSimulationMode::CpuDeterministic};
    GpuBulletSystem gpuBullets_ {};
    PerfProfiler profiler_ {};
    PatternGraphVm graphVm_ {};
    PatternGraphVm::RuntimeState graphVmState_ {};
    DefensiveSpecialSystem defensiveSpecial_ {};
    DifficultyModel difficultyModel_ {};
    ControlCenterToolSuite toolSuite_;
    std::string hotReloadErrorMessage_ {};
    float gpuUpdateMsFrame_ {0.0F};
    float gpuRenderMsFrame_ {0.0F};
    std::string difficultyProfileLabelCache_ {"Normal"};
    std::vector<CollisionTarget> collisionTargets_ {};
    std::vector<CollisionEvent> collisionEvents_ {};

  private:
    UpgradeViewStats buildCurrentViewStats() const;
    UpgradeViewStats buildProjectedViewStats(const Trait& trait) const;
    bool hasSynergyWithActive(const Trait& trait) const;

    EngineConfig& config_;
    Vec2 playerPos_ {0.0F, 0.0F};
    Vec2 aimTarget_ {160.0F, 0.0F};
    float playerRadius_ {12.0F};
    bool showHitboxes_ {true};
    bool showGrid_ {true};
    bool archetypeSelectionOpen_ {true};
    bool perfHudOpen_ {true};
    bool debugHudOpen_ {true};

    bool upgradeScreenOpen_ {false};
    std::size_t focusedUpgradeIndex_ {0};
    std::array<UpgradeCardAnimState, TraitSystem::choiceCount> cardAnim_ {};
    std::size_t zoneIndexMemo_ {0};
    std::size_t stageIndexMemo_ {0};

    float prevTotalCollisions_ {0};
    float prevHealthRecoveryAccum_ {0.0F};
    float collisionRateAccumulator_ {0.0F};
    float collisionRateWindowSeconds_ {0.0F};
    float playerHealth_ {100.0F};
};

} // namespace engine
