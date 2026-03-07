#include <engine/render_pipeline.h>

#include <engine/bullet_sprite_gen.h>

#include <engine/logging.h>
#include <engine/pattern_signature.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <sstream>

namespace engine {

bool RenderPipeline::initialize(SDL_Window* window, const EngineConfig& config, ControlCenterToolSuite& toolSuite) {
    config_ = config;
    if (!window) {
        logError("Renderer initialization failed: window handle is null.");
        return false;
    }
    const std::array<Uint32, 3> rendererFlags = {SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC, SDL_RENDERER_ACCELERATED, SDL_RENDERER_SOFTWARE};
    for (const Uint32 flags : rendererFlags) {
        renderer_ = SDL_CreateRenderer(window, -1, flags);
        if (renderer_) break;
    }
    if (!renderer_) {
        logError("GPU/renderer initialization failed. SDL error: " + std::string(SDL_GetError()));
        return false;
    }
    textures_ = std::make_unique<TextureStore>(renderer_);
    toolSuite.initialize(window, renderer_);
    (void)debugText_.init(renderer_);
    debugText_.registerTexture(*textures_, "debug_font");
    backgroundSystem_.initialize(renderer_, *textures_);
    renderContextReady_ = true;
    refreshDisplayMetrics(window);
    spriteBatch_.reserve(config_.projectileCapacity + 2048);
    return true;
}

void RenderPipeline::shutdown(ControlCenterToolSuite& toolSuite) {
    if (!renderContextReady_ && !renderer_ && !textures_) return;
    toolSuite.shutdown();
    modernRenderer_.shutdown();
    backgroundSystem_.clear();
    textures_.reset();
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    renderContextReady_ = false;
}

bool RenderPipeline::recreate(const char* reason, SDL_Window* window, const EngineConfig& config, ControlCenterToolSuite& toolSuite) {
    logWarn(std::string("Recreating render context: ") + (reason ? reason : "unknown reason"));
    shutdown(toolSuite);
    return initialize(window, config, toolSuite);
}

void RenderPipeline::refreshDisplayMetrics(SDL_Window* window) {
    if (!window) return;
    int windowW = 1;
    int windowH = 1;
    SDL_GetWindowSize(window, &windowW, &windowH);
    int drawableW = windowW;
    int drawableH = windowH;
    SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);
    drawableW = std::max(drawableW, 1);
    drawableH = std::max(drawableH, 1);
    windowW = std::max(windowW, 1);
    windowH = std::max(windowH, 1);
    dpiScaleX_ = static_cast<float>(drawableW) / static_cast<float>(windowW);
    dpiScaleY_ = static_cast<float>(drawableH) / static_cast<float>(windowH);
    uiTextScale_ = std::clamp((dpiScaleX_ + dpiScaleY_) * 0.5F, 1.0F, 2.0F);
    camera_.setViewport(drawableW, drawableH);
}


void RenderPipeline::generateBulletSprites(const PaletteFxTemplateRegistry& registry, const BulletPaletteTable& table) {
    if (!renderer_ || !textures_) return;
    BulletSpriteGenerator generator;
    const auto defaultFill = deriveProjectileFillFromCore(PaletteColor {1.0F, 1.0F, 1.0F, 1.0F});
    generator.generatePaletteSet(renderer_, *textures_, "0", defaultFill, 16);

    const auto& db = registry.database();
    std::uint8_t idx = 1;
    for (const PaletteTemplate& templ : db.palettes) {
        if (idx >= BulletPaletteTable::kMaxPalettes) break;
        const auto& entry = table.get(idx);
        PaletteFillResult fill;
        fill.core = PaletteColor {entry.core.r / 255.0F, entry.core.g / 255.0F, entry.core.b / 255.0F, entry.core.a / 255.0F};
        fill.highlight = PaletteColor {entry.highlight.r / 255.0F, entry.highlight.g / 255.0F, entry.highlight.b / 255.0F, entry.highlight.a / 255.0F};
        fill.glow = PaletteColor {entry.glow.r / 255.0F, entry.glow.g / 255.0F, entry.glow.b / 255.0F, entry.glow.a / 255.0F};
        fill.trail = PaletteColor {entry.trail.r / 255.0F, entry.trail.g / 255.0F, entry.trail.b / 255.0F, entry.trail.a / 255.0F};
        generator.generatePaletteSet(renderer_, *textures_, std::to_string(idx), fill, 16);
        generator.generatePaletteSet(renderer_, *textures_, templ.name, fill, 16);
        ++idx;
    }
}


void RenderPipeline::generatePatternSignatures(const PatternBank& bank, const std::uint64_t seed) {
    if (!renderer_ || !textures_) return;
    PatternSignatureGenerator generator;
    for (const CompiledPatternGraph& graph : bank.compiledGraphs()) {
        (void)generator.generate(renderer_, *textures_, graph, seed ^ static_cast<std::uint64_t>(std::hash<std::string> {}(graph.id)));
    }
}
bool RenderPipeline::toggleFullscreen(SDL_Window* window) {
    if (!window) return false;
    const Uint32 mode = fullscreen_ ? 0U : SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (SDL_SetWindowFullscreen(window, mode) != 0) {
        logError("Failed to toggle fullscreen mode: " + std::string(SDL_GetError()));
        return false;
    }
    fullscreen_ = !fullscreen_;
    refreshDisplayMetrics(window);
    return true;
}

void RenderPipeline::buildSceneOverlay(const SimSnapshot& snapshot, const double frameDelta) {
    const GameplaySession& s = snapshot.session;
    if (s.bulletSimMode_ == BulletSimulationMode::CpuDeterministic) {
        s.projectiles_.renderProcedural(spriteBatch_, s.bulletPaletteTable_);
        s.particleFx_.render(spriteBatch_, bulletTextureId("0", BulletShape::Circle));
        s.projectiles_.debugDraw(debugDraw_, true, true);
    }
    s.entitySystem_.debugDraw(debugDraw_);
    debugDraw_.circle(s.playerPos(), s.playerRadius(), Color {120, 255, 170, 255}, 24);
    debugDraw_.circle(s.aimTarget(), 8.0F, Color {120, 180, 255, 220}, 18);
}

void RenderPipeline::renderFrame(const SimSnapshot& snapshot, const double frameDelta, ControlCenterToolSuite& toolSuite) {
    if (!renderContextReady_ || !renderer_ || !textures_) return;
    constexpr std::array<Uint8, 4> clearColor {20, 28, 40, 255};
    SDL_SetRenderDrawColor(renderer_, clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    SDL_RenderClear(renderer_);
    camera_.update(static_cast<float>(frameDelta));
    backgroundSystem_.update(static_cast<float>(frameDelta));
    spriteBatch_.begin(camera_);
    backgroundSystem_.render(spriteBatch_, camera_);
    buildSceneOverlay(snapshot, frameDelta);
    spriteBatch_.flush(renderer_, *textures_);
    debugDraw_.flush(renderer_, camera_);
    snapshot.session.renderDangerFieldOverlay(renderer_, camera_);
    toolSuite.beginFrame();
    toolSuite.drawControlCenter({});
    toolSuite.endFrame();
    SDL_RenderPresent(renderer_);
}

} // namespace engine
