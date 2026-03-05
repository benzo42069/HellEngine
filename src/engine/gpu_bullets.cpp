#include <engine/gpu_bullets.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace engine {

void GpuBulletSystem::initialize(const std::uint32_t capacity, const float worldHalfExtent) {
    capacity_ = capacity;
    worldHalfExtent_ = worldHalfExtent;
    bullets_.assign(capacity_, GpuBullet {});
    for (GpuBullet& b : bullets_) b.flags = 0U;
    vertices_.clear();
    indices_.clear();
    vertices_.reserve(static_cast<std::size_t>(capacity_) * 4U);
    indices_.reserve(static_cast<std::size_t>(capacity_) * 6U);
}

void GpuBulletSystem::clear() {
    for (GpuBullet& b : bullets_) b.flags = 0U;
    vertices_.clear();
    indices_.clear();
}

bool GpuBulletSystem::emit(const GpuBullet& bullet) {
    for (GpuBullet& b : bullets_) {
        if ((b.flags & 1U) == 0U) {
            b = bullet;
            b.flags |= 1U;
            return true;
        }
    }
    return false;
}

void GpuBulletSystem::update(const float dt) {
    const float clampedDt = std::max(0.0F, dt);
    for (GpuBullet& b : bullets_) {
        if ((b.flags & 1U) == 0U) continue;
        b.lifetime -= clampedDt;
        if (b.lifetime <= 0.0F) {
            b.flags = 0U;
            continue;
        }

        if ((b.flags & 2U) != 0U) {
            const float a = b.angularVelocityDegPerSec * clampedDt * std::numbers::pi_v<float> / 180.0F;
            const float c = std::cos(a);
            const float s = std::sin(a);
            const float nx = b.velX * c - b.velY * s;
            const float ny = b.velX * s + b.velY * c;
            b.velX = nx;
            b.velY = ny;
        }

        b.posX += b.velX * clampedDt;
        b.posY += b.velY * clampedDt;
        if (std::fabs(b.posX) > worldHalfExtent_ || std::fabs(b.posY) > worldHalfExtent_) {
            b.flags = 0U;
        }
    }
}

void GpuBulletSystem::render(SDL_Renderer* renderer, const Camera2D& camera, const TextureResource* texture) {
    if (!renderer || !texture || !texture->texture) return;

    vertices_.clear();
    indices_.clear();

    const std::array<SDL_FPoint, 4> uvs = {
        SDL_FPoint {0.0F, 0.0F}, SDL_FPoint {1.0F, 0.0F}, SDL_FPoint {1.0F, 1.0F}, SDL_FPoint {0.0F, 1.0F}
    };

    int base = 0;
    for (const GpuBullet& b : bullets_) {
        if ((b.flags & 1U) == 0U) continue;

        const float r = std::max(0.5F, b.radius);
        const std::array<Vec2, 4> corners = {
            Vec2 {b.posX - r, b.posY - r}, Vec2 {b.posX + r, b.posY - r}, Vec2 {b.posX + r, b.posY + r}, Vec2 {b.posX - r, b.posY + r}
        };

        const SDL_Color color {
            static_cast<Uint8>((b.colorRgba >> 24U) & 0xFFU),
            static_cast<Uint8>((b.colorRgba >> 16U) & 0xFFU),
            static_cast<Uint8>((b.colorRgba >> 8U) & 0xFFU),
            static_cast<Uint8>(b.colorRgba & 0xFFU)
        };

        for (int i = 0; i < 4; ++i) {
            const Vec2 s = camera.worldToScreen(corners[i]);
            vertices_.push_back(SDL_Vertex {SDL_FPoint {s.x, s.y}, color, uvs[i]});
        }

        indices_.insert(indices_.end(), {base + 0, base + 1, base + 2, base + 2, base + 3, base + 0});
        base += 4;
    }

    if (!vertices_.empty() && !indices_.empty()) {
        SDL_RenderGeometry(renderer, texture->texture, vertices_.data(), static_cast<int>(vertices_.size()), indices_.data(), static_cast<int>(indices_.size()));
    }
}

std::uint32_t GpuBulletSystem::activeCount() const {
    std::uint32_t count = 0;
    for (const GpuBullet& b : bullets_) {
        if ((b.flags & 1U) != 0U) ++count;
    }
    return count;
}

std::uint32_t GpuBulletSystem::capacity() const { return capacity_; }

} // namespace engine
