#pragma once

#include <engine/render2d.h>

#include <SDL.h>

#include <cstdint>
#include <vector>

namespace engine {

enum class BulletSimulationMode {
    CpuDeterministic,
    GpuMassHybrid,
};

struct GpuBullet {
    float posX {0.0F};
    float posY {0.0F};
    float velX {0.0F};
    float velY {0.0F};
    float lifetime {0.0F};
    float radius {2.0F};
    std::uint32_t colorRgba {0xFFFFFFFFU};
    std::uint32_t flags {0U}; // bit0: active, bit1: angular velocity enabled
    float angularVelocityDegPerSec {0.0F};
};

class GpuBulletSystem {
  public:
    void initialize(std::uint32_t capacity, float worldHalfExtent);
    void clear();

    bool emit(const GpuBullet& bullet);
    void update(float dt);
    void render(SDL_Renderer* renderer, const Camera2D& camera, const TextureResource* texture);

    [[nodiscard]] std::uint32_t activeCount() const;
    [[nodiscard]] std::uint32_t capacity() const;

  private:
    std::uint32_t capacity_ {0};
    float worldHalfExtent_ {400.0F};
    std::vector<GpuBullet> bullets_ {};
    std::vector<SDL_Vertex> vertices_ {};
    std::vector<int> indices_ {};
    std::vector<std::uint32_t> freeList_ {};
    std::uint32_t activeCount_ {0};
};

} // namespace engine
