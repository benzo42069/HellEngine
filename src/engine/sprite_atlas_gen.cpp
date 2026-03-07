#include <engine/sprite_atlas_gen.h>

#include <engine/logging.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace engine {

namespace {

constexpr std::array<BulletShape, 6> kShapes = {
    BulletShape::Circle,
    BulletShape::Rice,
    BulletShape::Star,
    BulletShape::Diamond,
    BulletShape::Ring,
    BulletShape::Beam,
};

float clamp01(const float v) {
    return std::clamp(v, 0.0F, 1.0F);
}

float smoothstep(const float edge0, const float edge1, const float x) {
    if (edge0 == edge1) return x < edge0 ? 0.0F : 1.0F;
    const float t = clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0F - 2.0F * t);
}

float capsuleSdf(const float px, const float py, const float ax, const float ay, const float bx, const float by, const float radius) {
    const float pax = px - ax;
    const float pay = py - ay;
    const float bax = bx - ax;
    const float bay = by - ay;
    const float baDot = bax * bax + bay * bay;
    float h = 0.0F;
    if (baDot > 0.0F) h = clamp01((pax * bax + pay * bay) / baDot);
    const float dx = pax - bax * h;
    const float dy = pay - bay * h;
    return std::sqrt(dx * dx + dy * dy) - radius;
}

float luminanceForShape(const BulletShape shape, const float px, const float py) {
    const float len = std::sqrt(px * px + py * py);
    switch (shape) {
        case BulletShape::Circle:
            return 1.0F - smoothstep(0.0F, 0.5F, len);
        case BulletShape::Rice: {
            const float d = capsuleSdf(px, py, 0.0F, -0.35F, 0.0F, 0.35F, 0.12F);
            return 1.0F - smoothstep(0.0F, 0.2F, d);
        }
        case BulletShape::Star: {
            const float a = std::atan2(py, px);
            const float d = len - 0.35F + 0.12F * std::cos(5.0F * a);
            return 1.0F - smoothstep(0.0F, 0.4F, d);
        }
        case BulletShape::Diamond: {
            const float d = std::fabs(px) + std::fabs(py) - 0.4F;
            return 1.0F - smoothstep(0.0F, 0.4F, d);
        }
        case BulletShape::Ring: {
            const float d = std::fabs(len - 0.3F) - 0.05F;
            return 1.0F - smoothstep(0.0F, 0.15F, d);
        }
        case BulletShape::Beam: {
            const float d = std::max(std::fabs(px) - 0.45F, std::fabs(py) - 0.06F);
            return 1.0F - smoothstep(0.0F, 0.15F, d);
        }
        default:
            break;
    }
    return 0.0F;
}

} // namespace

bool GrayscaleSpriteAtlas::generate(const int shapeSize) {
    if (shapeSize <= 0) {
        logError("Sprite atlas generation failed: shape size must be positive.");
        return false;
    }

    shutdown();

    atlasWidth_ = shapeSize * static_cast<int>(kShapes.size());
    atlasHeight_ = shapeSize;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(atlasWidth_ * atlasHeight_), 0U);

    for (std::size_t shapeIndex = 0; shapeIndex < kShapes.size(); ++shapeIndex) {
        const BulletShape shape = kShapes[shapeIndex];
        const int xOffset = static_cast<int>(shapeIndex) * shapeSize;
        for (int y = 0; y < shapeSize; ++y) {
            for (int x = 0; x < shapeSize; ++x) {
                const float u = (static_cast<float>(x) + 0.5F) / static_cast<float>(shapeSize);
                const float v = (static_cast<float>(y) + 0.5F) / static_cast<float>(shapeSize);
                const float px = u * 2.0F - 1.0F;
                const float py = v * 2.0F - 1.0F;
                const float lum = clamp01(luminanceForShape(shape, px, py));
                pixels[static_cast<std::size_t>(y * atlasWidth_ + xOffset + x)] = static_cast<std::uint8_t>(lum * 255.0F);
            }
        }

        const float u0 = static_cast<float>(xOffset) / static_cast<float>(atlasWidth_);
        const float u1 = static_cast<float>(xOffset + shapeSize) / static_cast<float>(atlasWidth_);
        regions_[shape] = SpriteAtlasRegion {u0, 0.0F, u1, 1.0F};
    }

    glGenTextures(1, &texture_);
    if (texture_ == 0) {
        logError("Sprite atlas generation failed: glGenTextures returned 0.");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasWidth_, atlasHeight_, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    logInfo("Generated grayscale bullet sprite atlas " + std::to_string(atlasWidth_) + "x" + std::to_string(atlasHeight_) + ".");
    return true;
}

GLuint GrayscaleSpriteAtlas::textureId() const {
    return texture_;
}

SpriteAtlasRegion GrayscaleSpriteAtlas::region(const BulletShape shape) const {
    const auto found = regions_.find(shape);
    if (found == regions_.end()) return {};
    return found->second;
}

void GrayscaleSpriteAtlas::shutdown() {
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    atlasWidth_ = 0;
    atlasHeight_ = 0;
    regions_.clear();
}

} // namespace engine
