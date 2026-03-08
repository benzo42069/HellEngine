#include <engine/editor_tools.h>
#include <engine/public/plugins.h>

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui.h>

#include <filesystem>

namespace engine {

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
    drawWorkspaceShell(snapshot);
    drawPaletteFxEditorPanel(snapshot);
    drawPatternGraphEditorPanel(snapshot);
    drawProjectileEditorPanel();
    drawEncounterWaveEditorPanel();
    drawTraitUpgradePanel(snapshot);

for (engine::public_api::IToolPanelPlugin* panel : engine::public_api::toolPanelPlugins()) {
    if (!panel) continue;
    if (ImGui::Begin(panel->panelName().c_str())) {
        panel->drawPanel();
    }
    ImGui::End();
}

    drawValidationDiagnosticsPanel();
}

const UpgradeDebugOptions& ControlCenterToolSuite::upgradeDebugOptions() const { return upgradeDebug_; }

UpgradeDebugOptions& ControlCenterToolSuite::upgradeDebugOptionsMutable() { return upgradeDebug_; }

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
