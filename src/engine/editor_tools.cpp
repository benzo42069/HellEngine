#include <engine/editor_tools.h>
#include <engine/encounter_graph.h>
#include <engine/pattern_generator.h>

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <numeric>

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

void ControlCenterToolSuite::initialize(SDL_Window* window, SDL_Renderer* renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    renderer_ = renderer;
    initialized_ = true;
}

void ControlCenterToolSuite::shutdown() {
    if (!initialized_) return;
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    renderer_ = nullptr;
    initialized_ = false;
}

void ControlCenterToolSuite::processEvent(const SDL_Event& event) {
    if (!initialized_) return;
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void ControlCenterToolSuite::beginFrame() {
    if (!initialized_) return;
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ControlCenterToolSuite::drawControlCenter(const ToolRuntimeSnapshot& snapshot) {
    if (!initialized_) return;

    frameHistoryMs_.push_back(snapshot.frameMs);
    if (frameHistoryMs_.size() > 240) frameHistoryMs_.pop_front();

    if (browserEntries_.empty()) {
        browserEntries_ = {"data/entities.json", "data/difficulty_profiles.json", "assets/patterns/sandbox_patterns.json", "data/generated_demo/demo_traits.json", "data/generated_encounters/stage_encounter.json"};
    }

    if (validatorRequested_) {
        validationReport_ = runControlCenterValidation("data", snapshot);
        statusMessage_ = validationReport_.issues.empty() ? "Validator: PASS" : "Validator: issues found";
        validatorRequested_ = false;
    }

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Workspace")) {
            if (ImGui::MenuItem("Save Layout")) {
                std::filesystem::create_directories("data/layouts");
                ImGui::SaveIniSettingsToDisk("data/layouts/control_center_layout.ini");
                statusMessage_ = "Saved workspace layout";
                if (validatorAutoOnSave_) validatorRequested_ = true;
            }
            if (ImGui::MenuItem("Load Layout")) {
                ImGui::LoadIniSettingsFromDisk("data/layouts/control_center_layout.ini");
                statusMessage_ = "Loaded workspace layout";
            }
            ImGui::Separator();
            ImGui::Checkbox("Auto validate on save", &validatorAutoOnSave_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Generate Demo Content")) {
                std::string error;
                if (generateDemoContent("data/generated_demo", &error)) {
                    statusMessage_ = "Generated demo content in data/generated_demo";
                } else {
                    statusMessage_ = "Generate failed: " + error;
                }
            }
            if (ImGui::MenuItem("Run Validator")) {
                validatorRequested_ = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    auto buildEncounterAsset = [this]() {
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
    };

    ImGui::Begin("Control Center Status", &open_);
    ImGui::TextUnformatted(statusMessage_.c_str());
    ImGui::Text("Tick %llu", static_cast<unsigned long long>(snapshot.tick));
    ImGui::Text("Difficulty %s %.2f", snapshot.difficultyProfileLabel, snapshot.difficultyOverall);
    ImGui::End();

    if (showContentBrowser_) {
        ImGui::Begin("Content Browser");
        ImGui::InputText("Search", &browserSearch_);
        ImGui::InputText("Tag Filter", &browserTagFilter_);
        for (const std::string& e : browserEntries_) {
            if (!containsCaseInsensitive(e, browserSearch_)) continue;
            if (!browserTagFilter_.empty() && !containsCaseInsensitive(e, browserTagFilter_)) continue;
            ImGui::BulletText("%s", e.c_str());
        }
        ImGui::End();
    }

    if (showPatternEditor_) {
        ImGui::Begin("Pattern Graph Editor + Generator");
        ImGui::TextUnformatted("Edit pattern layers, cadence, spread and deterministic jitter.");

        const char* styles[] = {"Balanced", "Spiral Dance", "Burst Fan", "Sniper Lanes"};
        ImGui::Combo("Style Preset", &patternGenerator_.stylePreset, styles, 4);
        ImGui::InputScalar("Seed", ImGuiDataType_U64, &patternGenerator_.seed);
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

        ImGui::Text("Estimated difficulty: %.2f", patternGenerator_.difficultyScore);
        ImGui::PlotLines("Density Heatmap", patternGenerator_.heatmap.data(), static_cast<int>(patternGenerator_.heatmap.size()), 0, nullptr, 0.0F, 1.0F, ImVec2(0.0F, 70.0F));
        if (ImGui::Button("Generate Graph")) {
            patternGenerator_.requestGenerate = true;
            statusMessage_ = "Pattern graph generation requested";
        }
        ImGui::SameLine();
        if (ImGui::Button("Mutate")) patternGenerator_.requestMutate = true;
        ImGui::SameLine();
        if (ImGui::Button("Remix")) patternGenerator_.requestRemix = true;
        ImGui::End();
    }

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

    if (showWaveEditor_) {
        ImGui::Begin("Encounter / Wave Editor");
        const char* types[] = {"Wave", "Delay", "Elite", "Event", "Boss", "Difficulty"};
        if (encounterNodeIds_.empty()) {
            encounterNodeIds_.push_back("node-001");
            encounterNodeTypes_.push_back(0);
            encounterNodeTimes_.push_back(0.0F);
            encounterNodeDurations_.push_back(0.0F);
            encounterNodeValues_.push_back(20.0F);
            encounterNodePayloads_.push_back("default");
        }

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

    if (showTraitEditor_) {
        ImGui::Begin("Trait Editor + Upgrade UI Preview");
        ImGui::Checkbox("Spawn upgrade screen", &upgradeDebug_.spawnUpgradeScreen);
        ImGui::Checkbox("Show internal stat modifiers", &upgradeDebug_.showInternalStatModifiers);
        ImGui::Text("Force rarity");
        ImGui::RadioButton("Auto", &upgradeDebug_.forcedRarity, -1); ImGui::SameLine();
        ImGui::RadioButton("Common", &upgradeDebug_.forcedRarity, 0); ImGui::SameLine();
        ImGui::RadioButton("Rare", &upgradeDebug_.forcedRarity, 1); ImGui::SameLine();
        ImGui::RadioButton("Relic", &upgradeDebug_.forcedRarity, 2);
        ImGui::Text("Upgrade UI open: %s", snapshot.upgradeScreenOpen ? "yes" : "no");
        ImGui::Text("Runtime force rarity: %s", snapshot.forceRarityLabel);
        ImGui::End();
    }

    if (showProfiler_) {
        ImGui::Begin("Profiler + Replay Inspector");
        ImGui::Text("Frame %.2fms (%d FPS)", snapshot.frameTimeMs, snapshot.fps);
        ImGui::Text("Sim %.2f  Pat %.2f  Bul %.2f  Col %.2f  Ren %.2f", snapshot.simMs, snapshot.patternMs, snapshot.bulletMs, snapshot.collisionMs, snapshot.renderMs);
        ImGui::Text("DrawCalls %u  Batches %u", snapshot.drawCalls, snapshot.renderBatches);
        ImGui::Text("GPU Bullets %u  GPU Update %.3fms  GPU Render %.3fms", snapshot.gpuActiveBullets, snapshot.gpuUpdateMs, snapshot.gpuRenderMs);
        ImGui::Text("Replay Tick %llu", static_cast<unsigned long long>(snapshot.tick));
        if (snapshot.perfWarnSimulation || snapshot.perfWarnRender || snapshot.perfWarnCollisions) {
            ImGui::TextColored(ImVec4(1.0F, 0.5F, 0.2F, 1.0F), "Perf warning active");
        }
        ImGui::End();
    }

    if (showValidator_) {
        ImGui::Begin("Validator");
        if (ImGui::Button("Run Validator Command")) validatorRequested_ = true;
        ImGui::Text("Files scanned: %u", validationReport_.filesScanned);
        if (validationReport_.issues.empty()) {
            ImGui::TextColored(ImVec4(0.2F, 0.95F, 0.3F, 1.0F), "PASS: no issues");
        } else {
            for (const auto& issue : validationReport_.issues) {
                const ImVec4 color = issue.warning ? ImVec4(1.0F, 0.75F, 0.2F, 1.0F) : ImVec4(1.0F, 0.35F, 0.35F, 1.0F);
                ImGui::TextColored(color, "[%s] %s", issue.category.c_str(), issue.message.c_str());
            }
        }
        ImGui::End();
    }
}

const UpgradeDebugOptions& ControlCenterToolSuite::upgradeDebugOptions() const { return upgradeDebug_; }

ProjectileDebugOptions ControlCenterToolSuite::consumeProjectileDebugOptions() {
    ProjectileDebugOptions out = projectileDebug_;
    projectileDebug_ = {};
    return out;
}

PatternGeneratorDebugState& ControlCenterToolSuite::patternGeneratorState() { return patternGenerator_; }

std::string ControlCenterToolSuite::consumeGeneratedGraphPath() {
    std::string out = patternGenerator_.generatedGraphPath;
    patternGenerator_.generatedGraphPath.clear();
    return out;
}

void ControlCenterToolSuite::endFrame() {
    if (!initialized_) return;
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
}

} // namespace engine
