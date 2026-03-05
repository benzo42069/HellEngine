#include <engine/upgrade_ui_model.h>

#include <cstdlib>
#include <iostream>
#include <string>

int main() {
    engine::ArchetypeDefinition archetype;
    archetype.stats.power = 7.0F;
    archetype.stats.fireRate = 6.0F;
    archetype.stats.moveSpeed = 5.0F;
    archetype.stats.defense = 4.0F;

    engine::MetaBonuses mb;
    mb.powerBonus = 1.0F;
    mb.fireRateBonus = 2.0F;
    mb.moveSpeedBonus = 0.5F;
    mb.defenseBonus = 1.5F;

    engine::TraitModifiers tm;
    tm.projectileSpeedMul = 1.10F;
    tm.projectileRadiusAdd = 0.8F;
    tm.patternCooldownScale = 0.9F;
    tm.patternExtraBullets = 2;
    tm.playerRadiusAdd = 1.0F;
    tm.playerHarvestMultiplier = 1.2F;

    const engine::UpgradeViewStats current = engine::buildUpgradeViewStats(archetype, mb, tm);

    engine::Trait t;
    t.modifiers.projectileSpeedMul = 1.15F;
    t.modifiers.projectileRadiusAdd = 0.5F;
    t.modifiers.patternCooldownScale = 0.92F;
    t.modifiers.patternExtraBullets = 1;
    t.modifiers.playerHarvestMultiplier = 1.1F;
    t.modifiers.playerRadiusAdd = 0.5F;

    const engine::UpgradeViewStats projected = engine::projectUpgradeViewStats(current, t);
    if (projected.power <= current.power || projected.fireRate <= current.fireRate || projected.moveSpeed <= current.moveSpeed || projected.defense <= current.defense) {
        std::cerr << "projected stats should improve in this fixture\n";
        return EXIT_FAILURE;
    }

    if (std::string(engine::rarityLabel(engine::TraitRarity::Relic)) != "Relic") {
        std::cerr << "rarity label mismatch\n";
        return EXIT_FAILURE;
    }

    std::cout << "upgrade_ui_model_tests passed\n";
    return EXIT_SUCCESS;
}
