#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <engine/persistence.h>

namespace engine {

struct MetaBonuses {
    float powerBonus {0.0F};
    float fireRateBonus {0.0F};
    float moveSpeedBonus {0.0F};
    float defenseBonus {0.0F};
    float rarityBonus {0.0F};
    std::uint32_t archetypeUnlockTier {0};
};

struct MetaUpgradeNode {
    std::string id;
    std::string name;
    std::string description;
    std::uint32_t cost {1};

    float powerBonus {0.0F};
    float fireRateBonus {0.0F};
    float moveSpeedBonus {0.0F};
    float defenseBonus {0.0F};
    float rarityBonus {0.0F};
    std::uint32_t archetypeUnlockTier {0};
};

class MetaProgression {
  public:
    void initializeDefaults();

    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;

    [[nodiscard]] bool purchaseNode(std::size_t index);
    void grantRunProgress(std::uint32_t amount);

    void applyProgressSnapshot(const RuntimeProgressSnapshot& snapshot);
    [[nodiscard]] RuntimeProgressSnapshot makeProgressSnapshot() const;

    [[nodiscard]] const std::vector<MetaUpgradeNode>& tree() const;
    [[nodiscard]] const std::vector<std::size_t>& purchasedNodeIndices() const;
    [[nodiscard]] const MetaBonuses& bonuses() const;
    [[nodiscard]] std::uint32_t progressionPoints() const;

  private:
    void rebuildBonuses();

    std::vector<MetaUpgradeNode> tree_;
    std::vector<std::size_t> purchased_;
    std::uint32_t progressionPoints_ {0};
    MetaBonuses bonuses_ {};
};

} // namespace engine
