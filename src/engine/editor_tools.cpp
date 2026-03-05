#include <engine/editor_tools.h>
#include <engine/encounter_graph.h>
#include <engine/pattern_generator.h>
#include <engine/standards.h>
#include <engine/palette_fx_templates.h>
#include <engine/pattern_graph.h>
#include <engine/public/plugins.h>

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <random>
#include <sstream>

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

    std::filesystem::create_directories("data/layouts");
    std::string paletteError;
    paletteRegistryLoaded_ = paletteRegistry_.loadFromJsonFile("data/palettes/palette_fx_templates.json", &paletteError);
    if (!paletteRegistryLoaded_) {
        appendConsole("Palette template load failed: " + paletteError);
    } else {
        appendConsole("Loaded Palette & FX templates");
    }
    if (std::filesystem::exists("data/layouts/control_center_layout.ini")) {
        ImGui::LoadIniSettingsFromDisk("data/layouts/control_center_layout.ini");
        workspaceLayoutInitialized_ = true;
    }

    appendConsole("Control Center initialized");
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

void ControlCenterToolSuite::appendConsole(const std::string& message) {
    consoleLines_.push_back(message);
    while (consoleLines_.size() > 256) {
        consoleLines_.pop_front();
    }
}

void ControlCenterToolSuite::drawControlCenter(const ToolRuntimeSnapshot& snapshot) {
    if (!initialized_) return;

    frameHistoryMs_.push_back(snapshot.frameMs);
    if (frameHistoryMs_.size() > 240) frameHistoryMs_.pop_front();

    if (browserEntries_.empty()) {
        browserEntries_ = {"data/entities.json", "data/difficulty_profiles.json", "assets/patterns/sandbox_patterns.json", "data/generated_demo/demo_traits.json", "data/generated_encounters/stage_encounter.json"};
        selectedAssetPath_ = browserEntries_.front();
    }

    if (validatorRequested_) {
        validationReport_ = runControlCenterValidation("data", snapshot);
        statusMessage_ = validationReport_.issues.empty() ? "Validator: PASS" : "Validator: issues found";
        appendConsole(statusMessage_);
        validatorRequested_ = false;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    const ImGuiWindowFlags workspaceFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::Begin("Tools Workspace", nullptr, workspaceFlags);
    ImGui::PopStyleVar(2);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::BeginMenu("Workspace")) {
                if (ImGui::MenuItem("Save Layout")) {
                    std::filesystem::create_directories("data/layouts");
                    ImGui::SaveIniSettingsToDisk("data/layouts/control_center_layout.ini");
                    statusMessage_ = "Saved workspace layout";
                    appendConsole(statusMessage_);
                    if (validatorAutoOnSave_) validatorRequested_ = true;
                }
                if (ImGui::MenuItem("Load Layout")) {
                    ImGui::LoadIniSettingsFromDisk("data/layouts/control_center_layout.ini");
                    statusMessage_ = "Loaded workspace layout";
                    appendConsole(statusMessage_);
                }
                ImGui::Checkbox("Auto validate on save", &validatorAutoOnSave_);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Panels")) {
                ImGui::MenuItem("Content Browser", nullptr, &showContentBrowser_);
                ImGui::MenuItem("Inspector", nullptr, &showInspector_);
                ImGui::MenuItem("Preview Viewport", nullptr, &showPreviewViewport_);
                ImGui::MenuItem("Console / Log", nullptr, &showConsole_);
                ImGui::MenuItem("Pattern Generator", nullptr, &showPatternEditor_);
                ImGui::MenuItem("Projectile Editor", nullptr, &showEntityEditor_);
                ImGui::MenuItem("Wave Editor", nullptr, &showWaveEditor_);
                ImGui::MenuItem("Trait Editor", nullptr, &showTraitEditor_);
                ImGui::MenuItem("Profiler", nullptr, &showProfiler_);
                ImGui::MenuItem("Validator", nullptr, &showValidator_);
                ImGui::MenuItem("Palette & FX Templates", nullptr, &showPaletteFxTemplates_);
                ImGui::MenuItem("Standards", nullptr, &showStandards_);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Actions")) {
                if (ImGui::MenuItem("Generate Demo Content")) {
                    std::string error;
                    if (generateDemoContent("data/generated_demo", &error)) {
                        statusMessage_ = "Generated demo content in data/generated_demo";
                    } else {
                        statusMessage_ = "Generate failed: " + error;
                    }
                    appendConsole(statusMessage_);
                }
                if (ImGui::MenuItem("Run Validator")) {
                    validatorRequested_ = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::TextUnformatted("Unified tools workspace");
    ImGui::SameLine();
    ImGui::TextDisabled("| %s", statusMessage_.c_str());

    if (ImGui::BeginTable("tools_workspace_grid", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch, 0.26F);
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.48F);
        ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch, 0.26F);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        if (showContentBrowser_ && ImGui::BeginChild("Content Browser", ImVec2(0.0F, 0.0F), ImGuiChildFlags_Border)) {
            ImGui::TextUnformatted("Content Browser");
            ImGui::InputText("Search", &browserSearch_);
            ImGui::InputText("Tag Filter", &browserTagFilter_);
            ImGui::Separator();
            for (const std::string& e : browserEntries_) {
                if (!containsCaseInsensitive(e, browserSearch_)) continue;
                if (!browserTagFilter_.empty() && !containsCaseInsensitive(e, browserTagFilter_)) continue;
                const bool selected = (selectedAssetPath_ == e);
                if (ImGui::Selectable(e.c_str(), selected)) {
                    selectedAssetPath_ = e;
                    selectedAssetTag_ = browserTagFilter_;
                }
            }
            ImGui::EndChild();
        }

        ImGui::TableSetColumnIndex(1);
        if (showPreviewViewport_ && ImGui::BeginChild("Preview Viewport", ImVec2(0.0F, 0.0F), ImGuiChildFlags_Border)) {
            ImGui::TextUnformatted("Preview Viewport");
            ImGui::Text("Tick %llu", static_cast<unsigned long long>(snapshot.tick));
            ImGui::Text("Difficulty %s  (x%.2f)", snapshot.difficultyProfileLabel, snapshot.difficultyOverall);
            ImGui::Text("Projectiles %u | Entities %u", snapshot.projectileCount, snapshot.entityCount);
            ImGui::Separator();
            ImGui::TextWrapped("Preview placeholder bound to runtime counters. This keeps tooling cohesive while preview hooks are integrated.");
            if (showProfiler_) {
                ImGui::SeparatorText("Profiler");
                ImGui::Text("Frame %.2fms (%d FPS)", snapshot.frameTimeMs, snapshot.fps);
                ImGui::Text("Sim %.2f  Pat %.2f  Bul %.2f  Col %.2f  Ren %.2f", snapshot.simMs, snapshot.patternMs, snapshot.bulletMs, snapshot.collisionMs, snapshot.renderMs);
                ImGui::Text("DrawCalls %u  Batches %u", snapshot.drawCalls, snapshot.renderBatches);
                ImGui::Text("GPU Bullets %u  GPU Update %.3fms  GPU Render %.3fms", snapshot.gpuActiveBullets, snapshot.gpuUpdateMs, snapshot.gpuRenderMs);
            }
            ImGui::EndChild();
        }

        ImGui::TableSetColumnIndex(2);
        if (showInspector_ && ImGui::BeginChild("Inspector", ImVec2(0.0F, 0.0F), ImGuiChildFlags_Border)) {
            ImGui::TextUnformatted("Inspector");
            if (selectedAssetPath_.empty()) {
                ImGui::TextDisabled("No asset selected.");
            } else {
                ImGui::Text("Path: %s", selectedAssetPath_.c_str());
                ImGui::Text("Tag: %s", selectedAssetTag_.empty() ? "(none)" : selectedAssetTag_.c_str());
                std::error_code ec;
                const auto size = std::filesystem::exists(selectedAssetPath_, ec) ? std::filesystem::file_size(selectedAssetPath_, ec) : 0;
                if (!ec && size > 0) {
                    ImGui::Text("Size: %llu bytes", static_cast<unsigned long long>(size));
                }
                ImGui::TextWrapped("Selected asset metadata and entity hooks appear here.");
            }
            ImGui::Separator();
            ImGui::Text("Upgrade UI open: %s", snapshot.upgradeScreenOpen ? "yes" : "no");
            ImGui::Text("Runtime force rarity: %s", snapshot.forceRarityLabel);
            ImGui::EndChild();
        }

        ImGui::EndTable();
    }

    if (showConsole_) {
        ImGui::BeginChild("Console", ImVec2(0.0F, 170.0F), ImGuiChildFlags_Border);
        ImGui::TextUnformatted("Console / Log");
        ImGui::Separator();
        for (const std::string& line : consoleLines_) {
            ImGui::TextWrapped("%s", line.c_str());
        }
        if (ImGui::Button("Clear Console")) {
            consoleLines_.clear();
        }
        ImGui::EndChild();
    }

    ImGui::End();

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


    if (showPaletteFxTemplates_) {
        ImGui::Begin("Palette & FX Templates");
        std::string reloadError;
        if (paletteRegistry_.reloadIfChanged(&reloadError)) {
            appendConsole("Palette/FX templates hot reloaded");
        }

        if (!paletteRegistryLoaded_) {
            ImGui::TextDisabled("Template database not loaded from data/palettes/palette_fx_templates.json");
        }

        const auto categories = paletteRegistry_.categories();
        if (!categories.empty()) {
            paletteCategoryIndex_ = std::clamp(paletteCategoryIndex_, 0, static_cast<int>(categories.size()) - 1);
            if (ImGui::BeginCombo("Category", categories[paletteCategoryIndex_].c_str())) {
                for (int i = 0; i < static_cast<int>(categories.size()); ++i) {
                    const bool selected = i == paletteCategoryIndex_;
                    if (ImGui::Selectable(categories[i].c_str(), selected)) {
                        paletteCategoryIndex_ = i;
                        paletteTemplateIndex_ = 0;
                    }
                }
                ImGui::EndCombo();
            }

            const auto templates = paletteRegistry_.templatesForCategory(categories[paletteCategoryIndex_]);
            if (!templates.empty()) {
                paletteTemplateIndex_ = std::clamp(paletteTemplateIndex_, 0, static_cast<int>(templates.size()) - 1);
                if (ImGui::BeginCombo("Template", templates[paletteTemplateIndex_]->name.c_str())) {
                    for (int i = 0; i < static_cast<int>(templates.size()); ++i) {
                        const bool selected = i == paletteTemplateIndex_;
                        if (ImGui::Selectable(templates[i]->name.c_str(), selected)) {
                            paletteTemplateIndex_ = i;
                            paletteEditedTemplate_ = *templates[i];
                        }
                    }
                    ImGui::EndCombo();
                }

                if (!paletteEditedTemplate_.has_value()) paletteEditedTemplate_ = *templates[paletteTemplateIndex_];
                auto& edited = *paletteEditedTemplate_;

                ImGui::SeparatorText("Palette Editing");
                ImGui::Checkbox("Paint Mode", &palettePaintMode_);
                ImGui::SameLine();
                if (ImGui::Button("Fill Derive")) {
                    if (!edited.layerColors.empty()) {
                        if (edited.category == "Background") {
                            PaletteFillResult fill = deriveBackgroundFillFromAccent(edited.layerColors.front());
                            if (edited.layerColors.size() >= 3) {
                                edited.layerColors[0] = fill.core;
                                edited.layerColors[1] = fill.highlight;
                                edited.layerColors[2] = fill.glow;
                            }
                        } else {
                            PaletteFillResult fill = deriveProjectileFillFromCore(edited.layerColors.front());
                            if (edited.layerColors.size() >= 3) {
                                edited.layerColors[0] = fill.core;
                                edited.layerColors[1] = fill.highlight;
                                edited.layerColors[2] = fill.glow;
                            }
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Copy")) paletteClipboard_ = edited.layerColors;
                ImGui::SameLine();
                if (ImGui::Button("Paste") && !paletteClipboard_.empty()) edited.layerColors = paletteClipboard_;
                ImGui::SameLine();
                if (ImGui::Button("Revert")) edited = *templates[paletteTemplateIndex_];
                ImGui::SameLine();
                if (ImGui::Button("Randomize")) {
                    std::mt19937 rng(static_cast<unsigned>(snapshot.tick));
                    std::uniform_real_distribution<float> delta(-0.12F, 0.12F);
                    for (auto& c : edited.layerColors) {
                        c.r = std::clamp(c.r + delta(rng), 0.0F, 1.0F);
                        c.g = std::clamp(c.g + delta(rng), 0.0F, 1.0F);
                        c.b = std::clamp(c.b + delta(rng), 0.0F, 1.0F);
                    }
                }

                for (std::size_t i = 0; i < edited.layerNames.size() && i < edited.layerColors.size(); ++i) {
                    float color[4] = {edited.layerColors[i].r, edited.layerColors[i].g, edited.layerColors[i].b, edited.layerColors[i].a};
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::ColorEdit4(edited.layerNames[i].c_str(), color, ImGuiColorEditFlags_NoInputs)) {
                        edited.layerColors[i] = {color[0], color[1], color[2], color[3]};
                    }
                    if (palettePaintMode_ && ImGui::IsItemClicked()) {
                        paletteLayerIndex_ = static_cast<int>(i);
                    }
                    ImGui::PopID();
                }

                ImGui::SeparatorText("Gradient");
                const auto& db = paletteRegistry_.database();
                for (const auto& gradient : db.gradients) {
                    if (gradient.name != edited.gradientName) continue;
                    auto lut = generateGradientLut(gradient, 32);
                    for (const auto& c : lut) {
                        ImGui::SameLine(0.0F, 0.0F);
                        ImGui::ColorButton("##g", ImVec4(c.r, c.g, c.b, c.a), ImGuiColorEditFlags_NoTooltip, ImVec2(6.0F, 14.0F));
                    }
                    ImGui::NewLine();
                }

                ImGui::SeparatorText("Animated Palette");
                static const char* animModes[] = {"None", "HueShift", "GradientCycle", "PulseBrightness", "PhaseScroll"};
                int animMode = static_cast<int>(edited.animation.mode);
                if (ImGui::Combo("Mode", &animMode, animModes, 5)) edited.animation.mode = static_cast<PaletteAnimationMode>(animMode);
                ImGui::SliderFloat("Speed", &edited.animation.speed, 0.0F, 8.0F);
                ImGui::SliderFloat("Phase Offset", &edited.animation.phaseOffset, 0.0F, 1.0F);
                ImGui::SliderFloat("Per-instance Offset", &edited.animation.perInstanceOffset, 0.0F, 1.0F);

                ImGui::SeparatorText("FX Presets");
                ImGui::Checkbox("FX Mode Auto", &paletteFxAutoMode_);
                const auto& fx = db.fxPresets;
                if (!fx.empty()) {
                    paletteFxPresetIndex_ = std::clamp(paletteFxPresetIndex_, 0, static_cast<int>(fx.size()) - 1);
                    if (ImGui::BeginCombo("FX Preset", fx[paletteFxPresetIndex_].name.c_str())) {
                        for (int i = 0; i < static_cast<int>(fx.size()); ++i) {
                            if (ImGui::Selectable(fx[i].name.c_str(), i == paletteFxPresetIndex_)) paletteFxPresetIndex_ = i;
                        }
                        ImGui::EndCombo();
                    }
                    if (paletteFxAutoMode_ && !edited.autoFxPreset.empty()) {
                        for (int i = 0; i < static_cast<int>(fx.size()); ++i) if (fx[i].name == edited.autoFxPreset) paletteFxPresetIndex_ = i;
                    }
                    const auto& chosen = fx[paletteFxPresetIndex_];
                    ImGui::Text("Bloom T/I/R %.2f %.2f %.2f", chosen.bloomThreshold, chosen.bloomIntensity, chosen.bloomRadius);
                    ImGui::Text("Vignette %.2f Round %.2f", chosen.vignetteIntensity, chosen.vignetteRoundness);
                    if (ImGui::Button("Apply FX to Camera")) appendConsole("Applied FX preset to camera (stub): " + chosen.name);
                    ImGui::SameLine();
                    if (ImGui::Button("Revert FX")) appendConsole("Reverted FX preset from camera (stub)");
                }

                if (ImGui::Button("Apply to Selection")) {
                    demoSelectionMaterials_.clear();
                    demoSelectionMaterials_.push_back(buildMaterialParamsFromTemplate(edited));
                    appendConsole("Applied palette material params to selection (demo stub)");
                }
                if (!demoSelectionMaterials_.empty()) {
                    ImGui::Text("Selection material params ready: %zu", demoSelectionMaterials_.size());
                }
            }
        }
        ImGui::TextWrapped("Beam rendering default: strip mesh with UV.y mapped length + shader glow (fallback to stretched sprite where mesh path unavailable).");
        ImGui::TextWrapped("Post-stack integration is partial-safe: data/UI implemented, camera apply/revert currently routed via stubs to avoid regressions in current SDL2 renderer backend.");
        ImGui::End();
    }


    if (showStandards_) {
        ImGui::Begin("Standards");
        EngineStandards& stds = mutableEngineStandards();
        if (ImGui::Button("Reset to Spec Defaults")) {
            resetEngineStandardsToSpec();
            appendConsole("Standards reset to spec defaults");
        }
        ImGui::SeparatorText("Playfield & Coordinate Space");
        ImGui::SliderInt("Playfield Width", &stds.playfieldWidth, 480, 4096);
        ImGui::InputInt("Playfield Width (exact)", &stds.playfieldWidth);
        ImGui::SliderInt("Playfield Height", &stds.playfieldHeight, 640, 4096);
        ImGui::InputInt("Playfield Height (exact)", &stds.playfieldHeight);
        ImGui::Text("Origin: Bottom-Left, X:0..%d Y:0..%d", stds.playfieldWidth, stds.playfieldHeight);

        ImGui::SeparatorText("Rendering & Scaling");
        ImGui::SliderInt("Internal RT Width", &stds.renderTargetWidth, 640, 8192);
        ImGui::InputInt("Internal RT Width (exact)", &stds.renderTargetWidth);
        ImGui::SliderInt("Internal RT Height", &stds.renderTargetHeight, 640, 8192);
        ImGui::InputInt("Internal RT Height (exact)", &stds.renderTargetHeight);
        ImGui::TextWrapped("Scaling policy: preserve aspect ratio, centered playfield, no horizontal stretch.");

        ImGui::SeparatorText("Projectile Sizes");
        ImGui::SliderInt("Micro Min", &stds.projectileMicroMin, 2, 16); ImGui::SameLine(); ImGui::InputInt("Micro Min exact", &stds.projectileMicroMin);
        ImGui::SliderInt("Micro Max", &stds.projectileMicroMax, 2, 20); ImGui::SameLine(); ImGui::InputInt("Micro Max exact", &stds.projectileMicroMax);
        ImGui::SliderInt("Small Min", &stds.projectileSmallMin, 2, 20); ImGui::SameLine(); ImGui::InputInt("Small Min exact", &stds.projectileSmallMin);
        ImGui::SliderInt("Small Max", &stds.projectileSmallMax, 2, 24); ImGui::SameLine(); ImGui::InputInt("Small Max exact", &stds.projectileSmallMax);
        ImGui::SliderInt("Medium Min", &stds.projectileMediumMin, 4, 32); ImGui::SameLine(); ImGui::InputInt("Medium Min exact", &stds.projectileMediumMin);
        ImGui::SliderInt("Medium Max", &stds.projectileMediumMax, 4, 40); ImGui::SameLine(); ImGui::InputInt("Medium Max exact", &stds.projectileMediumMax);

        ImGui::SeparatorText("Player Hitbox");
        ImGui::SliderInt("Hitbox Standard Min", &stds.playerHitboxStandardMin, 2, 12); ImGui::SameLine(); ImGui::InputInt("Hitbox Standard Min exact", &stds.playerHitboxStandardMin);
        ImGui::SliderInt("Hitbox Standard Max", &stds.playerHitboxStandardMax, 2, 12); ImGui::SameLine(); ImGui::InputInt("Hitbox Standard Max exact", &stds.playerHitboxStandardMax);

        ImGui::SeparatorText("Beams");
        ImGui::SliderInt("Beam Thin Min", &stds.beamThinMin, 2, 40); ImGui::SameLine(); ImGui::InputInt("Beam Thin Min exact", &stds.beamThinMin);
        ImGui::SliderInt("Beam Standard Max", &stds.beamStandardMax, 8, 128); ImGui::SameLine(); ImGui::InputInt("Beam Standard Max exact", &stds.beamStandardMax);
        ImGui::SliderInt("Beam Boss Max", &stds.beamBossMax, 16, 200); ImGui::SameLine(); ImGui::InputInt("Beam Boss Max exact", &stds.beamBossMax);

        ImGui::SeparatorText("Pattern Helpers");
        ImGui::Text("Angle step 6: %.1f", getRadialAngleStep(6));
        ImGui::Text("Angle step 8: %.1f", getRadialAngleStep(8));
        ImGui::Text("Angle step 12: %.1f", getRadialAngleStep(12));
        ImGui::Text("Angle step 16: %.1f", getRadialAngleStep(16));
        ImGui::Text("Stream Gap Normal: %d", getStreamGapPreset(DifficultyGapPreset::Normal));
        ImGui::Text("Wall Gap Hard: %d", getWallGapPreset(DifficultyGapPreset::Hard));

        ImGui::SeparatorText("Density Targets");
        ImGui::SliderInt("Density Normal", &stds.densityNormal, 100, 20000); ImGui::SameLine(); ImGui::InputInt("Density Normal exact", &stds.densityNormal);
        ImGui::SliderInt("Density Boss", &stds.densityBoss, 500, 50000); ImGui::SameLine(); ImGui::InputInt("Density Boss exact", &stds.densityBoss);
        ImGui::SliderInt("Density Extreme", &stds.densityExtreme, 1000, 100000); ImGui::SameLine(); ImGui::InputInt("Density Extreme exact", &stds.densityExtreme);

        ImGui::SeparatorText("Background Constraints");
        ImGui::SliderFloat("Brightness Min", &stds.backgroundBrightnessMin, 0.0F, 1.0F);
        ImGui::InputFloat("Brightness Min (exact)", &stds.backgroundBrightnessMin, 0.01F, 0.05F, "%.3f");
        ImGui::SliderFloat("Brightness Max", &stds.backgroundBrightnessMax, 0.0F, 1.0F);
        ImGui::InputFloat("Brightness Max (exact)", &stds.backgroundBrightnessMax, 0.01F, 0.05F, "%.3f");
        ImGui::SliderFloat("Saturation Max", &stds.backgroundSaturationMax, 0.0F, 1.0F);
        ImGui::InputFloat("Saturation Max (exact)", &stds.backgroundSaturationMax, 0.01F, 0.05F, "%.3f");

        ImGui::SeparatorText("Render Order");
        for (std::size_t i = 0; i < stds.renderOrder.size(); ++i) {
            ImGui::Text("%zu: %s", i, toString(stds.renderOrder[i]));
        }

        ImGui::SeparatorText("Camera Policy");
        ImGui::SliderFloat("Camera Max Scroll", &stds.cameraMaxScrollUnitsPerSec, 10.0F, 2000.0F);
        ImGui::InputFloat("Camera Max Scroll (exact)", &stds.cameraMaxScrollUnitsPerSec, 5.0F, 20.0F, "%.2f");
        ImGui::SliderFloat("Camera Shake Max", &stds.cameraShakeMaxAmplitude, 0.0F, 100.0F);
        ImGui::InputFloat("Camera Shake Max (exact)", &stds.cameraShakeMaxAmplitude, 0.5F, 2.0F, "%.2f");
        ImGui::SliderFloat("Camera Zoom Min", &stds.cameraZoomMin, 0.1F, 4.0F);
        ImGui::InputFloat("Camera Zoom Min (exact)", &stds.cameraZoomMin, 0.01F, 0.05F, "%.3f");
        ImGui::SliderFloat("Camera Zoom Max", &stds.cameraZoomMax, 0.1F, 4.0F);
        ImGui::InputFloat("Camera Zoom Max (exact)", &stds.cameraZoomMax, 0.01F, 0.05F, "%.3f");

        clampStandards(stds);
        const float canonicalAspect = static_cast<float>(stds.playfieldWidth) / static_cast<float>(stds.playfieldHeight);
        if (std::abs(canonicalAspect - 0.75F) > 0.05F) {
            ImGui::TextColored(ImVec4(1.0F, 0.6F, 0.2F, 1.0F), "Warning: playfield deviates from canonical 3:4 ratio.");
        }

        ImGui::End();
    }


    if (showPatternEditor_) {
        ImGui::Begin("Pattern Graph Editor + Generator");
        ImGui::TextUnformatted("Compiled graph authoring (node palette + inspector + preview sandbox).");

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

        if (graphNodeIds_.empty()) {
            graphNodeIds_ = {"010-emit", "020-wait", "030-loop"};
            graphNodeTypes_ = {0, 5, 6};
            graphNodeA_ = {12.0F, 0.12F, 8.0F};
            graphNodeB_ = {140.0F, 0.0F, 0.0F};
            graphNodeC_ = {3.0F, 0.0F, 0.0F};
            graphNodeD_ = {0.0F, 0.0F, 0.0F};
            graphNodeTarget_ = {"", "", "010-emit"};
        }

        ImGui::SeparatorText("Node Palette");
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
            if (i < 9) ImGui::SameLine();
            ImGui::PopID();
        }

        ImGui::SeparatorText("Node Inspector");
        for (std::size_t i = 0; i < graphNodeIds_.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable(graphNodeIds_[i].c_str(), selectedGraphNode_ == static_cast<int>(i))) {
                selectedGraphNode_ = static_cast<int>(i);
            }
            ImGui::PopID();
        }
        selectedGraphNode_ = std::clamp(selectedGraphNode_, 0, static_cast<int>(graphNodeIds_.size()) - 1);
        if (!graphNodeIds_.empty()) {
            const std::size_t i = static_cast<std::size_t>(selectedGraphNode_);
            ImGui::InputText("Node ID", &graphNodeIds_[i]);
            ImGui::Combo("Node Type", &graphNodeTypes_[i], nodeTypes, 10);
            ImGui::SliderFloat("A", &graphNodeA_[i], -360.0F, 360.0F);
            ImGui::SliderFloat("B", &graphNodeB_[i], -360.0F, 360.0F);
            ImGui::SliderFloat("C", &graphNodeC_[i], -360.0F, 360.0F);
            ImGui::SliderFloat("D", &graphNodeD_[i], -360.0F, 360.0F);
            ImGui::InputText("Loop Target", &graphNodeTarget_[i]);
        }

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

        PatternGraphCompiler graphCompiler;
        CompiledPatternGraph compiled;
        const bool previewOk = graphCompiler.compile(previewAsset, compiled);
        ImGui::SeparatorText("Preview Sandbox");
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


    for (engine::public_api::IToolPanelPlugin* panel : engine::public_api::toolPanelPlugins()) {
        if (!panel) continue;
        if (ImGui::Begin(panel->panelName().c_str())) {
            panel->drawPanel();
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
