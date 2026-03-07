#pragma once

#include <engine/render2d.h>

#include <SDL.h>

#include <string>
#include <vector>

namespace engine {

struct BackgroundLayer {
    std::string textureId;
    float parallaxFactor {1.0F}; // 0.0 = static, 1.0 = moves with camera
    float scrollSpeedX {0.0F};   // autonomous scroll per second
    float scrollSpeedY {0.0F};
    float opacity {1.0F};        // 0.0-1.0
    Color tint {255, 255, 255, 255};
};

class BackgroundSystem {
  public:
    void initialize(SDL_Renderer* renderer, TextureStore& store);
    void addLayer(const BackgroundLayer& layer);
    void update(float frameDelta);
    void render(SpriteBatch& batch, const Camera2D& camera) const;
    void setPrimaryLayerTexture(const std::string& textureId);
    void clear();

  private:
    std::vector<BackgroundLayer> layers_;
    float scrollOffset_ {0.0F};
};

} // namespace engine
