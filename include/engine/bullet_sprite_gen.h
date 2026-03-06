#pragma once

#include <engine/palette_fx_templates.h>
#include <engine/render2d.h>

#include <SDL.h>

#include <cstdint>
#include <string>

namespace engine {

enum class BulletShape : std::uint8_t {
    Circle,
    Rice,
    Star,
    Diamond,
    Ring,
    Beam,
};

struct BulletSpriteParams {
    BulletShape shape {BulletShape::Circle};
    PaletteColor core;
    PaletteColor highlight;
    PaletteColor glow;
    int size {16};
    float glowFalloff {0.6F};
    float emissiveBoost {1.0F};
};

[[nodiscard]] const char* bulletShapeSuffix(BulletShape shape);
[[nodiscard]] std::string sanitizeBulletPaletteName(const std::string& paletteName);
[[nodiscard]] std::string bulletTextureId(const std::string& paletteName, BulletShape shape);

class BulletSpriteGenerator {
  public:
    std::string generate(
        SDL_Renderer* renderer,
        TextureStore& store,
        const std::string& idPrefix,
        const BulletSpriteParams& params);

    void generatePaletteSet(
        SDL_Renderer* renderer,
        TextureStore& store,
        const std::string& paletteName,
        const PaletteFillResult& fill,
        int size = 16);
};

} // namespace engine

