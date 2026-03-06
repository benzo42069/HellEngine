#pragma once

#include <engine/background_layers.h>
#include <engine/config.h>
#include <engine/editor_tools.h>
#include <engine/gameplay_session.h>
#include <engine/modern_renderer.h>
#include <engine/palette_fx_templates.h>
#include <engine/render2d.h>

#include <SDL.h>

#include <memory>

namespace engine {

struct SimSnapshot {
    const GameplaySession& session;
};

class RenderPipeline {
  public:
    bool initialize(SDL_Window* window, const EngineConfig& config, ControlCenterToolSuite& toolSuite);
    void shutdown(ControlCenterToolSuite& toolSuite);
    bool recreate(const char* reason, SDL_Window* window, const EngineConfig& config, ControlCenterToolSuite& toolSuite);
    void refreshDisplayMetrics(SDL_Window* window);
    void renderFrame(const SimSnapshot& snapshot, double frameDelta, ControlCenterToolSuite& toolSuite);
    [[nodiscard]] bool contextReady() const { return renderContextReady_; }
    [[nodiscard]] SDL_Renderer* renderer() const { return renderer_; }
    [[nodiscard]] Camera2D& camera() { return camera_; }
    [[nodiscard]] float dpiScaleX() const { return dpiScaleX_; }
    [[nodiscard]] float dpiScaleY() const { return dpiScaleY_; }
    [[nodiscard]] float uiTextScale() const { return uiTextScale_; }
    bool toggleFullscreen(SDL_Window* window);
    void generateBulletSprites(const PaletteFxTemplateRegistry& registry, const BulletPaletteTable& table);

  private:
    void buildSceneOverlay(const SimSnapshot& snapshot, double frameDelta);

    SDL_Renderer* renderer_ {nullptr};
    bool renderContextReady_ {false};
    bool fullscreen_ {false};
    float dpiScaleX_ {1.0F};
    float dpiScaleY_ {1.0F};
    float uiTextScale_ {1.0F};

    Camera2D camera_;
    std::unique_ptr<TextureStore> textures_;
    RendererModernPipeline modernRenderer_ {};
    bool useModernRenderer_ {false};
    SpriteBatch spriteBatch_;
    DebugDraw debugDraw_;
    DebugText debugText_;
    BackgroundSystem backgroundSystem_ {};
    EngineConfig config_ {};
};

} // namespace engine
