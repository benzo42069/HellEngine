#pragma once

#include <engine/bullet_palette.h>
#include <engine/palette_fx_templates.h>

#include <glad/glad.h>

#include <cstdint>
#include <vector>

namespace engine {

class PaletteRampTexture {
  public:
    struct RowAnimation {
        int mode {0};
        float speed {0.0F};
        float perInstanceOffset {0.0F};
        float emissiveBoost {1.0F};
    };

    bool generate(const PaletteFxTemplateRegistry& registry, const BulletPaletteTable& table, int rampWidth = 64);
    void shutdown();

    [[nodiscard]] GLuint textureId() const { return texture_; }
    [[nodiscard]] float rowV(std::uint8_t paletteIndex) const;
    [[nodiscard]] RowAnimation rowAnimation(std::uint8_t paletteIndex) const;
    [[nodiscard]] RowAnimation mostCommonAnimation() const;
    [[nodiscard]] bool valid() const { return texture_ != 0; }

  private:
    GLuint texture_ {0};
    int width_ {0};
    int height_ {0};
    std::vector<RowAnimation> rowAnimations_ {};
};

} // namespace engine
