#include <engine/meta_progression.h>
#include <engine/persistence.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
    using namespace engine;

    const std::string settingsPath = "persistence_settings_test.json";
    const std::string profilesPath = "persistence_profiles_test.json";
    const std::string corruptPath = "persistence_corrupt_test.json";

    UserSettings settings;
    settings.audioMasterVolume = 0.5F;
    settings.audioMusicVolume = 0.4F;
    settings.audioSfxVolume = 0.3F;
    settings.enableModernRenderer = true;
    settings.fullscreen = true;
    settings.difficultyProfile = "hard";

    std::string error;
    if (!saveUserSettingsToFile(settingsPath, settings, &error)) {
        std::cerr << "saveUserSettingsToFile failed: " << error << "\n";
        return EXIT_FAILURE;
    }

    UserSettings loadedSettings;
    const PersistenceLoadResult settingsLoad = loadUserSettingsFromFile(settingsPath, loadedSettings);
    if (!settingsLoad.ok || loadedSettings.fullscreen != settings.fullscreen || loadedSettings.difficultyProfile != settings.difficultyProfile) {
        std::cerr << "settings roundtrip failed\n";
        return EXIT_FAILURE;
    }

    {
        std::ofstream legacy(settingsPath);
        legacy << R"({"schemaVersion":1,"audioMasterVolume":0.9,"audioMusicVolume":0.8,"audioSfxVolume":0.7,"enableModernRenderer":true,"difficultyProfile":"nightmare"})";
    }
    UserSettings migratedSettings;
    const PersistenceLoadResult migratedSettingsResult = loadUserSettingsFromFile(settingsPath, migratedSettings);
    if (!migratedSettingsResult.ok || !migratedSettingsResult.migrated || migratedSettings.difficultyProfile != "nightmare") {
        std::cerr << "settings migration failed\n";
        return EXIT_FAILURE;
    }

    MetaProgression progression;
    progression.initializeDefaults();
    progression.grantRunProgress(5);
    if (!progression.purchaseNode(0)) {
        std::cerr << "meta progression setup failed\n";
        return EXIT_FAILURE;
    }

    SaveProfilesFile profiles;
    profiles.activeProfileId = "profile-a";
    SaveProfile profile;
    profile.id = "profile-a";
    profile.displayName = "Alpha";
    profile.progression = progression.makeProgressSnapshot();
    profiles.profiles.push_back(profile);

    if (!saveSaveProfilesToFile(profilesPath, profiles, &error)) {
        std::cerr << "saveSaveProfilesToFile failed: " << error << "\n";
        return EXIT_FAILURE;
    }

    SaveProfilesFile loadedProfiles;
    const PersistenceLoadResult profileLoad = loadSaveProfilesFromFile(profilesPath, loadedProfiles);
    if (!profileLoad.ok || loadedProfiles.profiles.empty() || loadedProfiles.profiles.front().progression.purchasedNodes.empty()) {
        std::cerr << "profile roundtrip failed\n";
        return EXIT_FAILURE;
    }

    {
        std::ofstream legacy(profilesPath);
        legacy << R"({"schemaVersion":1,"activeProfileId":"legacy","profiles":[{"id":"legacy","name":"Legacy","progressionPoints":3,"purchased":[0,0,2]}]})";
    }

    SaveProfilesFile migratedProfiles;
    const PersistenceLoadResult migratedProfileResult = loadSaveProfilesFromFile(profilesPath, migratedProfiles);
    if (!migratedProfileResult.ok || !migratedProfileResult.migrated || migratedProfiles.profiles.front().progression.purchasedNodes.size() != 2) {
        std::cerr << "profile migration failed\n";
        return EXIT_FAILURE;
    }

    {
        std::ofstream corrupt(corruptPath);
        corrupt << "{ this is invalid json";
    }
    UserSettings fallbackSettings;
    const PersistenceLoadResult corruptLoad = loadUserSettingsFromFile(corruptPath, fallbackSettings);
    if (corruptLoad.ok || !corruptLoad.usedFallback) {
        std::cerr << "corrupt fallback expectation failed\n";
        return EXIT_FAILURE;
    }

    std::filesystem::remove(settingsPath);
    std::filesystem::remove(profilesPath);
    std::filesystem::remove(corruptPath);

    std::cout << "persistence_tests passed\n";
    return EXIT_SUCCESS;
}
