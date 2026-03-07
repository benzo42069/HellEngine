#pragma once

#include <engine/render2d.h>

#include <engine/bullet_palette.h>
#include <engine/bullet_sprite_gen.h>
#include <engine/gradient_animator.h>

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace engine {

enum class ProjectileAllegiance : std::uint8_t {
    Enemy = 0,
    Player = 1,
};

struct ProjectileBehavior {
    float homingTurnRateDegPerSec {0.0F};
    float homingMaxAngleStepDeg {0.0F};

    float curvedAngularVelocityDegPerSec {0.0F};

    float accelerationPerSec {0.0F};
    float dragPerSec {0.0F};

    std::uint8_t maxBounces {0};

    std::uint8_t splitCount {0};
    float splitAngleSpreadDeg {0.0F};
    float splitDelaySeconds {0.0F};

    float explodeRadius {0.0F};
    std::uint8_t explodeShards {0};

    std::uint8_t beamSegmentSamples {0};
    float beamDurationSeconds {0.0F};

    bool enableTrails {false};
};

struct ProjectileSpawn {
    Vec2 pos;
    Vec2 vel;
    float radius {4.0F};
    ProjectileBehavior behavior {};
    ProjectileAllegiance allegiance {ProjectileAllegiance::Enemy};
    std::uint8_t paletteIndex {0};
    BulletShape shape {BulletShape::Circle};
    bool enableTrails {false};
};

struct ProjectileStats {
    std::uint32_t activeCount {0};
    std::uint32_t collisionsThisTick {0};
    std::uint32_t totalCollisions {0};
    std::uint32_t spawnedThisTick {0};
    std::uint32_t broadphaseChecksThisTick {0};
    std::uint32_t narrowphaseChecksThisTick {0};
};

struct CollisionTarget {
    Vec2 pos;
    float radius {0.0F};
    std::uint32_t id {0};
    std::uint8_t team {0};
};

struct CollisionEvent {
    std::uint32_t bulletIndex {0};
    std::uint32_t targetId {0};
    bool graze {false};
};

struct ProjectileDespawnEvent {
    Vec2 pos;
    std::uint8_t paletteIndex {0};
    ProjectileAllegiance allegiance {ProjectileAllegiance::Enemy};
    std::uint8_t explodeShards {0};
};

class ProjectileSystem {
  public:
    static constexpr std::uint8_t kTrailLength = 4;

    void initialize(std::uint32_t capacity, float worldHalfExtent, std::uint32_t gridX, std::uint32_t gridY);
    void clear();

    bool spawn(const ProjectileSpawn& spawn);
    void spawnRadialBurst(std::uint32_t count, float speed, float radius, std::uint64_t seedOffset = 0);

    void beginTick();
    void updateMotion(float dt, float enemyTimeScale = 1.0F, float playerTimeScale = 1.0F);
    void buildGrid();
    void resolveCollisions(std::span<const CollisionTarget> targets, std::vector<CollisionEvent>& outEvents);
    void update(float dt, Vec2 playerPos, float playerRadius, float enemyTimeScale = 1.0F, float playerTimeScale = 1.0F);

    void debugDraw(DebugDraw& draw, bool drawHitboxes, bool drawGrid) const;
    void render(SpriteBatch& batch, const std::string& textureId, const BulletPaletteTable& paletteTable) const;
    void renderProcedural(SpriteBatch& batch, const BulletPaletteTable& paletteTable, float simClock) const;
    void configurePaletteAnimations(SDL_Renderer* renderer, TextureStore& store, const PaletteFxTemplateRegistry& registry);

    [[nodiscard]] const ProjectileStats& stats() const;
    [[nodiscard]] std::uint32_t capacity() const;
    [[nodiscard]] std::uint64_t debugStateHash() const;
    [[nodiscard]] std::uint32_t collectGrazePoints(Vec2 playerPos, float playerRadius, float innerPad, float outerPad, std::uint64_t tick, std::uint64_t cooldownTicks);
    [[nodiscard]] std::span<const ProjectileDespawnEvent> despawnEvents() const;

    [[nodiscard]] const std::vector<int>& gridHead() const { return gridHead_; }
    [[nodiscard]] const std::vector<int>& gridNext() const { return gridNext_; }
    [[nodiscard]] const std::vector<float>& posX() const { return posX_; }
    [[nodiscard]] const std::vector<float>& posY() const { return posY_; }
    [[nodiscard]] const std::vector<std::uint8_t>& active() const { return active_; }
    [[nodiscard]] std::uint32_t gridX() const { return gridX_; }
    [[nodiscard]] std::uint32_t gridY() const { return gridY_; }
    [[nodiscard]] float worldHalfExtent() const { return worldHalfExtent_; }

  private:
    std::uint32_t gridIndexFor(float x, float y) const;

    std::uint32_t capacity_ {0};
    float worldHalfExtent_ {400.0F};
    std::uint32_t gridX_ {32};
    std::uint32_t gridY_ {18};

    std::vector<float> posX_;
    std::vector<float> posY_;
    std::vector<float> velX_;
    std::vector<float> velY_;
    std::vector<float> radius_;
    std::vector<std::uint8_t> active_;

    std::vector<ProjectileBehavior> behavior_;
    std::vector<float> life_;
    std::vector<std::uint8_t> bounceCount_;
    std::vector<std::uint8_t> splitDone_;
    std::vector<std::uint8_t> allegiance_;
    std::vector<std::uint8_t> paletteIndex_;
    std::vector<std::uint8_t> shape_;
    std::vector<std::uint8_t> enableTrails_;
    std::vector<float> trailX_;
    std::vector<float> trailY_;
    std::vector<std::uint8_t> trailHead_;
    std::vector<std::uint64_t> grazeAwardTick_;

    GradientAnimator gradientAnimator_ {};
    std::array<std::uint8_t, BulletPaletteTable::kMaxPalettes> paletteAnimIds_ {};

    static constexpr std::uint32_t kMaxPendingSpawns = 4096;
    std::array<ProjectileSpawn, kMaxPendingSpawns> pendingSpawns_ {};
    std::uint32_t pendingSpawnCount_ {0};

    std::vector<ProjectileDespawnEvent> despawnEvents_ {};

    std::vector<std::uint32_t> freeList_;
    std::vector<std::uint32_t> activeIndices_;
    std::vector<std::uint32_t> indexInActive_;
    std::uint32_t activeCount_ {0};

    std::vector<int> gridHead_;
    std::vector<int> gridNext_;
    std::vector<std::uint32_t> collisionVisitedStamp_;
    std::vector<std::uint32_t> collisionHitStamp_;
    std::vector<std::uint32_t> collisionScratch_;
    std::uint32_t collisionStampCursor_ {1};

    ProjectileStats stats_ {};
};

} // namespace engine
