#include <engine/persistence.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace engine {

namespace {
constexpr std::uint32_t kCurrentUserSettingsSchemaVersion = 2;
constexpr std::uint32_t kCurrentSaveProfilesSchemaVersion = 2;

std::vector<std::size_t> uniqueSorted(const std::vector<std::size_t>& values) {
    std::vector<std::size_t> normalized = values;
    std::sort(normalized.begin(), normalized.end());
    normalized.erase(std::unique(normalized.begin(), normalized.end()), normalized.end());
    return normalized;
}

void migrateSettingsV1ToV2(nlohmann::json& json) {
    const float master = json.value("audioMasterVolume", 1.0F);
    const float music = json.value("audioMusicVolume", 0.75F);
    const float sfx = json.value("audioSfxVolume", 0.9F);
    const bool modernRenderer = json.value("enableModernRenderer", false);
    const std::string difficulty = json.value("difficultyProfile", std::string("normal"));

    json = {
        {"schemaVersion", 2},
        {"audio", {{"masterVolume", master}, {"musicVolume", music}, {"sfxVolume", sfx}}},
        {"video", {{"enableModernRenderer", modernRenderer}, {"fullscreen", false}}},
        {"gameplay", {{"difficultyProfile", difficulty}}},
    };
}

bool readJsonFile(const std::string& path, nlohmann::json& outJson, std::string& outError) {
    std::ifstream input(path);
    if (!input.good()) {
        outError = "file not found";
        return false;
    }

    try {
        input >> outJson;
    } catch (const std::exception& ex) {
        outError = std::string("invalid json: ") + ex.what();
        return false;
    }
    return true;
}

nlohmann::json toJson(const UserSettings& settings) {
    return {
        {"schemaVersion", kCurrentUserSettingsSchemaVersion},
        {"audio", {{"masterVolume", settings.audioMasterVolume}, {"musicVolume", settings.audioMusicVolume}, {"sfxVolume", settings.audioSfxVolume}}},
        {"video", {{"enableModernRenderer", settings.enableModernRenderer}, {"fullscreen", settings.fullscreen}}},
        {"gameplay", {{"difficultyProfile", settings.difficultyProfile}}},
    };
}

nlohmann::json toJson(const RuntimeProgressSnapshot& progression) {
    return {
        {"progressionPoints", progression.progressionPoints},
        {"purchasedNodes", uniqueSorted(progression.purchasedNodes)},
        {"lifetimeRunsStarted", progression.lifetimeRunsStarted},
        {"lifetimeRunsCleared", progression.lifetimeRunsCleared},
    };
}

nlohmann::json toJson(const SaveProfile& profile) {
    return {
        {"id", profile.id},
        {"displayName", profile.displayName},
        {"createdUtc", profile.createdUtc},
        {"lastPlayedUtc", profile.lastPlayedUtc},
        {"progression", toJson(profile.progression)},
    };
}

bool writeJsonFile(const std::string& path, const nlohmann::json& json, std::string* outError) {
    std::ofstream out(path);
    if (!out.good()) {
        if (outError != nullptr) *outError = "failed to open file for writing";
        return false;
    }
    out << json.dump(2);
    return true;
}

void parseProgression(const nlohmann::json& json, RuntimeProgressSnapshot& out) {
    out.progressionPoints = json.value("progressionPoints", 0U);
    if (json.contains("purchasedNodes") && json["purchasedNodes"].is_array()) {
        out.purchasedNodes.clear();
        for (const auto& node : json["purchasedNodes"]) {
            out.purchasedNodes.push_back(node.get<std::size_t>());
        }
        out.purchasedNodes = uniqueSorted(out.purchasedNodes);
    }
    out.lifetimeRunsStarted = json.value("lifetimeRunsStarted", 0U);
    out.lifetimeRunsCleared = json.value("lifetimeRunsCleared", 0U);
}

void migrateProfilesV1ToV2(nlohmann::json& json) {
    nlohmann::json migratedProfiles = nlohmann::json::array();
    if (json.contains("profiles") && json["profiles"].is_array()) {
        for (const auto& p : json["profiles"]) {
            RuntimeProgressSnapshot progression;
            progression.progressionPoints = p.value("progressionPoints", 0U);
            if (p.contains("purchased") && p["purchased"].is_array()) {
                for (const auto& idx : p["purchased"]) {
                    progression.purchasedNodes.push_back(idx.get<std::size_t>());
                }
            }
            progression.purchasedNodes = uniqueSorted(progression.purchasedNodes);

            migratedProfiles.push_back(
                {
                    {"id", p.value("id", std::string("profile-1"))},
                    {"displayName", p.value("name", std::string("Profile 1"))},
                    {"createdUtc", p.value("createdUtc", std::string())},
                    {"lastPlayedUtc", p.value("lastPlayedUtc", std::string())},
                    {"progression", toJson(progression)},
                }
            );
        }
    } else {
        RuntimeProgressSnapshot progression;
        progression.progressionPoints = json.value("progressionPoints", 0U);
        if (json.contains("purchased") && json["purchased"].is_array()) {
            for (const auto& idx : json["purchased"]) {
                progression.purchasedNodes.push_back(idx.get<std::size_t>());
            }
        }
        progression.purchasedNodes = uniqueSorted(progression.purchasedNodes);

        migratedProfiles.push_back(
            {
                {"id", "profile-1"},
                {"displayName", "Profile 1"},
                {"createdUtc", ""},
                {"lastPlayedUtc", ""},
                {"progression", toJson(progression)},
            }
        );
    }

    const std::string activeProfileId = json.value("activeProfileId", std::string("profile-1"));
    json = {
        {"schemaVersion", 2},
        {"activeProfileId", activeProfileId},
        {"profiles", migratedProfiles},
    };
}

} // namespace

PersistenceLoadResult loadUserSettingsFromFile(const std::string& path, UserSettings& outSettings) {
    PersistenceLoadResult result;
    outSettings = UserSettings {};

    nlohmann::json json;
    std::string error;
    if (!readJsonFile(path, json, error)) {
        result.ok = false;
        result.usedFallback = true;
        result.error = error;
        return result;
    }

    const std::uint32_t schemaVersion = json.value("schemaVersion", 1U);
    if (schemaVersion == 1U) {
        migrateSettingsV1ToV2(json);
        result.migrated = true;
    } else if (schemaVersion > kCurrentUserSettingsSchemaVersion) {
        result.ok = false;
        result.usedFallback = true;
        result.error = "unsupported user settings schema version";
        return result;
    }

    if (json.contains("audio") && json["audio"].is_object()) {
        const auto& a = json["audio"];
        outSettings.audioMasterVolume = a.value("masterVolume", outSettings.audioMasterVolume);
        outSettings.audioMusicVolume = a.value("musicVolume", outSettings.audioMusicVolume);
        outSettings.audioSfxVolume = a.value("sfxVolume", outSettings.audioSfxVolume);
    }

    if (json.contains("video") && json["video"].is_object()) {
        const auto& v = json["video"];
        outSettings.enableModernRenderer = v.value("enableModernRenderer", outSettings.enableModernRenderer);
        outSettings.fullscreen = v.value("fullscreen", outSettings.fullscreen);
    }

    if (json.contains("gameplay") && json["gameplay"].is_object()) {
        const auto& g = json["gameplay"];
        outSettings.difficultyProfile = g.value("difficultyProfile", outSettings.difficultyProfile);
    }

    result.ok = true;
    return result;
}

bool saveUserSettingsToFile(const std::string& path, const UserSettings& settings, std::string* outError) {
    return writeJsonFile(path, toJson(settings), outError);
}

PersistenceLoadResult loadSaveProfilesFromFile(const std::string& path, SaveProfilesFile& outProfiles) {
    PersistenceLoadResult result;
    outProfiles = SaveProfilesFile {};

    nlohmann::json json;
    std::string error;
    if (!readJsonFile(path, json, error)) {
        result.ok = false;
        result.usedFallback = true;
        result.error = error;
        return result;
    }

    const std::uint32_t schemaVersion = json.value("schemaVersion", 1U);
    if (schemaVersion == 1U) {
        migrateProfilesV1ToV2(json);
        result.migrated = true;
    } else if (schemaVersion > kCurrentSaveProfilesSchemaVersion) {
        result.ok = false;
        result.usedFallback = true;
        result.error = "unsupported save profiles schema version";
        return result;
    }

    outProfiles.activeProfileId = json.value("activeProfileId", std::string());
    if (json.contains("profiles") && json["profiles"].is_array()) {
        for (const auto& p : json["profiles"]) {
            SaveProfile parsed;
            parsed.id = p.value("id", std::string());
            parsed.displayName = p.value("displayName", std::string("Profile"));
            parsed.createdUtc = p.value("createdUtc", std::string());
            parsed.lastPlayedUtc = p.value("lastPlayedUtc", std::string());
            if (p.contains("progression") && p["progression"].is_object()) {
                parseProgression(p["progression"], parsed.progression);
            }
            outProfiles.profiles.push_back(parsed);
        }
    }

    result.ok = true;
    if (outProfiles.profiles.empty()) {
        result.usedFallback = true;
        result.error = "save profile file is valid but empty";
    }

    if (outProfiles.activeProfileId.empty() && !outProfiles.profiles.empty()) {
        outProfiles.activeProfileId = outProfiles.profiles.front().id;
    }

    return result;
}

bool saveSaveProfilesToFile(const std::string& path, const SaveProfilesFile& profiles, std::string* outError) {
    nlohmann::json profileJson = {
        {"schemaVersion", kCurrentSaveProfilesSchemaVersion},
        {"activeProfileId", profiles.activeProfileId},
        {"profiles", nlohmann::json::array()},
    };

    for (const auto& p : profiles.profiles) {
        profileJson["profiles"].push_back(toJson(p));
    }

    return writeJsonFile(path, profileJson, outError);
}

} // namespace engine
