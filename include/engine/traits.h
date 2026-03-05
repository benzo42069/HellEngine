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
    Relic,
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
    std::string description;
    std::string iconToken;
    TraitRarity rarity {TraitRarity::Common};
    TraitModifiers modifiers {};
    std::array<std::string, 3> synergyTags {};
    std::uint8_t synergyTagCount {0};
};

struct TraitValidationIssue {
    std::string traitId;
    std::string message;
};

class TraitSystem {
  public:
    static constexpr std::size_t choiceCount = 3;

    void initialize(std::uint64_t seed);

    [[nodiscard]] std::array<Trait, choiceCount> rollChoices();
    [[nodiscard]] bool rerollChoices();
    [[nodiscard]] bool canReroll() const;
    void forcePendingRarity(TraitRarity rarity);
    void onTick(std::uint64_t tick);
    void setRareChanceMultiplier(float multiplier);
    bool choose(std::size_t index);

    [[nodiscard]] bool hasPendingChoices() const;
    [[nodiscard]] const std::array<Trait, choiceCount>& pendingChoices() const;
    [[nodiscard]] const std::vector<Trait>& activeTraits() const;
    [[nodiscard]] const TraitModifiers& modifiers() const;
    [[nodiscard]] std::vector<TraitValidationIssue> validateCatalog() const;
    [[nodiscard]] const std::vector<TraitValidationIssue>& validationIssues() const;
    [[nodiscard]] int rerollCharges() const;
    [[nodiscard]] std::uint64_t rerollCooldownRemainingTicks(std::uint64_t tick) const;

  private:
    const Trait& rollOne();
    void rebuildAggregate();

    DeterministicRng rng_ {1337};
    std::vector<Trait> catalog_;
    std::vector<Trait> active_;
    TraitModifiers aggregate_ {};

    std::array<Trait, choiceCount> pending_ {};
    std::vector<TraitValidationIssue> validationCache_ {};
    bool hasPending_ {false};
    float rareChanceMultiplier_ {1.0F};

    int rerollCharges_ {2};
    std::uint64_t rerollCooldownTicks_ {180};
    std::uint64_t nextRerollTick_ {0};
    std::uint64_t currentTick_ {0};
};

std::string toString(TraitRarity rarity);

} // namespace engine
