#pragma once

#include <engine/render2d.h>

#include <cstdint>
#include <vector>

namespace engine {

struct ProjectileSpawn {
    Vec2 pos;
    Vec2 vel;
    float radius {4.0F};
};

struct ProjectileStats {
    std::uint32_t activeCount {0};
    std::uint32_t collisionsThisTick {0};
    std::uint32_t totalCollisions {0};
};

class ProjectileSystem {
  public:
    void initialize(std::uint32_t capacity, float worldHalfExtent, std::uint32_t gridX, std::uint32_t gridY);
    void clear();

    bool spawn(const ProjectileSpawn& spawn);
    void spawnRadialBurst(std::uint32_t count, float speed, float radius, std::uint64_t seedOffset = 0);

    void update(float dt, Vec2 playerPos, float playerRadius);

    void debugDraw(DebugDraw& draw, bool drawHitboxes, bool drawGrid) const;
    void render(SpriteBatch& batch, const std::string& textureId) const;

    [[nodiscard]] const ProjectileStats& stats() const;
    [[nodiscard]] std::uint32_t capacity() const;

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

    std::vector<std::uint32_t> freeList_;

    std::vector<int> gridHead_;
    std::vector<int> gridNext_;

    ProjectileStats stats_ {};
};

} // namespace engine
