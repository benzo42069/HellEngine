#include <engine/modern_renderer.h>

#include <engine/logging.h>

#include <nlohmann/json.hpp>

#include <SDL_opengl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>

namespace engine {
namespace {

constexpr const char* kFullscreenVertSrc = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;
out vec2 vUv;
void main() {
    vUv = aUv;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

constexpr const char* kBloomFragPath = "assets/shaders/post_bloom_fs.glsl";
constexpr const char* kVignetteFragPath = "assets/shaders/post_vignette_fs.glsl";
constexpr const char* kCompositeFragPath = "assets/shaders/post_composite_fs.glsl";

float clamp01(const float v) {
    return std::clamp(v, 0.0F, 1.0F);
}

Uint8 toByte(const float v) {
    return static_cast<Uint8>(std::clamp(v, 0.0F, 1.0F) * 255.0F);
}

const char* modeToString(const MaterialColorizationMode mode) {
    switch (mode) {
        case MaterialColorizationMode::GradientRamp: return "gradient_ramp";
        case MaterialColorizationMode::Band3:
        default: return "3_band";
    }
}

MaterialColorizationMode modeFromString(const std::string& mode) {
    if (mode == "gradient_ramp") return MaterialColorizationMode::GradientRamp;
    return MaterialColorizationMode::Band3;
}

std::string readTextFile(const char* path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) return {};
    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

GLuint compileShader(const GLenum type, const std::string& src, std::string* error) {
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        if (error) *error = "glCreateShader failed";
        return 0;
    }
    const char* ptr = src.c_str();
    glShaderSource(shader, 1, &ptr, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) return shader;

    GLint len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    std::string log(static_cast<std::size_t>(std::max(len, 1)), '\0');
    GLsizei written = 0;
    glGetShaderInfoLog(shader, len, &written, log.data());
    glDeleteShader(shader);
    if (error) *error = log;
    return 0;
}

} // namespace

PostFxSettings postFxFromPreset(const FxPreset& preset) {
    PostFxSettings fx;
    fx.enabled = true;
    fx.bloomEnabled = preset.bloomIntensity > 0.001F;
    fx.vignetteEnabled = preset.vignetteIntensity > 0.001F;
    fx.colorGradingEnabled = true;
    fx.bloomThreshold = preset.bloomThreshold;
    fx.bloomIntensity = preset.bloomIntensity;
    fx.bloomRadius = preset.bloomRadius;
    fx.vignetteIntensity = preset.vignetteIntensity;
    fx.vignetteRoundness = preset.vignetteRoundness;
    fx.exposure = preset.exposure;
    fx.contrast = preset.contrast;
    fx.saturation = preset.saturation;
    fx.gamma = preset.gamma;
    fx.chromaticAberration = preset.chromaticAberration;
    fx.filmGrain = preset.filmGrain;
    fx.scanlineIntensity = preset.scanlineIntensity;
    return fx;
}

nlohmann::json toJson(const MaterialParamBlock& block) {
    nlohmann::json j;
    j["materialName"] = block.materialName;
    j["mode"] = modeToString(block.mode);
    j["palette"] = {
        {"core", toHexColor(block.palette.paletteCore)},
        {"highlight", toHexColor(block.palette.paletteHighlight)},
        {"glow", toHexColor(block.palette.paletteGlow)},
        {"trail", toHexColor(block.palette.paletteTrail)},
        {"impact", toHexColor(block.palette.paletteImpact)},
        {"beamCore", toHexColor(block.palette.beamCore)},
        {"beamInner", toHexColor(block.palette.beamInner)},
        {"beamOuter", toHexColor(block.palette.beamOuter)},
        {"beamGlow", toHexColor(block.palette.beamGlow)},
        {"thresholdA", block.palette.thresholdA},
        {"thresholdB", block.palette.thresholdB},
        {"emissiveBoost", block.palette.emissiveBoost},
        {"alphaBoost", block.palette.alphaBoost},
        {"gradientName", block.palette.gradientName},
    };
    j["beam"] = {
        {"scrollSpeed", block.beamScrollSpeed},
        {"pulseSpeed", block.beamPulseSpeed},
        {"noiseDistortion", block.beamNoiseDistortion},
    };
    return j;
}

bool materialParamBlockFromJson(const nlohmann::json& json, MaterialParamBlock& out, std::string& error) {
    if (!json.is_object()) {
        error = "material block must be object";
        return false;
    }

    out = {};
    out.materialName = json.value("materialName", "default");
    out.mode = modeFromString(json.value("mode", "3_band"));

    if (json.contains("palette") && json["palette"].is_object()) {
        const auto& p = json["palette"];
        std::string parseError;
        (void)parseHexColor(p.value("core", "#FFFFFF"), out.palette.paletteCore, &parseError);
        (void)parseHexColor(p.value("highlight", "#FFFFFF"), out.palette.paletteHighlight, &parseError);
        (void)parseHexColor(p.value("glow", "#FFFFFF"), out.palette.paletteGlow, &parseError);
        (void)parseHexColor(p.value("trail", "#FFFFFF"), out.palette.paletteTrail, &parseError);
        (void)parseHexColor(p.value("impact", "#FFFFFF"), out.palette.paletteImpact, &parseError);
        (void)parseHexColor(p.value("beamCore", "#FFFFFF"), out.palette.beamCore, &parseError);
        (void)parseHexColor(p.value("beamInner", "#FFFFFF"), out.palette.beamInner, &parseError);
        (void)parseHexColor(p.value("beamOuter", "#FFFFFF"), out.palette.beamOuter, &parseError);
        (void)parseHexColor(p.value("beamGlow", "#FFFFFF"), out.palette.beamGlow, &parseError);
        out.palette.thresholdA = p.value("thresholdA", 0.33F);
        out.palette.thresholdB = p.value("thresholdB", 0.66F);
        out.palette.emissiveBoost = p.value("emissiveBoost", 1.0F);
        out.palette.alphaBoost = p.value("alphaBoost", 1.0F);
        out.palette.gradientName = p.value("gradientName", "");
    }

    if (json.contains("beam") && json["beam"].is_object()) {
        const auto& beam = json["beam"];
        out.beamScrollSpeed = beam.value("scrollSpeed", 0.0F);
        out.beamPulseSpeed = beam.value("pulseSpeed", 0.0F);
        out.beamNoiseDistortion = beam.value("noiseDistortion", 0.0F);
    }

    return true;
}

bool parseShaderManifest(const nlohmann::json& json, ShaderManifest& out, std::string& error) {
    out = {};
    if (!json.is_object() || !json.contains("shaders") || !json["shaders"].is_array()) {
        error = "shader manifest must contain array 'shaders'";
        return false;
    }

    for (const auto& s : json["shaders"]) {
        if (!s.is_object()) {
            error = "shader entry must be object";
            return false;
        }
        ShaderManifestEntry e;
        e.name = s.value("name", "");
        e.stage = s.value("stage", "");
        e.entry = s.value("entry", "");
        e.blobPath = s.value("blobPath", "");
        out.entries.push_back(e);
    }
    return validateShaderManifest(out, error);
}

bool validateShaderManifest(const ShaderManifest& manifest, std::string& error) {
    if (manifest.entries.empty()) {
        error = "shader manifest cannot be empty";
        return false;
    }
    for (const auto& e : manifest.entries) {
        if (e.name.empty() || e.stage.empty() || e.entry.empty() || e.blobPath.empty()) {
            error = "shader entry missing required field";
            return false;
        }
        if (e.stage != "vs" && e.stage != "ps" && e.stage != "cs") {
            error = "shader stage must be vs/ps/cs";
            return false;
        }
    }
    return true;
}

bool RendererModernPipeline::compileProgram(GLuint& program, const char* fragShaderPath, std::string* error) {
    const std::string fragSource = readTextFile(fragShaderPath);
    if (fragSource.empty()) {
        if (error) *error = std::string("failed to read shader: ") + fragShaderPath;
        return false;
    }

    std::string shaderError;
    if (postVertShader_ == 0) {
        postVertShader_ = compileShader(GL_VERTEX_SHADER, kFullscreenVertSrc, &shaderError);
        if (postVertShader_ == 0) {
            if (error) *error = "fullscreen vertex compile failed: " + shaderError;
            return false;
        }
    }

    const GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSource, &shaderError);
    if (fragShader == 0) {
        if (error) *error = "fragment compile failed: " + shaderError;
        return false;
    }

    program = glCreateProgram();
    glAttachShader(program, postVertShader_);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glDeleteShader(fragShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) return true;

    GLint len = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
    std::string log(static_cast<std::size_t>(std::max(len, 1)), '\0');
    GLsizei written = 0;
    glGetProgramInfoLog(program, len, &written, log.data());
    if (error) *error = "program link failed: " + log;
    glDeleteProgram(program);
    program = 0;
    return false;
}

bool RendererModernPipeline::checkFramebuffer(const GLuint fbo, std::string* error) const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) return true;
    if (error) *error = "framebuffer incomplete";
    return false;
}

void RendererModernPipeline::ensureQuadGeometry() {
    if (quadVao_ != 0) return;
    const std::array<float, 24> quad {
        -1.0F, -1.0F, 0.0F, 0.0F,
         1.0F, -1.0F, 1.0F, 0.0F,
         1.0F,  1.0F, 1.0F, 1.0F,
        -1.0F, -1.0F, 0.0F, 0.0F,
         1.0F,  1.0F, 1.0F, 1.0F,
        -1.0F,  1.0F, 0.0F, 1.0F,
    };

    glGenVertexArrays(1, &quadVao_);
    glGenBuffers(1, &quadVbo_);
    glBindVertexArray(quadVao_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(quad.size() * sizeof(float)), quad.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
}

void RendererModernPipeline::drawFullscreenQuad() const {
    glBindVertexArray(quadVao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

bool RendererModernPipeline::createTarget(RenderTarget& target, const int width, const int height, std::string* error) {
    releaseTarget(target);
    target.width = std::max(width, 1);
    target.height = std::max(height, 1);

    glGenFramebuffers(1, &target.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);

    glGenTextures(1, &target.colorTex);
    glBindTexture(GL_TEXTURE_2D, target.colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, target.width, target.height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.colorTex, 0);

    if (!checkFramebuffer(target.fbo, error)) {
        releaseTarget(target);
        return false;
    }
    return true;
}

void RendererModernPipeline::releaseTarget(RenderTarget& target) {
    if (target.colorTex != 0) {
        glDeleteTextures(1, &target.colorTex);
        target.colorTex = 0;
    }
    if (target.fbo != 0) {
        glDeleteFramebuffers(1, &target.fbo);
        target.fbo = 0;
    }
    target.width = 1;
    target.height = 1;
}

bool RendererModernPipeline::initializeGlResources(std::string* error) {
    ensureQuadGeometry();
    if (!compileProgram(bloomProgram_, kBloomFragPath, error)) return false;
    if (!compileProgram(vignetteProgram_, kVignetteFragPath, error)) return false;
    if (!compileProgram(compositeProgram_, kCompositeFragPath, error)) return false;
    return true;
}

void RendererModernPipeline::releaseGlResources() {
    if (bloomProgram_ != 0) {
        glDeleteProgram(bloomProgram_);
        bloomProgram_ = 0;
    }
    if (vignetteProgram_ != 0) {
        glDeleteProgram(vignetteProgram_);
        vignetteProgram_ = 0;
    }
    if (compositeProgram_ != 0) {
        glDeleteProgram(compositeProgram_);
        compositeProgram_ = 0;
    }
    if (postVertShader_ != 0) {
        glDeleteShader(postVertShader_);
        postVertShader_ = 0;
    }
    if (quadVbo_ != 0) {
        glDeleteBuffers(1, &quadVbo_);
        quadVbo_ = 0;
    }
    if (quadVao_ != 0) {
        glDeleteVertexArrays(1, &quadVao_);
        quadVao_ = 0;
    }
}

bool RendererModernPipeline::createTargets(const int width, const int height, std::string* error) {
    releaseTargets();
    width_ = std::max(width, 1);
    height_ = std::max(height, 1);

    if (glAvailable_) {
        if (!createTarget(sceneBuffer_, width_, height_, error)) return false;
        if (!createTarget(bloomBuffer_, std::max(1, width_ / 2), std::max(1, height_ / 2), error)) return false;
        if (!createTarget(outputBuffer_, width_, height_, error)) return false;
        return true;
    }

    sceneTargetFallback_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width_, height_);
    bloomTargetFallback_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width_, height_);
    if (!sceneTargetFallback_ || !bloomTargetFallback_) {
        if (error) *error = "failed to create fallback render targets";
        releaseTargets();
        return false;
    }
    SDL_SetTextureBlendMode(sceneTargetFallback_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(bloomTargetFallback_, SDL_BLENDMODE_ADD);
    return true;
}

void RendererModernPipeline::releaseTargets() {
    if (glAvailable_) {
        releaseTarget(sceneBuffer_);
        releaseTarget(bloomBuffer_);
        releaseTarget(outputBuffer_);
    }

    if (sceneTargetFallback_) {
        SDL_DestroyTexture(sceneTargetFallback_);
        sceneTargetFallback_ = nullptr;
    }
    if (bloomTargetFallback_) {
        SDL_DestroyTexture(bloomTargetFallback_);
        bloomTargetFallback_ = nullptr;
    }
}

bool RendererModernPipeline::initialize(SDL_Renderer* renderer, const int width, const int height, std::string* error) {
    renderer_ = renderer;
    if (!renderer_) {
        if (error) *error = "renderer is null";
        return false;
    }

    glAvailable_ = SDL_GL_GetCurrentContext() != nullptr;
    if (glAvailable_) {
        if (!initializeGlResources(error)) {
            logWarn("Modern renderer OpenGL setup failed; using fallback path.");
            glAvailable_ = false;
            releaseGlResources();
        }
    }

    return createTargets(width, height, error);
}

void RendererModernPipeline::shutdown() {
    releaseTargets();
    releaseGlResources();
    renderer_ = nullptr;
    glAvailable_ = false;
}

bool RendererModernPipeline::resize(const int width, const int height, std::string* error) {
    if (!renderer_) {
        if (error) *error = "renderer is null";
        return false;
    }
    if (width == width_ && height == height_) return true;
    return createTargets(width, height, error);
}

bool RendererModernPipeline::beginScene(const Color& clearColor) {
    if (!renderer_) return false;
    if (glAvailable_) {
        glBindFramebuffer(GL_FRAMEBUFFER, sceneBuffer_.fbo);
        glViewport(0, 0, sceneBuffer_.width, sceneBuffer_.height);
        glClearColor(
            static_cast<float>(clearColor.r) / 255.0F,
            static_cast<float>(clearColor.g) / 255.0F,
            static_cast<float>(clearColor.b) / 255.0F,
            static_cast<float>(clearColor.a) / 255.0F
        );
        glClear(GL_COLOR_BUFFER_BIT);
        return true;
    }

    if (!sceneTargetFallback_) return false;
    SDL_SetRenderTarget(renderer_, sceneTargetFallback_);
    SDL_SetRenderDrawColor(renderer_, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    SDL_RenderClear(renderer_);
    return true;
}

void RendererModernPipeline::runBloomPass() {
    if (!fx_.bloomEnabled || fx_.bloomIntensity <= 0.001F) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneBuffer_.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outputBuffer_.fbo);
        glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        return;
    }

    glUseProgram(bloomProgram_);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(bloomProgram_, "u_sceneTex"), 0);
    glUniform1f(glGetUniformLocation(bloomProgram_, "u_bloomThreshold"), fx_.bloomThreshold);
    glUniform1f(glGetUniformLocation(bloomProgram_, "u_bloomIntensity"), fx_.bloomIntensity);

    glViewport(0, 0, bloomBuffer_.width, bloomBuffer_.height);
    glBindFramebuffer(GL_FRAMEBUFFER, bloomBuffer_.fbo);
    glBindTexture(GL_TEXTURE_2D, sceneBuffer_.colorTex);
    glUniform1i(glGetUniformLocation(bloomProgram_, "u_mode"), 0);
    glUniform2f(glGetUniformLocation(bloomProgram_, "u_texelSize"), 1.0F / static_cast<float>(width_), 1.0F / static_cast<float>(height_));
    drawFullscreenQuad();

    float step = std::max(1.0F, fx_.bloomRadius * 2.0F);
    for (int i = 0; i < 4; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, outputBuffer_.fbo);
        glViewport(0, 0, outputBuffer_.width, outputBuffer_.height);
        glBindTexture(GL_TEXTURE_2D, bloomBuffer_.colorTex);
        glUniform1i(glGetUniformLocation(bloomProgram_, "u_mode"), 1);
        glUniform2f(glGetUniformLocation(bloomProgram_, "u_texelSize"), step / static_cast<float>(bloomBuffer_.width), step / static_cast<float>(bloomBuffer_.height));
        drawFullscreenQuad();

        glBindFramebuffer(GL_FRAMEBUFFER, bloomBuffer_.fbo);
        glViewport(0, 0, bloomBuffer_.width, bloomBuffer_.height);
        glBindTexture(GL_TEXTURE_2D, outputBuffer_.colorTex);
        glUniform1i(glGetUniformLocation(bloomProgram_, "u_mode"), 2);
        glUniform2f(glGetUniformLocation(bloomProgram_, "u_texelSize"), step / static_cast<float>(bloomBuffer_.width), step / static_cast<float>(bloomBuffer_.height));
        drawFullscreenQuad();
        step *= 0.85F;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, outputBuffer_.fbo);
    glViewport(0, 0, outputBuffer_.width, outputBuffer_.height);
    glBindTexture(GL_TEXTURE_2D, sceneBuffer_.colorTex);
    glUniform1i(glGetUniformLocation(bloomProgram_, "u_mode"), 3);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomBuffer_.colorTex);
    glUniform1i(glGetUniformLocation(bloomProgram_, "u_bloomTex"), 1);
    glActiveTexture(GL_TEXTURE0);
    drawFullscreenQuad();
}

void RendererModernPipeline::runVignettePass(const GLuint inputTex, const GLuint outputFbo) {
    if (!fx_.vignetteEnabled || fx_.vignetteIntensity <= 0.001F) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, outputBuffer_.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outputFbo);
        glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, outputFbo);
    glViewport(0, 0, width_, height_);
    glUseProgram(vignetteProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex);
    glUniform1i(glGetUniformLocation(vignetteProgram_, "u_sceneTex"), 0);
    glUniform1f(glGetUniformLocation(vignetteProgram_, "u_vignetteIntensity"), fx_.vignetteIntensity);
    glUniform1f(glGetUniformLocation(vignetteProgram_, "u_vignetteRoundness"), fx_.vignetteRoundness);
    drawFullscreenQuad();
}

void RendererModernPipeline::runCompositePass(const GLuint inputTex, const GLuint outputFbo) {
    glBindFramebuffer(GL_FRAMEBUFFER, outputFbo);
    glViewport(0, 0, width_, height_);
    glUseProgram(compositeProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex);
    glUniform1i(glGetUniformLocation(compositeProgram_, "u_sceneTex"), 0);
    glUniform1f(glGetUniformLocation(compositeProgram_, "u_exposure"), fx_.exposure);
    glUniform1f(glGetUniformLocation(compositeProgram_, "u_contrast"), fx_.contrast);
    glUniform1f(glGetUniformLocation(compositeProgram_, "u_saturation"), fx_.saturation);
    glUniform1f(glGetUniformLocation(compositeProgram_, "u_gamma"), fx_.gamma);
    glUniform1f(glGetUniformLocation(compositeProgram_, "u_chromaticAberration"), fx_.chromaticAberration);
    glUniform1f(glGetUniformLocation(compositeProgram_, "u_filmGrain"), fx_.filmGrain);
    glUniform1f(glGetUniformLocation(compositeProgram_, "u_scanlineIntensity"), fx_.scanlineIntensity);
    glUniform1f(glGetUniformLocation(compositeProgram_, "u_time"), timeSeconds_);
    drawFullscreenQuad();
}

void RendererModernPipeline::endScene() {
    if (!renderer_) return;
    if (!glAvailable_) {
        SDL_SetRenderTarget(renderer_, nullptr);
        return;
    }

    if (!fx_.enabled) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneBuffer_.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        return;
    }

    runBloomPass();
    runVignettePass(outputBuffer_.colorTex, sceneBuffer_.fbo);
    runCompositePass(sceneBuffer_.colorTex, 0);
}

void RendererModernPipeline::drawBloomLiteFallback() {
    if (!fx_.bloomEnabled || fx_.bloomIntensity <= 0.001F) return;

    SDL_SetRenderTarget(renderer_, bloomTargetFallback_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    SDL_SetTextureAlphaMod(sceneTargetFallback_, toByte(clamp01(fx_.bloomIntensity * 0.5F)));
    const int radius = std::max(1, static_cast<int>(fx_.bloomRadius * 8.0F));
    for (int y = -radius; y <= radius; y += std::max(1, radius / 2)) {
        for (int x = -radius; x <= radius; x += std::max(1, radius / 2)) {
            SDL_Rect dst {x, y, width_, height_};
            SDL_RenderCopy(renderer_, sceneTargetFallback_, nullptr, &dst);
        }
    }
    SDL_SetTextureAlphaMod(sceneTargetFallback_, 255);

    SDL_SetRenderTarget(renderer_, nullptr);
}

void RendererModernPipeline::drawVignetteFallback() {
    if (!fx_.vignetteEnabled || fx_.vignetteIntensity <= 0.001F) return;
    const int layers = 8;
    const Uint8 alpha = toByte(clamp01(fx_.vignetteIntensity) * 0.5F);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < layers; ++i) {
        const int inset = i * 12;
        SDL_Rect r {inset, inset, std::max(1, width_ - inset * 2), std::max(1, height_ - inset * 2)};
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, static_cast<Uint8>(alpha / (i + 1)));
        SDL_RenderDrawRect(renderer_, &r);
    }
}

void RendererModernPipeline::drawColorGradeFallback() {
    if (!fx_.colorGradingEnabled) return;

    const float exposure = std::clamp(fx_.exposure, 0.5F, 2.0F);
    const float contrast = std::clamp(fx_.contrast, 0.5F, 2.0F);
    const float saturation = std::clamp(fx_.saturation, 0.0F, 2.0F);

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    if (exposure > 1.0F) {
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, toByte((exposure - 1.0F) * 0.15F));
        SDL_Rect full {0, 0, width_, height_};
        SDL_RenderFillRect(renderer_, &full);
    }

    if (contrast != 1.0F) {
        const Uint8 cAlpha = toByte(std::abs(contrast - 1.0F) * 0.12F);
        if (contrast > 1.0F) {
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, cAlpha);
        } else {
            SDL_SetRenderDrawColor(renderer_, 127, 127, 127, cAlpha);
        }
        SDL_Rect full {0, 0, width_, height_};
        SDL_RenderDrawRect(renderer_, &full);
    }

    if (saturation < 1.0F) {
        const Uint8 satAlpha = toByte((1.0F - saturation) * 0.35F);
        SDL_SetRenderDrawColor(renderer_, 128, 128, 128, satAlpha);
        SDL_Rect full {0, 0, width_, height_};
        SDL_RenderFillRect(renderer_, &full);
    }
}

void RendererModernPipeline::present() {
    if (!renderer_) return;

    timeSeconds_ += 1.0F / 60.0F;
    if (glAvailable_) return;
    if (!sceneTargetFallback_) return;

    if (fx_.enabled) {
        drawBloomLiteFallback();
    }

    SDL_RenderCopy(renderer_, sceneTargetFallback_, nullptr, nullptr);

    if (fx_.enabled && fx_.bloomEnabled && bloomTargetFallback_) {
        SDL_RenderCopy(renderer_, bloomTargetFallback_, nullptr, nullptr);
    }

    if (fx_.enabled) {
        drawColorGradeFallback();
        drawVignetteFallback();
    }
}

void RendererModernPipeline::setPostFx(const PostFxSettings& fx) {
    fx_ = fx;
}

const PostFxSettings& RendererModernPipeline::postFx() const {
    return fx_;
}

} // namespace engine
