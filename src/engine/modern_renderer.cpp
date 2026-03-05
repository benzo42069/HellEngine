#include <engine/modern_renderer.h>

#include <nlohmann/json.hpp>

#include <algorithm>

namespace engine {
namespace {

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

} // namespace

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

bool RendererModernPipeline::createTargets(const int width, const int height, std::string* error) {
    releaseTargets();
    width_ = std::max(width, 1);
    height_ = std::max(height, 1);

    sceneTarget_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width_, height_);
    bloomTarget_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width_, height_);
    if (!sceneTarget_ || !bloomTarget_) {
        if (error) *error = "failed to create render targets";
        releaseTargets();
        return false;
    }

    SDL_SetTextureBlendMode(sceneTarget_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(bloomTarget_, SDL_BLENDMODE_ADD);
    return true;
}

void RendererModernPipeline::releaseTargets() {
    if (sceneTarget_) {
        SDL_DestroyTexture(sceneTarget_);
        sceneTarget_ = nullptr;
    }
    if (bloomTarget_) {
        SDL_DestroyTexture(bloomTarget_);
        bloomTarget_ = nullptr;
    }
}

bool RendererModernPipeline::initialize(SDL_Renderer* renderer, const int width, const int height, std::string* error) {
    renderer_ = renderer;
    if (!renderer_) {
        if (error) *error = "renderer is null";
        return false;
    }
    return createTargets(width, height, error);
}

void RendererModernPipeline::shutdown() {
    releaseTargets();
    renderer_ = nullptr;
}

bool RendererModernPipeline::resize(const int width, const int height, std::string* error) {
    if (!renderer_) {
        if (error) *error = "renderer is null";
        return false;
    }
    if (width == width_ && height == height_ && sceneTarget_ && bloomTarget_) return true;
    return createTargets(width, height, error);
}

bool RendererModernPipeline::beginScene(const Color& clearColor) {
    if (!renderer_ || !sceneTarget_) return false;
    SDL_SetRenderTarget(renderer_, sceneTarget_);
    SDL_SetRenderDrawColor(renderer_, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    SDL_RenderClear(renderer_);
    return true;
}

void RendererModernPipeline::endScene() {
    if (!renderer_) return;
    SDL_SetRenderTarget(renderer_, nullptr);
}

void RendererModernPipeline::drawBloomLite() {
    if (!fx_.bloomEnabled || fx_.bloomIntensity <= 0.001F) return;

    SDL_SetRenderTarget(renderer_, bloomTarget_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    SDL_SetTextureAlphaMod(sceneTarget_, toByte(clamp01(fx_.bloomIntensity * 0.5F)));
    const int radius = std::max(1, static_cast<int>(fx_.bloomRadius * 8.0F));
    for (int y = -radius; y <= radius; y += std::max(1, radius / 2)) {
        for (int x = -radius; x <= radius; x += std::max(1, radius / 2)) {
            SDL_Rect dst {x, y, width_, height_};
            SDL_RenderCopy(renderer_, sceneTarget_, nullptr, &dst);
        }
    }
    SDL_SetTextureAlphaMod(sceneTarget_, 255);

    SDL_SetRenderTarget(renderer_, nullptr);
}

void RendererModernPipeline::drawVignette() {
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

void RendererModernPipeline::drawColorGrade() {
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
    if (!renderer_ || !sceneTarget_) return;

    if (fx_.enabled) {
        drawBloomLite();
    }

    SDL_RenderCopy(renderer_, sceneTarget_, nullptr, nullptr);

    if (fx_.enabled && fx_.bloomEnabled && bloomTarget_) {
        SDL_RenderCopy(renderer_, bloomTarget_, nullptr, nullptr);
    }

    if (fx_.enabled) {
        drawColorGrade();
        drawVignette();
    }
}

void RendererModernPipeline::setPostFx(const PostFxSettings& fx) {
    fx_ = fx;
}

const PostFxSettings& RendererModernPipeline::postFx() const {
    return fx_;
}

} // namespace engine
