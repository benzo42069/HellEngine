#include <engine/camera_shake.h>

#include <algorithm>
#include <cmath>

namespace engine {

namespace {
constexpr float kTau = 6.28318530717958647692F;
constexpr float kPi = 3.14159265358979323846F;
} // namespace

Vec2 CameraShakeSystem::update(const float dt) {
    Vec2 currentOffset {0.0F, 0.0F};
    const float step = std::clamp(dt, 0.0F, 0.25F);

    for (ActiveShake& shake : shakes_) {
        if (!shake.active) continue;
        shake.elapsed += step;

        if (shake.params.profile != ShakeProfile::Ambient && shake.params.duration > 0.0F && shake.elapsed > shake.params.duration) {
            shake.active = false;
            continue;
        }

        const Vec2 local = profileOffset(shake);
        currentOffset.x += local.x;
        currentOffset.y += local.y;
    }

    currentOffset.x = std::clamp(currentOffset.x, -kMaxAxisOffset, kMaxAxisOffset);
    currentOffset.y = std::clamp(currentOffset.y, -kMaxAxisOffset, kMaxAxisOffset);
    return currentOffset;
}

void CameraShakeSystem::trigger(const ShakeParams& params) {
    ActiveShake active;
    active.params = params;
    active.params.amplitude = std::max(0.0F, params.amplitude);
    active.params.duration = std::max(0.0F, params.duration);
    active.params.frequency = std::max(0.0F, params.frequency);
    active.params.damping = std::max(0.0F, params.damping);
    active.params.direction = normalized(params.direction);
    active.elapsed = 0.0F;
    active.active = active.params.amplitude > 0.0F;

    for (ActiveShake& slot : shakes_) {
        if (!slot.active) {
            slot = active;
            return;
        }
    }

    shakes_[nextReplaceIndex_] = active;
    nextReplaceIndex_ = (nextReplaceIndex_ + 1U) % shakes_.size();
}

void CameraShakeSystem::clear() {
    for (ActiveShake& shake : shakes_) shake.active = false;
}

Vec2 CameraShakeSystem::normalized(const Vec2 v) {
    const float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len <= 0.0001F) return {0.0F, 0.0F};
    return {v.x / len, v.y / len};
}

float CameraShakeSystem::noise(const float value) {
    const float hash = std::sin(value * 12.9898F + 78.233F) * 43758.5453F;
    return (hash - std::floor(hash)) * 2.0F - 1.0F;
}

Vec2 CameraShakeSystem::profileOffset(const ActiveShake& shake) {
    const ShakeParams& p = shake.params;
    const float t = shake.elapsed;
    const float safeDuration = std::max(0.0001F, p.duration);

    switch (p.profile) {
        case ShakeProfile::Impact: {
            const Vec2 dir = normalized(p.direction);
            const float wave = std::sin(t * p.frequency * kTau);
            const float decay = std::exp(-p.damping * t);
            const float scale = p.amplitude * wave * decay;
            return {dir.x * scale, dir.y * scale};
        }
        case ShakeProfile::BossRumble: {
            const float life = std::clamp(1.0F - (t / safeDuration), 0.0F, 1.0F);
            return {std::sin(t * 8.0F) * p.amplitude * life, std::cos(t * 6.0F) * p.amplitude * life};
        }
        case ShakeProfile::GrazeTremor: {
            const float sample = t * 120.0F;
            const float decay = std::exp(-p.damping * t);
            return {noise(sample) * p.amplitude * decay, noise(sample + 99.0F) * p.amplitude * decay};
        }
        case ShakeProfile::SpecialPulse: {
            const float envelope = std::sin(kPi * std::clamp(t / safeDuration, 0.0F, 1.0F));
            return {std::sin(t * 12.0F) * p.amplitude * envelope, std::cos(t * 12.0F) * p.amplitude * envelope};
        }
        case ShakeProfile::Explosion: {
            const Vec2 dir = normalized(p.direction);
            const float decay = std::exp(-(p.damping * 2.0F) * t);
            const float jitterScale = p.amplitude * 0.15F * std::exp(-p.damping * t);
            return {
                dir.x * p.amplitude * decay + noise(t * 91.0F) * jitterScale,
                dir.y * p.amplitude * decay + noise(t * 91.0F + 19.0F) * jitterScale,
            };
        }
        case ShakeProfile::Ambient: {
            return {std::sin(t * 2.3F) * p.amplitude, std::sin(t * 1.7F) * p.amplitude};
        }
    }

    return {0.0F, 0.0F};
}

} // namespace engine
