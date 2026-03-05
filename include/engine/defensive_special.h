#pragma once

#include <engine/traits.h>

#include <cstdint>

namespace engine {

struct TimeDilationPreset {
    float enemyScale {0.40F};
    float playerProjectileScale {0.70F};
};

struct TimeDilationScales {
    float playerMovement {1.0F};
    float playerProjectiles {1.0F};
    float enemyProjectiles {1.0F};
    float enemyMovement {1.0F};
    float cameraScroll {1.0F};
};

class TimeDilationController {
  public:
    void setPreset(TimeDilationPreset preset);
    void setActive(bool active);

    [[nodiscard]] TimeDilationScales scales() const;
    [[nodiscard]] TimeDilationPreset preset() const;

  private:
    TimeDilationPreset preset_ {};
    bool active_ {false};
};

struct DefensiveSpecialConfig {
    float durationSeconds {2.25F};
    float cooldownPerChargeSeconds {12.0F};
    float minCooldownSeconds {6.0F};
    int baseCapacity {1};
    int maxCapacity {3};
    float playerDamageMulWhileActive {0.75F};

    float grazeRechargeSecondsPerPoint {0.06F};
    float damageRechargeSecondsPerPoint {0.05F};
    float killRechargeSecondsPerEvent {0.80F};
    float pickupRechargeSecondsPerEvent {0.60F};

    float grazeBandInnerPadding {4.0F};
    float grazeBandOuterPadding {24.0F};
    std::uint64_t grazeCooldownTicks {30};
};

class DefensiveSpecialSystem {
  public:
    void initialize(const DefensiveSpecialConfig& config, const TimeDilationPreset& preset);
    void setPreset(const TimeDilationPreset& preset);

    void update(float dt, const TraitModifiers& modifiers);
    bool tryActivate();

    void addGrazePoints(std::uint32_t points);
    void addDamagePoints(float damagePoints);
    void addKillEvent(std::uint32_t count = 1);
    void addPickupEvent(std::uint32_t count = 1);

    [[nodiscard]] int capacity() const;
    [[nodiscard]] int currentCharges() const;
    [[nodiscard]] bool active() const;
    [[nodiscard]] float activeTimeRemaining() const;
    [[nodiscard]] float playerDamageMultiplier() const;
    [[nodiscard]] float cooldownProgressForCharge(int chargeIndexFromBack) const;
    [[nodiscard]] TimeDilationScales dilationScales() const;
    [[nodiscard]] std::uint64_t deterministicHash() const;

    [[nodiscard]] const DefensiveSpecialConfig& config() const;

  private:
    void reduceCooldown(float seconds);

    DefensiveSpecialConfig config_ {};
    TimeDilationController dilation_ {};

    int capacity_ {1};
    int charges_ {1};
    float activeRemaining_ {0.0F};
    float rechargeAccumulator_ {0.0F};
};

TimeDilationPreset presetFromLabel(const char* label);

} // namespace engine
