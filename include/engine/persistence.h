#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace engine {

struct UserSettings {
    std::uint32_t schemaVersion {2};
    float audioMasterVolume {1.0F};
    float audioMusicVolume {0.75F};
    float audioSfxVolume {0.9F};
    bool enableModernRenderer {false};
    bool fullscreen {false};
    std::string difficultyProfile {"normal"};
};

struct RuntimeProgressSnapshot {
    std::uint32_t progressionPoints {0};
    std::vector<std::size_t> purchasedNodes;
    std::uint32_t lifetimeRunsStarted {0};
    std::uint32_t lifetimeRunsCleared {0};
};

struct SaveProfile {
    std::string id;
    std::string displayName;
    std::string createdUtc;
    std::string lastPlayedUtc;
    RuntimeProgressSnapshot progression;
};

struct SaveProfilesFile {
    std::uint32_t schemaVersion {2};
    std::string activeProfileId;
    std::vector<SaveProfile> profiles;
};

struct PersistenceLoadResult {
    bool ok {false};
    bool usedFallback {false};
    bool migrated {false};
    std::string error;
};

PersistenceLoadResult loadUserSettingsFromFile(const std::string& path, UserSettings& outSettings);
bool saveUserSettingsToFile(const std::string& path, const UserSettings& settings, std::string* outError = nullptr);

PersistenceLoadResult loadSaveProfilesFromFile(const std::string& path, SaveProfilesFile& outProfiles);
bool saveSaveProfilesToFile(const std::string& path, const SaveProfilesFile& profiles, std::string* outError = nullptr);

} // namespace engine
