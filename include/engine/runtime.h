#pragma once

#include <engine/archetypes.h>
#include <engine/config.h>
#include <engine/deterministic_rng.h>
#include <engine/editor_tools.h>
#include <engine/entities.h>
#include <engine/job_system.h>
#include <engine/memory.h>
#include <engine/meta_progression.h>
#include <engine/patterns.h>
#include <engine/projectiles.h>
#include <engine/replay.h>
#include <engine/render2d.h>
#include <engine/run_structure.h>
#include <engine/traits.h>

#include <SDL.h>

#include <cstdint>
#include <memory>

namespace engine {

class Runtime {
  public:
    explicit Runtime(EngineConfig config);
    ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    int run();

  private:
    void simTick(double dt);
    void renderFrame(double frameDelta);
    void buildSceneOverlay(double frameDelta);

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

    float playerHealth_ {100.0F};
    std::uint32_t prevTotalCollisions_ {0};
    float prevHealthRecoveryAccum_ {0.0F};
    std::uint32_t currentInputMask_ {0};
};

} // namespace engine
