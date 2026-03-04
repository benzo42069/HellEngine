#include <engine/editor_tools.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <numeric>

namespace engine {

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

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Generate Demo Content")) {
                std::string error;
                if (generateDemoContent("data/generated_demo", &error)) {
                    statusMessage_ = "Generated demo content in data/generated_demo";
                } else {
                    statusMessage_ = "Generate failed: " + error;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (!open_) return;
    ImGui::Begin("Control Center", &open_);
    ImGui::TextUnformatted(statusMessage_.c_str());

    if (ImGui::CollapsingHeader("content browser", ImGuiTreeNodeFlags_DefaultOpen) && showContentBrowser_) {
        ImGui::TextUnformatted("Browse: data/, assets/, generated_demo/");
    }
    if (ImGui::CollapsingHeader("pattern editor", ImGuiTreeNodeFlags_DefaultOpen) && showPatternEditor_) {
        ImGui::TextUnformatted("Edit pattern layers, cadence, spread and deterministic jitter.");
    }
    if (ImGui::CollapsingHeader("entity editor", ImGuiTreeNodeFlags_DefaultOpen) && showEntityEditor_) {
        ImGui::TextUnformatted("Edit entity templates, movement, spawn rules and boss phases.");
    }
    if (ImGui::CollapsingHeader("trait editor", ImGuiTreeNodeFlags_DefaultOpen) && showTraitEditor_) {
        ImGui::TextUnformatted("Edit trait catalog entries and rarity weights.");
    }
    if (ImGui::CollapsingHeader("wave editor", ImGuiTreeNodeFlags_DefaultOpen) && showWaveEditor_) {
        ImGui::TextUnformatted("Author stage/zone pacing and encounter sequence.");
    }
    if (ImGui::CollapsingHeader("validator", ImGuiTreeNodeFlags_DefaultOpen) && showValidator_) {
        ImGui::Text("Runtime tick: %llu", static_cast<unsigned long long>(snapshot.tick));
        ImGui::Text("Validation: pattern/entity counts non-zero -> %s", (snapshot.entityCount > 0 || snapshot.projectileCount > 0) ? "PASS" : "WARN");
    }
    if (ImGui::CollapsingHeader("performance profiler", ImGuiTreeNodeFlags_DefaultOpen) && showProfiler_) {
        ImGui::Text("FPS: %d", snapshot.fps);
        ImGui::Text("Frame: %.2f ms", snapshot.frameMs);
        ImGui::Text("Projectiles: %u  Entities: %u  Collisions: %u", snapshot.projectileCount, snapshot.entityCount, snapshot.collisionsTotal);
        if (!frameHistoryMs_.empty()) {
            const float avg = std::accumulate(frameHistoryMs_.begin(), frameHistoryMs_.end(), 0.0F) / static_cast<float>(frameHistoryMs_.size());
            ImGui::Text("Avg frame: %.2f ms", avg);
        }
    }

    ImGui::End();
}

void ControlCenterToolSuite::endFrame() {
    if (!initialized_) return;
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
}

} // namespace engine
