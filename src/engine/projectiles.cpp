#include <engine/projectiles.h>
#include <engine/deterministic_math.h>

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
    const float c = dmath::cos(radians);
    const float s = dmath::sin(radians);
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
    allegiance_.assign(capacity_, static_cast<std::uint8_t>(ProjectileAllegiance::Enemy));
    paletteIndex_.assign(capacity_, 0);
    shape_.assign(capacity_, static_cast<std::uint8_t>(BulletShape::Circle));
    enableTrails_.assign(capacity_, 0);
    trailX_.assign(static_cast<std::size_t>(capacity_) * kTrailLength, 0.0F);
    trailY_.assign(static_cast<std::size_t>(capacity_) * kTrailLength, 0.0F);
    trailHead_.assign(capacity_, 0);
    grazeAwardTick_.assign(capacity_, 0ULL);

    freeList_.clear();
    freeList_.reserve(capacity_);
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        freeList_.push_back(capacity_ - 1U - i);
    }
    activeIndices_.assign(capacity_, 0U);
    indexInActive_.assign(capacity_, 0U);
    activeCount_ = 0;

    gridHead_.assign(static_cast<std::size_t>(gridX_) * gridY_, -1);
    gridNext_.assign(capacity_, -1);
    collisionVisitedStamp_.assign(capacity_, 0U);
    collisionHitStamp_.assign(capacity_, 0U);
    collisionScratch_.assign(capacity_, 0U);
    collisionStampCursor_ = 1U;

    pendingSpawnCount_ = 0;
    despawnEvents_.clear();
    despawnEvents_.reserve(capacity_);

    stats_ = {};
}

void ProjectileSystem::clear() {
    std::fill(active_.begin(), active_.end(), static_cast<std::uint8_t>(0));
    freeList_.clear();
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        freeList_.push_back(capacity_ - 1U - i);
    }
    activeCount_ = 0;
    pendingSpawnCount_ = 0;
    despawnEvents_.clear();
    std::fill(trailX_.begin(), trailX_.end(), 0.0F);
    std::fill(trailY_.begin(), trailY_.end(), 0.0F);
    std::fill(trailHead_.begin(), trailHead_.end(), static_cast<std::uint8_t>(0));
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
    allegiance_[slot] = static_cast<std::uint8_t>(spawnData.allegiance);
    const bool trailsEnabled = spawnData.enableTrails || spawnData.behavior.enableTrails
        || spawnData.allegiance == ProjectileAllegiance::Player;
    enableTrails_[slot] = static_cast<std::uint8_t>(trailsEnabled);
    const std::uint32_t trailBase = slot * kTrailLength;
    for (std::uint8_t t = 0; t < kTrailLength; ++t) {
        trailX_[trailBase + t] = 0.0F;
        trailY_[trailBase + t] = 0.0F;
    }
    trailHead_[slot] = 0;
    grazeAwardTick_[slot] = 0ULL;
    paletteIndex_[slot] = spawnData.paletteIndex;
    shape_[slot] = static_cast<std::uint8_t>(spawnData.shape);
    active_[slot] = 1;
    indexInActive_[slot] = activeCount_;
    activeIndices_[activeCount_] = slot;
    ++activeCount_;
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
            .vel = {dmath::cos(angle) * speed, dmath::sin(angle) * speed},
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
    despawnEvents_.clear();
}

void ProjectileSystem::updateMotion(const float dt, const float enemyTimeScale, const float playerTimeScale) {
    pendingSpawnCount_ = 0;
    bool hadRemovals = false;

    auto deactivateAt = [&](const std::uint32_t bulletIndex) {
        if (!active_[bulletIndex]) return;
        despawnEvents_.push_back(ProjectileDespawnEvent {
            .pos = {posX_[bulletIndex], posY_[bulletIndex]},
            .paletteIndex = paletteIndex_[bulletIndex],
            .allegiance = static_cast<ProjectileAllegiance>(allegiance_[bulletIndex]),
        });
        active_[bulletIndex] = 0;
        freeList_.push_back(bulletIndex);
        const std::uint32_t pos = indexInActive_[bulletIndex];
        const std::uint32_t lastPos = activeCount_ - 1U;
        const std::uint32_t lastBullet = activeIndices_[lastPos];
        activeIndices_[pos] = lastBullet;
        indexInActive_[lastBullet] = pos;
        --activeCount_;
        --stats_.activeCount;
        hadRemovals = true;
    };

    std::uint32_t j = 0;
    while (j < activeCount_) {
        const std::uint32_t i = activeIndices_[j];
        ++stats_.broadphaseChecksThisTick;

        ProjectileBehavior& b = behavior_[i];
        const bool enemyOwned = allegiance_[i] == static_cast<std::uint8_t>(ProjectileAllegiance::Enemy);
        const float localDt = dt * (enemyOwned ? enemyTimeScale : playerTimeScale);

        life_[i] += localDt;

        const float speed = dmath::sqrt(velX_[i] * velX_[i] + velY_[i] * velY_[i]);
        if (b.accelerationPerSec != 0.0F) {
            const float accel = b.accelerationPerSec * localDt;
            if (speed > 0.0001F) {
                const float dirX = velX_[i] / speed;
                const float dirY = velY_[i] / speed;
                velX_[i] += dirX * accel;
                velY_[i] += dirY * accel;
            }
        }
        if (b.dragPerSec > 0.0F) {
            const float drag = std::max(0.0F, 1.0F - b.dragPerSec * localDt);
            velX_[i] *= drag;
            velY_[i] *= drag;
        }

        if (b.curvedAngularVelocityDegPerSec != 0.0F) {
            rotateVec(velX_[i], velY_[i], degToRad(b.curvedAngularVelocityDegPerSec * localDt));
        }

        if (b.homingTurnRateDegPerSec != 0.0F && speed > 0.001F) {
            const float targetAngle = dmath::atan2(-posY_[i], -posX_[i]) * 180.0F / std::numbers::pi_v<float>;
            const float currentAngle = dmath::atan2(velY_[i], velX_[i]) * 180.0F / std::numbers::pi_v<float>;
            const float delta = wrapDegrees(targetAngle - currentAngle);
            const float maxStep = std::max(0.0F, b.homingTurnRateDegPerSec * localDt);
            const float step = std::clamp(delta, -maxStep, maxStep);
            rotateVec(velX_[i], velY_[i], degToRad(step));
        }

        if (b.splitCount > 0 && splitDone_[i] == 0 && life_[i] >= b.splitDelaySeconds) {
            const float baseAngle = dmath::atan2(velY_[i], velX_[i]) * 180.0F / std::numbers::pi_v<float>;
            const float speedNow = dmath::sqrt(velX_[i] * velX_[i] + velY_[i] * velY_[i]);
            const std::uint8_t splitCount = b.splitCount;
            const float spread = b.splitAngleSpreadDeg;
            for (std::uint8_t c = 0; c < splitCount; ++c) {
                const float t = splitCount > 1 ? static_cast<float>(c) / static_cast<float>(splitCount - 1U) : 0.5F;
                const float off = (t - 0.5F) * spread;
                const float a = degToRad(baseAngle + off);
                if (pendingSpawnCount_ < kMaxPendingSpawns) {
                    pendingSpawns_[pendingSpawnCount_++] = ProjectileSpawn {
                    .pos = {posX_[i], posY_[i]},
                    .vel = {dmath::cos(a) * speedNow, dmath::sin(a) * speedNow},
                    .radius = radius_[i] * 0.9F,
                    .behavior = ProjectileBehavior {},
                    .allegiance = static_cast<ProjectileAllegiance>(allegiance_[i]),
                    .paletteIndex = paletteIndex_[i],
                    .shape = static_cast<BulletShape>(shape_[i]),
                    .enableTrails = enableTrails_[i] != 0,
                };
                }
            }
            splitDone_[i] = 1;
            deactivateAt(i);
            continue;
        }

        if (enableTrails_[i] != 0) {
            const std::uint32_t trailBase = i * kTrailLength;
            trailX_[trailBase + trailHead_[i]] = posX_[i];
            trailY_[trailBase + trailHead_[i]] = posY_[i];
            trailHead_[i] = static_cast<std::uint8_t>((trailHead_[i] + 1U) % kTrailLength);
        }

        posX_[i] += velX_[i] * localDt;
        posY_[i] += velY_[i] * localDt;

        bool deactivate = false;
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

        if (deactivate || (b.beamSegmentSamples > 1 && b.beamDurationSeconds > 0.0F && life_[i] > b.beamDurationSeconds)) {
            deactivateAt(i);
            continue;
        }

        ++j;
    }

    for (std::uint32_t si = 0; si < pendingSpawnCount_; ++si) {
        (void)spawn(pendingSpawns_[si]);
    }

    if (hadRemovals) {
        std::sort(activeIndices_.begin(), activeIndices_.begin() + static_cast<std::ptrdiff_t>(activeCount_));
        for (std::uint32_t idx = 0; idx < activeCount_; ++idx) {
            indexInActive_[activeIndices_[idx]] = idx;
        }
    }
}


void ProjectileSystem::buildGrid() {
    std::fill(gridHead_.begin(), gridHead_.end(), -1);
    std::fill(gridNext_.begin(), gridNext_.end(), -1);
    for (std::uint32_t j = 0; j < activeCount_; ++j) {
        const std::uint32_t i = activeIndices_[j];
        const std::uint32_t cell = gridIndexFor(posX_[i], posY_[i]);
        gridNext_[i] = gridHead_[cell];
        gridHead_[cell] = static_cast<int>(i);
    }
}

void ProjectileSystem::resolveCollisions(const std::span<const CollisionTarget> targets, std::vector<CollisionEvent>& outEvents) {
    outEvents.clear();
    if (targets.empty()) return;
    ++collisionStampCursor_;
    if (collisionStampCursor_ == 0U) {
        std::fill(collisionVisitedStamp_.begin(), collisionVisitedStamp_.end(), 0U);
        std::fill(collisionHitStamp_.begin(), collisionHitStamp_.end(), 0U);
        collisionStampCursor_ = 1U;
    }

    const float minCoord = -worldHalfExtent_;
    const float size = worldHalfExtent_ * 2.0F;
    for (const CollisionTarget& target : targets) {
        const float minX = target.pos.x - target.radius;
        const float maxX = target.pos.x + target.radius;
        const float minY = target.pos.y - target.radius;
        const float maxY = target.pos.y + target.radius;
        const int gx0 = std::clamp(static_cast<int>(((minX - minCoord) / size) * static_cast<float>(gridX_)), 0, static_cast<int>(gridX_) - 1);
        const int gx1 = std::clamp(static_cast<int>(((maxX - minCoord) / size) * static_cast<float>(gridX_)), 0, static_cast<int>(gridX_) - 1);
        const int gy0 = std::clamp(static_cast<int>(((minY - minCoord) / size) * static_cast<float>(gridY_)), 0, static_cast<int>(gridY_) - 1);
        const int gy1 = std::clamp(static_cast<int>(((maxY - minCoord) / size) * static_cast<float>(gridY_)), 0, static_cast<int>(gridY_) - 1);

        std::size_t scratchCount = 0;
        for (int gy = gy0; gy <= gy1; ++gy) {
            for (int gx = gx0; gx <= gx1; ++gx) {
                const std::uint32_t cell = static_cast<std::uint32_t>(gy) * gridX_ + static_cast<std::uint32_t>(gx);
                for (int n = gridHead_[cell]; n >= 0; n = gridNext_[static_cast<std::size_t>(n)]) {
                    const std::uint32_t i = static_cast<std::uint32_t>(n);
                    if (!active_[i]) continue;
                    if (collisionVisitedStamp_[i] == collisionStampCursor_) continue;
                    collisionVisitedStamp_[i] = collisionStampCursor_;
                    collisionScratch_[scratchCount++] = i;
                }
            }
        }

        for (std::size_t k = 0; k < scratchCount; ++k) {
            const std::uint32_t i = collisionScratch_[k];
            ++stats_.narrowphaseChecksThisTick;
            if (target.team == 0U && allegiance_[i] == static_cast<std::uint8_t>(ProjectileAllegiance::Player)) continue;
            if (target.team == 1U && allegiance_[i] == static_cast<std::uint8_t>(ProjectileAllegiance::Enemy)) continue;

            const float dx = posX_[i] - target.pos.x;
            const float dy = posY_[i] - target.pos.y;
            const float rr = radius_[i] + target.radius;
            if (dx * dx + dy * dy > rr * rr) continue;
            if (collisionHitStamp_[i] == collisionStampCursor_) continue;
            collisionHitStamp_[i] = collisionStampCursor_;
            outEvents.push_back(CollisionEvent {.bulletIndex = i, .targetId = target.id, .graze = false});
        }
    }

    std::sort(outEvents.begin(), outEvents.end(), [](const CollisionEvent& a, const CollisionEvent& b) {
        if (a.targetId != b.targetId) return a.targetId < b.targetId;
        return a.bulletIndex < b.bulletIndex;
    });

    for (const CollisionEvent& e : outEvents) {
        if (e.bulletIndex >= capacity_ || !active_[e.bulletIndex]) continue;
        ++stats_.collisionsThisTick;
        ++stats_.totalCollisions;
        despawnEvents_.push_back(ProjectileDespawnEvent {
            .pos = {posX_[e.bulletIndex], posY_[e.bulletIndex]},
            .paletteIndex = paletteIndex_[e.bulletIndex],
            .allegiance = static_cast<ProjectileAllegiance>(allegiance_[e.bulletIndex]),
        });
    }
}

void ProjectileSystem::update(const float dt, const Vec2 playerPos, const float playerRadius, const float enemyTimeScale, const float playerTimeScale) {
    updateMotion(dt, enemyTimeScale, playerTimeScale);
    buildGrid();
    CollisionTarget playerTarget {.pos = playerPos, .radius = playerRadius, .id = 0U, .team = 0U};
    std::vector<CollisionEvent> events;
    events.reserve(capacity_);
    resolveCollisions(std::span<const CollisionTarget>(&playerTarget, 1), events);
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
        for (std::uint32_t j = 0; j < activeCount_; ++j) {
            const std::uint32_t i = activeIndices_[j];
            draw.circle({posX_[i], posY_[i]}, radius_[i], Color {250, 90, 90, 180}, 12);
        }
    }
}

void ProjectileSystem::render(SpriteBatch& batch, const std::string& textureId, const BulletPaletteTable& paletteTable) const {
    for (std::uint32_t j = 0; j < activeCount_; ++j) {
        const std::uint32_t i = activeIndices_[j];
        const float d = radius_[i] * 2.0F;
        const Color bulletColor = paletteIndex_[i] == 0
            ? (allegiance_[i] == static_cast<std::uint8_t>(ProjectileAllegiance::Enemy) ? Color {255, 220, 120, 220} : Color {120, 220, 255, 220})
            : paletteTable.get(paletteIndex_[i]).core;

        if (enableTrails_[i] != 0) {
            const std::uint32_t trailBase = i * kTrailLength;
            for (std::uint8_t t = 0; t < kTrailLength; ++t) {
                const std::uint8_t idx = static_cast<std::uint8_t>((trailHead_[i] + t) % kTrailLength);
                const float tx = trailX_[trailBase + idx];
                const float ty = trailY_[trailBase + idx];
                if (tx == 0.0F && ty == 0.0F) continue;

                const float alpha = 0.15F + 0.15F * static_cast<float>(t);
                const float scale = 0.6F + 0.1F * static_cast<float>(t);
                const float trailR = radius_[i] * scale;
                const float trailD = trailR * 2.0F;

                Color trailColor = paletteIndex_[i] == 0 ? bulletColor : paletteTable.get(paletteIndex_[i]).trail;
                trailColor.a = static_cast<Uint8>(alpha * 255.0F);

                batch.draw(SpriteDrawCmd {
                    .textureId = textureId,
                    .dest = SDL_FRect {tx - trailR, ty - trailR, trailD, trailD},
                    .src = std::nullopt,
                    .rotationDeg = 0.0F,
                    .color = trailColor,
                });
            }
        }

        batch.draw(SpriteDrawCmd {
            .textureId = textureId,
            .dest = SDL_FRect {posX_[i] - radius_[i], posY_[i] - radius_[i], d, d},
            .src = std::nullopt,
            .rotationDeg = 0.0F,
            .color = bulletColor,
        });
    }
}


void ProjectileSystem::renderProcedural(SpriteBatch& batch, const BulletPaletteTable& paletteTable) const {
    for (std::uint32_t j = 0; j < activeCount_; ++j) {
        const std::uint32_t i = activeIndices_[j];
        const float d = radius_[i] * 2.0F;
        const BulletShape shape = static_cast<BulletShape>(shape_[i]);
        const std::string textureId = bulletTextureId(std::to_string(paletteIndex_[i]), shape);
        const Color bulletColor = paletteIndex_[i] == 0
            ? (allegiance_[i] == static_cast<std::uint8_t>(ProjectileAllegiance::Enemy) ? Color {255, 220, 120, 220} : Color {120, 220, 255, 220})
            : paletteTable.get(paletteIndex_[i]).core;

        if (enableTrails_[i] != 0) {
            const std::uint32_t trailBase = i * kTrailLength;
            for (std::uint8_t t = 0; t < kTrailLength; ++t) {
                const std::uint8_t idx = static_cast<std::uint8_t>((trailHead_[i] + t) % kTrailLength);
                const float tx = trailX_[trailBase + idx];
                const float ty = trailY_[trailBase + idx];
                if (tx == 0.0F && ty == 0.0F) continue;

                const float alpha = 0.15F + 0.15F * static_cast<float>(t);
                const float scale = 0.6F + 0.1F * static_cast<float>(t);
                const float trailR = radius_[i] * scale;
                const float trailD = trailR * 2.0F;

                Color trailColor = paletteIndex_[i] == 0 ? bulletColor : paletteTable.get(paletteIndex_[i]).trail;
                trailColor.a = static_cast<Uint8>(alpha * 255.0F);

                batch.draw(SpriteDrawCmd {
                    .textureId = textureId,
                    .dest = SDL_FRect {tx - trailR, ty - trailR, trailD, trailD},
                    .src = std::nullopt,
                    .rotationDeg = 0.0F,
                    .color = trailColor,
                });
            }
        }

        batch.draw(SpriteDrawCmd {
            .textureId = textureId,
            .dest = SDL_FRect {posX_[i] - radius_[i], posY_[i] - radius_[i], d, d},
            .src = std::nullopt,
            .rotationDeg = 0.0F,
            .color = bulletColor,
        });
    }
}

const ProjectileStats& ProjectileSystem::stats() const { return stats_; }

std::span<const ProjectileDespawnEvent> ProjectileSystem::despawnEvents() const { return despawnEvents_; }

std::uint32_t ProjectileSystem::collectGrazePoints(const Vec2 playerPos, const float playerRadius, const float innerPad, const float outerPad, const std::uint64_t tick, const std::uint64_t cooldownTicks) {
    const float inner = std::max(0.0F, playerRadius + innerPad);
    const float outer = std::max(inner, playerRadius + outerPad);
    const float inner2 = inner * inner;
    const float outer2 = outer * outer;
    std::uint32_t points = 0;

    for (std::uint32_t j = 0; j < activeCount_; ++j) {
        const std::uint32_t i = activeIndices_[j];
        if (allegiance_[i] != static_cast<std::uint8_t>(ProjectileAllegiance::Enemy)) continue;
        const float dx = posX_[i] - playerPos.x;
        const float dy = posY_[i] - playerPos.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 < inner2 || d2 > outer2) continue;
        if (tick < grazeAwardTick_[i] + cooldownTicks) continue;
        grazeAwardTick_[i] = tick;
        ++points;
    }
    return points;
}


std::uint32_t ProjectileSystem::capacity() const { return capacity_; }


std::uint64_t ProjectileSystem::debugStateHash() const {
    std::uint64_t h = 1469598103934665603ULL;
    auto mix = [&h](const std::uint64_t v) {
        h ^= v + 0x9E3779B97F4A7C15ULL + (h << 6U) + (h >> 2U);
    };
    for (std::uint32_t j = 0; j < activeCount_; ++j) {
        const std::uint32_t i = activeIndices_[j];
        const std::uint64_t px = static_cast<std::uint64_t>(std::llround(posX_[i] * 1000.0F));
        const std::uint64_t py = static_cast<std::uint64_t>(std::llround(posY_[i] * 1000.0F));
        const std::uint64_t vx = static_cast<std::uint64_t>(std::llround(velX_[i] * 1000.0F));
        const std::uint64_t vy = static_cast<std::uint64_t>(std::llround(velY_[i] * 1000.0F));
        mix((static_cast<std::uint64_t>(i) << 1U) ^ px ^ (py << 8U) ^ (vx << 16U) ^ (vy << 24U));
        mix(static_cast<std::uint64_t>(bounceCount_[i]) | (static_cast<std::uint64_t>(splitDone_[i]) << 8U) | (static_cast<std::uint64_t>(allegiance_[i]) << 16U));
    }
    mix(activeCount_);
    mix(stats_.activeCount);
    mix(stats_.totalCollisions);
    return h;
}

} // namespace engine
