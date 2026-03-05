#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace engine {

enum class EncounterNodeType {
    Wave,
    Delay,
    EliteMarker,
    Event,
    BossTrigger,
    DifficultyScalar,
};

struct EncounterNode {
    std::string id;
    EncounterNodeType type {EncounterNodeType::Wave};
    float timeSeconds {0.0F};
    float durationSeconds {0.0F};
    float value {0.0F};
    std::string payload;
};

struct EncounterGraphAsset {
    std::string id;
    std::vector<EncounterNode> nodes;
};

struct EncounterScheduleEvent {
    float atSeconds {0.0F};
    EncounterNodeType type {EncounterNodeType::Wave};
    float value {0.0F};
    std::string payload;
};

struct EncounterCompileDiagnostic {
    std::string nodeId;
    std::string message;
    bool warning {false};
};

struct EncounterSchedule {
    std::string id;
    std::vector<EncounterScheduleEvent> events;
    std::vector<EncounterCompileDiagnostic> diagnostics;
};

class EncounterCompiler {
  public:
    bool compile(const EncounterGraphAsset& asset, EncounterSchedule& out) const;
};

bool saveEncounterGraphJson(const EncounterGraphAsset& asset, const std::string& path, std::string* error = nullptr);
bool loadEncounterGraphJson(const std::string& path, EncounterGraphAsset& out, std::string* error = nullptr);

} // namespace engine
