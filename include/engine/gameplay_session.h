#pragma once

#include <engine/archetypes.h>
#include <engine/bullet_palette.h>
#include <engine/config.h>
#include <engine/danger_field.h>
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
#include <engine/particle_fx.h>
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
#include <vector>

namespace engine {

struct SessionSimulationState {
    FrameAllocator frameAllocator {1024 * 1024};
    JobSystem jobSystem {};
    RngStreams rngStreams {};
    std::uint64_t tickIndex {0};
    double simClock {0.0};

    explicit SessionSimulationState(const std::uint64_t seed)
        : rngStreams(seed) {}
};

struct PlayerCombatState {
    Vec2 playerPos {0.0F, 0.0F};
    Vec2 aimTarget {160.0F, 0.0F};
    float playerRadius {12.0F};
    float playerHealth {100.0F};
};

struct ProgressionState {
    bool archetypeSelectionOpen {true};
    bool upgradeScreenOpen {false};
    std::size_t focusedUpgradeIndex {0};
    std::size_t zoneIndexMemo {0};
    std::size_t stageIndexMemo {0};
};

struct PresentationState {
    mutable DangerFieldOverlay dangerField {};
    ParticleFxSystem particleFx {};
    mutable std::vector<ShakeParams> cameraShakeEvents {};
    bool dangerFieldEnabled {false};

    PresentationState() { cameraShakeEvents.reserve(16); }
};

struct DebugToolState {
    bool showHitboxes {true};
    bool showGrid {true};
    bool perfHudOpen {true};
    bool debugHudOpen {true};
    ControlCenterToolSuite toolSuite {};
    std::string hotReloadErrorMessage {};
    float gpuUpdateMsFrame {0.0F};
    float gpuRenderMsFrame {0.0F};
    std::string difficultyProfileLabelCache {"Normal"};
};

struct EncounterRuntimeState {
    static constexpr std::uint32_t kMaxCollisionTargets = 512;
    static constexpr std::uint32_t kMaxCollisionEvents = 8192;
    std::array<CollisionTarget, kMaxCollisionTargets> collisionTargets {};
    std::uint32_t collisionTargetCount {0};
    std::array<CollisionEvent, kMaxCollisionEvents> collisionEvents {};
    std::uint32_t collisionEventCount {0};
    float prevTotalCollisions {0};
    float prevHealthRecoveryAccum {0.0F};
    float collisionRateAccumulator {0.0F};
    float collisionRateWindowSeconds {0.0F};
};

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
    void renderDangerFieldOverlay(SDL_Renderer* renderer, const Camera2D& camera, float opacity = 0.25F) const;
    [[nodiscard]] std::vector<ShakeParams> consumeCameraShakeEvents() const;

    [[nodiscard]] Vec2 playerPos() const { return playerState_.playerPos; }
    [[nodiscard]] Vec2 aimTarget() const { return playerState_.aimTarget; }
    [[nodiscard]] float playerRadius() const { return playerState_.playerRadius; }
    [[nodiscard]] float playerHealth() const { return playerState_.playerHealth; }
    [[nodiscard]] bool upgradeScreenOpen() const { return progression_.upgradeScreenOpen; }
    [[nodiscard]] bool perfHudOpen() const { return debugTools_.perfHudOpen; }
    [[nodiscard]] bool debugHudOpen() const { return debugTools_.debugHudOpen; }
    [[nodiscard]] bool archetypeSelectionOpen() const { return progression_.archetypeSelectionOpen; }
    [[nodiscard]] bool dangerFieldEnabled() const { return presentation_.dangerFieldEnabled; }
    [[nodiscard]] std::uint64_t tickIndex() const { return simulation_.tickIndex; }
    [[nodiscard]] bool replayPlaybackMode() const { return replayPlaybackMode_; }
    [[nodiscard]] bool replayVerificationFailed() const { return replayVerificationFailed_; }
    void setReplayVerificationFailed(bool failed) { replayVerificationFailed_ = failed; }
    void setDangerFieldEnabled(bool enabled) { presentation_.dangerFieldEnabled = enabled; }
    void toggleDangerField() { presentation_.dangerFieldEnabled = !presentation_.dangerFieldEnabled; }

    SessionSimulationState simulation_;
    PlayerCombatState playerState_ {};
    ProgressionState progression_ {};
    PresentationState presentation_ {};
    DebugToolState debugTools_ {};
    EncounterRuntimeState encounter_ {};

    ProjectileSystem projectiles_;
    PaletteFxTemplateRegistry bulletPaletteRegistry_ {};
    BulletPaletteTable bulletPaletteTable_ {};
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
    std::array<UpgradeCardAnimState, TraitSystem::choiceCount> cardAnim_ {};

  private:
    UpgradeViewStats buildCurrentViewStats() const;
    UpgradeViewStats buildProjectedViewStats(const Trait& trait) const;
    bool hasSynergyWithActive(const Trait& trait) const;

    EngineConfig& config_;
};

} // namespace engine
