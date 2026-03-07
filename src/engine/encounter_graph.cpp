#include <engine/encounter_graph.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace engine {

namespace {
std::string toString(const EncounterNodeType t) {
    switch (t) {
        case EncounterNodeType::Wave: return "wave";
        case EncounterNodeType::Delay: return "delay";
        case EncounterNodeType::EliteMarker: return "elite";
        case EncounterNodeType::Event: return "event";
        case EncounterNodeType::BossTrigger: return "boss";
        case EncounterNodeType::DifficultyScalar: return "difficulty";
        case EncounterNodeType::HazardSync: return "hazardSync";
        case EncounterNodeType::Telegraph: return "telegraph";
        case EncounterNodeType::PhaseGate: return "phaseGate";
    }
    return "wave";
}

EncounterNodeType parseType(const std::string& s) {
    if (s == "wave") return EncounterNodeType::Wave;
    if (s == "delay") return EncounterNodeType::Delay;
    if (s == "elite") return EncounterNodeType::EliteMarker;
    if (s == "event") return EncounterNodeType::Event;
    if (s == "boss") return EncounterNodeType::BossTrigger;
    if (s == "difficulty") return EncounterNodeType::DifficultyScalar;
    if (s == "hazardSync") return EncounterNodeType::HazardSync;
    if (s == "telegraph") return EncounterNodeType::Telegraph;
    if (s == "phaseGate") return EncounterNodeType::PhaseGate;
    return EncounterNodeType::Wave;
}
} // namespace

bool EncounterCompiler::compile(const EncounterGraphAsset& asset, EncounterSchedule& out) const {
    out = {};
    out.id = asset.id;

    if (asset.nodes.empty()) {
        out.diagnostics.push_back(EncounterCompileDiagnostic {.nodeId = asset.id, .message = "encounter graph has no nodes", .warning = false});
        return false;
    }

    std::vector<EncounterNode> nodes = asset.nodes;
    std::sort(nodes.begin(), nodes.end(), [](const EncounterNode& a, const EncounterNode& b) {
        if (a.timeSeconds == b.timeSeconds) return a.id < b.id;
        return a.timeSeconds < b.timeSeconds;
    });

    float cursor = 0.0F;
    for (const EncounterNode& n : nodes) {
        float at = std::max(0.0F, n.timeSeconds);
        if (n.type == EncounterNodeType::Delay) {
            cursor = std::max(cursor, at) + std::max(0.0F, n.durationSeconds);
            continue;
        }
        if (n.type == EncounterNodeType::BossTrigger && at < 15.0F) {
            out.diagnostics.push_back(EncounterCompileDiagnostic {.nodeId = n.id, .message = "boss trigger before 15s may be too early", .warning = true});
        }
        if (n.type == EncounterNodeType::Wave && n.value > 150.0F) {
            out.diagnostics.push_back(EncounterCompileDiagnostic {.nodeId = n.id, .message = "wave value exceeds readability threshold", .warning = true});
        }
        if (n.type == EncounterNodeType::Telegraph && n.durationSeconds <= 0.0F) {
            out.diagnostics.push_back(EncounterCompileDiagnostic {.nodeId = n.id, .message = "telegraph should define durationSeconds", .warning = true});
        }

        const float scheduledAt = std::max(at, cursor);
        std::string owner = "encounter";
        if (n.type == EncounterNodeType::BossTrigger || n.type == EncounterNodeType::PhaseGate || n.type == EncounterNodeType::Telegraph) owner = "boss";
        if (n.type == EncounterNodeType::HazardSync) owner = "hazards";
        out.events.push_back(EncounterScheduleEvent {.atSeconds = scheduledAt, .type = n.type, .value = n.value, .durationSeconds = n.durationSeconds, .payload = n.payload, .owner = owner});
    }

    std::sort(out.events.begin(), out.events.end(), [](const EncounterScheduleEvent& a, const EncounterScheduleEvent& b) {
        if (a.atSeconds == b.atSeconds) return static_cast<int>(a.type) < static_cast<int>(b.type);
        return a.atSeconds < b.atSeconds;
    });

    return true;
}

bool saveEncounterGraphJson(const EncounterGraphAsset& asset, const std::string& path, std::string* error) {
    try {
        nlohmann::json root;
        root["id"] = asset.id;
        root["nodes"] = nlohmann::json::array();
        for (const EncounterNode& n : asset.nodes) {
            root["nodes"].push_back({
                {"id", n.id},
                {"type", toString(n.type)},
                {"timeSeconds", n.timeSeconds},
                {"durationSeconds", n.durationSeconds},
                {"value", n.value},
                {"payload", n.payload},
                {"owner", "encounter"},
            });
        }
        std::ofstream(path) << root.dump(2);
        return true;
    } catch (const std::exception& ex) {
        if (error) *error = ex.what();
        return false;
    }
}

bool loadEncounterGraphJson(const std::string& path, EncounterGraphAsset& out, std::string* error) {
    try {
        std::ifstream in(path);
        if (!in.good()) return false;
        nlohmann::json root;
        in >> root;
        out = {};
        out.id = root.value("id", "encounter");
        if (root.contains("nodes") && root["nodes"].is_array()) {
            for (const auto& n : root["nodes"]) {
                out.nodes.push_back(EncounterNode {
                    .id = n.value("id", "node"),
                    .type = parseType(n.value("type", "wave")),
                    .timeSeconds = n.value("timeSeconds", 0.0F),
                    .durationSeconds = n.value("durationSeconds", 0.0F),
                    .value = n.value("value", 0.0F),
                    .payload = n.value("payload", ""),
                });
            }
        }
        return true;
    } catch (const std::exception& ex) {
        if (error) *error = ex.what();
        return false;
    }
}

} // namespace engine
