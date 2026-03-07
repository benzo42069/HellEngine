#include <engine/meta_progression.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace engine {

namespace {
MetaUpgradeNode makeNode(
    const char* id,
    const char* name,
    const char* description,
    const std::uint32_t cost,
    const float power,
    const float fireRate,
    const float moveSpeed,
    const float defense,
    const float rarity,
    const std::uint32_t unlockTier
) {
    return MetaUpgradeNode {
        .id = id,
        .name = name,
        .description = description,
        .cost = cost,
        .powerBonus = power,
        .fireRateBonus = fireRate,
        .moveSpeedBonus = moveSpeed,
        .defenseBonus = defense,
        .rarityBonus = rarity,
        .archetypeUnlockTier = unlockTier,
    };
}
} // namespace

void MetaProgression::initializeDefaults() {
    tree_.clear();
    purchased_.clear();
    progressionPoints_ = 0;

    tree_.push_back(makeNode("core-training", "Core Training", "+1 Power and +1 Defense", 1, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0));
    tree_.push_back(makeNode("rapid-doctrine", "Rapid Doctrine", "+1 Fire Rate", 1, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0));
    tree_.push_back(makeNode("fleet-foot", "Fleet Foot", "+1 Move Speed", 1, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0));
    tree_.push_back(makeNode("fortune-thread", "Fortune Thread", "+1 Rare Chance", 2, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0));
    tree_.push_back(makeNode("archetype-license-i", "Archetype License I", "Unlock archetypes tier 1", 2, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1));
    tree_.push_back(makeNode("archetype-license-ii", "Archetype License II", "Unlock archetypes tier 2", 3, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 2));
    tree_.push_back(makeNode("veteran-frame", "Veteran Frame", "+1 Power +1 Fire Rate +1 Defense", 3, 1.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0));
    tree_.push_back(makeNode("mythic-lens", "Mythic Lens", "+2 Rare Chance", 4, 0.0F, 0.0F, 0.0F, 0.0F, 2.0F, 0));

    rebuildBonuses();
}

bool MetaProgression::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.good()) return false;

    try {
        nlohmann::json json;
        in >> json;

        RuntimeProgressSnapshot snapshot;
        const std::uint32_t schemaVersion = json.value("schemaVersion", 1U);
        if (schemaVersion <= 1U) {
            snapshot.progressionPoints = json.value("progressionPoints", 0U);
            if (json.contains("purchased") && json["purchased"].is_array()) {
                for (const auto& idx : json["purchased"]) {
                    snapshot.purchasedNodes.push_back(idx.get<std::size_t>());
                }
            }
        } else if (schemaVersion == 2U && json.contains("progression") && json["progression"].is_object()) {
            const auto& p = json["progression"];
            snapshot.progressionPoints = p.value("progressionPoints", 0U);
            snapshot.lifetimeRunsStarted = p.value("lifetimeRunsStarted", 0U);
            snapshot.lifetimeRunsCleared = p.value("lifetimeRunsCleared", 0U);
            if (p.contains("purchasedNodes") && p["purchasedNodes"].is_array()) {
                for (const auto& idx : p["purchasedNodes"]) {
                    snapshot.purchasedNodes.push_back(idx.get<std::size_t>());
                }
            }
        } else {
            return false;
        }

        applyProgressSnapshot(snapshot);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool MetaProgression::saveToFile(const std::string& path) const {
    const RuntimeProgressSnapshot snapshot = makeProgressSnapshot();
    nlohmann::json json = {
        {"schemaVersion", 2},
        {"progression", {{"progressionPoints", snapshot.progressionPoints}}},
    };
    json["progression"]["purchasedNodes"] = snapshot.purchasedNodes;
    json["progression"]["lifetimeRunsStarted"] = snapshot.lifetimeRunsStarted;
    json["progression"]["lifetimeRunsCleared"] = snapshot.lifetimeRunsCleared;

    std::ofstream out(path);
    if (!out.good()) return false;
    out << json.dump(2);
    return true;
}

bool MetaProgression::purchaseNode(const std::size_t index) {
    if (index >= tree_.size()) return false;
    if (std::find(purchased_.begin(), purchased_.end(), index) != purchased_.end()) return false;

    const MetaUpgradeNode& node = tree_[index];
    if (progressionPoints_ < node.cost) return false;

    progressionPoints_ -= node.cost;
    purchased_.push_back(index);
    rebuildBonuses();
    return true;
}

void MetaProgression::grantRunProgress(const std::uint32_t amount) { progressionPoints_ += amount; }

void MetaProgression::applyProgressSnapshot(const RuntimeProgressSnapshot& snapshot) {
    progressionPoints_ = snapshot.progressionPoints;
    purchased_.clear();
    for (const std::size_t idx : snapshot.purchasedNodes) {
        if (idx < tree_.size()) purchased_.push_back(idx);
    }
    std::sort(purchased_.begin(), purchased_.end());
    purchased_.erase(std::unique(purchased_.begin(), purchased_.end()), purchased_.end());
    rebuildBonuses();
}

RuntimeProgressSnapshot MetaProgression::makeProgressSnapshot() const {
    RuntimeProgressSnapshot snapshot;
    snapshot.progressionPoints = progressionPoints_;
    snapshot.purchasedNodes = purchased_;
    return snapshot;
}

void MetaProgression::rebuildBonuses() {
    bonuses_ = {};
    for (const std::size_t index : purchased_) {
        if (index >= tree_.size()) continue;
        const MetaUpgradeNode& node = tree_[index];
        bonuses_.powerBonus += node.powerBonus;
        bonuses_.fireRateBonus += node.fireRateBonus;
        bonuses_.moveSpeedBonus += node.moveSpeedBonus;
        bonuses_.defenseBonus += node.defenseBonus;
        bonuses_.rarityBonus += node.rarityBonus;
        bonuses_.archetypeUnlockTier = std::max(bonuses_.archetypeUnlockTier, node.archetypeUnlockTier);
    }
}

const std::vector<MetaUpgradeNode>& MetaProgression::tree() const { return tree_; }

const std::vector<std::size_t>& MetaProgression::purchasedNodeIndices() const { return purchased_; }

const MetaBonuses& MetaProgression::bonuses() const { return bonuses_; }

std::uint32_t MetaProgression::progressionPoints() const { return progressionPoints_; }

} // namespace engine
