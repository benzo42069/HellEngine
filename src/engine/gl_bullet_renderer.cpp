#include <engine/gl_bullet_renderer.h>
#include <engine/projectiles.h>

#include <algorithm>
#include <array>
#include <cstddef>

namespace engine {

namespace {
constexpr const char* kBulletShaderName = "bullet";

std::array<float, 4> toLinearColor(const Color& c) {
    return {
        static_cast<float>(c.r) / 255.0F,
        static_cast<float>(c.g) / 255.0F,
        static_cast<float>(c.b) / 255.0F,
        static_cast<float>(c.a) / 255.0F,
    };
}

BulletShape decodeShape(const std::uint8_t raw) {
    switch (static_cast<BulletShape>(raw)) {
        case BulletShape::Circle:
        case BulletShape::Rice:
        case BulletShape::Star:
        case BulletShape::Diamond:
        case BulletShape::Ring:
        case BulletShape::Beam:
            return static_cast<BulletShape>(raw);
        default:
            return BulletShape::Circle;
    }
}

} // namespace

bool GlBulletRenderer::initialize(ShaderCache& shaders, const std::uint32_t maxBullets, std::string* error) {
    shutdown();

    program_ = shaders.get(kBulletShaderName);
    if (!program_) {
        if (error) *error = "Missing bullet shader program.";
        return false;
    }

    maxBullets_ = maxBullets;
    vertices_.resize(static_cast<std::size_t>(maxBullets_) * 4U);
    indices_.resize(static_cast<std::size_t>(maxBullets_) * 6U);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    if (vao_ == 0 || vbo_ == 0 || ebo_ == 0) {
        if (error) *error = "OpenGL VAO/VBO/EBO creation failed.";
        shutdown();
        return false;
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(BulletVertex)), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices_.size() * sizeof(GLuint)), nullptr, GL_DYNAMIC_DRAW);

    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(BulletVertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(BulletVertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(BulletVertex, u)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(BulletVertex, r)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return true;
}

void GlBulletRenderer::shutdown() {
    if (ebo_ != 0) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    program_ = nullptr;
    quadCount_ = 0;
    maxBullets_ = 0;
    vertices_.clear();
    indices_.clear();
}

void GlBulletRenderer::buildVertexBuffer(
    const ProjectileSoAView& view,
    const Camera2D& camera,
    const GrayscaleSpriteAtlas& atlas,
    const PaletteRampTexture& paletteRamp,
    const BulletPaletteTable& paletteTable,
    const GradientAnimator& gradientAnimator,
    const float simClock
) {
    (void)view.velX;
    (void)view.velY;
    if (!initialized()) return;

    quadCount_ = 0;
    const auto appendQuad = [this](
                                const float x,
                                const float y,
                                const float drawRadius,
                                const SpriteAtlasRegion& uv,
                                const std::array<float, 4>& color) {
        if (quadCount_ >= maxBullets_) return;
        const float left = x - drawRadius;
        const float right = x + drawRadius;
        const float top = y - drawRadius;
        const float bottom = y + drawRadius;

        const std::size_t v = static_cast<std::size_t>(quadCount_) * 4U;
        const std::size_t i = static_cast<std::size_t>(quadCount_) * 6U;
        vertices_[v + 0] = {left, top, uv.u0, uv.v0, color[0], color[1], color[2], color[3]};
        vertices_[v + 1] = {right, top, uv.u1, uv.v0, color[0], color[1], color[2], color[3]};
        vertices_[v + 2] = {right, bottom, uv.u1, uv.v1, color[0], color[1], color[2], color[3]};
        vertices_[v + 3] = {left, bottom, uv.u0, uv.v1, color[0], color[1], color[2], color[3]};

        const GLuint base = static_cast<GLuint>(v);
        indices_[i + 0] = base + 0U;
        indices_[i + 1] = base + 1U;
        indices_[i + 2] = base + 2U;
        indices_[i + 3] = base + 2U;
        indices_[i + 4] = base + 3U;
        indices_[i + 5] = base + 0U;
        ++quadCount_;
    };

    const std::size_t soaSize = std::min({
        view.posX.size(),
        view.posY.size(),
        view.radius.size(),
        view.life.size(),
        view.paletteIndex.size(),
        view.shape.size(),
        view.active.size(),
        view.allegiance.size(),
        view.enableTrails.size(),
        view.trailHead.size(),
    });

    for (std::uint32_t j = 0; j < view.activeCount && j < view.activeIndices.size(); ++j) {
        const std::uint32_t i = view.activeIndices[j];
        if (i >= maxBullets_ || i >= soaSize) continue;
        if (view.active[i] == 0) continue;

        const std::uint8_t palette = view.paletteIndex[i];
        std::array<float, 4> color;
        if (palette > 0) {
            const PaletteAnimationSettings& anim = paletteRamp.animationFor(palette);
            if (anim.mode != PaletteAnimationMode::None) {
                color = toLinearColor(gradientAnimator.sample(palette, simClock, view.life[i], i));
            } else {
                color = toLinearColor(paletteTable.get(palette).core);
            }
        } else {
            const bool enemy = view.allegiance[i] == static_cast<std::uint8_t>(ProjectileAllegiance::Enemy);
            color = enemy ? std::array<float, 4> {1.0F, 220.0F / 255.0F, 120.0F / 255.0F, 220.0F / 255.0F}
                          : std::array<float, 4> {120.0F / 255.0F, 220.0F / 255.0F, 1.0F, 220.0F / 255.0F};
        }

        const SpriteAtlasRegion uv = atlas.region(decodeShape(view.shape[i]));
        const Vec2 screen = camera.worldToScreen({view.posX[i], view.posY[i]});
        const float screenRadius = view.radius[i] * camera.zoom();

        if (view.enableTrails[i] != 0 && view.trailLength > 0) {
            const std::uint32_t trailBase = i * view.trailLength;
            if (trailBase + view.trailLength > view.trailX.size() || trailBase + view.trailLength > view.trailY.size()) {
                continue;
            }
            for (std::uint8_t t = 0; t < view.trailLength; ++t) {
                const std::uint8_t idx = static_cast<std::uint8_t>((view.trailHead[i] + t) % view.trailLength);
                const float tx = view.trailX[trailBase + idx];
                const float ty = view.trailY[trailBase + idx];
                if (tx == 0.0F && ty == 0.0F) continue;

                const float alpha = std::min(0.15F + 0.15F * static_cast<float>(t), 1.0F);
                const float scale = 0.6F + 0.1F * static_cast<float>(t);
                std::array<float, 4> trailColor;
                if (palette == 0) {
                    trailColor = color;
                } else {
                    trailColor = toLinearColor(paletteTable.get(palette).trail);
                }
                trailColor[3] = alpha;

                const Vec2 trailScreen = camera.worldToScreen({tx, ty});
                appendQuad(trailScreen.x, trailScreen.y, screenRadius * scale, uv, trailColor);
            }
        }

        appendQuad(screen.x, screen.y, screenRadius, uv, color);
    }
}

void GlBulletRenderer::render(
    const Camera2D& camera,
    const float simClock,
    const GrayscaleSpriteAtlas& atlas,
    const PaletteRampTexture& paletteRamp,
    const int viewportWidth,
    const int viewportHeight
) {
    (void)camera;
    if (!initialized() || quadCount_ == 0) return;
    if (viewportWidth <= 0 || viewportHeight <= 0) return;

    const float left = 0.0F;
    const float right = static_cast<float>(viewportWidth);
    const float top = 0.0F;
    const float bottom = static_cast<float>(viewportHeight);
    const float nearPlane = -1.0F;
    const float farPlane = 1.0F;

    const float ortho[16] = {
        2.0F / (right - left), 0.0F, 0.0F, 0.0F,
        0.0F, 2.0F / (top - bottom), 0.0F, 0.0F,
        0.0F, 0.0F, -2.0F / (farPlane - nearPlane), 0.0F,
        -((right + left) / (right - left)),
        -((top + bottom) / (top - bottom)),
        -((farPlane + nearPlane) / (farPlane - nearPlane)),
        1.0F,
    };

    glUseProgram(program_->id);
    glUniformMatrix4fv(program_->locViewProj, 1, GL_FALSE, ortho);
    glUniform1f(program_->locTime, simClock);
    glUniform1f(program_->locEmissiveBoost, 1.2F);
    if (program_->locSpriteAtlas >= 0) glUniform1i(program_->locSpriteAtlas, 0);
    if (program_->locPaletteRamp >= 0) glUniform1i(program_->locPaletteRamp, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas.textureId());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, paletteRamp.textureId());

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(quadCount_ * 4U * sizeof(BulletVertex)), vertices_.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(quadCount_ * 6U * sizeof(GLuint)), indices_.data());

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(quadCount_ * 6U), GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

} // namespace engine
