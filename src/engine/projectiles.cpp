#include <engine/projectiles.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace engine {

void ProjectileSystem::initialize(const std::uint32_t capacity, const float worldHalfExtent, const std::uint32_t gridX, const std::uint32_t gridY) {
    capacity_ = capacity;
    worldHalfExtent_ = worldHalfExtent;
    gridX_ = std::max(gridX, 1U);
    gridY_ = std::max(gridY, 1U);

    posX_.assign(capacity_, 0.0F);
    posY_.assign(capacity_, 0.0F);
    velX_.assign(capacity_, 0.0F);
    velY_.assign(capacity_, 0.0F);
    radius_.assign(capacity_, 3.0F);
    active_.assign(capacity_, 0);

    freeList_.clear();
    freeList_.reserve(capacity_);
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        freeList_.push_back(capacity_ - 1U - i);
    }

    gridHead_.assign(static_cast<std::size_t>(gridX_) * gridY_, -1);
    gridNext_.assign(capacity_, -1);

    stats_ = {};
}

void ProjectileSystem::clear() {
    std::fill(active_.begin(), active_.end(), static_cast<std::uint8_t>(0));
    freeList_.clear();
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        freeList_.push_back(capacity_ - 1U - i);
    }
    stats_ = {};
}

bool ProjectileSystem::spawn(const ProjectileSpawn& spawnData) {
    if (freeList_.empty()) return false;

    const std::uint32_t slot = freeList_.back();
    freeList_.pop_back();

    posX_[slot] = spawnData.pos.x;
    posY_[slot] = spawnData.pos.y;
    velX_[slot] = spawnData.vel.x;
    velY_[slot] = spawnData.vel.y;
    radius_[slot] = spawnData.radius;
    active_[slot] = 1;
    ++stats_.activeCount;
    return true;
}

void ProjectileSystem::spawnRadialBurst(const std::uint32_t count, const float speed, const float radius, const std::uint64_t seedOffset) {
    const float baseAngle = static_cast<float>((seedOffset % 360U)) * std::numbers::pi_v<float> / 180.0F;
    for (std::uint32_t i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(count);
        const float angle = baseAngle + t * std::numbers::pi_v<float> * 2.0F;
        spawn(ProjectileSpawn {
            .pos = {0.0F, 0.0F},
            .vel = {std::cos(angle) * speed, std::sin(angle) * speed},
            .radius = radius,
        });
    }
}

std::uint32_t ProjectileSystem::gridIndexFor(const float x, const float y) const {
    const float minX = -worldHalfExtent_;
    const float minY = -worldHalfExtent_;
    const float size = worldHalfExtent_ * 2.0F;

    const int gx = static_cast<int>(((x - minX) / size) * static_cast<float>(gridX_));
    const int gy = static_cast<int>(((y - minY) / size) * static_cast<float>(gridY_));

    const int cx = std::clamp(gx, 0, static_cast<int>(gridX_) - 1);
    const int cy = std::clamp(gy, 0, static_cast<int>(gridY_) - 1);
    return static_cast<std::uint32_t>(cy) * gridX_ + static_cast<std::uint32_t>(cx);
}

void ProjectileSystem::update(const float dt, const Vec2 playerPos, const float playerRadius) {
    stats_.collisionsThisTick = 0;

    std::fill(gridHead_.begin(), gridHead_.end(), -1);

    for (std::uint32_t i = 0; i < capacity_; ++i) {
        if (!active_[i]) continue;

        posX_[i] += velX_[i] * dt;
        posY_[i] += velY_[i] * dt;

        if (std::fabs(posX_[i]) > worldHalfExtent_ + radius_[i] || std::fabs(posY_[i]) > worldHalfExtent_ + radius_[i]) {
            active_[i] = 0;
            freeList_.push_back(i);
            --stats_.activeCount;
            continue;
        }

        const float dx = posX_[i] - playerPos.x;
        const float dy = posY_[i] - playerPos.y;
        const float rr = radius_[i] + playerRadius;
        if (dx * dx + dy * dy <= rr * rr) {
            ++stats_.collisionsThisTick;
            ++stats_.totalCollisions;
        }

        const std::uint32_t cell = gridIndexFor(posX_[i], posY_[i]);
        gridNext_[i] = gridHead_[cell];
        gridHead_[cell] = static_cast<int>(i);
    }
}

void ProjectileSystem::debugDraw(DebugDraw& draw, const bool drawHitboxes, const bool drawGrid) const {
    if (drawGrid) {
        const float min = -worldHalfExtent_;
        const float max = worldHalfExtent_;
        const float stepX = (max - min) / static_cast<float>(gridX_);
        const float stepY = (max - min) / static_cast<float>(gridY_);
        for (std::uint32_t x = 0; x <= gridX_; ++x) {
            const float px = min + stepX * static_cast<float>(x);
            draw.line({px, min}, {px, max}, Color {45, 60, 80, 255});
        }
        for (std::uint32_t y = 0; y <= gridY_; ++y) {
            const float py = min + stepY * static_cast<float>(y);
            draw.line({min, py}, {max, py}, Color {45, 60, 80, 255});
        }
    }

    if (drawHitboxes) {
        for (std::uint32_t i = 0; i < capacity_; ++i) {
            if (!active_[i]) continue;
            draw.circle({posX_[i], posY_[i]}, radius_[i], Color {250, 90, 90, 180}, 12);
        }
    }
}

void ProjectileSystem::render(SpriteBatch& batch, const std::string& textureId) const {
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        if (!active_[i]) continue;
        const float d = radius_[i] * 2.0F;
        batch.draw(SpriteDrawCmd {
            .textureId = textureId,
            .dest = SDL_FRect {posX_[i] - radius_[i], posY_[i] - radius_[i], d, d},
            .src = std::nullopt,
            .rotationDeg = 0.0F,
            .color = Color {255, 220, 120, 220},
        });
    }
}

const ProjectileStats& ProjectileSystem::stats() const { return stats_; }

std::uint32_t ProjectileSystem::capacity() const { return capacity_; }

} // namespace engine
