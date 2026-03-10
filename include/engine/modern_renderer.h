#pragma once

#include <engine/palette_fx_templates.h>
#include <engine/render2d.h>

#include <glad/glad.h>

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
    float bloomThreshold {1.0F};
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

PostFxSettings postFxFromPreset(const FxPreset& preset);

nlohmann::json toJson(const MaterialParamBlock& block);
bool materialParamBlockFromJson(const nlohmann::json& json, MaterialParamBlock& out, std::string& error);

bool parseShaderManifest(const nlohmann::json& json, ShaderManifest& out, std::string& error);
bool validateShaderManifest(const ShaderManifest& manifest, std::string& error);

// Scene/post-processing orchestrator for modern rendering mode.
// It owns offscreen targets and post-fx passes, not projectile simulation content.
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
    struct UniformLocations {
        GLint sceneTex {-1};
        GLint bloomTex {-1};
        GLint mode {-1};
        GLint bloomThreshold {-1};
        GLint bloomIntensity {-1};
        GLint texelSize {-1};
        GLint vignetteIntensity {-1};
        GLint vignetteRoundness {-1};
        GLint exposure {-1};
        GLint contrast {-1};
        GLint saturation {-1};
        GLint gamma {-1};
        GLint chromaticAberration {-1};
        GLint filmGrain {-1};
        GLint scanlineIntensity {-1};
        GLint time {-1};
    };

    struct RenderTarget {
        GLuint fbo {0};
        GLuint colorTex {0};
        int width {1};
        int height {1};
    };

    bool createTargets(int width, int height, std::string* error);
    void releaseTargets();
    bool createTarget(RenderTarget& target, int width, int height, std::string* error);
    void releaseTarget(RenderTarget& target);
    bool initializeGlResources(std::string* error);
    void releaseGlResources();
    bool compileProgram(GLuint& program, const char* fragShaderPath, std::string* error);
    bool checkFramebuffer(GLuint fbo, std::string* error) const;
    void ensureQuadGeometry();
    void drawFullscreenQuad() const;
    void runBloomPass();
    void runVignettePass(GLuint inputTex, GLuint outputFbo);
    void runCompositePass(GLuint inputTex, GLuint outputFbo);
    void cacheUniformLocations();
    void drawBloomLiteFallback();
    void drawVignetteFallback();
    void drawColorGradeFallback();

    SDL_Renderer* renderer_ {nullptr};
    bool glAvailable_ {false};
    GLuint postVertShader_ {0};
    GLuint bloomProgram_ {0};
    GLuint vignetteProgram_ {0};
    GLuint compositeProgram_ {0};
    GLuint quadVao_ {0};
    GLuint quadVbo_ {0};
    RenderTarget sceneBuffer_ {};
    RenderTarget bloomBuffer_ {};
    RenderTarget outputBuffer_ {};
    UniformLocations bloomLoc_ {};
    UniformLocations vignetteLoc_ {};
    UniformLocations compositeLoc_ {};

    SDL_Texture* sceneTargetFallback_ {nullptr};
    SDL_Texture* bloomTargetFallback_ {nullptr};
    int width_ {1};
    int height_ {1};
    float timeSeconds_ {0.0F};
    PostFxSettings fx_ {};
};

} // namespace engine
