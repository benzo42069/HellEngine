#include <engine/palette_ramp.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kRampWidth, paletteCount_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
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

float PaletteRampTexture::rowV(const std::uint8_t paletteIndex) const {
    if (paletteCount_ > 0) {
        const int row = std::clamp<int>(paletteIndex, 0, paletteCount_ - 1);
        return (static_cast<float>(row) + 0.5F) / static_cast<float>(paletteCount_);
    }
    if (height_ > 0) {
        const int row = std::clamp<int>(paletteIndex, 0, height_ - 1);
        return (static_cast<float>(row) + 0.5F) / static_cast<float>(height_);
    }
    return 0.5F;
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
    width_ = 0;
    height_ = 0;
    rowAnimations_.clear();
    paletteCount_ = 0;
    paletteNameToIndex_.clear();
    registry_ = nullptr;
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

    auto colorFor = [&](const int x) -> PaletteColor {
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
