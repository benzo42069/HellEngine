#include <engine/traits.h>

#include <cstdlib>
#include <iostream>

int main() {
    engine::TraitSystem traits;
    traits.initialize(777);

    (void)traits.rollChoices();
    const auto before = traits.pendingChoices();

    if (!traits.rerollChoices()) {
        std::cerr << "reroll should be available at start\n";
        return EXIT_FAILURE;
    }

    const auto after = traits.pendingChoices();

    if (traits.rerollCharges() != 1) {
        std::cerr << "reroll charge should decrement after reroll\n";
        return EXIT_FAILURE;
    }

    if (traits.rerollCooldownRemainingTicks(0) == 0) {
        std::cerr << "reroll cooldown should be active immediately\n";
        return EXIT_FAILURE;
    }

    const bool changed = before[0].id != after[0].id || before[1].id != after[1].id || before[2].id != after[2].id;
    if (!changed) {
        std::cerr << "reroll should regenerate options\n";
        return EXIT_FAILURE;
    }

    traits.forcePendingRarity(engine::TraitRarity::Relic);
    const auto relicChoices = traits.pendingChoices();
    for (const auto& c : relicChoices) {
        if (c.rarity != engine::TraitRarity::Relic) {
            std::cerr << "force rarity failed\n";
            return EXIT_FAILURE;
        }
    }

    if (!traits.choose(0)) {
        std::cerr << "failed to choose trait\n";
        return EXIT_FAILURE;
    }

    const auto& mods = traits.modifiers();
    if (mods.projectileSpeedMul <= 0.0F || mods.patternCooldownScale <= 0.0F) {
        std::cerr << "invalid modifiers after choose\n";
        return EXIT_FAILURE;
    }

    if (!traits.validateCatalog().empty()) {
        std::cerr << "default catalog should validate\n";
        return EXIT_FAILURE;
    }

    std::cout << "upgrade_ui_tests passed\n";
    return EXIT_SUCCESS;
}
