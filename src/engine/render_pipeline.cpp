#include <engine/render_pipeline.h>

#include <engine/bullet_sprite_gen.h>

#include <engine/logging.h>
#include <engine/pattern_signature.h>

#include <glad/glad.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <sstream>

namespace engine {

namespace {
constexpr const char* kBulletShaderName = "bullet";
constexpr const char* kBulletVertPath = "assets/shaders/bullet_vs.glsl";
constexpr const char* kBulletFragPath = "assets/shaders/bullet_fs.glsl";

PostFxSettings resolveAutoPostFx(const GameplaySession& session) {
    PostFxSettings fx {};
    const auto& registry = session.bulletPaletteRegistry_;
    const auto& db = registry.database();

    const std::string& archetypePalette = session.archetypeSystem_.selected().projectilePaletteName;
    std::string presetName;
    for (const PaletteTemplate& templ : db.palettes) {
        if (templ.name == archetypePalette && !templ.autoFxPreset.empty()) {
            presetName = templ.autoFxPreset;
            break;
        }
    }

    if (presetName.empty()) {
        const ZoneDefinition* zone = session.runStructure_.currentZone();
        if (zone != nullptr) {
            const char* zonePreset = nullptr;
            switch (zone->type) {
                case ZoneType::Elite: zonePreset = "Arcade Glow"; break;
                case ZoneType::Boss: zonePreset = "Boss Pressure"; break;
                case ZoneType::Event: zonePreset = "Calm Echo"; break;
                case ZoneType::Combat:
                default: zonePreset = "Arcade Glow"; break;
            }
            if (zonePreset != nullptr) presetName = zonePreset;
        }
    }

    if (!presetName.empty()) {
        if (const FxPreset* preset = registry.findFxPreset(presetName)) {
            fx = postFxFromPreset(*preset);
        }
    }
    return fx;
}

}

bool RenderPipeline::initialize(SDL_Window* window, const EngineConfig& config, ControlCenterToolSuite& toolSuite) {
    config_ = config;
    if (!window) {
        logError("Renderer initialization failed: window handle is null.");
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    glContext_ = SDL_GL_CreateContext(window);
    if (glContext_) {
        if (SDL_GL_MakeCurrent(window, glContext_) == 0) {
            if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)) != 0) {
                glReady_ = true;
                SDL_GL_SetSwapInterval(1);
                logInfo(std::string("OpenGL initialized: ") + reinterpret_cast<const char*>(glGetString(GL_VERSION)));
                (void)shaderCache_.compileFromFiles(kBulletShaderName, kBulletVertPath, kBulletFragPath);
                (void)spriteAtlas_.generate(32);
            } else {
                logWarn("GLAD initialization failed. Falling back to SDL_Renderer-only path.");
            }
        } else {
            logWarn("Unable to make GL context current. Falling back to SDL_Renderer-only path.");
        }
    } else {
        logWarn("Unable to create OpenGL context. Falling back to SDL_Renderer-only path.");
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
    lastBgStageIndex_ = std::numeric_limits<std::size_t>::max();
    lastBgZoneIndex_ = std::numeric_limits<std::size_t>::max();
    renderContextReady_ = true;
    refreshDisplayMetrics(window);
    if (glReady_) {
        const bool rampOk = paletteRamp_.generate(PaletteFxTemplateRegistry {}, BulletPaletteTable {}, 64);
        std::string bulletError;
        if (!rampOk || !glBulletRenderer_.initialize(shaderCache_, config_.projectileCapacity, &bulletError)) {
            logWarn("OpenGL bullet renderer init failed: " + bulletError + " Falling back to SpriteBatch bullets.");
            glBulletRenderer_.shutdown();
        }
    }
    spriteBatch_.reserve(config_.projectileCapacity + 2048);

    std::string modernError;
    modernPipelineEnabled_ = modernRenderer_.initialize(renderer_, camera_.viewportWidth(), camera_.viewportHeight(), &modernError);
    if (!modernPipelineEnabled_) {
        logWarn("Modern renderer initialization failed: " + modernError);
    }
    return true;
}

void RenderPipeline::shutdown(ControlCenterToolSuite& toolSuite) {
    if (!renderContextReady_ && !renderer_ && !textures_) return;
    toolSuite.shutdown();
    modernRenderer_.shutdown();
    glBulletRenderer_.shutdown();
    paletteRamp_.shutdown();
    shaderCache_.shutdown();
    proceduralPaletteRamp_.shutdown();
    spriteAtlas_.shutdown();
    backgroundSystem_.clear();
    textures_.reset();
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (glContext_) {
        SDL_GL_DeleteContext(glContext_);
        glContext_ = nullptr;
    }
    glReady_ = false;
    lastBgStageIndex_ = std::numeric_limits<std::size_t>::max();
    lastBgZoneIndex_ = std::numeric_limits<std::size_t>::max();
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
    if (modernPipelineEnabled_) {
        std::string resizeError;
        if (!modernRenderer_.resize(drawableW, drawableH, &resizeError)) {
            logWarn("Modern renderer resize failed: " + resizeError);
        }
    }
}



void RenderPipeline::generateBulletSprites(const PaletteFxTemplateRegistry& registry, const BulletPaletteTable& table) {
    if (!renderer_ || !textures_) return;
    BulletSpriteGenerator generator;
    const auto defaultFill = deriveProjectileFillFromCore(PaletteColor {1.0F, 1.0F, 1.0F, 1.0F});
    generator.generatePaletteSet(renderer_, *textures_, "0", defaultFill, 16);

    (void)proceduralPaletteRamp_.buildFromRegistry(registry);

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

    if (glReady_) {
        (void)paletteRamp_.generate(registry, table, 64);
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


RenderPipeline::ProjectileRenderPath RenderPipeline::resolveProjectileRenderPath(const GameplaySession& session) const {
    if (session.bulletSimMode_ != BulletSimulationMode::CpuCollisionDeterministic) {
        return ProjectileRenderPath::Disabled;
    }
    if (!glReady_ || !glBulletRenderer_.initialized() || !paletteRamp_.valid()) {
        return ProjectileRenderPath::ProceduralSpriteBatch;
    }
    return ProjectileRenderPath::GlInstanced;
}


void RenderPipeline::ensureZoneBackground(const GameplaySession& session) {
    if (!renderer_ || !textures_) return;
    const ZoneDefinition* zone = session.runStructure_.currentZone();
    if (!zone) return;

    const std::size_t stageIndex = session.runStructure_.stageIndex();
    const std::size_t zoneIndex = session.runStructure_.zoneIndex();
    if (stageIndex == lastBgStageIndex_ && zoneIndex == lastBgZoneIndex_) return;

    const std::string textureId = levelTileGenerator_.generateForZone(
        renderer_,
        *textures_,
        zone->type,
        static_cast<std::uint32_t>(stageIndex),
        config_.simulationSeed
    );
    if (!textureId.empty()) {
        backgroundSystem_.setPrimaryLayerTexture(textureId);
        lastBgStageIndex_ = stageIndex;
        lastBgZoneIndex_ = zoneIndex;
    }
}

void RenderPipeline::buildSceneOverlay(const SimSnapshot& snapshot, const double frameDelta) {
    (void)frameDelta;
    const GameplaySession& s = snapshot.session;
    const ProjectileRenderPath projectileRenderPath = resolveProjectileRenderPath(s);

    if (projectileRenderPath == ProjectileRenderPath::GlInstanced) {
        const auto& posX = s.projectiles_.posX();
        const auto& posY = s.projectiles_.posY();
        const auto& velX = s.projectiles_.velX();
        const auto& velY = s.projectiles_.velY();
        const auto& radius = s.projectiles_.radius();
        const auto& life = s.projectiles_.life();
        const auto& paletteIndex = s.projectiles_.paletteIndex();
        const auto& shape = s.projectiles_.shape();
        const auto& active = s.projectiles_.active();
        const auto& allegiance = s.projectiles_.allegiance();
        const auto& enableTrails = s.projectiles_.enableTrails();
        const auto& trailX = s.projectiles_.trailX();
        const auto& trailY = s.projectiles_.trailY();
        const auto& trailHead = s.projectiles_.trailHead();
        const auto& activeIndices = s.projectiles_.activeIndices();
        glBulletRenderer_.buildVertexBuffer(
            posX.data(),
            posY.data(),
            velX.data(),
            velY.data(),
            radius.data(),
            life.data(),
            paletteIndex.data(),
            shape.data(),
            active.data(),
            allegiance.data(),
            enableTrails.data(),
            trailX.data(),
            trailY.data(),
            trailHead.data(),
            ProjectileSystem::kTrailLength,
            activeIndices.data(),
            s.projectiles_.activeCount(),
            camera_,
            spriteAtlas_,
            paletteRamp_,
            s.bulletPaletteTable_,
            s.projectiles_.gradientAnimator(),
            static_cast<float>(s.simulation_.simClock)
        );
    } else if (projectileRenderPath == ProjectileRenderPath::ProceduralSpriteBatch) {
        s.projectiles_.renderProcedural(spriteBatch_, s.bulletPaletteTable_, static_cast<float>(s.simulation_.simClock));
    }

    if (projectileRenderPath != ProjectileRenderPath::Disabled) {
        s.presentation_.particleFx.render(spriteBatch_, bulletTextureId("0", BulletShape::Circle));
        s.projectiles_.debugDraw(debugDraw_, true, true);
    }

    s.entitySystem_.debugDraw(debugDraw_);
    debugDraw_.circle(s.playerPos(), s.playerRadius(), Color {120, 255, 170, 255}, 24);
    debugDraw_.circle(s.aimTarget(), 8.0F, Color {120, 180, 255, 220}, 18);
}

void RenderPipeline::renderFrame(const SimSnapshot& snapshot, const double frameDelta, ControlCenterToolSuite& toolSuite) {
    if (!renderContextReady_ || !renderer_ || !textures_) return;
    constexpr std::array<Uint8, 4> clearColor {20, 28, 40, 255};

    PostFxSettings fx = resolveAutoPostFx(snapshot.session);
    modernRenderer_.setPostFx(fx);

    if (modernPipelineEnabled_) {
        (void)modernRenderer_.beginScene(Color {clearColor[0], clearColor[1], clearColor[2], clearColor[3]});
    } else {
        SDL_SetRenderDrawColor(renderer_, clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        SDL_RenderClear(renderer_);
    }
    camera_.setCenter(snapshot.session.playerPos());
    for (const ShakeParams& shake : snapshot.session.consumeCameraShakeEvents()) {
        camera_.shakeSystem().trigger(shake);
    }
    camera_.update(static_cast<float>(frameDelta));
    ensureZoneBackground(snapshot.session);
    backgroundSystem_.update(static_cast<float>(frameDelta));
    spriteBatch_.begin(camera_);
    backgroundSystem_.render(spriteBatch_, camera_);
    buildSceneOverlay(snapshot, frameDelta);
    spriteBatch_.flush(renderer_, *textures_);
    if (resolveProjectileRenderPath(snapshot.session) == ProjectileRenderPath::GlInstanced) {
        glBulletRenderer_.render(camera_, static_cast<float>(snapshot.session.simulation_.simClock), spriteAtlas_, paletteRamp_, camera_.viewportWidth(), camera_.viewportHeight());
    }
    debugDraw_.flush(renderer_, camera_);
    snapshot.session.renderDangerFieldOverlay(renderer_, camera_);
    toolSuite.beginFrame();
    toolSuite.drawControlCenter({});
    toolSuite.endFrame();

    if (modernPipelineEnabled_) {
        modernRenderer_.endScene();
        modernRenderer_.present();
    }
    SDL_RenderPresent(renderer_);
}

} // namespace engine
