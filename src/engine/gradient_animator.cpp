#include <engine/gradient_animator.h>

#include <algorithm>
#include <cmath>

namespace engine {
namespace {

constexpr float kTwoPi = 6.28318530717958647692F;

float clamp01(const float v) {
    return std::clamp(v, 0.0F, 1.0F);
}

float wrap01(float v) {
    v = std::fmod(v, 1.0F);
    if (v < 0.0F) v += 1.0F;
    return v;
}

PaletteColor lerp(const PaletteColor& a, const PaletteColor& b, const float t) {
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t,
    };
}

struct Hsv {
    float h;
    float s;
    float v;
};

Hsv rgbToHsv(const PaletteColor& c) {
    const float maxv = std::max({c.r, c.g, c.b});
    const float minv = std::min({c.r, c.g, c.b});
    const float d = maxv - minv;
    Hsv hsv {0.0F, maxv <= 0.0F ? 0.0F : d / maxv, maxv};
    if (d <= 0.00001F) return hsv;
    if (maxv == c.r) hsv.h = std::fmod(((c.g - c.b) / d), 6.0F);
    else if (maxv == c.g) hsv.h = ((c.b - c.r) / d) + 2.0F;
    else hsv.h = ((c.r - c.g) / d) + 4.0F;
    hsv.h /= 6.0F;
    if (hsv.h < 0.0F) hsv.h += 1.0F;
    return hsv;
}

PaletteColor hsvToRgb(const Hsv& hsv) {
    const float h = hsv.h * 6.0F;
    const float c = hsv.v * hsv.s;
    const float x = c * (1.0F - std::fabs(std::fmod(h, 2.0F) - 1.0F));
    const float m = hsv.v - c;
    PaletteColor out {};
    if (h < 1.0F) out = {c, x, 0.0F, 1.0F};
    else if (h < 2.0F) out = {x, c, 0.0F, 1.0F};
    else if (h < 3.0F) out = {0.0F, c, x, 1.0F};
    else if (h < 4.0F) out = {0.0F, x, c, 1.0F};
    else if (h < 5.0F) out = {x, 0.0F, c, 1.0F};
    else out = {c, 0.0F, x, 1.0F};
    out.r += m;
    out.g += m;
    out.b += m;
    out.a = 1.0F;
    return out;
}

} // namespace

void GradientAnimator::initialize(SDL_Renderer* renderer, TextureStore& store) {
    (void)renderer;
    (void)store;
    for (std::size_t i = 0; i < pulseTable_.size(); ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(pulseTable_.size());
        pulseTable_[i] = 0.7F + 0.3F * std::sin(t * kTwoPi);
    }
    initialized_ = true;
}

std::uint8_t GradientAnimator::addGradient(const GradientDefinition& gradient, const PaletteAnimationSettings& anim, const int lutWidth) {
    if (gradients_.size() >= 255) return kInvalidAnimId;
    const std::uint32_t width = static_cast<std::uint32_t>(std::max(2, lutWidth));
    gradients_.push_back(AnimatedGradient {
        .lut = generateGradientLut(gradient, width),
        .anim = anim,
    });
    return static_cast<std::uint8_t>(gradients_.size() - 1);
}

Color GradientAnimator::sample(const std::uint8_t animId, const float time, const float bulletAge, const std::uint32_t instanceIndex) const {
    if (animId == kInvalidAnimId || animId >= gradients_.size()) return Color {};
    const AnimatedGradient& g = gradients_[animId];
    if (g.lut.empty()) return Color {};

    const PaletteAnimationSettings& anim = g.anim;
    const float phase = time * anim.speed + static_cast<float>(instanceIndex) * anim.perInstanceOffset + anim.phaseOffset;

    float samplePos = wrap01(bulletAge * 0.5F + phase);
    PaletteColor color = sampleLut(g.lut, samplePos);

    switch (anim.mode) {
    case PaletteAnimationMode::HueShift:
        color = applyHueShift(color, wrap01(phase));
        break;
    case PaletteAnimationMode::GradientCycle:
        samplePos = wrap01(phase);
        color = sampleLut(g.lut, samplePos);
        break;
    case PaletteAnimationMode::PulseBrightness: {
        const float pulsePhase = wrap01(phase);
        const std::size_t pulseIndex = static_cast<std::size_t>(pulsePhase * static_cast<float>(pulseTable_.size() - 1));
        color = scaleBrightness(color, pulseTable_[pulseIndex]);
    } break;
    case PaletteAnimationMode::PhaseScroll:
        samplePos = wrap01(bulletAge * 0.5F + phase + static_cast<float>(instanceIndex) * anim.perInstanceOffset);
        color = sampleLut(g.lut, samplePos);
        break;
    case PaletteAnimationMode::None:
    default:
        break;
    }

    return toColor(color);
}

PaletteColor GradientAnimator::sampleLut(const std::vector<PaletteColor>& lut, const float t) {
    if (lut.empty()) return {};
    if (lut.size() == 1) return lut.front();

    const float u = wrap01(t) * static_cast<float>(lut.size() - 1);
    const std::size_t i0 = static_cast<std::size_t>(u);
    const std::size_t i1 = std::min(i0 + 1, lut.size() - 1);
    return lerp(lut[i0], lut[i1], u - static_cast<float>(i0));
}

PaletteColor GradientAnimator::applyHueShift(const PaletteColor& in, const float amount) {
    Hsv hsv = rgbToHsv(in);
    hsv.h = wrap01(hsv.h + amount);
    PaletteColor out = hsvToRgb(hsv);
    out.a = in.a;
    return out;
}

PaletteColor GradientAnimator::scaleBrightness(const PaletteColor& in, const float brightness) {
    return {
        clamp01(in.r * brightness),
        clamp01(in.g * brightness),
        clamp01(in.b * brightness),
        in.a,
    };
}

Color GradientAnimator::toColor(const PaletteColor& in) {
    auto toByte = [](const float channel) {
        return static_cast<Uint8>(std::clamp(std::lround(channel * 255.0F), 0L, 255L));
    };
    return Color {
        toByte(in.r),
        toByte(in.g),
        toByte(in.b),
        toByte(in.a),
    };
}

} // namespace engine
