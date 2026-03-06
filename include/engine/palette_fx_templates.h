#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json_fwd.hpp>

namespace engine {

struct PaletteColor {
    float r {1.0F};
    float g {1.0F};
    float b {1.0F};
    float a {1.0F};
};

struct GradientStop {
    float position {0.0F};
    PaletteColor color {};
};

struct GradientDefinition {
    std::string name;
    std::vector<GradientStop> stops;
};

enum class PaletteAnimationMode {
    None,
    HueShift,
    GradientCycle,
    PulseBrightness,
    PhaseScroll,
};

struct PaletteAnimationSettings {
    PaletteAnimationMode mode {PaletteAnimationMode::None};
    float speed {0.0F};
    float phaseOffset {0.0F};
    float perInstanceOffset {0.0F};
};

struct PaletteTemplate {
    std::string category;
    std::string name;
    std::vector<std::string> layerNames;
    std::vector<PaletteColor> layerColors;
    std::string gradientName;
    PaletteAnimationSettings animation {};
    std::string autoFxPreset;
};

struct FxPreset {
    std::string name;
    float bloomThreshold {1.0F};
    float bloomIntensity {0.0F};
    float bloomRadius {0.0F};
    float vignetteIntensity {0.0F};
    float vignetteRoundness {1.0F};
    float exposure {1.0F};
    float contrast {1.0F};
    float saturation {1.0F};
    float gamma {1.0F};
    float chromaticAberration {0.0F};
    float filmGrain {0.0F};
    float scanlineIntensity {0.0F};
};

struct PaletteFxTemplateDatabase {
    std::vector<PaletteTemplate> palettes;
    std::vector<GradientDefinition> gradients;
    std::vector<FxPreset> fxPresets;
};

struct PaletteMaterialParams {
    PaletteColor paletteCore {};
    PaletteColor paletteHighlight {};
    PaletteColor paletteGlow {};
    PaletteColor paletteTrail {};
    PaletteColor paletteImpact {};
    PaletteColor beamCore {};
    PaletteColor beamInner {};
    PaletteColor beamOuter {};
    PaletteColor beamGlow {};
    float monochromeMode {0.0F}; // 0=3-band 1=ramp
    float thresholdA {0.33F};
    float thresholdB {0.66F};
    float emissiveBoost {1.0F};
    float alphaBoost {1.0F};
    std::string gradientName;
};

struct PaletteFillResult {
    PaletteColor core {};
    PaletteColor highlight {};
    PaletteColor glow {};
    PaletteColor trail {};
};

class PaletteFxTemplateRegistry {
  public:
    bool loadFromJsonFile(const std::filesystem::path& path, std::string* error = nullptr);
    bool reloadIfChanged(std::string* error = nullptr);

    [[nodiscard]] const PaletteFxTemplateDatabase& database() const;
    [[nodiscard]] std::vector<std::string> categories() const;
    [[nodiscard]] std::vector<const PaletteTemplate*> templatesForCategory(const std::string& category) const;
    [[nodiscard]] const FxPreset* findFxPreset(const std::string& name) const;

  private:
    std::filesystem::path sourcePath_;
    std::filesystem::file_time_type sourceWriteTime_ {};
    PaletteFxTemplateDatabase database_ {};
};

bool parseHexColor(std::string_view hex, PaletteColor& out, std::string* error = nullptr);
std::string toHexColor(const PaletteColor& color, bool includeAlpha = false);
PaletteFillResult deriveProjectileFillFromCore(const PaletteColor& core);
PaletteFillResult deriveBackgroundFillFromAccent(const PaletteColor& accent);

std::vector<PaletteColor> generateGradientLut(const GradientDefinition& gradient, std::uint32_t width);
PaletteMaterialParams buildMaterialParamsFromTemplate(const PaletteTemplate& templ);

bool parsePaletteFxTemplateDatabase(const nlohmann::json& json, PaletteFxTemplateDatabase& out, std::string& error);
nlohmann::json toJson(const PaletteFxTemplateDatabase& db);

} // namespace engine
