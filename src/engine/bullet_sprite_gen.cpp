#include <engine/bullet_sprite_gen.h>

#include <engine/logging.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace engine {
namespace {

constexpr std::array<BulletShape, 6> kAllBulletShapes {
    BulletShape::Circle,
    BulletShape::Rice,
    BulletShape::Star,
    BulletShape::Diamond,
    BulletShape::Ring,
    BulletShape::Beam,
};

float shapeSdf(const BulletShape shape, const float x, const float y) {
    switch (shape) {
    case BulletShape::Circle:
        return std::sqrt(x * x + y * y) - 0.45F;
    case BulletShape::Rice: {
        const float ax = 0.0F;
        const float ay = -0.4F;
        const float bx = 0.0F;
        const float by = 0.4F;
        const float pax = x - ax;
        const float pay = y - ay;
        const float bax = bx - ax;
        const float bay = by - ay;
        const float h = std::clamp((pax * bax + pay * bay) / std::max(0.0001F, bax * bax + bay * bay), 0.0F, 1.0F);
        const float dx = pax - bax * h;
        const float dy = pay - bay * h;
        return std::sqrt(dx * dx + dy * dy) - 0.15F;
    }
    case BulletShape::Star: {
        const float angle = std::atan2(y, x);
        return std::sqrt(x * x + y * y) - 0.4F + 0.15F * std::cos(5.0F * angle);
    }
    case BulletShape::Diamond:
        return std::abs(x) + std::abs(y) - 0.45F;
    case BulletShape::Ring:
        return std::abs(std::sqrt(x * x + y * y) - 0.35F) - 0.06F;
    case BulletShape::Beam:
        return std::max(std::abs(x) - 0.45F, std::abs(y) - 0.08F);
    }
    return 1.0F;
}

std::uint8_t toByte(float value) {
    value = std::clamp(value, 0.0F, 1.0F);
    return static_cast<std::uint8_t>(std::lround(value * 255.0F));
}

Color toColor(const PaletteColor& color) {
    return Color {
        toByte(color.r),
        toByte(color.g),
        toByte(color.b),
        toByte(color.a),
    };
}

Color lerpColor(const Color& a, const Color& b, const float t) {
    const float clamped = std::clamp(t, 0.0F, 1.0F);
    auto lerpByte = [clamped](const std::uint8_t x, const std::uint8_t y) {
        return static_cast<std::uint8_t>(std::lround(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * clamped));
    };
    return Color {
        lerpByte(a.r, b.r),
        lerpByte(a.g, b.g),
        lerpByte(a.b, b.b),
        lerpByte(a.a, b.a),
    };
}

} // namespace

const char* bulletShapeSuffix(const BulletShape shape) {
    switch (shape) {
    case BulletShape::Circle:
        return "circle";
    case BulletShape::Rice:
        return "rice";
    case BulletShape::Star:
        return "star";
    case BulletShape::Diamond:
        return "diamond";
    case BulletShape::Ring:
        return "ring";
    case BulletShape::Beam:
        return "beam";
    }
    return "circle";
}

std::string sanitizeBulletPaletteName(const std::string& paletteName) {
    std::string out;
    out.reserve(paletteName.size());
    for (const unsigned char c : paletteName) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            out.push_back(static_cast<char>(c));
        }
    }
    if (out.empty()) out = "default";
    return out;
}

std::string bulletTextureId(const std::string& paletteName, const BulletShape shape) {
    return "bullet_" + sanitizeBulletPaletteName(paletteName) + "_" + bulletShapeSuffix(shape);
}

std::string BulletSpriteGenerator::generate(
    SDL_Renderer* renderer,
    TextureStore& store,
    const std::string& idPrefix,
    const BulletSpriteParams& params) {
    if (!renderer) {
        logWarn("Bullet sprite generation skipped for '" + idPrefix + "': renderer is null.");
        return {};
    }

    const int size = std::clamp(params.size, 8, 32);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, size, size);
    if (!texture) {
        logWarn("Bullet sprite generation failed for '" + idPrefix + "': " + std::string(SDL_GetError()));
        return {};
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    const Color core = toColor(params.core);
    const Color highlight = toColor(params.highlight);
    Color glow = toColor(params.glow);
    const float boost = std::clamp(params.emissiveBoost, 0.1F, 4.0F);
    glow.r = static_cast<Uint8>(std::clamp(std::lround(static_cast<float>(glow.r) * boost), 0L, 255L));
    glow.g = static_cast<Uint8>(std::clamp(std::lround(static_cast<float>(glow.g) * boost), 0L, 255L));
    glow.b = static_cast<Uint8>(std::clamp(std::lround(static_cast<float>(glow.b) * boost), 0L, 255L));

    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(size) * static_cast<std::size_t>(size), 0U);
    const float denom = static_cast<float>(size);
    const float feather = 0.06F;
    const float highlightBand = 0.12F;
    const float glowStart = 0.02F;
    const float glowWidth = std::max(0.02F, params.glowFalloff);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float nx = ((static_cast<float>(x) + 0.5F) / denom) * 2.0F - 1.0F;
            const float ny = ((static_cast<float>(y) + 0.5F) / denom) * 2.0F - 1.0F;
            const float d = shapeSdf(params.shape, nx, ny);

            Color out = core;
            float alpha = 0.0F;

            if (d <= -highlightBand) {
                out = core;
                alpha = static_cast<float>(core.a);
            } else if (d < 0.0F) {
                const float t = (d + highlightBand) / highlightBand;
                out = lerpColor(core, highlight, t);
                alpha = static_cast<float>(out.a);
            } else if (d <= glowStart + glowWidth) {
                const float glowT = std::clamp((d - glowStart) / glowWidth, 0.0F, 1.0F);
                out = lerpColor(highlight, glow, glowT);
                const float fade = 1.0F - glowT;
                alpha = static_cast<float>(out.a) * fade;
            }

            if (d > -feather && d < feather) {
                const float edge = std::clamp((feather - d) / (2.0F * feather), 0.0F, 1.0F);
                alpha *= edge;
            }

            const std::uint8_t a = static_cast<std::uint8_t>(std::clamp(std::lround(alpha), 0L, 255L));
            pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(size) + static_cast<std::size_t>(x)]
                = static_cast<std::uint32_t>(out.r)
                | (static_cast<std::uint32_t>(out.g) << 8U)
                | (static_cast<std::uint32_t>(out.b) << 16U)
                | (static_cast<std::uint32_t>(a) << 24U);
        }
    }

    if (SDL_UpdateTexture(texture, nullptr, pixels.data(), size * static_cast<int>(sizeof(std::uint32_t))) != 0) {
        logWarn("SDL_UpdateTexture failed for generated bullet sprite '" + idPrefix + "': " + std::string(SDL_GetError()));
        SDL_DestroyTexture(texture);
        return {};
    }

    const std::string textureId = idPrefix + "_" + bulletShapeSuffix(params.shape);
    store.adoptTexture(textureId, texture, size, size);
    return textureId;
}

void BulletSpriteGenerator::generatePaletteSet(
    SDL_Renderer* renderer,
    TextureStore& store,
    const std::string& paletteName,
    const PaletteFillResult& fill,
    const int size) {
    const std::string idPrefix = "bullet_" + sanitizeBulletPaletteName(paletteName);
    BulletSpriteParams params;
    params.core = fill.core;
    params.highlight = fill.highlight;
    params.glow = fill.glow;
    params.size = size;

    for (const BulletShape shape : kAllBulletShapes) {
        params.shape = shape;
        (void)generate(renderer, store, idPrefix, params);
    }
}

} // namespace engine
