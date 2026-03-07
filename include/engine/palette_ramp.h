#pragma once

#include <engine/bullet_palette.h>
#include <engine/palette_fx_templates.h>

#include <glad/glad.h>

#include <cstdint>
#include <vector>
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

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
    static constexpr int kRampWidth = 64;
    static constexpr int kMaxPalettes = 64;

    bool buildFromRegistry(const PaletteFxTemplateRegistry& registry);
    void updateRow(std::uint8_t index, const PaletteTemplate& templ);

    [[nodiscard]] GLuint textureId() const;
    [[nodiscard]] int paletteCount() const;
    [[nodiscard]] float rowV(std::uint8_t index) const;
    [[nodiscard]] const PaletteAnimationSettings& animationFor(std::uint8_t index) const;
    [[nodiscard]] std::uint8_t indexForPaletteName(const std::string& name) const;

    void shutdown();

  private:
    void buildRow(std::uint8_t index, const PaletteTemplate& templ);

    GLuint texture_ {0};
    int paletteCount_ {0};
    std::array<PaletteAnimationSettings, kMaxPalettes> animations_ {};
    std::array<std::array<std::uint8_t, kRampWidth * 4>, kMaxPalettes> pixelRows_ {};
    std::unordered_map<std::string, std::uint8_t> paletteNameToIndex_ {};
    const PaletteFxTemplateRegistry* registry_ {nullptr};
};

} // namespace engine
