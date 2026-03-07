#pragma once

#include <engine/bullet_sprite_gen.h>

#include <glad/glad.h>

#include <unordered_map>

namespace engine {

struct SpriteAtlasRegion {
    float u0 {0.0F};
    float v0 {0.0F};
    float u1 {0.0F};
    float v1 {0.0F};
};

class GrayscaleSpriteAtlas {
  public:
    bool generate(int shapeSize = 32);
    [[nodiscard]] GLuint textureId() const;
    [[nodiscard]] SpriteAtlasRegion region(BulletShape shape) const;
    void shutdown();

  private:
    GLuint texture_ {0};
    int atlasWidth_ {0};
    int atlasHeight_ {0};
    std::unordered_map<BulletShape, SpriteAtlasRegion> regions_;
};

} // namespace engine
