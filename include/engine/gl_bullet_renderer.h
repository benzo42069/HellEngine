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
#include <span>
#include <string>
#include <vector>

namespace engine {

// GPU draw backend for projectile presentation when GL resources are available.
// Ownership: consumes ProjectileSystem SoA snapshots prepared by RenderPipeline.
class GlBulletRenderer {
  public:
    struct ProjectileSoAView {
        std::span<const float> posX;
        std::span<const float> posY;
        std::span<const float> velX;
        std::span<const float> velY;
        std::span<const float> radius;
        std::span<const float> life;
        std::span<const std::uint8_t> paletteIndex;
        std::span<const std::uint8_t> shape;
        std::span<const std::uint8_t> active;
        std::span<const std::uint8_t> allegiance;
        std::span<const std::uint8_t> enableTrails;
        std::span<const float> trailX;
        std::span<const float> trailY;
        std::span<const std::uint8_t> trailHead;
        std::uint8_t trailLength {0};
        std::span<const std::uint32_t> activeIndices;
        std::uint32_t activeCount {0};
    };

    bool initialize(ShaderCache& shaders, std::uint32_t maxBullets, std::string* error = nullptr);
    void shutdown();
    [[nodiscard]] bool initialized() const { return program_ != nullptr && vao_ != 0 && vbo_ != 0 && ebo_ != 0; }

    void buildVertexBuffer(
        const ProjectileSoAView& view,
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
