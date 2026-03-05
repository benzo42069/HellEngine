#include <engine/difficulty_scaling.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace engine {

namespace {
constexpr std::size_t profileIndex(const DifficultyProfile p) {
    return static_cast<std::size_t>(p);
}

std::size_t zoneIndex(const ZoneType t) {
    switch (t) {
        case ZoneType::Combat: return 0;
        case ZoneType::Elite: return 1;
        case ZoneType::Event: return 2;
        case ZoneType::Boss: return 3;
    }
    return 0;
}
} // namespace

void DifficultyModel::initializeDefaults() {
    profiles_.fill(ProfileConfig {});

    profiles_[profileIndex(DifficultyProfile::Normal)] = ProfileConfig {};

    ProfileConfig hard = profiles_[profileIndex(DifficultyProfile::Normal)];
    hard.baseDifficulty = 1.25F;
    hard.stageRamp = 0.15F;
    hard.perfMax = 0.25F;
    hard.difficultyMax = 4.5F;
    profiles_[profileIndex(DifficultyProfile::Hard)] = hard;

    ProfileConfig nightmare = hard;
    nightmare.baseDifficulty = 1.55F;
    nightmare.stageRamp = 0.22F;
    nightmare.zoneAdd = {{0.12F, 0.22F, 0.0F, 0.38F}};
    nightmare.difficultyMax = 5.0F;
    profiles_[profileIndex(DifficultyProfile::Nightmare)] = nightmare;

    ProfileConfig endless = nightmare;
    endless.baseDifficulty = 1.35F;
    endless.endlessGrowthPerMinute = 0.08F;
    endless.difficultyMax = 8.0F;
    profiles_[profileIndex(DifficultyProfile::Endless)] = endless;

    currentDifficulty_ = 1.0F;
    scalars_ = {};
}

bool DifficultyModel::loadProfilesFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.good()) return false;

    nlohmann::json root;
    in >> root;
    if (!root.contains("profiles") || !root["profiles"].is_array()) return false;

    for (const auto& p : root["profiles"]) {
        const DifficultyProfile profile = difficultyProfileFromString(p.value("name", "normal"));
        ProfileConfig cfg = profiles_[profileIndex(profile)];
        cfg.baseDifficulty = p.value("baseDifficulty", cfg.baseDifficulty);
        cfg.stageRamp = p.value("stageRamp", cfg.stageRamp);
        cfg.performanceEnabled = p.value("performanceEnabled", cfg.performanceEnabled);
        cfg.perfHealthWeight = p.value("perfHealthWeight", cfg.perfHealthWeight);
        cfg.perfCollisionWeight = p.value("perfCollisionWeight", cfg.perfCollisionWeight);
        cfg.perfBossWeight = p.value("perfBossWeight", cfg.perfBossWeight);
        cfg.perfMin = p.value("perfMin", cfg.perfMin);
        cfg.perfMax = p.value("perfMax", cfg.perfMax);
        cfg.endlessGrowthPerMinute = p.value("endlessGrowthPerMinute", cfg.endlessGrowthPerMinute);
        cfg.difficultyMin = p.value("difficultyMin", cfg.difficultyMin);
        cfg.difficultyMax = p.value("difficultyMax", cfg.difficultyMax);
        cfg.patternSpeedFactor = p.value("patternSpeedFactor", cfg.patternSpeedFactor);
        cfg.spawnRateFactor = p.value("spawnRateFactor", cfg.spawnRateFactor);
        cfg.enemyHpFactor = p.value("enemyHpFactor", cfg.enemyHpFactor);

        if (p.contains("zoneAdd") && p["zoneAdd"].is_object()) {
            cfg.zoneAdd[0] = p["zoneAdd"].value("combat", cfg.zoneAdd[0]);
            cfg.zoneAdd[1] = p["zoneAdd"].value("elite", cfg.zoneAdd[1]);
            cfg.zoneAdd[2] = p["zoneAdd"].value("event", cfg.zoneAdd[2]);
            cfg.zoneAdd[3] = p["zoneAdd"].value("boss", cfg.zoneAdd[3]);
        }

        profiles_[profileIndex(profile)] = cfg;
    }

    return true;
}

void DifficultyModel::setProfile(const DifficultyProfile profile) { activeProfile_ = profile; }
DifficultyProfile DifficultyModel::profile() const { return activeProfile_; }
std::string DifficultyModel::profileLabel() const { return toString(activeProfile_); }

void DifficultyModel::setStageZone(const std::size_t stageIndex, const ZoneType zoneType) {
    stageIndex_ = stageIndex;
    zoneType_ = zoneType;
}

void DifficultyModel::update(const float dt, const DeterministicPerformanceMetrics& metrics) {
    elapsedSeconds_ += std::max(0.0F, dt);
    const ProfileConfig& cfg = profiles_[profileIndex(activeProfile_)];

    const float base = cfg.baseDifficulty + static_cast<float>(stageIndex_) * cfg.stageRamp + cfg.zoneAdd[zoneIndex(zoneType_)];
    float performance = 0.0F;
    if (cfg.performanceEnabled) {
        performance = metrics.playerHealth01 * cfg.perfHealthWeight
            - metrics.collisionsPerMinute * cfg.perfCollisionWeight
            + static_cast<float>(metrics.defeatedBosses) * cfg.perfBossWeight;
        performance = std::clamp(performance, cfg.perfMin, cfg.perfMax);
    }
    const float endless = activeProfile_ == DifficultyProfile::Endless ? (elapsedSeconds_ / 60.0F) * cfg.endlessGrowthPerMinute : 0.0F;

    currentDifficulty_ = std::clamp(base + performance + endless, cfg.difficultyMin, cfg.difficultyMax);
    scalars_.overall = currentDifficulty_;
    scalars_.patternSpeed = std::clamp(1.0F + (currentDifficulty_ - 1.0F) * cfg.patternSpeedFactor, 0.5F, 5.0F);
    scalars_.spawnRate = std::clamp(1.0F + (currentDifficulty_ - 1.0F) * cfg.spawnRateFactor, 0.5F, 6.0F);
    scalars_.enemyHp = std::clamp(1.0F + (currentDifficulty_ - 1.0F) * cfg.enemyHpFactor, 0.5F, 8.0F);
}

float DifficultyModel::currentDifficulty() const { return currentDifficulty_; }
const DifficultyScalars& DifficultyModel::scalars() const { return scalars_; }

std::string toString(const DifficultyProfile profile) {
    switch (profile) {
        case DifficultyProfile::Normal: return "Normal";
        case DifficultyProfile::Hard: return "Hard";
        case DifficultyProfile::Nightmare: return "Nightmare";
        case DifficultyProfile::Endless: return "Endless";
    }
    return "Normal";
}

DifficultyProfile difficultyProfileFromString(const std::string& s) {
    if (s == "normal" || s == "Normal") return DifficultyProfile::Normal;
    if (s == "hard" || s == "Hard") return DifficultyProfile::Hard;
    if (s == "nightmare" || s == "Nightmare") return DifficultyProfile::Nightmare;
    if (s == "endless" || s == "Endless") return DifficultyProfile::Endless;
    return DifficultyProfile::Normal;
}

} // namespace engine
