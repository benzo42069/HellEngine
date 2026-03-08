#include <engine/editor_tools.h>

#include <engine/encounter_graph.h>
#include <engine/pattern_generator.h>
#include <engine/pattern_graph.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <numeric>
#include <random>
#include <sstream>

namespace engine {

void ControlCenterToolSuite::drawPatternGraphEditorPanel(const ToolRuntimeSnapshot& snapshot) {
    (void)snapshot;
    if (!showPatternEditor_) {
        return;
    }

    ImGui::Begin("Pattern Graph Editor + Generator");
    ImGui::TextUnformatted("Compiled graph authoring (node palette + inspector + preview sandbox).");

    drawPatternGenerationControls();
    drawPatternSeedAndTestingControls();

    ensurePatternGraphSeeded();
    drawPatternGraphNodePalette();
    drawPatternGraphNodeInspector();

    const PatternGraphAsset previewAsset = buildPatternPreviewAsset();
    drawPatternPreviewAndAnalysis(previewAsset);

    ImGui::End();
}

void ControlCenterToolSuite::drawPatternGenerationControls() {
    ImGui::SeparatorText("Generation Controls");
    const char* styles[] = {"Balanced", "Spiral Dance", "Burst Fan", "Sniper Lanes"};
    ImGui::Combo("Style Preset", &patternGenerator_.stylePreset, styles, 4);
    ImGui::SliderFloat("Density", &patternGenerator_.density, 0.0F, 1.0F);
    ImGui::SliderFloat("Speed", &patternGenerator_.speed, 0.0F, 1.0F);
    ImGui::SliderFloat("Symmetry", &patternGenerator_.symmetry, 0.0F, 1.0F);
    ImGui::SliderFloat("Chaos", &patternGenerator_.chaos, 0.0F, 1.0F);
    ImGui::SliderFloat("Fairness", &patternGenerator_.fairness, 0.0F, 1.0F);

    PatternGeneratorInput input;
    input.style = static_cast<PatternStylePreset>(patternGenerator_.stylePreset);
    input.seed = patternGenerator_.seed;
    input.density = patternGenerator_.density;
    input.speed = patternGenerator_.speed;
    input.symmetry = patternGenerator_.symmetry;
    input.chaos = patternGenerator_.chaos;
    input.fairness = patternGenerator_.fairness;

    const PatternGeneratorResult r = generatePatternGraphJson(input);
    patternGenerator_.difficultyScore = r.metrics.estimatedDifficultyScore;
    patternGenerator_.dodgeGapEstimate = r.metrics.averageDodgeGapEstimate;
    patternGenerator_.bulletsPerSecond = r.metrics.estimatedBulletsPerSecond;
    std::copy(r.metrics.densityHeatmap.begin(), r.metrics.densityHeatmap.end(), patternGenerator_.heatmap.begin());
}

void ControlCenterToolSuite::drawPatternSeedAndTestingControls() {
    ImGui::SeparatorText("Seed + Testing");
    ImGui::InputScalar("Seed", ImGuiDataType_U64, &patternGenerator_.seed);
    ImGui::Text("Estimated difficulty: %.2f", patternGenerator_.difficultyScore);
    ImGui::Text("Dodge gap estimate: %.2f", patternGenerator_.dodgeGapEstimate);
    ImGui::Text("Estimated bullets/s: %.2f", patternGenerator_.bulletsPerSecond);
    ImGui::PlotLines("Density Heatmap", patternGenerator_.heatmap.data(), static_cast<int>(patternGenerator_.heatmap.size()), 0, nullptr, 0.0F, 1.0F, ImVec2(0.0F, 70.0F));

    if (ImGui::Button("Generate Graph")) {
        patternGenerator_.requestGenerate = true;
        statusMessage_ = "Pattern graph generation requested";
    }
    ImGui::SameLine();
    if (ImGui::Button("Mutate")) {
        patternGenerator_.requestMutate = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Remix")) {
        patternGenerator_.requestRemix = true;
    }
}

void ControlCenterToolSuite::drawPatternGraphNodePalette() {
    ImGui::SeparatorText("Graph Editing: Node Palette");
    const char* nodeTypes[] = {"Emit Ring", "Emit Spread", "Emit Spiral", "Emit Wave", "Emit Aimed", "Wait", "Loop", "Rotate", "Phase", "Random"};
    for (int i = 0; i < 10; ++i) {
        ImGui::PushID(i);
        if (ImGui::Button(nodeTypes[i])) {
            const int next = static_cast<int>(graphNodeIds_.size()) + 1;
            char id[32];
            std::snprintf(id, sizeof(id), "%03d-node", 10 * next);
            graphNodeIds_.push_back(id);
            graphNodeTypes_.push_back(i);
            graphNodeA_.push_back(i == 5 ? 0.1F : 8.0F);
            graphNodeB_.push_back(120.0F);
            graphNodeC_.push_back(3.0F);
            graphNodeD_.push_back(0.0F);
            graphNodeTarget_.push_back("010-emit");
            selectedGraphNode_ = static_cast<int>(graphNodeIds_.size()) - 1;
        }
        if (i < 9) {
            ImGui::SameLine();
        }
        ImGui::PopID();
    }
}

void ControlCenterToolSuite::drawPatternGraphNodeInspector() {
    ImGui::SeparatorText("Graph Editing: Node Inspector");
    const char* nodeTypes[] = {"Emit Ring", "Emit Spread", "Emit Spiral", "Emit Wave", "Emit Aimed", "Wait", "Loop", "Rotate", "Phase", "Random"};

    for (std::size_t i = 0; i < graphNodeIds_.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Selectable(graphNodeIds_[i].c_str(), selectedGraphNode_ == static_cast<int>(i))) {
            selectedGraphNode_ = static_cast<int>(i);
        }
        ImGui::PopID();
    }

    selectedGraphNode_ = std::clamp(selectedGraphNode_, 0, static_cast<int>(graphNodeIds_.size()) - 1);
    if (graphNodeIds_.empty()) {
        return;
    }

    const std::size_t i = static_cast<std::size_t>(selectedGraphNode_);
    ImGui::InputText("Node ID", &graphNodeIds_[i]);
    ImGui::Combo("Node Type", &graphNodeTypes_[i], nodeTypes, 10);
    ImGui::SliderFloat("A", &graphNodeA_[i], -360.0F, 360.0F);
    ImGui::SliderFloat("B", &graphNodeB_[i], -360.0F, 360.0F);
    ImGui::SliderFloat("C", &graphNodeC_[i], -360.0F, 360.0F);
    ImGui::SliderFloat("D", &graphNodeD_[i], -360.0F, 360.0F);
    ImGui::InputText("Loop Target", &graphNodeTarget_[i]);
}

PatternGraphAsset ControlCenterToolSuite::buildPatternPreviewAsset() const {
    PatternGraphAsset previewAsset;
    previewAsset.id = "editor-preview";

    for (std::size_t i = 0; i < graphNodeIds_.size(); ++i) {
        PatternGraphNode node;
        node.id = graphNodeIds_[i];
        switch (std::clamp(graphNodeTypes_[i], 0, 9)) {
            case 0: node.type = PatternGraphNodeType::EmitRing; node.params = {{"count", graphNodeA_[i]}, {"speed", graphNodeB_[i]}, {"radius", graphNodeC_[i]}, {"angle", graphNodeD_[i]}}; break;
            case 1: node.type = PatternGraphNodeType::EmitSpread; node.params = {{"count", graphNodeA_[i]}, {"speed", graphNodeB_[i]}, {"radius", graphNodeC_[i]}, {"angle", graphNodeD_[i]}}; break;
            case 2: node.type = PatternGraphNodeType::EmitSpiral; node.params = {{"count", graphNodeA_[i]}, {"speed", graphNodeB_[i]}, {"radius", graphNodeC_[i]}, {"angle", graphNodeD_[i]}}; break;
            case 3: node.type = PatternGraphNodeType::EmitWave; node.params = {{"count", graphNodeA_[i]}, {"speed", graphNodeB_[i]}, {"radius", graphNodeC_[i]}, {"angle", graphNodeD_[i]}}; break;
            case 4: node.type = PatternGraphNodeType::EmitAimed; node.params = {{"count", graphNodeA_[i]}, {"speed", graphNodeB_[i]}, {"radius", graphNodeC_[i]}, {"angle", graphNodeD_[i]}}; break;
            case 5: node.type = PatternGraphNodeType::Wait; node.params = {{"seconds", std::max(0.01F, graphNodeA_[i])}}; break;
            case 6: node.type = PatternGraphNodeType::Loop; node.params = {{"count", graphNodeA_[i]}}; node.targetNodeId = graphNodeTarget_[i]; break;
            case 7: node.type = PatternGraphNodeType::ModifierRotate; node.params = {{"deg", graphNodeA_[i]}}; break;
            case 8: node.type = PatternGraphNodeType::ModifierPhaseOffset; node.params = {{"deg", graphNodeA_[i]}}; break;
            case 9: node.type = PatternGraphNodeType::RandomRange; node.params = {{"min", graphNodeA_[i]}, {"max", graphNodeB_[i]}, {"stream", std::max(0.0F, graphNodeC_[i])}}; break;
            default: break;
        }
        previewAsset.nodes.push_back(std::move(node));
    }

    return previewAsset;
}

void ControlCenterToolSuite::drawPatternPreviewAndAnalysis(const PatternGraphAsset& previewAsset) {
    PatternGraphCompiler graphCompiler;
    CompiledPatternGraph compiled;
    const bool previewOk = graphCompiler.compile(previewAsset, compiled);

    ImGui::SeparatorText("Simulation Preview + Analysis");
    ImGui::Text("Compile: %s", previewOk ? "PASS" : "FAIL");
    ImGui::Text("Ops: %d", static_cast<int>(compiled.ops.size()));
    ImGui::Text("Estimated spawns/s: %.1f", compiled.staticSpawnRateEstimatePerSecond);
    for (const PatternGraphDiagnostic& d : compiled.diagnostics) {
        ImVec4 c = d.warning ? ImVec4(1.0F, 0.8F, 0.2F, 1.0F) : ImVec4(1.0F, 0.3F, 0.3F, 1.0F);
        ImGui::TextColored(c, "[%s] %s", d.nodeId.c_str(), d.message.c_str());
    }

    if (ImGui::Button("Save Preview Graph")) {
        std::filesystem::create_directories("data/generated_graphs");
        std::string err;
        if (savePatternGraphsToFile("data/generated_graphs/preview_graph.json", {previewAsset}, &err)) {
            statusMessage_ = "Saved preview graph";
        } else {
            statusMessage_ = "Save failed: " + err;
        }
    }
}

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

void ControlCenterToolSuite::ensurePatternGraphSeeded() {
    if (!graphNodeIds_.empty()) return;
    graphNodeIds_ = {"010-emit", "020-wait", "030-loop"};
    graphNodeTypes_ = {0, 5, 6};
    graphNodeA_ = {12.0F, 0.12F, 8.0F};
    graphNodeB_ = {140.0F, 0.0F, 0.0F};
    graphNodeC_ = {3.0F, 0.0F, 0.0F};
    graphNodeD_ = {0.0F, 0.0F, 0.0F};
    graphNodeTarget_ = {"", "", "010-emit"};
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
