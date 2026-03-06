#pragma once

#include <engine/render2d.h>

#include <cstdint>
#include <string>
#include <vector>

namespace engine {

class ParticleFxSystem {
  public:
    void initialize(std::uint32_t capacity);
    void burst(Vec2 pos, Color color, std::uint8_t count = 4, float speed = 80.0F);
    void update(float frameDelta);
    void render(SpriteBatch& batch, const std::string& textureId) const;
    void clear();

  private:
    struct Particle {
        float x {0.0F};
        float y {0.0F};
        float vx {0.0F};
        float vy {0.0F};
        float life {0.0F};
        float maxLife {0.0F};
        Color color {};
        std::uint32_t spawnId {0};
    };

    std::vector<Particle> particles_;
    std::uint32_t activeCount_ {0};
    std::uint32_t capacity_ {0};
    std::uint32_t spawnCursor_ {0};
};

} // namespace engine
