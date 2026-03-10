#include <engine/gpu_bullets.h>
#include <engine/deterministic_math.h>

#include <algorithm>
#include <array>
#include <limits>
#include <cmath>
#include <numbers>

namespace engine {

void CpuMassBulletRenderSystem::initialize(const std::uint32_t capacity, const float worldHalfExtent) {
    capacity_ = capacity;
    worldHalfExtent_ = worldHalfExtent;
    bullets_.assign(capacity_, GpuBullet {});
    for (GpuBullet& b : bullets_) b.flags = 0U;
    vertices_.clear();
    indices_.clear();
    vertices_.reserve(static_cast<std::size_t>(capacity_) * 4U);
    indices_.reserve(static_cast<std::size_t>(capacity_) * 6U);
    freeList_.clear();
    freeList_.reserve(capacity_);
    activeSlots_.clear();
    activeSlots_.reserve(capacity_);
    slotToActiveIndex_.assign(capacity_, capacity_);
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        freeList_.push_back(capacity_ - 1U - i);
    }
    activeCount_ = 0;
    preparedQuadCount_ = 0;
}

void CpuMassBulletRenderSystem::clear() {
    for (GpuBullet& b : bullets_) b.flags = 0U;
    vertices_.clear();
    indices_.clear();
    freeList_.clear();
    freeList_.reserve(capacity_);
    activeSlots_.clear();
    std::fill(slotToActiveIndex_.begin(), slotToActiveIndex_.end(), capacity_);
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        freeList_.push_back(capacity_ - 1U - i);
    }
    activeCount_ = 0;
    preparedQuadCount_ = 0;
}

bool CpuMassBulletRenderSystem::emit(const GpuBullet& bullet) {
    if (freeList_.empty()) return false;
    const std::uint32_t slot = freeList_.back();
    freeList_.pop_back();

    GpuBullet& b = bullets_[slot];
    b = bullet;
    b.flags |= 1U;

    slotToActiveIndex_[slot] = static_cast<std::uint32_t>(activeSlots_.size());
    activeSlots_.push_back(slot);
    ++activeCount_;
    return true;
}

void CpuMassBulletRenderSystem::releaseSlot(const std::uint32_t slot) {
    GpuBullet& b = bullets_[slot];
    if ((b.flags & 1U) == 0U) return;
    b.flags = 0U;

    const std::uint32_t activeIndex = slotToActiveIndex_[slot];
    const std::uint32_t lastSlot = activeSlots_.back();
    activeSlots_[activeIndex] = lastSlot;
    slotToActiveIndex_[lastSlot] = activeIndex;
    activeSlots_.pop_back();
    slotToActiveIndex_[slot] = capacity_;

    freeList_.push_back(slot);
    --activeCount_;
}

void CpuMassBulletRenderSystem::update(const float dt) {
    const float clampedDt = std::max(0.0F, dt);
    std::size_t i = 0;
    while (i < activeSlots_.size()) {
        const std::uint32_t slot = activeSlots_[i];
        GpuBullet& b = bullets_[slot];
        b.lifetime -= clampedDt;
        if (b.lifetime <= 0.0F) {
            releaseSlot(slot);
            continue;
        }

        if ((b.flags & 2U) != 0U) {
            const float a = b.angularVelocityDegPerSec * clampedDt * std::numbers::pi_v<float> / 180.0F;
            const float c = dmath::cos(a);
            const float s = dmath::sin(a);
            const float nx = b.velX * c - b.velY * s;
            const float ny = b.velX * s + b.velY * c;
            b.velX = nx;
            b.velY = ny;
        }

        b.posX += b.velX * clampedDt;
        b.posY += b.velY * clampedDt;
        if (std::fabs(b.posX) > worldHalfExtent_ || std::fabs(b.posY) > worldHalfExtent_) {
            releaseSlot(slot);
            continue;
        }
        ++i;
    }
}

void CpuMassBulletRenderSystem::render(SDL_Renderer* renderer, const Camera2D& camera, const TextureResource* texture) {
    if (!renderer || !texture || !texture->texture) return;

    vertices_.clear();
    indices_.clear();
    preparedQuadCount_ = 0;

    const std::array<SDL_FPoint, 4> uvs = {
        SDL_FPoint {0.0F, 0.0F}, SDL_FPoint {1.0F, 0.0F}, SDL_FPoint {1.0F, 1.0F}, SDL_FPoint {0.0F, 1.0F}
    };

    int base = 0;
    for (const std::uint32_t slot : activeSlots_) {
        const GpuBullet& b = bullets_[slot];

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
        ++preparedQuadCount_;
    }

    if (!vertices_.empty() && !indices_.empty()) {
        if (vertices_.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) || indices_.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            return;
        }
        SDL_RenderGeometry(renderer, texture->texture, vertices_.data(), static_cast<int>(vertices_.size()), indices_.data(), static_cast<int>(indices_.size()));
    }
}

std::uint32_t CpuMassBulletRenderSystem::activeCount() const { return activeCount_; }

std::uint32_t CpuMassBulletRenderSystem::capacity() const { return capacity_; }

std::uint32_t CpuMassBulletRenderSystem::preparedQuadCount() const { return preparedQuadCount_; }

} // namespace engine
