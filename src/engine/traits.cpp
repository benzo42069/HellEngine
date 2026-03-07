#include <engine/traits.h>

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

namespace engine {

namespace {
Trait makeTrait(
    const char* id,
    const char* name,
    const char* description,
    const char* iconToken,
    const TraitRarity rarity,
    const TraitModifiers& m,
    std::initializer_list<const char*> synergyTags) {
    Trait t {.id = id, .name = name, .description = description, .iconToken = iconToken, .rarity = rarity, .modifiers = m};
    std::size_t index = 0;
    for (const char* tag : synergyTags) {
        if (index >= t.synergyTags.size()) break;
        t.synergyTags[index++] = tag;
    }
    t.synergyTagCount = static_cast<std::uint8_t>(index);
    return t;
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

namespace {
TraitRarity parseTraitRarity(const std::string& s) {
    if (s == "common" || s == "Common") return TraitRarity::Common;
    if (s == "rare" || s == "Rare") return TraitRarity::Rare;
    if (s == "relic" || s == "Relic") return TraitRarity::Relic;
    return TraitRarity::Common;
}

TraitModifiers parseTraitModifiers(const nlohmann::json& json) {
    TraitModifiers m;
    m.projectileSpeedMul = json.value("projectileSpeedMul", m.projectileSpeedMul);
    m.projectileRadiusAdd = json.value("projectileRadiusAdd", m.projectileRadiusAdd);
    m.patternCooldownScale = json.value("patternCooldownScale", m.patternCooldownScale);
    m.patternExtraBullets = json.value("patternExtraBullets", m.patternExtraBullets);
    m.patternJitterAddDeg = json.value("patternJitterAddDeg", m.patternJitterAddDeg);
    m.playerRadiusAdd = json.value("playerRadiusAdd", m.playerRadiusAdd);
    m.playerHarvestMultiplier = json.value("playerHarvestMultiplier", m.playerHarvestMultiplier);
    m.enemyFireRateScale = json.value("enemyFireRateScale", m.enemyFireRateScale);
    m.enemyProjectileSpeedScale = json.value("enemyProjectileSpeedScale", m.enemyProjectileSpeedScale);
    m.defensiveSpecialCooldownReduction = json.value("defensiveSpecialCooldownReduction", m.defensiveSpecialCooldownReduction);
    m.defensiveSpecialCapacityAdd = json.value("defensiveSpecialCapacityAdd", m.defensiveSpecialCapacityAdd);
    m.offensiveSpecialPowerMul = json.value("offensiveSpecialPowerMul", m.offensiveSpecialPowerMul);
    return m;
}

void parseSynergyTags(const nlohmann::json& json, Trait& trait) {
    trait.synergyTagCount = 0;
    if (!json.is_array()) return;
    const std::size_t count = std::min<std::size_t>(trait.synergyTags.size(), json.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (!json[i].is_string()) continue;
        trait.synergyTags[i] = json[i].get<std::string>();
        ++trait.synergyTagCount;
    }
}
} // namespace

void TraitSystem::initialize(const std::uint64_t seed) {
    rng_ = DeterministicRng(seed ^ 0xA11CE55ULL);
    catalog_.clear();

    {
        auto m = mods(); m.projectileSpeedMul = 1.20F;
        catalog_.push_back(makeTrait("swift-shots", "Swift Shots", "Boost muzzle velocity for tighter lead control.", "ico_speed", TraitRarity::Common, m, {"projectile", "tempo"}));
    }
    {
        auto m = mods(); m.projectileRadiusAdd = 0.8F; m.projectileSpeedMul = 0.95F;
        catalog_.push_back(makeTrait("heavy-rounds", "Heavy Rounds", "Increase impact width with slightly slower rounds.", "ico_radius", TraitRarity::Common, m, {"projectile", "burst"}));
    }
    {
        auto m = mods(); m.patternCooldownScale = 0.92F;
        catalog_.push_back(makeTrait("tempo-core", "Tempo Core", "Lower firing cycle cooldown for sustained pressure.", "ico_cooldown", TraitRarity::Common, m, {"tempo", "rapid"}));
    }
    {
        auto m = mods(); m.patternExtraBullets = 2;
        catalog_.push_back(makeTrait("scatter-core", "Scatter Core", "Add extra pellets to every firing pattern.", "ico_scatter", TraitRarity::Common, m, {"burst", "projectile"}));
    }
    {
        auto m = mods(); m.patternJitterAddDeg = -0.4F; m.projectileSpeedMul = 1.05F;
        catalog_.push_back(makeTrait("focus-core", "Focus Core", "Tighten spread and nudge projectile speed.", "ico_focus", TraitRarity::Rare, m, {"tempo", "precision"}));
    }
    {
        auto m = mods(); m.playerHarvestMultiplier = 1.35F;
        catalog_.push_back(makeTrait("lucky-harvest", "Lucky Harvest", "Improves upgrade currency gain from resources.", "ico_resource", TraitRarity::Common, m, {"economy", "survive"}));
    }
    {
        auto m = mods(); m.playerRadiusAdd = 4.0F; m.playerHarvestMultiplier = 1.10F;
        catalog_.push_back(makeTrait("vital-shell", "Vital Shell", "Expand defensive shell and improve resource pickup.", "ico_shield", TraitRarity::Rare, m, {"survive", "economy"}));
    }
    {
        auto m = mods(); m.enemyFireRateScale = 0.88F; m.projectileSpeedMul = 1.08F;
        catalog_.push_back(makeTrait("overclock", "Overclock", "Accelerate your shots while suppressing enemy cadence.", "ico_overclock", TraitRarity::Rare, m, {"tempo", "control"}));
    }
    {
        auto m = mods(); m.enemyFireRateScale = 1.12F; m.enemyProjectileSpeedScale = 0.82F;
        catalog_.push_back(makeTrait("jammer", "Jammer", "Slow hostile projectiles with a risky aggression spike.", "ico_jammer", TraitRarity::Relic, m, {"control", "risk"}));
    }
    {
        auto m = mods(); m.patternJitterAddDeg = 0.9F; m.patternExtraBullets = 1;
        catalog_.push_back(makeTrait("barrel-spin", "Barrel Spin", "Adds bonus pellets with chaotic spread bloom.", "ico_spin", TraitRarity::Rare, m, {"burst", "risk"}));
    }
    {
        auto m = mods(); m.projectileRadiusAdd = 1.4F; m.patternCooldownScale = 0.90F;
        catalog_.push_back(makeTrait("arc-furnace", "Arc Furnace", "Large projectiles and rapid cadence for area denial.", "ico_arc", TraitRarity::Relic, m, {"projectile", "tempo"}));
    }
    {
        auto m = mods(); m.enemyProjectileSpeedScale = 1.12F; m.projectileSpeedMul = 1.05F;
        catalog_.push_back(makeTrait("predator-mark", "Predator Mark", "Boost offense by forcing faster enemy bullet lanes.", "ico_predator", TraitRarity::Rare, m, {"precision", "risk"}));
    }
    {
        auto m = mods(); m.defensiveSpecialCooldownReduction = 0.10F;
        catalog_.push_back(makeTrait("defensive-recycle", "Defensive Recycle", "Reduces DefensiveSpecial charge cooldown.", "ico_def_cooldown", TraitRarity::Common, m, {"specials", "defense"}));
    }
    {
        auto m = mods(); m.defensiveSpecialCapacityAdd = 1;
        catalog_.push_back(makeTrait("defensive-reserve", "Defensive Reserve", "Adds one DefensiveSpecial charge capacity.", "ico_def_capacity", TraitRarity::Rare, m, {"specials", "defense"}));
    }
    {
        auto m = mods(); m.offensiveSpecialPowerMul = 1.10F;
        catalog_.push_back(makeTrait("offensive-overdrive", "Offensive Overdrive", "Increases offensive special output while sharing the Specials pool.", "ico_off_special", TraitRarity::Common, m, {"specials", "offense"}));
    }

    active_.clear();
    aggregate_ = {};
    hasPending_ = false;
    rareChanceMultiplier_ = 1.0F;
    rerollCharges_ = 2;
    nextRerollTick_ = 0;
    currentTick_ = 0;
    validationCache_ = validateCatalog();
}

bool TraitSystem::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.good()) return false;

    nlohmann::json json;
    try {
        in >> json;
    } catch (const std::exception&) {
        return false;
    }

    if (!json.contains("traits") || !json["traits"].is_array()) return false;

    std::vector<Trait> parsedCatalog;
    parsedCatalog.reserve(json["traits"].size());

    for (const auto& t : json["traits"]) {
        if (!t.is_object()) continue;
        Trait trait;
        trait.id = t.value("id", std::string {});
        trait.name = t.value("name", trait.id);
        trait.description = t.value("description", std::string {});
        trait.iconToken = t.value("iconToken", std::string {});
        trait.rarity = parseTraitRarity(t.value("rarity", std::string {"common"}));
        if (t.contains("modifiers") && t["modifiers"].is_object()) {
            trait.modifiers = parseTraitModifiers(t["modifiers"]);
        }
        if (t.contains("synergyTags")) parseSynergyTags(t["synergyTags"], trait);
        parsedCatalog.push_back(std::move(trait));
    }

    if (parsedCatalog.empty()) return false;

    catalog_ = std::move(parsedCatalog);
    validationCache_ = validateCatalog();
    if (!validationCache_.empty()) return false;

    return true;
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

std::array<Trait, TraitSystem::choiceCount> TraitSystem::rollChoices() {
    for (std::size_t i = 0; i < pending_.size(); ++i) {
        pending_[i] = rollOne();
    }
    hasPending_ = true;
    return pending_;
}

bool TraitSystem::rerollChoices() {
    if (!hasPending_ || !canReroll()) return false;
    --rerollCharges_;
    nextRerollTick_ = currentTick_ + rerollCooldownTicks_;
    return !rollChoices().empty();
}

bool TraitSystem::canReroll() const {
    return rerollCharges_ > 0;
}


void TraitSystem::forcePendingRarity(const TraitRarity rarity) {
    if (!hasPending_) return;
    for (std::size_t i = 0; i < pending_.size(); ++i) {
        for (int attempts = 0; attempts < 64; ++attempts) {
            const Trait& candidate = rollOne();
            if (candidate.rarity == rarity) {
                pending_[i] = candidate;
                break;
            }
        }
    }
}

void TraitSystem::onTick(const std::uint64_t tick) {
    currentTick_ = tick;
    if (tick >= nextRerollTick_ && rerollCharges_ < 2) {
        ++rerollCharges_;
        nextRerollTick_ = tick + rerollCooldownTicks_;
    }
}

bool TraitSystem::choose(const std::size_t index) {
    if (!hasPending_ || index >= pending_.size()) return false;
    active_.push_back(pending_[index]);
    hasPending_ = false;
    nextRerollTick_ = 0;
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
        aggregate_.defensiveSpecialCooldownReduction += t.modifiers.defensiveSpecialCooldownReduction;
        aggregate_.defensiveSpecialCapacityAdd += t.modifiers.defensiveSpecialCapacityAdd;
        aggregate_.offensiveSpecialPowerMul *= t.modifiers.offensiveSpecialPowerMul;
    }

    aggregate_.projectileSpeedMul = std::clamp(aggregate_.projectileSpeedMul, 0.5F, 3.0F);
    aggregate_.patternCooldownScale = std::clamp(aggregate_.patternCooldownScale, 0.5F, 2.0F);
    aggregate_.enemyFireRateScale = std::clamp(aggregate_.enemyFireRateScale, 0.5F, 2.0F);
    aggregate_.enemyProjectileSpeedScale = std::clamp(aggregate_.enemyProjectileSpeedScale, 0.5F, 2.0F);
    aggregate_.defensiveSpecialCooldownReduction = std::clamp(aggregate_.defensiveSpecialCooldownReduction, 0.0F, 0.5F);
    aggregate_.defensiveSpecialCapacityAdd = std::clamp(aggregate_.defensiveSpecialCapacityAdd, 0, 2);
    aggregate_.offensiveSpecialPowerMul = std::clamp(aggregate_.offensiveSpecialPowerMul, 0.5F, 2.0F);
}

void TraitSystem::setRareChanceMultiplier(const float multiplier) {
    rareChanceMultiplier_ = std::clamp(multiplier, 0.5F, 3.0F);
}

bool TraitSystem::hasPendingChoices() const { return hasPending_; }

const std::array<Trait, TraitSystem::choiceCount>& TraitSystem::pendingChoices() const { return pending_; }

const std::vector<Trait>& TraitSystem::activeTraits() const { return active_; }

const TraitModifiers& TraitSystem::modifiers() const { return aggregate_; }

std::vector<TraitValidationIssue> TraitSystem::validateCatalog() const {
    std::vector<TraitValidationIssue> issues;
    for (const Trait& t : catalog_) {
        if (t.description.empty()) {
            issues.push_back(TraitValidationIssue {.traitId = t.id, .message = "description is required"});
        }
        if (t.id.empty()) {
            issues.push_back(TraitValidationIssue {.traitId = "<empty>", .message = "id is required"});
        }
        if (t.modifiers.projectileSpeedMul <= 0.0F || t.modifiers.patternCooldownScale <= 0.0F || t.modifiers.playerHarvestMultiplier <= 0.0F
            || t.modifiers.enemyFireRateScale <= 0.0F || t.modifiers.enemyProjectileSpeedScale <= 0.0F || t.modifiers.offensiveSpecialPowerMul <= 0.0F) {
            issues.push_back(TraitValidationIssue {.traitId = t.id, .message = "multiplicative modifiers must be > 0"});
        }
    }
    return issues;
}


const std::vector<TraitValidationIssue>& TraitSystem::validationIssues() const {
    return validationCache_;
}
int TraitSystem::rerollCharges() const { return rerollCharges_; }

std::uint64_t TraitSystem::rerollCooldownRemainingTicks(const std::uint64_t tick) const {
    if (tick >= nextRerollTick_) return 0;
    return nextRerollTick_ - tick;
}

std::string toString(const TraitRarity rarity) {
    switch (rarity) {
        case TraitRarity::Common: return "Common";
        case TraitRarity::Rare: return "Rare";
        case TraitRarity::Relic: return "Relic";
    }
    return "Unknown";
}

} // namespace engine
