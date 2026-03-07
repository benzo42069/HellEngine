#include <engine/gl_bullet_renderer.h>

#include <algorithm>
#include <cstddef>
#include <cmath>

namespace engine {

bool GlBulletRenderer::initialize(
    ShaderCache& shaders,
    const GrayscaleSpriteAtlas& atlas,
    const PaletteRampTexture& paletteRamp,
    const std::uint32_t maxBullets,
    std::string* error
) {
    shutdown();

    program_ = shaders.get("bullet");
    if (!program_) {
        if (error) *error = "Missing bullet shader program.";
        return false;
    }
    if (atlas.textureId() == 0 || paletteRamp.textureId() == 0) {
        if (error) *error = "Bullet atlas or palette ramp texture is not initialized.";
        return false;
    }

    atlas_ = &atlas;
    paletteRamp_ = &paletteRamp;
    maxBullets_ = maxBullets;
    vertices_.resize(static_cast<std::size_t>(maxBullets_) * 4U);
    indices_.resize(static_cast<std::size_t>(maxBullets_) * 6U);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    if (vao_ == 0 || vbo_ == 0 || ebo_ == 0) {
        if (error) *error = "OpenGL buffer/VAO creation failed.";
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
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(BulletVertex, paletteRow)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(BulletVertex, age)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(BulletVertex, instanceId)));

    glBindVertexArray(0);
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
    atlas_ = nullptr;
    paletteRamp_ = nullptr;
    quadCount_ = 0;
    maxBullets_ = 0;
    vertices_.clear();
    indices_.clear();
}

void GlBulletRenderer::buildVertexBuffer(
    const float* posX,
    const float* posY,
    const float* velX,
    const float* velY,
    const float* radius,
    const float* life,
    const std::uint8_t* paletteIndex,
    const std::uint8_t* active,
    const std::uint32_t* activeIndices,
    const std::uint32_t activeCount,
    const std::uint32_t capacity,
    const BulletShape defaultShape
) {
    if (!atlas_ || !program_) return;

    quadCount_ = 0;
    const std::uint32_t sourceCount = activeIndices ? activeCount : capacity;
    const std::uint32_t count = std::min(sourceCount, maxBullets_);

    const SpriteAtlasRegion uv = atlas_->region(defaultShape);
    for (std::uint32_t j = 0; j < count; ++j) {
        const std::uint32_t i = activeIndices ? activeIndices[j] : j;
        if (i >= capacity || !active[i]) continue;

        const float px = posX[i];
        const float py = posY[i];
        const float r = radius[i];

        float dirX = velX[i];
        float dirY = velY[i];
        const float len = std::sqrt((dirX * dirX) + (dirY * dirY));
        if (len > 0.0001F) {
            dirX /= len;
            dirY /= len;
        } else {
            dirX = 1.0F;
            dirY = 0.0F;
        }

        const float tangentX = -dirY;
        const float tangentY = dirX;

        const float hx = dirX * r;
        const float hy = dirY * r;
        const float tx = tangentX * r;
        const float ty = tangentY * r;

        const std::size_t vBase = static_cast<std::size_t>(quadCount_) * 4U;
        const std::size_t iBase = static_cast<std::size_t>(quadCount_) * 6U;

        const float rowV = (paletteIndex && paletteRamp_) ? paletteRamp_->rowV(paletteIndex[i]) : 0.5F;
        vertices_[vBase + 0] = BulletVertex {px - hx - tx, py - hy - ty, uv.u0, uv.v1, rowV, life ? life[i] : 0.0F, static_cast<float>(i)};
        vertices_[vBase + 1] = BulletVertex {px + hx - tx, py + hy - ty, uv.u1, uv.v1, rowV, life ? life[i] : 0.0F, static_cast<float>(i)};
        vertices_[vBase + 2] = BulletVertex {px + hx + tx, py + hy + ty, uv.u1, uv.v0, rowV, life ? life[i] : 0.0F, static_cast<float>(i)};
        vertices_[vBase + 3] = BulletVertex {px - hx + tx, py - hy + ty, uv.u0, uv.v0, rowV, life ? life[i] : 0.0F, static_cast<float>(i)};

        const GLuint base = static_cast<GLuint>(quadCount_ * 4U);
        indices_[iBase + 0] = base + 0U;
        indices_[iBase + 1] = base + 1U;
        indices_[iBase + 2] = base + 2U;
        indices_[iBase + 3] = base + 2U;
        indices_[iBase + 4] = base + 3U;
        indices_[iBase + 5] = base + 0U;
        ++quadCount_;
    }
}

void GlBulletRenderer::render(
    const Camera2D& camera,
    const float simTime,
    const PaletteRampTexture& paletteRamp,
    const int viewportWidth,
    const int viewportHeight
) {
    if (!program_ || !atlas_ || vao_ == 0 || quadCount_ == 0) return;

    frameAnim_ = paletteRamp.mostCommonAnimation();

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(quadCount_ * 4U * sizeof(BulletVertex)), vertices_.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(quadCount_ * 6U * sizeof(GLuint)), indices_.data());

    const float halfW = (static_cast<float>(viewportWidth) * 0.5F) / std::max(0.001F, camera.zoom());
    const float halfH = (static_cast<float>(viewportHeight) * 0.5F) / std::max(0.001F, camera.zoom());
    const Vec2 c = camera.center();
    const float l = c.x - halfW;
    const float r = c.x + halfW;
    const float t = c.y - halfH;
    const float b = c.y + halfH;

    const float viewProj[16] = {
        2.0F / (r - l), 0.0F, 0.0F, 0.0F,
        0.0F, 2.0F / (t - b), 0.0F, 0.0F,
        0.0F, 0.0F, -1.0F, 0.0F,
        -((r + l) / (r - l)), -((t + b) / (t - b)), 0.0F, 1.0F,
    };

    glUseProgram(program_->id);
    if (program_->locViewProj >= 0) glUniformMatrix4fv(program_->locViewProj, 1, GL_FALSE, viewProj);
    if (program_->locTime >= 0) glUniform1f(program_->locTime, simTime);
    if (program_->locAnimSpeed >= 0) glUniform1f(program_->locAnimSpeed, frameAnim_.speed);
    if (program_->locPerInstanceOffset >= 0) glUniform2f(program_->locPerInstanceOffset, frameAnim_.perInstanceOffset, 0.11F);
    if (program_->locEmissiveBoost >= 0) glUniform1f(program_->locEmissiveBoost, frameAnim_.emissiveBoost);
    if (program_->locAnimMode >= 0) glUniform1i(program_->locAnimMode, frameAnim_.mode);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas_->textureId());
    if (program_->locSpriteAtlas >= 0) glUniform1i(program_->locSpriteAtlas, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, paletteRamp.textureId());
    if (program_->locPaletteRamp >= 0) glUniform1i(program_->locPaletteRamp, 1);

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(quadCount_ * 6U), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace engine
