#include <engine/editor_tools.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace engine {

namespace {
bool containsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
    return it != haystack.end();
}
} // namespace

bool generateDemoContent(const std::string& outputDir, std::string* error) {
    try {
        const std::filesystem::path root(outputDir);
        std::filesystem::create_directories(root);

        nlohmann::json patterns;
        patterns["patterns"] = {{{"name", "Demo Burst"}, {"layers", {{{"type", "radial"}, {"bulletCount", 16}, {"bulletSpeed", 150.0}, {"projectileRadius", 3.0}}}}}};

        nlohmann::json entities;
        entities["entities"] = {{{"name", "Demo Drone"}, {"type", "enemy"}, {"movement", "sine"}, {"spawnPosition", {0.0, -120.0}}, {"baseVelocity", {0.0, 20.0}}, {"attacksEnabled", true}, {"attackPatternName", "Demo Burst"}, {"attackIntervalSeconds", 0.7}, {"spawnRule", {{"enabled", true}, {"initialDelaySeconds", 1.0}, {"intervalSeconds", 3.0}, {"maxAlive", 2}}}}};

        nlohmann::json traits;
        traits["traits"] = {{{"id", "demo-focus"}, {"name", "Demo Focus"}, {"rarity", "rare"}, {"modifiers", {{"projectileSpeedMul", 1.1}, {"patternCooldownScale", 0.95}}}}};

        nlohmann::json waves;
        waves["waves"] = {{{"name", "Demo Wave"}, {"zones", {"combat", "elite", "event", "boss"}}, {"durationSeconds", 45.0}}};

        std::ofstream(root / "demo_patterns.json") << patterns.dump(2);
        std::ofstream(root / "demo_entities.json") << entities.dump(2);
        std::ofstream(root / "demo_traits.json") << traits.dump(2);
        std::ofstream(root / "demo_waves.json") << waves.dump(2);
        return true;
    } catch (const std::exception& ex) {
        if (error) *error = ex.what();
        return false;
    }
}

ControlCenterValidationReport runControlCenterValidation(const std::string& contentRoot, const ToolRuntimeSnapshot& snapshot) {
    ControlCenterValidationReport report;
    namespace fs = std::filesystem;

    const fs::path root(contentRoot);
    if (!fs::exists(root)) {
        report.issues.push_back({.category = "missing-content", .message = "content root does not exist: " + contentRoot, .warning = false});
    }

    bool foundPatterns = false;
    bool foundEntities = false;
    bool foundTraits = false;

    if (fs::exists(root)) {
        for (const auto& e : fs::directory_iterator(root)) {
            if (!e.is_regular_file()) continue;
            ++report.filesScanned;
            const std::string name = e.path().filename().string();
            if (containsCaseInsensitive(name, "pattern")) foundPatterns = true;
            if (containsCaseInsensitive(name, "entit")) foundEntities = true;
            if (containsCaseInsensitive(name, "trait")) foundTraits = true;

            if (e.path().extension() == ".json") {
                try {
                    nlohmann::json j;
                    std::ifstream(e.path()) >> j;
                    const bool schemaLike = j.contains("patterns") || j.contains("entities") || j.contains("traits") || j.contains("waves") || j.contains("profiles");
                    if (!schemaLike) {
                        report.issues.push_back({.category = "schema-violation", .message = "unrecognized schema in " + name, .warning = true});
                    }
                } catch (...) {
                    report.issues.push_back({.category = "schema-violation", .message = "invalid JSON in " + name, .warning = false});
                }
            }
        }
    }

    if (!foundPatterns) report.issues.push_back({.category = "missing-content", .message = "no pattern content file discovered", .warning = false});
    if (!foundEntities) report.issues.push_back({.category = "missing-content", .message = "no entity content file discovered", .warning = false});
    if (!foundTraits) report.issues.push_back({.category = "missing-content", .message = "no trait content file discovered", .warning = false});

    if (snapshot.spawnsPerSecond > 250.0F) {
        report.issues.push_back({.category = "perf-hazard", .message = "high spawn rate hazard (spawns/sec > 250)", .warning = true});
    }

    if (snapshot.narrowphaseChecks > snapshot.broadphaseChecks && snapshot.broadphaseChecks > 0) {
        report.issues.push_back({.category = "determinism-hazard", .message = "narrowphase checks exceed broadphase checks", .warning = true});
    }

    if (snapshot.perfWarnSimulation || snapshot.perfWarnCollisions) {
        report.issues.push_back({.category = "determinism-hazard", .message = "simulation/collision budget warning may indicate unstable frame-step pressure", .warning = true});
    }

    return report;
}

} // namespace engine
