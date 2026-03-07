#include <engine/palette_ramp.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace engine {
namespace {

std::uint8_t toByte(const float channel) {
    return static_cast<std::uint8_t>(std::clamp(std::lround(channel * 255.0F), 0L, 255L));
}

const GradientDefinition* findGradient(const PaletteFxTemplateRegistry* registry, const std::string& name) {
    if (!registry || name.empty()) return nullptr;
    for (const GradientDefinition& gradient : registry->database().gradients) {
        if (gradient.name == name) return &gradient;
    }
    return nullptr;
}

} // namespace

bool PaletteRampTexture::buildFromRegistry(const PaletteFxTemplateRegistry& registry) {
    registry_ = &registry;
    paletteNameToIndex_.clear();

    const auto& db = registry.database();
    const std::size_t maxTemplateRows = static_cast<std::size_t>(kMaxPalettes - 1);
    const std::size_t templateCount = std::min(db.palettes.size(), maxTemplateRows);
    paletteCount_ = static_cast<int>(templateCount + 1);

    const PaletteTemplate emptyTemplate {};
    buildRow(0, emptyTemplate);
    animations_[0] = PaletteAnimationSettings {};

    for (std::size_t i = 0; i < templateCount; ++i) {
        const auto row = static_cast<std::uint8_t>(i + 1);
        buildRow(row, db.palettes[i]);
        animations_[row] = db.palettes[i].animation;
        paletteNameToIndex_[db.palettes[i].name] = row;
    }

    for (int row = paletteCount_; row < kMaxPalettes; ++row) {
        pixelRows_[static_cast<std::size_t>(row)].fill(0);
        animations_[static_cast<std::size_t>(row)] = PaletteAnimationSettings {};
    }

    if (texture_ == 0) {
        glGenTextures(1, &texture_);
    }

    std::vector<std::uint8_t> image(static_cast<std::size_t>(paletteCount_) * kRampWidth * 4, 0);
    for (int row = 0; row < paletteCount_; ++row) {
        std::memcpy(
            image.data() + static_cast<std::size_t>(row) * kRampWidth * 4,
            pixelRows_[static_cast<std::size_t>(row)].data(),
            static_cast<std::size_t>(kRampWidth * 4)
        );
    }

    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        kRampWidth,
        paletteCount_,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void PaletteRampTexture::updateRow(const std::uint8_t index, const PaletteTemplate& templ) {
    if (index >= static_cast<std::uint8_t>(paletteCount_)) return;
    buildRow(index, templ);
    animations_[index] = templ.animation;
    if (!templ.name.empty()) paletteNameToIndex_[templ.name] = index;

    if (texture_ == 0) return;

    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        static_cast<GLint>(index),
        kRampWidth,
        1,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixelRows_[index].data()
    );
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint PaletteRampTexture::textureId() const {
    return texture_;
}

int PaletteRampTexture::paletteCount() const {
    return paletteCount_;
}

float PaletteRampTexture::rowV(const std::uint8_t index) const {
    if (paletteCount_ <= 0) return 0.5F;
    const std::uint8_t clamped = static_cast<std::uint8_t>(std::min<int>(index, paletteCount_ - 1));
    return (static_cast<float>(clamped) + 0.5F) / static_cast<float>(paletteCount_);
}

const PaletteAnimationSettings& PaletteRampTexture::animationFor(const std::uint8_t index) const {
    if (index >= static_cast<std::uint8_t>(kMaxPalettes)) return animations_[0];
    return animations_[index];
}

std::uint8_t PaletteRampTexture::indexForPaletteName(const std::string& name) const {
    const auto found = paletteNameToIndex_.find(name);
    if (found == paletteNameToIndex_.end()) return 0;
    return found->second;
}

void PaletteRampTexture::shutdown() {
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    paletteCount_ = 0;
    paletteNameToIndex_.clear();
    registry_ = nullptr;
}

void PaletteRampTexture::buildRow(const std::uint8_t index, const PaletteTemplate& templ) {
    if (index >= static_cast<std::uint8_t>(kMaxPalettes)) return;

    auto& row = pixelRows_[index];
    row.fill(0);

    const PaletteMaterialParams params = buildMaterialParamsFromTemplate(templ);
    const GradientDefinition* gradient = findGradient(registry_, params.gradientName);

    if (gradient != nullptr && !params.gradientName.empty()) {
        const std::vector<PaletteColor> lut = generateGradientLut(*gradient, static_cast<std::uint32_t>(kRampWidth));
        for (int x = 0; x < kRampWidth; ++x) {
            const PaletteColor c = x < static_cast<int>(lut.size()) ? lut[static_cast<std::size_t>(x)] : PaletteColor {};
            row[static_cast<std::size_t>(x) * 4 + 0] = toByte(c.r);
            row[static_cast<std::size_t>(x) * 4 + 1] = toByte(c.g);
            row[static_cast<std::size_t>(x) * 4 + 2] = toByte(c.b);
            row[static_cast<std::size_t>(x) * 4 + 3] = toByte(c.a);
        }
        return;
    }

    const int endA = std::clamp(static_cast<int>(std::round(params.thresholdA * static_cast<float>(kRampWidth))), 0, kRampWidth);
    const int endB = std::clamp(static_cast<int>(std::round(params.thresholdB * static_cast<float>(kRampWidth))), 0, kRampWidth);
    constexpr int kBlend = 4;

    auto colorFor = [&](int x) -> PaletteColor {
        PaletteColor out = params.paletteCore;

        if (x < endA) {
            out = params.paletteGlow;
        } else if (x < endB) {
            out = params.paletteHighlight;
        }

        const float blendA = std::clamp((static_cast<float>(x - (endA - kBlend))) / static_cast<float>(kBlend), 0.0F, 1.0F);
        if (x >= endA - kBlend && x < endA) {
            out = {
                params.paletteGlow.r + (params.paletteHighlight.r - params.paletteGlow.r) * blendA,
                params.paletteGlow.g + (params.paletteHighlight.g - params.paletteGlow.g) * blendA,
                params.paletteGlow.b + (params.paletteHighlight.b - params.paletteGlow.b) * blendA,
                params.paletteGlow.a + (params.paletteHighlight.a - params.paletteGlow.a) * blendA,
            };
        }

        const float blendB = std::clamp((static_cast<float>(x - (endB - kBlend))) / static_cast<float>(kBlend), 0.0F, 1.0F);
        if (x >= endB - kBlend && x < endB) {
            out = {
                params.paletteHighlight.r + (params.paletteCore.r - params.paletteHighlight.r) * blendB,
                params.paletteHighlight.g + (params.paletteCore.g - params.paletteHighlight.g) * blendB,
                params.paletteHighlight.b + (params.paletteCore.b - params.paletteHighlight.b) * blendB,
                params.paletteHighlight.a + (params.paletteCore.a - params.paletteHighlight.a) * blendB,
            };
        }
        return out;
    };

    for (int x = 0; x < kRampWidth; ++x) {
        const PaletteColor c = colorFor(x);
        row[static_cast<std::size_t>(x) * 4 + 0] = toByte(c.r);
        row[static_cast<std::size_t>(x) * 4 + 1] = toByte(c.g);
        row[static_cast<std::size_t>(x) * 4 + 2] = toByte(c.b);
        row[static_cast<std::size_t>(x) * 4 + 3] = toByte(c.a);
    }
}

} // namespace engine
