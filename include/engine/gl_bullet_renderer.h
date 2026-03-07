#pragma once

#include <engine/bullet_sprite_gen.h>
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
    bool initialize(
        ShaderCache& shaders,
        const GrayscaleSpriteAtlas& atlas,
        const PaletteRampTexture& paletteRamp,
        std::uint32_t maxBullets,
        std::string* error = nullptr
    );
    void shutdown();

    void buildVertexBuffer(
        const float* posX,
        const float* posY,
        const float* velX,
        const float* velY,
        const float* radius,
        const float* life,
        const std::uint8_t* paletteIndex,
        const std::uint8_t* active,
        const std::uint32_t* activeIndices,
        std::uint32_t activeCount,
        std::uint32_t capacity,
        BulletShape defaultShape = BulletShape::Circle
    );

    void render(
        const Camera2D& camera,
        float simTime,
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
        float paletteRow;
        float age;
        float instanceId;
    };

    GLuint vao_ {0};
    GLuint vbo_ {0};
    GLuint ebo_ {0};
    const ShaderCache::Program* program_ {nullptr};
    const GrayscaleSpriteAtlas* atlas_ {nullptr};
    const PaletteRampTexture* paletteRamp_ {nullptr};
    std::vector<BulletVertex> vertices_ {};
    std::vector<GLuint> indices_ {};
    std::uint32_t quadCount_ {0};
    std::uint32_t maxBullets_ {0};
    PaletteRampTexture::RowAnimation frameAnim_ {};
};

} // namespace engine
