#pragma once

#include <SDL.h>

#include <engine/encounter_graph.h>
#include <engine/palette_fx_templates.h>
#include <engine/standards.h>

#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace engine {

struct PatternGraphAsset;

struct ToolRuntimeSnapshot {
    std::uint64_t tick {0};
    int fps {0};
    float frameMs {0.0F};
    std::uint32_t projectileCount {0};
    std::uint32_t entityCount {0};
    std::uint32_t collisionsTotal {0};
    bool upgradeScreenOpen {false};
    const char* forceRarityLabel {"auto"};
    float frameTimeMs {0.0F};
    float simMs {0.0F};
    float patternMs {0.0F};
    float bulletMs {0.0F};
    float collisionMs {0.0F};
    float renderMs {0.0F};
    std::uint32_t activeBullets {0};
    float spawnsPerSecond {0.0F};
    std::uint32_t broadphaseChecks {0};
    std::uint32_t narrowphaseChecks {0};
    std::uint32_t drawCalls {0};
    std::uint32_t renderBatches {0};
    std::uint32_t gpuActiveBullets {0};
    float gpuUpdateMs {0.0F};
    float gpuRenderMs {0.0F};
    bool perfWarnSimulation {false};
    bool perfWarnRender {false};
    bool perfWarnCollisions {false};
    const char* difficultyProfileLabel {"Normal"};
    float difficultyOverall {1.0F};
    float difficultyPatternSpeed {1.0F};
    float difficultySpawnRate {1.0F};
    float difficultyEnemyHp {1.0F};
};

struct ProjectileDebugOptions {
    bool spawnHoming {false};
    bool spawnCurved {false};
    bool spawnAccelDrag {false};
    bool spawnBounce {false};
    bool spawnSplit {false};
    bool spawnExplode {false};
    bool spawnBeam {false};
};

struct PatternGeneratorDebugState {
    bool requestGenerate {false};
    bool requestMutate {false};
    bool requestRemix {false};
    std::uint64_t seed {1337};
    int stylePreset {0};
    float density {0.5F};
    float speed {0.5F};
    float symmetry {0.5F};
    float chaos {0.3F};
    float fairness {0.6F};
    float difficultyScore {0.0F};
    float dodgeGapEstimate {0.0F};
    float bulletsPerSecond {0.0F};
    std::array<float, 64> heatmap {};
    std::string generatedGraphPath;
};

struct EncounterEditorState {
    bool simulate60s {false};
    float previewTime {0.0F};
    float difficultyScalar {1.0F};
    std::array<float, 60> spawnTimeline {};
};

struct UpgradeDebugOptions {
    bool spawnUpgradeScreen {false};
    int forcedRarity {-1};
    bool showInternalStatModifiers {false};
    bool showPerfHud {true};
    bool showDangerField {false};
};

struct ControlCenterValidationIssue {
    std::string category;
    std::string message;
    bool warning {false};
};

struct ControlCenterValidationReport {
    std::vector<ControlCenterValidationIssue> issues;
    std::uint32_t filesScanned {0};
};

bool generateDemoContent(const std::string& outputDir, std::string* error = nullptr);
ControlCenterValidationReport runControlCenterValidation(const std::string& contentRoot, const ToolRuntimeSnapshot& snapshot);

class ControlCenterToolSuite {
  public:
    void initialize(SDL_Window* window, SDL_Renderer* renderer);
    void shutdown();
    void processEvent(const SDL_Event& event);

    void beginFrame();
    void drawControlCenter(const ToolRuntimeSnapshot& snapshot);
    void endFrame();

    [[nodiscard]] const UpgradeDebugOptions& upgradeDebugOptions() const;
    UpgradeDebugOptions& upgradeDebugOptionsMutable();
    [[nodiscard]] ProjectileDebugOptions consumeProjectileDebugOptions();
    PatternGeneratorDebugState& patternGeneratorState();
    [[nodiscard]] std::string consumeGeneratedGraphPath();

  private:
    void drawWorkspaceShell(const ToolRuntimeSnapshot& snapshot);
    void drawPaletteFxEditorPanel(const ToolRuntimeSnapshot& snapshot);
    void drawPatternGraphEditorPanel(const ToolRuntimeSnapshot& snapshot);
    void drawPatternGenerationControls();
    void drawPatternSeedAndTestingControls();
    void drawPatternGraphNodePalette();
    void drawPatternGraphNodeInspector();
    void drawPatternPreviewAndAnalysis(const PatternGraphAsset& previewAsset);
    [[nodiscard]] PatternGraphAsset buildPatternPreviewAsset() const;
    void drawProjectileEditorPanel();
    void drawEncounterWaveEditorPanel();
    void drawTraitUpgradePanel(const ToolRuntimeSnapshot& snapshot);
    void drawValidationDiagnosticsPanel();
    EncounterGraphAsset buildEncounterAsset() const;
    void ensurePatternGraphSeeded();
    void ensureEncounterNodesSeeded();
    void refreshContentBrowserEntries();
    void appendConsole(const std::string& message);

    bool initialized_ {false};
    bool open_ {true};

    bool showContentBrowser_ {true};
    bool showInspector_ {true};
    bool showPreviewViewport_ {true};
    bool showConsole_ {true};
    bool showPatternEditor_ {true};
    bool showEntityEditor_ {true};
    bool showTraitEditor_ {true};
    bool showWaveEditor_ {true};
    bool showValidator_ {true};
    bool showProfiler_ {true};
    bool showPaletteFxTemplates_ {true};
    bool showStandards_ {true};

    std::string statusMessage_ {"Ready"};
    std::deque<float> frameHistoryMs_;
    SDL_Renderer* renderer_ {nullptr};
    UpgradeDebugOptions upgradeDebug_ {};
    ProjectileDebugOptions projectileDebug_ {};
    PatternGeneratorDebugState patternGenerator_ {};
    std::vector<std::string> graphNodeIds_ {};
    std::vector<int> graphNodeTypes_ {};
    std::vector<float> graphNodeA_ {};
    std::vector<float> graphNodeB_ {};
    std::vector<float> graphNodeC_ {};
    std::vector<float> graphNodeD_ {};
    std::vector<std::string> graphNodeTarget_ {};
    int selectedGraphNode_ {0};
    std::vector<std::string> encounterNodeIds_ {};
    std::vector<int> encounterNodeTypes_ {};
    std::vector<float> encounterNodeTimes_ {};
    std::vector<float> encounterNodeDurations_ {};
    std::vector<float> encounterNodeValues_ {};
    std::vector<std::string> encounterNodePayloads_ {};
    EncounterEditorState encounterState_ {};
    std::string browserSearch_;
    std::string selectedAssetPath_;
    std::string selectedAssetTag_;
    std::string browserTagFilter_;
    std::vector<std::string> browserEntries_ {};
    ControlCenterValidationReport validationReport_ {};
    std::deque<std::string> consoleLines_ {};
    bool workspaceLayoutInitialized_ {false};
    bool validatorRequested_ {false};
    bool validatorAutoOnSave_ {true};
    PaletteFxTemplateRegistry paletteRegistry_ {};
    bool paletteRegistryLoaded_ {false};
    int paletteCategoryIndex_ {0};
    int paletteTemplateIndex_ {0};
    int paletteLayerIndex_ {0};
    bool palettePaintMode_ {true};
    bool paletteFxAutoMode_ {false};
    int paletteFxPresetIndex_ {0};
    std::vector<PaletteColor> paletteClipboard_ {};
    std::optional<PaletteTemplate> paletteEditedTemplate_ {};
    std::vector<PaletteMaterialParams> demoSelectionMaterials_ {};
};

} // namespace engine
