#pragma once

#include <engine/palette_fx_templates.h>
#include <engine/render2d.h>

#include <SDL.h>
#include <nlohmann/json_fwd.hpp>

#include <string>
#include <vector>

namespace engine {

enum class MaterialColorizationMode {
    Band3 = 0,
    GradientRamp = 1,
};

struct MaterialParamBlock {
    std::string materialName {"default"};
    MaterialColorizationMode mode {MaterialColorizationMode::Band3};
    PaletteMaterialParams palette {};
    float beamScrollSpeed {0.0F};
    float beamPulseSpeed {0.0F};
    float beamNoiseDistortion {0.0F};
};

struct ShaderManifestEntry {
    std::string name;
    std::string stage;
    std::string entry;
    std::string blobPath;
};

struct ShaderManifest {
    std::vector<ShaderManifestEntry> entries;
};

struct PostFxSettings {
    bool enabled {true};
    bool bloomEnabled {true};
    bool vignetteEnabled {true};
    bool colorGradingEnabled {true};
    float bloomIntensity {0.0F};
    float bloomRadius {0.0F};
    float vignetteIntensity {0.0F};
    float exposure {1.0F};
    float contrast {1.0F};
    float saturation {1.0F};
};

nlohmann::json toJson(const MaterialParamBlock& block);
bool materialParamBlockFromJson(const nlohmann::json& json, MaterialParamBlock& out, std::string& error);

bool parseShaderManifest(const nlohmann::json& json, ShaderManifest& out, std::string& error);
bool validateShaderManifest(const ShaderManifest& manifest, std::string& error);

class RendererModernPipeline {
  public:
    bool initialize(SDL_Renderer* renderer, int width, int height, std::string* error = nullptr);
    void shutdown();
    bool resize(int width, int height, std::string* error = nullptr);

    bool beginScene(const Color& clearColor);
    void endScene();
    void present();

    void setPostFx(const PostFxSettings& fx);
    [[nodiscard]] const PostFxSettings& postFx() const;

  private:
    bool createTargets(int width, int height, std::string* error);
    void releaseTargets();
    void drawBloomLite();
    void drawVignette();
    void drawColorGrade();

    SDL_Renderer* renderer_ {nullptr};
    SDL_Texture* sceneTarget_ {nullptr};
    SDL_Texture* bloomTarget_ {nullptr};
    int width_ {1};
    int height_ {1};
    PostFxSettings fx_ {};
};

} // namespace engine
