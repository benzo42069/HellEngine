#pragma once

#include <engine/bullet_sprite_gen.h>
#include <engine/bullet_palette.h>
#include <engine/gradient_animator.h>
#include <engine/palette_ramp.h>
#include <engine/render2d.h>
#include <engine/shader_cache.h>
#include <engine/sprite_atlas_gen.h>

#include <glad/glad.h>

#include <cstdint>
#include <string>
#include <vector>

namespace engine {

class GlBulletRenderer {
  public:
    bool initialize(ShaderCache& shaders, std::uint32_t maxBullets, std::string* error = nullptr);
    void shutdown();
    [[nodiscard]] bool initialized() const { return program_ != nullptr && vao_ != 0 && vbo_ != 0 && ebo_ != 0; }

    void buildVertexBuffer(
        const float* posX,
        const float* posY,
        const float* velX,
        const float* velY,
        const float* radius,
        const float* life,
        const std::uint8_t* paletteIndex,
        const std::uint8_t* shape,
        const std::uint8_t* active,
        const std::uint8_t* allegiance,
        const std::uint8_t* enableTrails,
        const float* trailX,
        const float* trailY,
        const std::uint8_t* trailHead,
        std::uint8_t trailLength,
        const std::uint32_t* activeIndices,
        std::uint32_t activeCount,
        const Camera2D& camera,
        const GrayscaleSpriteAtlas& atlas,
        const PaletteRampTexture& paletteRamp,
        const BulletPaletteTable& paletteTable,
        const GradientAnimator& gradientAnimator,
        float simClock
    );

    void render(
        const Camera2D& camera,
        float simClock,
        const GrayscaleSpriteAtlas& atlas,
        const PaletteRampTexture& paletteRamp,
        int viewportWidth,
        int viewportHeight
    );

  private:
    struct BulletVertex {
        float x;
        float y;
        float u;
        float v;
        float r;
        float g;
        float b;
        float a;
    };

    GLuint vao_ {0};
    GLuint vbo_ {0};
    GLuint ebo_ {0};
    const ShaderCache::Program* program_ {nullptr};
    std::vector<BulletVertex> vertices_ {};
    std::vector<GLuint> indices_ {};
    std::uint32_t quadCount_ {0};
    std::uint32_t maxBullets_ {0};
};

} // namespace engine
