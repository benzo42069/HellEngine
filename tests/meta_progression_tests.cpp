#include <engine/meta_progression.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

int main() {
    engine::MetaProgression mp;
    mp.initializeDefaults();

    mp.grantRunProgress(10);
    if (!mp.purchaseNode(0) || !mp.purchaseNode(4) || !mp.purchaseNode(5)) {
        std::cerr << "failed to buy expected nodes\n";
        return EXIT_FAILURE;
    }

    const auto& bonuses = mp.bonuses();
    if (bonuses.powerBonus <= 0.0F || bonuses.archetypeUnlockTier < 2) {
        std::cerr << "meta bonuses not applied\n";
        return EXIT_FAILURE;
    }

    const std::string path = "meta_progression_test_save.json";
    if (!mp.saveToFile(path)) {
        std::cerr << "failed to save meta progression\n";
        return EXIT_FAILURE;
    }

    engine::MetaProgression loaded;
    loaded.initializeDefaults();
    if (!loaded.loadFromFile(path)) {
        std::cerr << "failed to load meta progression\n";
        return EXIT_FAILURE;
    }

    if (loaded.bonuses().archetypeUnlockTier != bonuses.archetypeUnlockTier) {
        std::cerr << "loaded bonuses mismatch\n";
        return EXIT_FAILURE;
    }

    std::filesystem::remove(path);

    std::cout << "meta_progression_tests passed\n";
    return EXIT_SUCCESS;
}
