#pragma once

#include <engine/deterministic_rng.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace engine {

enum class TraitRarity {
    Common,
    Rare,
    Epic,
};

struct TraitModifiers {
    float projectileSpeedMul {1.0F};
    float projectileRadiusAdd {0.0F};

    float patternCooldownScale {1.0F};
    int patternExtraBullets {0};
    float patternJitterAddDeg {0.0F};

    float playerRadiusAdd {0.0F};
    float playerHarvestMultiplier {1.0F};

    float enemyFireRateScale {1.0F};
    float enemyProjectileSpeedScale {1.0F};
};

struct Trait {
    std::string id;
    std::string name;
    TraitRarity rarity {TraitRarity::Common};
    TraitModifiers modifiers {};
};

class TraitSystem {
  public:
    void initialize(std::uint64_t seed);

    [[nodiscard]] std::array<Trait, 3> rollChoices();
    void setRareChanceMultiplier(float multiplier);
    bool choose(std::size_t index);

    [[nodiscard]] bool hasPendingChoices() const;
    [[nodiscard]] const std::array<Trait, 3>& pendingChoices() const;
    [[nodiscard]] const std::vector<Trait>& activeTraits() const;
    [[nodiscard]] const TraitModifiers& modifiers() const;

  private:
    const Trait& rollOne();
    void rebuildAggregate();

    DeterministicRng rng_ {1337};
    std::vector<Trait> catalog_;
    std::vector<Trait> active_;
    TraitModifiers aggregate_ {};

    std::array<Trait, 3> pending_ {};
    bool hasPending_ {false};
    float rareChanceMultiplier_ {1.0F};
};

std::string toString(TraitRarity rarity);

} // namespace engine
