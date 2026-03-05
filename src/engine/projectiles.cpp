#include <engine/projectiles.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace engine {

namespace {
float degToRad(const float deg) { return deg * std::numbers::pi_v<float> / 180.0F; }

float wrapDegrees(float deg) {
    while (deg > 180.0F) deg -= 360.0F;
    while (deg < -180.0F) deg += 360.0F;
    return deg;
}

void rotateVec(float& x, float& y, const float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    const float nx = x * c - y * s;
    const float ny = x * s + y * c;
    x = nx;
    y = ny;
}
} // namespace

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

    behavior_.assign(capacity_, ProjectileBehavior {});
    life_.assign(capacity_, 0.0F);
    bounceCount_.assign(capacity_, 0);
    splitDone_.assign(capacity_, 0);

    freeList_.clear();
    freeList_.reserve(capacity_);
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        freeList_.push_back(capacity_ - 1U - i);
    }

    gridHead_.assign(static_cast<std::size_t>(gridX_) * gridY_, -1);
    gridNext_.assign(capacity_, -1);

    pendingSpawns_.clear();
    pendingSpawns_.reserve(capacity_ * 4U);

    stats_ = {};
}

void ProjectileSystem::clear() {
    std::fill(active_.begin(), active_.end(), static_cast<std::uint8_t>(0));
    freeList_.clear();
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        freeList_.push_back(capacity_ - 1U - i);
    }
    pendingSpawns_.clear();
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
    behavior_[slot] = spawnData.behavior;
    life_[slot] = 0.0F;
    bounceCount_[slot] = 0;
    splitDone_[slot] = 0;
    active_[slot] = 1;
    ++stats_.activeCount;
    ++stats_.spawnedThisTick;
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

void ProjectileSystem::beginTick() {
    stats_.collisionsThisTick = 0;
    stats_.spawnedThisTick = 0;
    stats_.broadphaseChecksThisTick = 0;
    stats_.narrowphaseChecksThisTick = 0;
}

void ProjectileSystem::update(const float dt, const Vec2 playerPos, const float playerRadius) {
    std::fill(gridHead_.begin(), gridHead_.end(), -1);
    pendingSpawns_.clear();

    for (std::uint32_t i = 0; i < capacity_; ++i) {
        if (!active_[i]) continue;
        ++stats_.broadphaseChecksThisTick;

        ProjectileBehavior& b = behavior_[i];
        life_[i] += dt;

        // 1) acceleration/drag
        const float speed = std::sqrt(velX_[i] * velX_[i] + velY_[i] * velY_[i]);
        if (speed > 0.001F) {
            float adjustedSpeed = speed + b.accelerationPerSec * dt;
            adjustedSpeed = std::max(0.0F, adjustedSpeed * std::max(0.0F, 1.0F - b.dragPerSec * dt));
            const float scale = adjustedSpeed / speed;
            velX_[i] *= scale;
            velY_[i] *= scale;
        }

        // 2) curved motion
        if (std::fabs(b.curvedAngularVelocityDegPerSec) > 0.001F) {
            rotateVec(velX_[i], velY_[i], degToRad(b.curvedAngularVelocityDegPerSec * dt));
        }

        // 3) homing
        if (b.homingTurnRateDegPerSec > 0.0F && b.homingMaxAngleStepDeg > 0.0F) {
            const float targetDx = playerPos.x - posX_[i];
            const float targetDy = playerPos.y - posY_[i];
            const float targetAng = std::atan2(targetDy, targetDx) * 180.0F / std::numbers::pi_v<float>;
            const float curAng = std::atan2(velY_[i], velX_[i]) * 180.0F / std::numbers::pi_v<float>;
            const float delta = wrapDegrees(targetAng - curAng);
            const float maxStep = std::min(b.homingTurnRateDegPerSec * dt, b.homingMaxAngleStepDeg);
            const float step = std::clamp(delta, -maxStep, maxStep);
            rotateVec(velX_[i], velY_[i], degToRad(step));
        }

        // 4) split
        if (b.splitCount > 0 && splitDone_[i] == 0 && life_[i] >= b.splitDelaySeconds) {
            const float baseAngle = std::atan2(velY_[i], velX_[i]) * 180.0F / std::numbers::pi_v<float>;
            const float speedNow = std::sqrt(velX_[i] * velX_[i] + velY_[i] * velY_[i]);
            const std::uint8_t splitCount = b.splitCount;
            const float spread = b.splitAngleSpreadDeg;
            for (std::uint8_t c = 0; c < splitCount; ++c) {
                const float t = splitCount > 1 ? static_cast<float>(c) / static_cast<float>(splitCount - 1U) : 0.5F;
                const float off = (t - 0.5F) * spread;
                const float a = degToRad(baseAngle + off);
                pendingSpawns_.push_back(ProjectileSpawn {
                    .pos = {posX_[i], posY_[i]},
                    .vel = {std::cos(a) * speedNow, std::sin(a) * speedNow},
                    .radius = radius_[i] * 0.9F,
                    .behavior = ProjectileBehavior {},
                });
            }
            splitDone_[i] = 1;
            active_[i] = 0;
            freeList_.push_back(i);
            --stats_.activeCount;
            continue;
        }

        // 5) move
        posX_[i] += velX_[i] * dt;
        posY_[i] += velY_[i] * dt;

        bool deactivate = false;

        // 6) bounce / out of bounds
        const float bound = worldHalfExtent_;
        if (posX_[i] < -bound || posX_[i] > bound || posY_[i] < -bound || posY_[i] > bound) {
            if (bounceCount_[i] < b.maxBounces) {
                if (posX_[i] < -bound || posX_[i] > bound) velX_[i] = -velX_[i];
                if (posY_[i] < -bound || posY_[i] > bound) velY_[i] = -velY_[i];
                posX_[i] = std::clamp(posX_[i], -bound, bound);
                posY_[i] = std::clamp(posY_[i], -bound, bound);
                ++bounceCount_[i];
            } else {
                deactivate = true;
            }
        }

        // 7) collisions / beams
        ++stats_.narrowphaseChecksThisTick;
        bool hit = false;
        if (b.beamSegmentSamples > 1 && b.beamDurationSeconds > 0.0F && life_[i] <= b.beamDurationSeconds) {
            const float len = std::max(8.0F, radius_[i] * 6.0F);
            const float vx = velX_[i];
            const float vy = velY_[i];
            const float vm = std::max(0.001F, std::sqrt(vx * vx + vy * vy));
            const float dirX = vx / vm;
            const float dirY = vy / vm;
            for (std::uint8_t s = 0; s < b.beamSegmentSamples; ++s) {
                const float t = static_cast<float>(s) / static_cast<float>(std::max<std::uint8_t>(1U, b.beamSegmentSamples - 1U));
                const float sx = posX_[i] + dirX * len * t;
                const float sy = posY_[i] + dirY * len * t;
                const float dx = sx - playerPos.x;
                const float dy = sy - playerPos.y;
                const float rr = radius_[i] + playerRadius;
                if (dx * dx + dy * dy <= rr * rr) {
                    hit = true;
                    break;
                }
            }
            if (life_[i] > b.beamDurationSeconds) deactivate = true;
        } else {
            const float dx = posX_[i] - playerPos.x;
            const float dy = posY_[i] - playerPos.y;
            const float rr = radius_[i] + playerRadius;
            hit = (dx * dx + dy * dy <= rr * rr);
        }

        if (hit) {
            ++stats_.collisionsThisTick;
            ++stats_.totalCollisions;
            if (b.explodeShards > 0 || (b.beamSegmentSamples > 1 && b.beamDurationSeconds > 0.0F)) {
                deactivate = true;
            }
        }

        if (deactivate) {
            if (b.explodeShards > 0 && b.explodeRadius > 0.0F) {
                const float shardSpeed = std::max(20.0F, b.explodeRadius * 12.0F);
                for (std::uint8_t s = 0; s < b.explodeShards; ++s) {
                    const float t = static_cast<float>(s) / static_cast<float>(b.explodeShards);
                    const float a = t * std::numbers::pi_v<float> * 2.0F;
                    pendingSpawns_.push_back(ProjectileSpawn {
                        .pos = {posX_[i], posY_[i]},
                        .vel = {std::cos(a) * shardSpeed, std::sin(a) * shardSpeed},
                        .radius = std::max(1.0F, radius_[i] * 0.55F),
                        .behavior = ProjectileBehavior {},
                    });
                }
            }
            active_[i] = 0;
            freeList_.push_back(i);
            --stats_.activeCount;
            continue;
        }

        const std::uint32_t cell = gridIndexFor(posX_[i], posY_[i]);
        gridNext_[i] = gridHead_[cell];
        gridHead_[cell] = static_cast<int>(i);
    }

    for (const ProjectileSpawn& spawnData : pendingSpawns_) {
        (void)spawn(spawnData);
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


std::uint64_t ProjectileSystem::debugStateHash() const {
    std::uint64_t h = 1469598103934665603ULL;
    auto mix = [&h](const std::uint64_t v) {
        h ^= v + 0x9E3779B97F4A7C15ULL + (h << 6U) + (h >> 2U);
    };
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        if (!active_[i]) continue;
        const std::uint64_t px = static_cast<std::uint64_t>(std::llround(posX_[i] * 1000.0F));
        const std::uint64_t py = static_cast<std::uint64_t>(std::llround(posY_[i] * 1000.0F));
        const std::uint64_t vx = static_cast<std::uint64_t>(std::llround(velX_[i] * 1000.0F));
        const std::uint64_t vy = static_cast<std::uint64_t>(std::llround(velY_[i] * 1000.0F));
        mix((static_cast<std::uint64_t>(i) << 1U) ^ px ^ (py << 8U) ^ (vx << 16U) ^ (vy << 24U));
        mix(static_cast<std::uint64_t>(bounceCount_[i]) | (static_cast<std::uint64_t>(splitDone_[i]) << 8U));
    }
    mix(stats_.activeCount);
    mix(stats_.totalCollisions);
    return h;
}

} // namespace engine
