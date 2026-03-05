#pragma once

#include <engine/archetypes.h>
#include <engine/config.h>
#include <engine/deterministic_rng.h>
#include <engine/difficulty_scaling.h>
#include <engine/editor_tools.h>
#include <engine/gpu_bullets.h>
#include <engine/upgrade_ui_model.h>
#include <engine/entities.h>
#include <engine/job_system.h>
#include <engine/memory.h>
#include <engine/meta_progression.h>
#include <engine/patterns.h>
#include <engine/perf_profiler.h>
#include <engine/projectiles.h>
#include <engine/replay.h>
#include <engine/render2d.h>
#include <engine/run_structure.h>
#include <engine/traits.h>

#include <SDL.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace engine {

class Runtime {
  public:
    explicit Runtime(EngineConfig config);
    ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    int run();

  private:
    struct UpgradeCardAnimState {
        float appearT {0.0F};
        float hoverT {0.0F};
        float confirmT {0.0F};
    };

    void simTick(double dt);
    void renderFrame(double frameDelta);
    void buildSceneOverlay(double frameDelta);
    void drawUpgradeSelectionUi(double frameDelta);
    void handleUpgradeNavigation(const SDL_Event& event);
    UpgradeViewStats buildCurrentViewStats() const;
    UpgradeViewStats buildProjectedViewStats(const Trait& trait) const;
    bool hasSynergyWithActive(const Trait& trait) const;

    EngineConfig config_;
    bool running_ {true};

    SDL_Window* window_ {nullptr};
    SDL_Renderer* renderer_ {nullptr};

    FrameAllocator frameAllocator_;
    JobSystem jobSystem_;
    RngStreams rngStreams_;

    std::uint64_t tickIndex_ {0};
    double simClock_ {0.0};

    Camera2D camera_;
    std::unique_ptr<TextureStore> textures_;
    SpriteBatch spriteBatch_;
    DebugDraw debugDraw_;
    DebugText debugText_;

    ProjectileSystem projectiles_;
    PatternBank patternBank_;
    PatternPlayer patternPlayer_;

    EntityDatabase entityDatabase_;
    EntitySystem entitySystem_;
    TraitSystem traitSystem_;
    ArchetypeSystem archetypeSystem_;
    RunStructure runStructure_;
    MetaProgression metaProgression_;
    ControlCenterToolSuite toolSuite_;
    ReplayRecorder replayRecorder_;
    ReplayPlayer replayPlayer_;
    bool replayPlaybackMode_ {false};
    std::string replayContentVersion_;

    Vec2 playerPos_ {0.0F, 0.0F};
    Vec2 aimTarget_ {160.0F, 0.0F};
    float playerRadius_ {12.0F};
    bool showHitboxes_ {true};
    bool showGrid_ {true};
    bool archetypeSelectionOpen_ {true};
    bool perfHudOpen_ {true};
    bool useCompiledPatternGraph_ {false};
    BulletSimulationMode bulletSimMode_ {BulletSimulationMode::CpuDeterministic};
    GpuBulletSystem gpuBullets_ {};

    bool upgradeScreenOpen_ {false};
    std::size_t focusedUpgradeIndex_ {0};
    std::array<UpgradeCardAnimState, TraitSystem::choiceCount> cardAnim_ {};
    std::size_t zoneIndexMemo_ {0};
    std::size_t stageIndexMemo_ {0};

    PerfProfiler profiler_ {};
    PatternGraphVm graphVm_ {};
    PatternGraphVm::RuntimeState graphVmState_ {};

    float playerHealth_ {100.0F};
    std::uint32_t prevTotalCollisions_ {0};
    float prevHealthRecoveryAccum_ {0.0F};
    DifficultyModel difficultyModel_ {};
    float collisionRateAccumulator_ {0.0F};
    float collisionRateWindowSeconds_ {0.0F};
    std::string difficultyProfileLabelCache_ {"Normal"};
    float gpuUpdateMsFrame_ {0.0F};
    float gpuRenderMsFrame_ {0.0F};
    std::uint32_t currentInputMask_ {0};
};

} // namespace engine
