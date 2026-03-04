#include <engine/traits.h>

#include <algorithm>

namespace engine {

namespace {
Trait makeTrait(const char* id, const char* name, TraitRarity rarity, const TraitModifiers& m) {
    return Trait {.id = id, .name = name, .rarity = rarity, .modifiers = m};
}

float rarityWeight(const TraitRarity r) {
    if (r == TraitRarity::Common) return 70.0F;
    if (r == TraitRarity::Rare) return 23.0F;
    return 7.0F;
}

float adjustedRarityWeight(const TraitRarity rarity, const float rareChanceMultiplier) {
    const float base = rarityWeight(rarity);
    if (rarity == TraitRarity::Common) {
        return base / std::max(0.1F, rareChanceMultiplier);
    }
    if (rarity == TraitRarity::Rare) {
        return base * rareChanceMultiplier;
    }
    return base * rareChanceMultiplier * rareChanceMultiplier;
}

TraitModifiers mods() { return TraitModifiers {}; }
} // namespace

void TraitSystem::initialize(const std::uint64_t seed) {
    rng_ = DeterministicRng(seed ^ 0xA11CE55ULL);
    catalog_.clear();

    // 12 functional traits
    {
        auto m = mods(); m.projectileSpeedMul = 1.20F;
        catalog_.push_back(makeTrait("swift-shots", "Swift Shots", TraitRarity::Common, m));
    }
    {
        auto m = mods(); m.projectileRadiusAdd = 0.8F; m.projectileSpeedMul = 0.95F;
        catalog_.push_back(makeTrait("heavy-rounds", "Heavy Rounds", TraitRarity::Common, m));
    }
    {
        auto m = mods(); m.patternCooldownScale = 0.92F;
        catalog_.push_back(makeTrait("tempo-core", "Tempo Core", TraitRarity::Common, m));
    }
    {
        auto m = mods(); m.patternExtraBullets = 2;
        catalog_.push_back(makeTrait("scatter-core", "Scatter Core", TraitRarity::Common, m));
    }
    {
        auto m = mods(); m.patternJitterAddDeg = -0.4F; m.projectileSpeedMul = 1.05F;
        catalog_.push_back(makeTrait("focus-core", "Focus Core", TraitRarity::Rare, m));
    }
    {
        auto m = mods(); m.playerHarvestMultiplier = 1.35F;
        catalog_.push_back(makeTrait("lucky-harvest", "Lucky Harvest", TraitRarity::Common, m));
    }
    {
        auto m = mods(); m.playerRadiusAdd = 4.0F; m.playerHarvestMultiplier = 1.10F;
        catalog_.push_back(makeTrait("vital-shell", "Vital Shell", TraitRarity::Rare, m));
    }
    {
        auto m = mods(); m.enemyFireRateScale = 0.88F; m.projectileSpeedMul = 1.08F;
        catalog_.push_back(makeTrait("overclock", "Overclock", TraitRarity::Rare, m));
    }
    {
        auto m = mods(); m.enemyFireRateScale = 1.12F; m.enemyProjectileSpeedScale = 0.82F;
        catalog_.push_back(makeTrait("jammer", "Jammer", TraitRarity::Epic, m));
    }
    {
        auto m = mods(); m.patternJitterAddDeg = 0.9F; m.patternExtraBullets = 1;
        catalog_.push_back(makeTrait("barrel-spin", "Barrel Spin", TraitRarity::Rare, m));
    }
    {
        auto m = mods(); m.projectileRadiusAdd = 1.4F; m.patternCooldownScale = 0.90F;
        catalog_.push_back(makeTrait("arc-furnace", "Arc Furnace", TraitRarity::Epic, m));
    }
    {
        auto m = mods(); m.enemyProjectileSpeedScale = 1.12F; m.projectileSpeedMul = 1.05F;
        catalog_.push_back(makeTrait("predator-mark", "Predator Mark", TraitRarity::Rare, m));
    }

    active_.clear();
    aggregate_ = {};
    hasPending_ = false;
    rareChanceMultiplier_ = 1.0F;
}

const Trait& TraitSystem::rollOne() {
    float total = 0.0F;
    for (const Trait& t : catalog_) total += adjustedRarityWeight(t.rarity, rareChanceMultiplier_);

    const float r = rng_.nextFloat01() * total;
    float acc = 0.0F;
    for (const Trait& t : catalog_) {
        acc += adjustedRarityWeight(t.rarity, rareChanceMultiplier_);
        if (r <= acc) return t;
    }
    return catalog_.front();
}

std::array<Trait, 3> TraitSystem::rollChoices() {
    for (std::size_t i = 0; i < pending_.size(); ++i) {
        pending_[i] = rollOne();
    }
    hasPending_ = true;
    return pending_;
}

bool TraitSystem::choose(const std::size_t index) {
    if (!hasPending_ || index >= pending_.size()) return false;
    active_.push_back(pending_[index]);
    hasPending_ = false;
    rebuildAggregate();
    return true;
}

void TraitSystem::rebuildAggregate() {
    aggregate_ = {};
    for (const Trait& t : active_) {
        aggregate_.projectileSpeedMul *= t.modifiers.projectileSpeedMul;
        aggregate_.projectileRadiusAdd += t.modifiers.projectileRadiusAdd;
        aggregate_.patternCooldownScale *= t.modifiers.patternCooldownScale;
        aggregate_.patternExtraBullets += t.modifiers.patternExtraBullets;
        aggregate_.patternJitterAddDeg += t.modifiers.patternJitterAddDeg;
        aggregate_.playerRadiusAdd += t.modifiers.playerRadiusAdd;
        aggregate_.playerHarvestMultiplier *= t.modifiers.playerHarvestMultiplier;
        aggregate_.enemyFireRateScale *= t.modifiers.enemyFireRateScale;
        aggregate_.enemyProjectileSpeedScale *= t.modifiers.enemyProjectileSpeedScale;
    }

    aggregate_.projectileSpeedMul = std::clamp(aggregate_.projectileSpeedMul, 0.5F, 3.0F);
    aggregate_.patternCooldownScale = std::clamp(aggregate_.patternCooldownScale, 0.5F, 2.0F);
    aggregate_.enemyFireRateScale = std::clamp(aggregate_.enemyFireRateScale, 0.5F, 2.0F);
    aggregate_.enemyProjectileSpeedScale = std::clamp(aggregate_.enemyProjectileSpeedScale, 0.5F, 2.0F);
}


void TraitSystem::setRareChanceMultiplier(const float multiplier) {
    rareChanceMultiplier_ = std::clamp(multiplier, 0.5F, 3.0F);
}

bool TraitSystem::hasPendingChoices() const { return hasPending_; }

const std::array<Trait, 3>& TraitSystem::pendingChoices() const { return pending_; }

const std::vector<Trait>& TraitSystem::activeTraits() const { return active_; }

const TraitModifiers& TraitSystem::modifiers() const { return aggregate_; }

std::string toString(const TraitRarity rarity) {
    switch (rarity) {
        case TraitRarity::Common: return "Common";
        case TraitRarity::Rare: return "Rare";
        case TraitRarity::Epic: return "Epic";
    }
    return "Unknown";
}

} // namespace engine
