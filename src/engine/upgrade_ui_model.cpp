#include <engine/upgrade_ui_model.h>

namespace engine {

UpgradeViewStats buildUpgradeViewStats(const ArchetypeDefinition& archetype, const MetaBonuses& mb, const TraitModifiers& tm) {
    UpgradeViewStats stats;
    stats.power = archetype.stats.power + mb.powerBonus + (tm.projectileSpeedMul - 1.0F) * 8.0F + tm.projectileRadiusAdd * 2.0F;
    stats.fireRate = archetype.stats.fireRate + mb.fireRateBonus + (1.0F - tm.patternCooldownScale) * 12.0F + static_cast<float>(tm.patternExtraBullets);
    stats.moveSpeed = archetype.stats.moveSpeed + mb.moveSpeedBonus + tm.playerHarvestMultiplier;
    stats.defense = archetype.stats.defense + mb.defenseBonus + tm.playerRadiusAdd * 0.45F;
    return stats;
}

UpgradeViewStats projectUpgradeViewStats(const UpgradeViewStats& current, const Trait& trait) {
    UpgradeViewStats stats = current;
    stats.power += (trait.modifiers.projectileSpeedMul - 1.0F) * 8.0F + trait.modifiers.projectileRadiusAdd * 2.0F;
    stats.fireRate += (1.0F - trait.modifiers.patternCooldownScale) * 12.0F + static_cast<float>(trait.modifiers.patternExtraBullets);
    stats.moveSpeed += trait.modifiers.playerHarvestMultiplier - 1.0F;
    stats.defense += trait.modifiers.playerRadiusAdd * 0.45F;
    return stats;
}

const char* rarityLabel(const TraitRarity rarity) {
    switch (rarity) {
        case TraitRarity::Common: return "Common";
        case TraitRarity::Rare: return "Rare";
        case TraitRarity::Relic: return "Relic";
    }
    return "Unknown";
}

} // namespace engine
