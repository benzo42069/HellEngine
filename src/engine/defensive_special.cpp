#include <engine/defensive_special.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace engine {

void TimeDilationController::setPreset(const TimeDilationPreset preset) {
    preset_.enemyScale = std::clamp(preset.enemyScale, 0.15F, 0.85F);
    preset_.playerProjectileScale = std::clamp(preset.playerProjectileScale, 0.15F, 0.95F);
}

void TimeDilationController::setActive(const bool active) {
    active_ = active;
}

TimeDilationScales TimeDilationController::scales() const {
    if (!active_) {
        return {};
    }
    return {
        1.0F,
        preset_.playerProjectileScale,
        preset_.enemyScale,
        preset_.enemyScale,
        preset_.enemyScale,
    };
}

TimeDilationPreset TimeDilationController::preset() const {
    return preset_;
}

void DefensiveSpecialSystem::initialize(const DefensiveSpecialConfig& config, const TimeDilationPreset& preset) {
    config_ = config;
    config_.baseCapacity = std::clamp(config_.baseCapacity, 1, config_.maxCapacity);
    config_.maxCapacity = std::clamp(config_.maxCapacity, 1, 3);
    config_.cooldownPerChargeSeconds = std::max(config_.cooldownPerChargeSeconds, 0.1F);
    config_.minCooldownSeconds = std::clamp(config_.minCooldownSeconds, 0.1F, config_.cooldownPerChargeSeconds);

    dilation_.setPreset(preset);
    dilation_.setActive(false);

    capacity_ = config_.baseCapacity;
    charges_ = capacity_;
    activeRemaining_ = 0.0F;
    rechargeAccumulator_ = 0.0F;
}

void DefensiveSpecialSystem::setPreset(const TimeDilationPreset& preset) {
    dilation_.setPreset(preset);
}

void DefensiveSpecialSystem::update(const float dt, const TraitModifiers& modifiers) {
    const int upgradedCap = std::clamp(config_.baseCapacity + modifiers.defensiveSpecialCapacityAdd, 1, config_.maxCapacity);
    capacity_ = upgradedCap;
    charges_ = std::clamp(charges_, 0, capacity_);

    if (activeRemaining_ > 0.0F) {
        activeRemaining_ = std::max(0.0F, activeRemaining_ - dt);
        if (activeRemaining_ <= 0.0F) {
            dilation_.setActive(false);
        }
    }

    const float cooldownScale = std::clamp(1.0F - modifiers.defensiveSpecialCooldownReduction, 0.1F, 1.0F);
    const float effectiveCooldown = std::max(config_.minCooldownSeconds, config_.cooldownPerChargeSeconds * cooldownScale);

    if (charges_ < capacity_) {
        rechargeAccumulator_ += dt;
        while (rechargeAccumulator_ >= effectiveCooldown && charges_ < capacity_) {
            rechargeAccumulator_ -= effectiveCooldown;
            ++charges_;
        }
    } else {
        rechargeAccumulator_ = 0.0F;
    }
}

bool DefensiveSpecialSystem::tryActivate() {
    if (charges_ <= 0 || activeRemaining_ > 0.0F) return false;

    --charges_;
    activeRemaining_ = std::max(0.01F, config_.durationSeconds);
    dilation_.setActive(true);
    return true;
}

void DefensiveSpecialSystem::reduceCooldown(const float seconds) {
    if (charges_ >= capacity_) return;
    rechargeAccumulator_ += std::max(seconds, 0.0F);
}

void DefensiveSpecialSystem::addGrazePoints(const std::uint32_t points) {
    reduceCooldown(static_cast<float>(points) * config_.grazeRechargeSecondsPerPoint);
}

void DefensiveSpecialSystem::addDamagePoints(const float damagePoints) {
    reduceCooldown(std::max(damagePoints, 0.0F) * config_.damageRechargeSecondsPerPoint);
}

void DefensiveSpecialSystem::addKillEvent(const std::uint32_t count) {
    reduceCooldown(static_cast<float>(count) * config_.killRechargeSecondsPerEvent);
}

void DefensiveSpecialSystem::addPickupEvent(const std::uint32_t count) {
    reduceCooldown(static_cast<float>(count) * config_.pickupRechargeSecondsPerEvent);
}

int DefensiveSpecialSystem::capacity() const { return capacity_; }

int DefensiveSpecialSystem::currentCharges() const { return charges_; }

bool DefensiveSpecialSystem::active() const { return activeRemaining_ > 0.0F; }

float DefensiveSpecialSystem::activeTimeRemaining() const { return activeRemaining_; }

float DefensiveSpecialSystem::playerDamageMultiplier() const {
    return active() ? std::clamp(config_.playerDamageMulWhileActive, 0.1F, 1.0F) : 1.0F;
}

float DefensiveSpecialSystem::cooldownProgressForCharge(const int chargeIndexFromBack) const {
    if (charges_ > chargeIndexFromBack) return 1.0F;
    const float denom = std::max(config_.cooldownPerChargeSeconds, 0.0001F);
    return std::clamp(rechargeAccumulator_ / denom, 0.0F, 1.0F);
}

TimeDilationScales DefensiveSpecialSystem::dilationScales() const { return dilation_.scales(); }

const DefensiveSpecialConfig& DefensiveSpecialSystem::config() const { return config_; }

std::uint64_t DefensiveSpecialSystem::deterministicHash() const {
    std::uint64_t h = 1469598103934665603ULL;
    auto mix = [&h](const std::uint64_t v) {
        h ^= v + 0x9E3779B97F4A7C15ULL + (h << 6U) + (h >> 2U);
    };
    mix(static_cast<std::uint64_t>(capacity_));
    mix(static_cast<std::uint64_t>(charges_));
    mix(static_cast<std::uint64_t>(std::llround(activeRemaining_ * 1000.0F)));
    mix(static_cast<std::uint64_t>(std::llround(rechargeAccumulator_ * 1000.0F)));
    const auto preset = dilation_.preset();
    mix(static_cast<std::uint64_t>(std::llround(preset.enemyScale * 1000.0F)));
    mix(static_cast<std::uint64_t>(std::llround(preset.playerProjectileScale * 1000.0F)));
    return h;
}

TimeDilationPreset presetFromLabel(const char* label) {
    const std::string s = label ? std::string(label) : "standard";
    if (s == "mild") return {0.60F, 0.80F};
    if (s == "strong") return {0.25F, 0.625F};
    return {0.40F, 0.70F};
}

} // namespace engine
