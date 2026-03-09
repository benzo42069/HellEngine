#include <engine/editor_tools.h>
#include <engine/standards.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <random>

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

void ControlCenterToolSuite::refreshContentBrowserEntries() {
    browserEntries_.clear();

    namespace fs = std::filesystem;
    const fs::path root("data");
    std::error_code ec;
    if (!fs::exists(root, ec)) {
        return;
    }

    for (const fs::directory_entry& e : fs::recursive_directory_iterator(root, ec)) {
        if (ec) {
            break;
        }
        if (!e.is_regular_file()) {
            continue;
        }
        const fs::path rel = fs::relative(e.path(), fs::current_path(), ec);
        if (ec) {
            continue;
        }
        const std::string ext = e.path().extension().string();
        if (ext == ".json" || ext == ".png" || ext == ".ogg" || ext == ".wav") {
            browserEntries_.push_back(rel.generic_string());
        }
    }

    std::sort(browserEntries_.begin(), browserEntries_.end());
    if (selectedAssetPath_.empty() && !browserEntries_.empty()) {
        selectedAssetPath_ = browserEntries_.front();
    }
}

void ControlCenterToolSuite::drawWorkspaceShell(const ToolRuntimeSnapshot& snapshot) {

if (!initialized_) return;

frameHistoryMs_.push_back(snapshot.frameMs);
if (frameHistoryMs_.size() > 240) frameHistoryMs_.pop_front();

if (browserEntries_.empty()) {
    refreshContentBrowserEntries();
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

ImGui::SeparatorText("Workflow Shortcuts");
if (ImGui::Button("Content Pass")) {
    showContentBrowser_ = true;
    showInspector_ = true;
    showPreviewViewport_ = false;
    showPatternEditor_ = false;
    showPaletteFxTemplates_ = false;
    showWaveEditor_ = false;
    showValidator_ = true;
    statusMessage_ = "Workflow: Content Pass";
}
ImGui::SameLine();
if (ImGui::Button("Pattern Pass")) {
    showContentBrowser_ = true;
    showInspector_ = true;
    showPreviewViewport_ = true;
    showPatternEditor_ = true;
    showPaletteFxTemplates_ = false;
    statusMessage_ = "Workflow: Pattern Pass";
}
ImGui::SameLine();
if (ImGui::Button("Palette/FX Pass")) {
    showPaletteFxTemplates_ = true;
    showPreviewViewport_ = true;
    showPatternEditor_ = false;
    statusMessage_ = "Workflow: Palette / FX Pass";
}
ImGui::SameLine();
if (ImGui::Button("Diagnostics Pass")) {
    showPreviewViewport_ = true;
    showProfiler_ = true;
    showValidator_ = true;
    statusMessage_ = "Workflow: Diagnostics Pass";
    validatorRequested_ = true;
}

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
        if (browserEntries_.empty()) {
            ImGui::TextDisabled("No content files discovered under data/.");
            if (ImGui::Button("Rescan Content")) {
                refreshContentBrowserEntries();
                statusMessage_ = browserEntries_.empty() ? "No content discovered" : "Content browser rescanned";
            }
        }
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
        ImGui::TextWrapped("Preview is currently telemetry-driven (deterministic runtime counters and profiling snapshots).");
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
            ImGui::TextDisabled("No asset selected. Use Content Browser > Rescan Content.");
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
}

void ControlCenterToolSuite::drawPaletteFxEditorPanel(const ToolRuntimeSnapshot& snapshot) {
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
                const bool cameraFxRuntimeAvailable = false;
                ImGui::BeginDisabled(!cameraFxRuntimeAvailable);
                if (ImGui::Button("Apply FX to Camera")) appendConsole("Applied FX preset to camera: " + chosen.name);
                ImGui::SameLine();
                if (ImGui::Button("Revert FX")) appendConsole("Reverted FX preset from camera");
                ImGui::EndDisabled();
                if (!cameraFxRuntimeAvailable) {
                    ImGui::TextDisabled("Camera post-stack path is unavailable in the current SDL2 renderer backend.");
                }
            }

            if (ImGui::Button("Apply to Selection")) {
                demoSelectionMaterials_.clear();
                demoSelectionMaterials_.push_back(buildMaterialParamsFromTemplate(edited));
                appendConsole("Applied palette material params to selection");
            }
            if (!demoSelectionMaterials_.empty()) {
                ImGui::Text("Selection material params ready: %zu", demoSelectionMaterials_.size());
            }
        }
    }
    ImGui::TextWrapped("Beam rendering default: strip mesh with UV.y mapped length + shader glow (fallback to stretched sprite where mesh path unavailable).");
    ImGui::TextWrapped("Post-stack integration is gated by renderer capability checks; unavailable actions are disabled and reported in-panel.");
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
}

void ControlCenterToolSuite::drawValidationDiagnosticsPanel() {
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

} // namespace engine
