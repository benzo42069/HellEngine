#include <engine/editor_tools.h>

#include <engine/encounter_graph.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace engine {

void ControlCenterToolSuite::drawProjectileEditorPanel() {
if (showEntityEditor_) {
    ImGui::Begin("Projectile Editor");
    if (ImGui::Button("Spawn Homing")) projectileDebug_.spawnHoming = true;
    ImGui::SameLine();
    if (ImGui::Button("Spawn Curved")) projectileDebug_.spawnCurved = true;
    ImGui::SameLine();
    if (ImGui::Button("Spawn Accel/Drag")) projectileDebug_.spawnAccelDrag = true;
    if (ImGui::Button("Spawn Bounce")) projectileDebug_.spawnBounce = true;
    ImGui::SameLine();
    if (ImGui::Button("Spawn Split")) projectileDebug_.spawnSplit = true;
    ImGui::SameLine();
    if (ImGui::Button("Spawn Explode")) projectileDebug_.spawnExplode = true;
    if (ImGui::Button("Spawn Beam")) projectileDebug_.spawnBeam = true;
    ImGui::End();
}
}

void ControlCenterToolSuite::drawEncounterWaveEditorPanel() {
if (showWaveEditor_) {
    ImGui::Begin("Encounter / Wave Editor");
    const char* types[] = {"Wave", "Delay", "Elite", "Event", "Boss", "Difficulty"};
    ensureEncounterNodesSeeded();

    if (ImGui::Button("Add Node")) {
        const int next = static_cast<int>(encounterNodeIds_.size()) + 1;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "node-%03d", next);
        encounterNodeIds_.push_back(buf);
        encounterNodeTypes_.push_back(0);
        encounterNodeTimes_.push_back(static_cast<float>(next) * 2.0F);
        encounterNodeDurations_.push_back(0.0F);
        encounterNodeValues_.push_back(20.0F);
        encounterNodePayloads_.push_back("payload");
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        encounterNodeIds_.clear();
        encounterNodeTypes_.clear();
        encounterNodeTimes_.clear();
        encounterNodeDurations_.clear();
        encounterNodeValues_.clear();
        encounterNodePayloads_.clear();
    }

    for (std::size_t i = 0; i < encounterNodeIds_.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::InputText("ID", &encounterNodeIds_[i]);
        ImGui::Combo("Type", &encounterNodeTypes_[i], types, 6);
        ImGui::SliderFloat("Time", &encounterNodeTimes_[i], 0.0F, 120.0F);
        ImGui::SliderFloat("Duration", &encounterNodeDurations_[i], 0.0F, 30.0F);
        ImGui::SliderFloat("Value", &encounterNodeValues_[i], 0.0F, 200.0F);
        ImGui::InputText("Payload", &encounterNodePayloads_[i]);
        ImGui::Separator();
        ImGui::PopID();
    }

    EncounterCompiler compiler;
    EncounterSchedule schedule;
    const EncounterGraphAsset asset = buildEncounterAsset();
    (void)compiler.compile(asset, schedule);

    encounterState_.spawnTimeline.fill(0.0F);
    encounterState_.difficultyScalar = 1.0F;
    for (const EncounterScheduleEvent& e : schedule.events) {
        const int sec = std::clamp(static_cast<int>(e.atSeconds), 0, 59);
        if (e.type == EncounterNodeType::Wave || e.type == EncounterNodeType::EliteMarker || e.type == EncounterNodeType::BossTrigger) {
            encounterState_.spawnTimeline[sec] += std::max(1.0F, e.value * 0.1F);
        }
        if (e.type == EncounterNodeType::DifficultyScalar) encounterState_.difficultyScalar = std::max(0.1F, e.value);
    }

    ImGui::PlotHistogram("Spawn Visualization", encounterState_.spawnTimeline.data(), static_cast<int>(encounterState_.spawnTimeline.size()), 0, "spawn intensity", 0.0F, 30.0F, ImVec2(0.0F, 80.0F));
    if (ImGui::Button("Save Encounter")) {
        std::filesystem::create_directories("data/generated_encounters");
        std::string err;
        if (saveEncounterGraphJson(asset, "data/generated_encounters/stage_encounter.json", &err)) {
            statusMessage_ = "Saved encounter graph";
            if (validatorAutoOnSave_) validatorRequested_ = true;
        } else {
            statusMessage_ = "Encounter save failed: " + err;
        }
    }
    ImGui::End();
}
}

void ControlCenterToolSuite::drawTraitUpgradePanel(const ToolRuntimeSnapshot& snapshot) {
if (showTraitEditor_) {
    ImGui::Begin("Trait Editor + Upgrade UI Preview");
    ImGui::Checkbox("Spawn upgrade screen", &upgradeDebug_.spawnUpgradeScreen);
    ImGui::Checkbox("Show internal stat modifiers", &upgradeDebug_.showInternalStatModifiers);
    ImGui::Checkbox("Show danger field overlay", &upgradeDebug_.showDangerField);
    ImGui::Text("Force rarity");
    ImGui::RadioButton("Auto", &upgradeDebug_.forcedRarity, -1); ImGui::SameLine();
    ImGui::RadioButton("Common", &upgradeDebug_.forcedRarity, 0); ImGui::SameLine();
    ImGui::RadioButton("Rare", &upgradeDebug_.forcedRarity, 1); ImGui::SameLine();
    ImGui::RadioButton("Relic", &upgradeDebug_.forcedRarity, 2);
    ImGui::Text("Upgrade UI open: %s", snapshot.upgradeScreenOpen ? "yes" : "no");
    ImGui::Text("Runtime force rarity: %s", snapshot.forceRarityLabel);
    ImGui::End();
}
}

void ControlCenterToolSuite::ensureEncounterNodesSeeded() {
    if (!encounterNodeIds_.empty()) return;
    encounterNodeIds_.push_back("node-001");
    encounterNodeTypes_.push_back(0);
    encounterNodeTimes_.push_back(0.0F);
    encounterNodeDurations_.push_back(0.0F);
    encounterNodeValues_.push_back(20.0F);
    encounterNodePayloads_.push_back("default");
}

EncounterGraphAsset ControlCenterToolSuite::buildEncounterAsset() const {
    EncounterGraphAsset asset;
    asset.id = "designer_stage_encounter";
    for (std::size_t i = 0; i < encounterNodeIds_.size(); ++i) {
        asset.nodes.push_back(EncounterNode {
            .id = encounterNodeIds_[i],
            .type = static_cast<EncounterNodeType>(std::clamp(encounterNodeTypes_[i], 0, 5)),
            .timeSeconds = encounterNodeTimes_[i],
            .durationSeconds = encounterNodeDurations_[i],
            .value = encounterNodeValues_[i],
            .payload = encounterNodePayloads_[i],
        });
    }
    return asset;
}


} // namespace engine
