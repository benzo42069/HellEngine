#include <engine/traits.h>

#include <cstdlib>
#include <iostream>

int main() {
    engine::TraitSystem t1;
    engine::TraitSystem t2;
    t1.initialize(123);
    t2.initialize(123);

    auto c1 = t1.rollChoices();
    auto c2 = t2.rollChoices();

    if (c1[0].id != c2[0].id || c1[1].id != c2[1].id || c1[2].id != c2[2].id) {
        std::cerr << "deterministic roll mismatch\n";
        return EXIT_FAILURE;
    }

    if (!t1.choose(1) || t1.activeTraits().empty()) {
        std::cerr << "trait choose failed\n";
        return EXIT_FAILURE;
    }

    const auto& m = t1.modifiers();
    if (m.projectileSpeedMul == 1.0F && m.patternCooldownScale == 1.0F && m.playerHarvestMultiplier == 1.0F) {
        std::cerr << "trait modifiers not applied\n";
        return EXIT_FAILURE;
    }

    std::cout << "trait_tests passed\n";
    return EXIT_SUCCESS;
}
