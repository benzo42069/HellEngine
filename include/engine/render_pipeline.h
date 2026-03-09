#pragma once

#include <engine/background_layers.h>
#include <engine/config.h>
#include <engine/editor_tools.h>
#include <engine/gameplay_session.h>
#include <engine/level_tile_gen.h>
#include <engine/modern_renderer.h>
#include <engine/palette_fx_templates.h>
#include <engine/palette_ramp.h>
#include <engine/render2d.h>
#include <engine/shader_cache.h>
#include <engine/sprite_atlas_gen.h>
#include <engine/gl_bullet_renderer.h>
#include <engine/patterns.h>

#include <SDL.h>

#include <limits>
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
    [[nodiscard]] TextureStore* textures() const { return textures_.get(); }
    [[nodiscard]] Camera2D& camera() { return camera_; }
    [[nodiscard]] float dpiScaleX() const { return dpiScaleX_; }
    [[nodiscard]] float dpiScaleY() const { return dpiScaleY_; }
    [[nodiscard]] float uiTextScale() const { return uiTextScale_; }
    bool toggleFullscreen(SDL_Window* window);
    void generateBulletSprites(const PaletteFxTemplateRegistry& registry, const BulletPaletteTable& table);
    void generatePatternSignatures(const PatternBank& bank, std::uint64_t seed);

  private:
    // Central projectile presentation routing. GameplaySession simulation mode gates
    // whether projectile visuals are rendered at all; renderer/backend readiness only
    // selects GL instanced vs procedural SDL SpriteBatch submission.
    enum class ProjectileRenderPath {
        Disabled,
        ProceduralSpriteBatch,
        GlInstanced,
    };

    [[nodiscard]] ProjectileRenderPath resolveProjectileRenderPath(const GameplaySession& session) const;
    void buildSceneOverlay(const SimSnapshot& snapshot, double frameDelta);
    void ensureZoneBackground(const GameplaySession& session);

    SDL_Renderer* renderer_ {nullptr};
    SDL_GLContext glContext_ {nullptr};
    bool glReady_ {false};
    bool renderContextReady_ {false};
    bool fullscreen_ {false};
    float dpiScaleX_ {1.0F};
    float dpiScaleY_ {1.0F};
    float uiTextScale_ {1.0F};

    Camera2D camera_;
    std::unique_ptr<TextureStore> textures_;
    RendererModernPipeline modernRenderer_ {};
    bool modernPipelineEnabled_ {false};
    SpriteBatch spriteBatch_;
    DebugDraw debugDraw_;
    DebugText debugText_;
    BackgroundSystem backgroundSystem_ {};
    LevelTileGenerator levelTileGenerator_ {};
    std::size_t lastBgStageIndex_ {std::numeric_limits<std::size_t>::max()};
    std::size_t lastBgZoneIndex_ {std::numeric_limits<std::size_t>::max()};
    EngineConfig config_ {};
    ShaderCache shaderCache_ {};
    GrayscaleSpriteAtlas spriteAtlas_ {};
    // Authoritative ramp sampled by GL projectile shaders (`GlBulletRenderer`).
    PaletteRampTexture paletteRamp_ {};
    GlBulletRenderer glBulletRenderer_ {};
    // Legacy/SDL sprite-generation ramp staging (non-GL projectile paths).
    PaletteRampTexture proceduralPaletteRamp_ {};
};

} // namespace engine
