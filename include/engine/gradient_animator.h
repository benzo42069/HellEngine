#pragma once

#include <engine/palette_fx_templates.h>
#include <engine/render2d.h>

#include <array>
#include <cstdint>
#include <vector>

namespace engine {

class GradientAnimator {
  public:
    static constexpr std::uint8_t kInvalidAnimId = 0xFF;

    void initialize(SDL_Renderer* renderer, TextureStore& store);

    std::uint8_t addGradient(const GradientDefinition& gradient, const PaletteAnimationSettings& anim, int lutWidth = 64);

    [[nodiscard]] Color sample(std::uint8_t animId, float time, float bulletAge, std::uint32_t instanceIndex) const;

  private:
    struct AnimatedGradient {
        std::vector<PaletteColor> lut;
        PaletteAnimationSettings anim;
    };

    [[nodiscard]] static PaletteColor sampleLut(const std::vector<PaletteColor>& lut, float t);
    [[nodiscard]] static PaletteColor applyHueShift(const PaletteColor& in, float amount);
    [[nodiscard]] static PaletteColor scaleBrightness(const PaletteColor& in, float brightness);
    [[nodiscard]] static Color toColor(const PaletteColor& in);

    bool initialized_ {false};
    std::array<float, 256> pulseTable_ {};
    std::vector<AnimatedGradient> gradients_;
};

} // namespace engine
