#pragma once

#include <engine/palette_fx_templates.h>
#include <engine/render2d.h>

#include <SDL.h>

#include <cstdint>
#include <vector>

namespace engine {

class DangerFieldOverlay {
  public:
    ~DangerFieldOverlay() { shutdown(); }

    void initialize(SDL_Renderer* renderer, int fieldWidth, int fieldHeight);
    void shutdown();

    void buildFromGrid(
        const std::vector<int>& gridHead,
        const std::vector<int>& gridNext,
        const std::vector<float>& posX,
        const std::vector<float>& posY,
        const std::vector<std::uint8_t>& active,
        std::uint32_t gridX,
        std::uint32_t gridY,
        float worldHalfExtent
    );

    void render(SDL_Renderer* renderer, const Camera2D& camera, float opacity = 0.25F);

    void setGradient(const std::vector<PaletteColor>& lut);

  private:
    SDL_Texture* fieldTexture_ {nullptr};
    int fieldW_ {160};
    int fieldH_ {90};
    std::vector<float> density_ {};
    std::vector<std::uint32_t> pixels_ {};
    std::vector<PaletteColor> gradientLut_ {};
};

} // namespace engine
