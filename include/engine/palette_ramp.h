#pragma once

#include <engine/bullet_palette.h>
#include <engine/palette_fx_templates.h>

#include <glad/glad.h>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
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
    bool buildFromRegistry(const PaletteFxTemplateRegistry& registry);
    void shutdown();

    [[nodiscard]] GLuint textureId() const;
    [[nodiscard]] float rowV(std::uint8_t paletteIndex) const;
    [[nodiscard]] RowAnimation rowAnimation(std::uint8_t paletteIndex) const;
    [[nodiscard]] RowAnimation mostCommonAnimation() const;
    [[nodiscard]] bool valid() const { return texture_ != 0; }
    [[nodiscard]] const PaletteAnimationSettings& animationFor(std::uint8_t index) const;

  private:
    static constexpr int kRampWidth = 64;
    static constexpr int kMaxPalettes = 64;

    void updateRow(std::uint8_t index, const PaletteTemplate& templ);
    [[nodiscard]] int paletteCount() const;
    [[nodiscard]] std::uint8_t indexForPaletteName(const std::string& name) const;
    void buildRow(std::uint8_t index, const PaletteTemplate& templ);

    GLuint texture_ {0};
    int width_ {0};
    int height_ {0};
    std::vector<RowAnimation> rowAnimations_ {};
    int paletteCount_ {0};
    std::array<PaletteAnimationSettings, kMaxPalettes> animations_ {};
    std::array<std::array<std::uint8_t, kRampWidth * 4>, kMaxPalettes> pixelRows_ {};
    std::unordered_map<std::string, std::uint8_t> paletteNameToIndex_ {};
    const PaletteFxTemplateRegistry* registry_ {nullptr};
};

} // namespace engine
