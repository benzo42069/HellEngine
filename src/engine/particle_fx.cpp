#include <engine/particle_fx.h>

#include <engine/deterministic_math.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace engine {

void ParticleFxSystem::initialize(const std::uint32_t capacity) {
    capacity_ = capacity;
    particles_.assign(capacity_, Particle {});
    activeCount_ = 0;
    spawnCursor_ = 0;
}

void ParticleFxSystem::burst(const Vec2 pos, const Color color, const std::uint8_t count, const float speed) {
    if (capacity_ == 0 || count == 0) return;

    const std::uint8_t clampedCount = static_cast<std::uint8_t>(std::max<std::uint8_t>(1U, count));
    for (std::uint8_t i = 0; i < clampedCount; ++i) {
        if (activeCount_ >= capacity_) {
            std::uint32_t oldest = 0;
            std::uint32_t oldestSpawnId = particles_[0].spawnId;
            for (std::uint32_t p = 1; p < activeCount_; ++p) {
                if (particles_[p].spawnId < oldestSpawnId) {
                    oldest = p;
                    oldestSpawnId = particles_[p].spawnId;
                }
            }
            particles_[oldest] = particles_[activeCount_ - 1U];
            --activeCount_;
        }

        const float t = static_cast<float>(i) / static_cast<float>(clampedCount);
        const float offset = 0.31F * static_cast<float>((spawnCursor_ + i) % 7U);
        const float angle = t * std::numbers::pi_v<float> * 2.0F + offset;
        const float speedJitter = 0.85F + 0.05F * static_cast<float>((spawnCursor_ + i) % 4U);
        const float life = 0.12F + 0.01F * static_cast<float>((spawnCursor_ + i) % 7U);

        Particle& particle = particles_[activeCount_++];
        particle.x = pos.x;
        particle.y = pos.y;
        particle.vx = dmath::cos(angle) * speed * speedJitter;
        particle.vy = dmath::sin(angle) * speed * speedJitter;
        particle.life = life;
        particle.maxLife = life;
        particle.color = color;
        particle.spawnId = spawnCursor_++;
    }
}

void ParticleFxSystem::update(const float frameDelta) {
    if (frameDelta <= 0.0F) return;

    std::uint32_t i = 0;
    while (i < activeCount_) {
        Particle& particle = particles_[i];
        particle.x += particle.vx * frameDelta;
        particle.y += particle.vy * frameDelta;
        particle.life -= frameDelta;

        if (particle.life <= 0.0F) {
            particles_[i] = particles_[activeCount_ - 1U];
            --activeCount_;
            continue;
        }

        const float t = std::clamp(particle.life / particle.maxLife, 0.0F, 1.0F);
        particle.color.a = static_cast<Uint8>(std::clamp(std::lround(t * 255.0F), 0L, 255L));
        ++i;
    }
}

void ParticleFxSystem::render(SpriteBatch& batch, const std::string& textureId) const {
    constexpr float kParticleSize = 3.0F;
    constexpr float kHalf = kParticleSize * 0.5F;

    for (std::uint32_t i = 0; i < activeCount_; ++i) {
        const Particle& particle = particles_[i];
        batch.draw(SpriteDrawCmd {
            .textureId = textureId,
            .dest = SDL_FRect {particle.x - kHalf, particle.y - kHalf, kParticleSize, kParticleSize},
            .src = std::nullopt,
            .rotationDeg = 0.0F,
            .color = particle.color,
        });
    }
}

void ParticleFxSystem::clear() {
    activeCount_ = 0;
}

} // namespace engine
