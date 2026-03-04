#include <engine/archetypes.h>

#include <cstdlib>
#include <iostream>

int main() {
    engine::ArchetypeSystem archetypes;
    archetypes.initializeDefaults();

    const auto& defs = archetypes.archetypes();
    if (defs.size() != 8) {
        std::cerr << "expected 8 archetypes\n";
        return EXIT_FAILURE;
    }

    for (const auto& def : defs) {
        if (def.primaryWeapon.empty() || def.activeAbility.empty() || def.passiveEffect.empty()) {
            std::cerr << "archetype missing loadout text\n";
            return EXIT_FAILURE;
        }
    }

    if (archetypes.select(7)) {
        std::cerr << "oracle should be locked at tier 0\n";
        return EXIT_FAILURE;
    }

    archetypes.setUnlockTier(2);
    if (!archetypes.select(7) || archetypes.selected().name != "Oracle") {
        std::cerr << "archetype selection failed after unlock\n";
        return EXIT_FAILURE;
    }

    std::cout << "archetype_tests passed\n";
    return EXIT_SUCCESS;
}
