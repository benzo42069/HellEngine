#include <engine/palette_ramp.h>

#include <algorithm>
#include <array>

namespace engine {

namespace {

int mapAnimationMode(const PaletteAnimationMode mode) {
    switch (mode) {
        case PaletteAnimationMode::None: return 0;
        case PaletteAnimationMode::HueShift: return 1;
        case PaletteAnimationMode::GradientCycle: return 2;
        case PaletteAnimationMode::PulseBrightness: return 3;
        case PaletteAnimationMode::PhaseScroll: return 4;
    }
    return 0;
}

float lerp(const float a, const float b, const float t) {
    return a + (b - a) * t;
}

} // namespace

bool PaletteRampTexture::generate(const PaletteFxTemplateRegistry& registry, const BulletPaletteTable& table, const int rampWidth) {
    shutdown();
    width_ = std::max(2, rampWidth);
    height_ = BulletPaletteTable::kMaxPalettes;
    rowAnimations_.assign(static_cast<std::size_t>(height_), RowAnimation {});

    std::vector<unsigned char> pixels(static_cast<std::size_t>(width_ * height_ * 4), 255);
    for (int row = 0; row < height_; ++row) {
        const auto entry = table.get(static_cast<std::uint8_t>(row));
        for (int x = 0; x < width_; ++x) {
            const float t = static_cast<float>(x) / static_cast<float>(width_ - 1);
            float r = 0.0F;
            float g = 0.0F;
            float b = 0.0F;
            if (t < 0.5F) {
                const float local = t * 2.0F;
                r = lerp(static_cast<float>(entry.highlight.r), static_cast<float>(entry.core.r), local);
                g = lerp(static_cast<float>(entry.highlight.g), static_cast<float>(entry.core.g), local);
                b = lerp(static_cast<float>(entry.highlight.b), static_cast<float>(entry.core.b), local);
            } else {
                const float local = (t - 0.5F) * 2.0F;
                r = lerp(static_cast<float>(entry.core.r), static_cast<float>(entry.glow.r), local);
                g = lerp(static_cast<float>(entry.core.g), static_cast<float>(entry.glow.g), local);
                b = lerp(static_cast<float>(entry.core.b), static_cast<float>(entry.glow.b), local);
            }

            const std::size_t idx = static_cast<std::size_t>((row * width_ + x) * 4);
            pixels[idx + 0] = static_cast<unsigned char>(std::clamp(r, 0.0F, 255.0F));
            pixels[idx + 1] = static_cast<unsigned char>(std::clamp(g, 0.0F, 255.0F));
            pixels[idx + 2] = static_cast<unsigned char>(std::clamp(b, 0.0F, 255.0F));
            pixels[idx + 3] = 255;
        }
    }

    const auto& db = registry.database();
    std::uint8_t paletteIndex = 1;
    for (const PaletteTemplate& templ : db.palettes) {
        if (paletteIndex >= rowAnimations_.size()) break;
        rowAnimations_[paletteIndex] = RowAnimation {
            .mode = mapAnimationMode(templ.animation.mode),
            .speed = templ.animation.speed,
            .perInstanceOffset = templ.animation.perInstanceOffset,
            .emissiveBoost = std::max(0.1F, buildMaterialParamsFromTemplate(templ).emissiveBoost),
        };
        ++paletteIndex;
    }

    glGenTextures(1, &texture_);
    if (texture_ == 0) return false;
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void PaletteRampTexture::shutdown() {
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    width_ = 0;
    height_ = 0;
    rowAnimations_.clear();
}

float PaletteRampTexture::rowV(const std::uint8_t paletteIndex) const {
    if (height_ <= 0) return 0.5F;
    const int row = std::clamp<int>(paletteIndex, 0, height_ - 1);
    return (static_cast<float>(row) + 0.5F) / static_cast<float>(height_);
}

PaletteRampTexture::RowAnimation PaletteRampTexture::rowAnimation(const std::uint8_t paletteIndex) const {
    if (paletteIndex >= rowAnimations_.size()) return RowAnimation {};
    return rowAnimations_[paletteIndex];
}

PaletteRampTexture::RowAnimation PaletteRampTexture::mostCommonAnimation() const {
    if (rowAnimations_.empty()) return RowAnimation {};
    std::array<std::uint32_t, 5> modeCount {0, 0, 0, 0, 0};
    std::array<RowAnimation, 5> sample {};
    for (const RowAnimation& anim : rowAnimations_) {
        const int mode = std::clamp(anim.mode, 0, 4);
        if (modeCount[mode] == 0U) sample[mode] = anim;
        ++modeCount[mode];
    }
    int winner = 0;
    for (int i = 1; i < static_cast<int>(modeCount.size()); ++i) {
        if (modeCount[i] > modeCount[winner]) winner = i;
    }
    return sample[winner];
}

} // namespace engine
