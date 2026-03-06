#include <engine/render_pipeline.h>

#include <engine/logging.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <chrono>
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
    (void)textures_->loadTexture("projectile", "assets/projectile.png");
    toolSuite.initialize(window, renderer_);
    (void)debugText_.init(renderer_);
    debugText_.registerTexture(*textures_, "debug_font");
    renderContextReady_ = true;
    refreshDisplayMetrics(window);
    spriteBatch_.reserve(config_.projectileCapacity + 2048);
    return true;
}

void RenderPipeline::shutdown(ControlCenterToolSuite& toolSuite) {
    if (!renderContextReady_ && !renderer_ && !textures_) return;
    toolSuite.shutdown();
    modernRenderer_.shutdown();
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
    camera_.update(static_cast<float>(frameDelta));
    const GameplaySession& s = snapshot.session;
    if (s.bulletSimMode_ == BulletSimulationMode::CpuDeterministic) {
        s.projectiles_.render(spriteBatch_, "projectile");
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
    spriteBatch_.begin(camera_);
    buildSceneOverlay(snapshot, frameDelta);
    spriteBatch_.flush(renderer_, *textures_);
    debugDraw_.flush(renderer_, camera_);
    toolSuite.beginFrame();
    toolSuite.drawControlCenter({});
    toolSuite.endFrame();
    SDL_RenderPresent(renderer_);
}

} // namespace engine
