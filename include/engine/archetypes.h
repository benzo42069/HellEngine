#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace engine {

struct ArchetypeStats {
    float power {0.0F};
    float fireRate {0.0F};
    float moveSpeed {0.0F};
    float defense {0.0F};
    float synergy {0.0F};
    float resourceGain {0.0F};
    float dotPower {0.0F};
    float rareChance {0.0F};
};

struct ArchetypeDefinition {
    std::string id;
    std::string name;
    ArchetypeStats stats {};
    std::string primaryWeapon;
    std::string activeAbility;
    std::string passiveEffect;
    std::string projectilePaletteName;
    std::uint32_t requiredUnlockTier {0};
};

class ArchetypeSystem {
  public:
    void initializeDefaults();
    void setUnlockTier(std::uint32_t tier);

    [[nodiscard]] const std::vector<ArchetypeDefinition>& archetypes() const;
    [[nodiscard]] std::size_t selectedIndex() const;
    [[nodiscard]] const ArchetypeDefinition& selected() const;
    [[nodiscard]] bool isUnlocked(std::size_t index) const;
    bool select(std::size_t index);

  private:
    std::vector<ArchetypeDefinition> archetypes_;
    std::size_t selectedIndex_ {0};
    std::uint32_t unlockTier_ {0};
};

} // namespace engine
