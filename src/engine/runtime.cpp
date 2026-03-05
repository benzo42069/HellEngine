#include <engine/runtime.h>

#include <engine/logging.h>
#include <engine/timing.h>
#include <engine/public/plugins.h>
#include <engine/standards.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <sstream>
#include <string>

namespace engine {

Runtime::Runtime(EngineConfig config)
    : config_(std::move(config)),
      frameAllocator_(1024 * 1024),
      jobSystem_(),
      rngStreams_(config_.simulationSeed) {}

Runtime::~Runtime() {
    destroyRenderContext();

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}


bool Runtime::initializeRenderContext() {
    if (!window_) {
        logError("Renderer initialization failed: window handle is null.");
        return false;
    }

    const std::array<Uint32, 3> rendererFlags = {
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC,
        SDL_RENDERER_ACCELERATED,
        SDL_RENDERER_SOFTWARE,
    };

    for (const Uint32 flags : rendererFlags) {
        renderer_ = SDL_CreateRenderer(window_, -1, flags);
        if (renderer_) {
            break;
        }
    }

    if (!renderer_) {
        logError("GPU/renderer initialization failed. Tried accelerated and software paths. SDL error: " + std::string(SDL_GetError()));
        return false;
    }

    textures_ = std::make_unique<TextureStore>(renderer_);
    if (!textures_->loadTexture("projectile", "assets/sprites/sample.bmp")) {
        textures_->createSolidTexture("projectile", 8, 8, Color {250, 200, 120, 255});
    }

#if HELLEngine_ENABLE_MODERN_RENDERER
    useModernRenderer_ = config_.enableModernRenderer;
    if (useModernRenderer_) {
        int dw = 1;
        int dh = 1;
        SDL_GetRendererOutputSize(renderer_, &dw, &dh);
        std::string modernError;
        if (!modernRenderer_.initialize(renderer_, dw, dh, &modernError)) {
            useModernRenderer_ = false;
            logWarn("Modern renderer disabled; initialization failed: " + modernError);
        } else {
            PostFxSettings fx;
            fx.enabled = true;
            fx.bloomEnabled = true;
            fx.vignetteEnabled = true;
            fx.colorGradingEnabled = true;
            fx.bloomIntensity = 0.35F;
            fx.bloomRadius = 0.45F;
            fx.vignetteIntensity = 0.22F;
            fx.exposure = 1.02F;
            fx.contrast = 1.03F;
            fx.saturation = 1.08F;
            modernRenderer_.setPostFx(fx);
            logInfo("Modern renderer initialized (opt-in)");
        }
    }
#else
    useModernRenderer_ = false;
#endif

    toolSuite_.initialize(window_, renderer_);

    if (!debugText_.init(renderer_)) {
        logWarn("Debug text initialization failed; continuing without debug text atlas texture.");
    }
    debugText_.registerTexture(*textures_, "debug_font");

    renderContextReady_ = true;
    refreshDisplayMetrics();
    spriteBatch_.reserve(config_.projectileCapacity + 2048);
    return true;
}

bool Runtime::recreateRenderContext(const char* reason) {
    logWarn(std::string("Recreating render context: ") + (reason ? reason : "unknown reason"));
    destroyRenderContext();
    return initializeRenderContext();
}

void Runtime::destroyRenderContext() {
    if (!renderContextReady_ && !renderer_ && !textures_) {
        return;
    }

    toolSuite_.shutdown();
    modernRenderer_.shutdown();
    textures_.reset();

    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    renderContextReady_ = false;
}

void Runtime::refreshDisplayMetrics() {
    if (!window_) {
        return;
    }

    int windowW = 1;
    int windowH = 1;
    SDL_GetWindowSize(window_, &windowW, &windowH);

    int drawableW = windowW;
    int drawableH = windowH;
    SDL_GL_GetDrawableSize(window_, &drawableW, &drawableH);

    drawableW = std::max(drawableW, 1);
    drawableH = std::max(drawableH, 1);
    windowW = std::max(windowW, 1);
    windowH = std::max(windowH, 1);

    dpiScaleX_ = static_cast<float>(drawableW) / static_cast<float>(windowW);
    dpiScaleY_ = static_cast<float>(drawableH) / static_cast<float>(windowH);
    uiTextScale_ = std::clamp((dpiScaleX_ + dpiScaleY_) * 0.5F, 1.0F, 2.0F);

    camera_.setViewport(drawableW, drawableH);
}

Vec2 Runtime::currentMouseWorldPosition() const {
    if (!window_) {
        return {0.0F, 0.0F};
    }

    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);

    int windowW = 1;
    int windowH = 1;
    SDL_GetWindowSize(window_, &windowW, &windowH);
    windowW = std::max(windowW, 1);
    windowH = std::max(windowH, 1);

    const float drawableX = static_cast<float>(mouseX) * dpiScaleX_;
    const float drawableY = static_cast<float>(mouseY) * dpiScaleY_;
    const float halfW = static_cast<float>(windowW) * dpiScaleX_ * 0.5F;
    const float halfH = static_cast<float>(windowH) * dpiScaleY_ * 0.5F;
    const Vec2 camCenter = camera_.center();
    const float invZoom = 1.0F / camera_.zoom();

    return {
        (drawableX - halfW) * invZoom + camCenter.x,
        (drawableY - halfH) * invZoom + camCenter.y,
    };
}

bool Runtime::toggleFullscreen() {
    if (!window_) {
        return false;
    }

    const Uint32 mode = fullscreen_ ? 0U : SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (SDL_SetWindowFullscreen(window_, mode) != 0) {
        logError("Failed to toggle fullscreen mode: " + std::string(SDL_GetError()));
        return false;
    }

    fullscreen_ = !fullscreen_;
    refreshDisplayMetrics();
    return true;
}

int Runtime::run() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        logError("SDL_Init failed: " + std::string(SDL_GetError()));
        return 1;
    }

    projectiles_.initialize(config_.projectileCapacity, 420.0F, 32, 18);
    gpuBullets_.initialize(std::max<std::uint32_t>(config_.projectileCapacity, 500000U), 420.0F);

    std::string packSearchPath = config_.contentPackPath;
    for (const auto* plugin : engine::public_api::contentPackPlugins()) {
        for (const std::string& extraPath : plugin->contentPackPaths()) {
            if (!extraPath.empty()) {
                if (!packSearchPath.empty()) packSearchPath += ";";
                packSearchPath += extraPath;
            }
        }
    }

    const bool loadedPack = patternBank_.loadFromFile(packSearchPath)
        || patternBank_.loadFromFile("assets/patterns/sandbox_patterns.json");
    if (!loadedPack) {
        ErrorReport err = makeError(ErrorCode::ContentLoadFailed, "Pattern content failed to load");
        addContext(err, "requestedPath", packSearchPath);
        addContext(err, "fallbackPath", "assets/patterns/sandbox_patterns.json");
        pushStack(err, "Runtime::run/loadPatterns");
        logErrorReport(err);

        patternBank_.loadFallbackDefaults();
        ErrorReport fallback = makeError(ErrorCode::ContentFallbackActivated, "Using fallback patterns");
        pushStack(fallback, "Runtime::run/loadPatterns");
        logErrorReport(fallback);
    }
    patternPlayer_.setBank(&patternBank_);
    patternPlayer_.setRunSeed(config_.simulationSeed);
    patternPlayer_.setPatternIndex(0);
    graphVm_.reset(graphVmState_, config_.simulationSeed);

    const bool loadedEntities = entityDatabase_.loadFromFile(packSearchPath)
        || entityDatabase_.loadFromFile("data/entities.json");
    if (!loadedEntities) {
        ErrorReport err = makeError(ErrorCode::ContentLoadFailed, "Entity content failed to load");
        addContext(err, "requestedPath", packSearchPath);
        addContext(err, "fallbackPath", "data/entities.json");
        pushStack(err, "Runtime::run/loadEntities");
        logErrorReport(err);

        entityDatabase_.loadFallbackDefaults();
        ErrorReport fallback = makeError(ErrorCode::ContentFallbackActivated, "Using fallback entities");
        pushStack(fallback, "Runtime::run/loadEntities");
        logErrorReport(fallback);
    }
    entitySystem_.setTemplates(&entityDatabase_.templates());
    entitySystem_.setPatternBank(&patternBank_);
    entitySystem_.setRunSeed(config_.simulationSeed ^ 0xE7717U);
    entitySystem_.reset();

    traitSystem_.initialize(config_.simulationSeed);
    archetypeSystem_.initializeDefaults();
    metaProgression_.initializeDefaults();
    (void)metaProgression_.loadFromFile("meta_progression.json");
    archetypeSystem_.setUnlockTier(metaProgression_.bonuses().archetypeUnlockTier);
    runStructure_.initializeDefaults();
    difficultyModel_.initializeDefaults();
    (void)difficultyModel_.loadProfilesFromFile("data/difficulty_profiles.json");
    difficultyModel_.setProfile(difficultyProfileFromString(config_.difficultyProfile));
    DefensiveSpecialConfig defensiveCfg;
    defensiveCfg.durationSeconds = config_.defensiveDurationSeconds;
    defensiveCfg.cooldownPerChargeSeconds = config_.defensiveCooldownSeconds;
    TimeDilationPreset preset = presetFromLabel(config_.defensiveSpecialPreset.c_str());
    if (config_.defensiveEnemyScaleOverride > 0.0F) preset.enemyScale = config_.defensiveEnemyScaleOverride;
    if (config_.defensivePlayerProjectileScaleOverride > 0.0F) preset.playerProjectileScale = config_.defensivePlayerProjectileScaleOverride;
    defensiveSpecial_.initialize(defensiveCfg, preset);
    EngineStandards& stds = mutableEngineStandards();
    stds.playfieldWidth = config_.standardsPlayfieldWidth;
    stds.playfieldHeight = config_.standardsPlayfieldHeight;
    stds.renderTargetWidth = config_.standardsRenderTargetWidth;
    stds.renderTargetHeight = config_.standardsRenderTargetHeight;
    clampStandards(stds);
    playerHealth_ = 100.0F;
    prevTotalCollisions_ = 0;
    prevHealthRecoveryAccum_ = 0.0F;
    replayContentVersion_ = buildContentVersionTag(config_.contentPackPath);
    replayContentHash_ = buildContentHashTag(config_.contentPackPath);
    replayPlaybackMode_ = !config_.replayPlaybackPath.empty();
    if (replayPlaybackMode_) {
        if (!replayPlayer_.load(config_.replayPlaybackPath) || !replayPlayer_.validFor(config_.simulationSeed, replayContentVersion_, replayContentHash_)) {
            logError("Replay load/validation failed: " + config_.replayPlaybackPath);
            return 1;
        }
    }
    if (!config_.replayRecordPath.empty()) {
        replayRecorder_.begin(config_.simulationSeed, replayContentVersion_, replayContentHash_, config_.replayHashPeriodTicks);
    }

    (void)traitSystem_.rollChoices();
    upgradeScreenOpen_ = true;
    playerPos_ = {engineStandards().playfieldWidth * 0.5F, engineStandards().playfieldHeight * 0.33F};

    if (config_.stress10k) {
        projectiles_.spawnRadialBurst(10000, 80.0F, 3.0F, config_.simulationSeed);
    }

    if (!config_.headless) {
        window_ = SDL_CreateWindow(
            config_.windowTitle.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            config_.windowWidth,
            config_.windowHeight,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
        );

        if (!window_) {
            logError("Window initialization failed: " + std::string(SDL_GetError()));
            return 1;
        }

        if (!initializeRenderContext()) {
            return 1;
        }

        camera_.setCenter({0.0F, 0.0F});

        if (config_.rendererSmokeTest) {
            SDL_SetRenderDrawColor(renderer_, 12, 12, 16, 255);
            SDL_RenderClear(renderer_);
            SDL_RenderPresent(renderer_);
            logInfo("Renderer smoke test completed successfully.");
            return 0;
        }
    }

    auto last = std::chrono::steady_clock::now();
    double accumulator = 0.0;

    while (running_) {
        auto now = std::chrono::steady_clock::now();
        const double frameDelta = std::chrono::duration<double>(now - last).count();
        last = now;

        accumulator += frameDelta;

        if (!config_.headless) {
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0) {
                toolSuite_.processEvent(event);
                if (event.type == SDL_QUIT) running_ = false;
                if (event.type == SDL_WINDOWEVENT) {
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        running_ = false;
                    }
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED) {
                        refreshDisplayMetrics();
                    }
                }
                if (event.type == SDL_MOUSEWHEEL) {
                    camera_.addZoom(static_cast<float>(event.wheel.y) * 0.1F);
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_h) {
                    showHitboxes_ = !showHitboxes_;
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_g) {
                    showGrid_ = !showGrid_;
                }
                if (event.type == SDL_RENDER_TARGETS_RESET) {
                    if (!recreateRenderContext("SDL_RENDER_TARGETS_RESET")) {
                        running_ = false;
                    }
                }
                if (event.type == SDL_RENDER_DEVICE_RESET) {
                    if (!recreateRenderContext("SDL_RENDER_DEVICE_RESET")) {
                        running_ = false;
                    }
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_f) {
                    (void)toggleFullscreen();
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKQUOTE) {
                    debugHudOpen_ = !debugHudOpen_;
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F10) {
                    perfHudOpen_ = !perfHudOpen_;
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F11) {
                    useCompiledPatternGraph_ = !useCompiledPatternGraph_;
                    graphVm_.reset(graphVmState_, config_.simulationSeed ^ tickIndex_);
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F12) {
                    const bool toGpu = bulletSimMode_ == BulletSimulationMode::CpuDeterministic;
                    bulletSimMode_ = toGpu ? BulletSimulationMode::GpuMassHybrid : BulletSimulationMode::CpuDeterministic;
                    projectiles_.clear();
                    gpuBullets_.clear();
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                    patternPlayer_.togglePause();
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r) {
                    patternPlayer_.resetCurrentPattern();
                    projectiles_.clear();
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_PERIOD) {
                    patternPlayer_.adjustPlaybackSpeed(0.1F);
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_COMMA) {
                    patternPlayer_.adjustPlaybackSpeed(-0.1F);
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB) {
                    patternPlayer_.cyclePattern(1);
                }
                if (event.type == SDL_CONTROLLERBUTTONDOWN && event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
                    currentInputMask_ |= InputDefensiveSpecial;
                }
                handleUpgradeNavigation(event);
                if (event.type == SDL_KEYDOWN && archetypeSelectionOpen_ && event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_8) {
                    const std::size_t idx = static_cast<std::size_t>(event.key.keysym.sym - SDLK_1);
                    archetypeSystem_.select(idx);
                }
                if (event.type == SDL_KEYDOWN && archetypeSelectionOpen_ && event.key.keysym.sym == SDLK_RETURN) {
                    archetypeSelectionOpen_ = false;
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1) {
                    archetypeSelectionOpen_ = !archetypeSelectionOpen_;
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym >= SDLK_F2 && event.key.keysym.sym <= SDLK_F9) {
                    const std::size_t idx = static_cast<std::size_t>(event.key.keysym.sym - SDLK_F2);
                    if (metaProgression_.purchaseNode(idx)) {
                        archetypeSystem_.setUnlockTier(metaProgression_.bonuses().archetypeUnlockTier);
                    }
                }
                if (!archetypeSelectionOpen_ && event.type == SDL_KEYDOWN && event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_9 && !upgradeScreenOpen_) {
                    const std::size_t idx = static_cast<std::size_t>(event.key.keysym.sym - SDLK_1);
                    patternPlayer_.setPatternIndex(idx);
                }
            }

            const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
            currentInputMask_ = 0;
            if (!replayPlaybackMode_) {
                if (keyboard[SDL_SCANCODE_A] || keyboard[SDL_SCANCODE_LEFT]) currentInputMask_ |= InputMoveLeft;
                if (keyboard[SDL_SCANCODE_D] || keyboard[SDL_SCANCODE_RIGHT]) currentInputMask_ |= InputMoveRight;
                if (keyboard[SDL_SCANCODE_W] || keyboard[SDL_SCANCODE_UP]) currentInputMask_ |= InputMoveUp;
                if (keyboard[SDL_SCANCODE_S] || keyboard[SDL_SCANCODE_DOWN]) currentInputMask_ |= InputMoveDown;
                if (keyboard[SDL_SCANCODE_LSHIFT] || keyboard[SDL_SCANCODE_RSHIFT]) currentInputMask_ |= InputDefensiveSpecial;
            }

            const float cameraScale = defensiveSpecial_.dilationScales().cameraScroll;
            const float cameraSpeed = engineStandards().cameraMaxScrollUnitsPerSec * static_cast<float>(frameDelta) * cameraScale / camera_.zoom();
            if (keyboard[SDL_SCANCODE_A] || keyboard[SDL_SCANCODE_LEFT]) camera_.pan({-cameraSpeed, 0.0F});
            if (keyboard[SDL_SCANCODE_D] || keyboard[SDL_SCANCODE_RIGHT]) camera_.pan({cameraSpeed, 0.0F});
            if (keyboard[SDL_SCANCODE_W] || keyboard[SDL_SCANCODE_UP]) camera_.pan({0.0F, -cameraSpeed});
            if (keyboard[SDL_SCANCODE_S] || keyboard[SDL_SCANCODE_DOWN]) camera_.pan({0.0F, cameraSpeed});
            if (keyboard[SDL_SCANCODE_Q]) camera_.addZoom(-0.6F * static_cast<float>(frameDelta));
            if (keyboard[SDL_SCANCODE_E]) camera_.addZoom(0.6F * static_cast<float>(frameDelta));
        }

        profiler_.beginFrame();
        const StepResult stepResult = consumeFixedSteps(accumulator, config_.fixedDeltaSeconds, config_.maxFrameSteps);
        for (std::uint32_t i = 0; i < stepResult.steps; ++i) {
            simTick(config_.fixedDeltaSeconds);
        }
        accumulator = stepResult.remainingAccumulator;

        if (config_.targetTicks > 0 && static_cast<int>(tickIndex_) >= config_.targetTicks) {
            running_ = false;
        }

        if (!config_.headless) {
            renderFrame(frameDelta);
        }
    }


    if (runStructure_.state() == RunState::Completed) {
        const std::uint32_t runReward = 2U + static_cast<std::uint32_t>(entitySystem_.stats().defeatedBosses);
        metaProgression_.grantRunProgress(runReward);
    }
    archetypeSystem_.setUnlockTier(metaProgression_.bonuses().archetypeUnlockTier);
    (void)metaProgression_.saveToFile("meta_progression.json");
    if (!config_.replayRecordPath.empty()) {
        (void)replayRecorder_.save(config_.replayRecordPath);
    }

    return replayVerificationFailed_ ? 2 : 0;
}

void Runtime::simTick(const double dt) {
    using Clock = std::chrono::steady_clock;
    const auto simStart = Clock::now();
    gpuUpdateMsFrame_ = 0.0F;
    gpuRenderMsFrame_ = 0.0F;

    frameAllocator_.reset();
    auto* scratch = static_cast<std::uint64_t*>(frameAllocator_.allocate(sizeof(std::uint64_t), alignof(std::uint64_t)));
    *scratch = tickIndex_;

    projectiles_.beginTick();
    traitSystem_.onTick(tickIndex_);

    aimTarget_.x = 180.0F * std::cos(static_cast<float>(simClock_) * 0.9F);
    aimTarget_.y = 120.0F * std::sin(static_cast<float>(simClock_) * 1.3F);

    std::uint32_t inputMask = currentInputMask_;
    if (replayPlaybackMode_) {
        inputMask = replayPlayer_.inputForTick(tickIndex_);
    }
    if (!config_.replayRecordPath.empty()) {
        replayRecorder_.recordTickInput(inputMask);
    }

    if ((inputMask & InputDefensiveSpecial) != 0U) {
        (void)defensiveSpecial_.tryActivate();
    }

    const TraitModifiers& traitModsForSpecial = traitSystem_.modifiers();
    defensiveSpecial_.update(static_cast<float>(dt), traitModsForSpecial);
    const TimeDilationScales dilation = defensiveSpecial_.dilationScales();

    const std::uint32_t grazePoints = projectiles_.collectGrazePoints(playerPos_, playerRadius_, defensiveSpecial_.config().grazeBandInnerPadding, defensiveSpecial_.config().grazeBandOuterPadding, tickIndex_, defensiveSpecial_.config().grazeCooldownTicks);
    if (grazePoints > 0) defensiveSpecial_.addGrazePoints(grazePoints);

    const float moveStep = static_cast<float>(dt) * 120.0F;
    if ((inputMask & InputMoveLeft) != 0U) playerPos_.x -= moveStep;
    if ((inputMask & InputMoveRight) != 0U) playerPos_.x += moveStep;
    if ((inputMask & InputMoveUp) != 0U) playerPos_.y -= moveStep;
    if ((inputMask & InputMoveDown) != 0U) playerPos_.y += moveStep;
    playerPos_.x = std::clamp(playerPos_.x, 0.0F, static_cast<float>(engineStandards().playfieldWidth));
    playerPos_.y = std::clamp(playerPos_.y, 0.0F, static_cast<float>(engineStandards().playfieldHeight));

    const auto patternStart = Clock::now();
    if (!config_.stress10k) {
        const ArchetypeDefinition& archetype = archetypeSystem_.selected();
        const MetaBonuses& mb = metaProgression_.bonuses();
        const auto rareChanceMultiplier = 1.0F + (archetype.stats.rareChance + mb.rarityBonus) * 0.06F;
        traitSystem_.setRareChanceMultiplier(rareChanceMultiplier);

        if (tickIndex_ > 0 && tickIndex_ % 300 == 0 && !traitSystem_.hasPendingChoices()) {
            (void)traitSystem_.rollChoices();
            upgradeScreenOpen_ = true;
        }

        const UpgradeDebugOptions& upgradeDebug = toolSuite_.upgradeDebugOptions();
        perfHudOpen_ = upgradeDebug.showPerfHud;

        const std::string generatedGraphPath = toolSuite_.consumeGeneratedGraphPath();
        if (!generatedGraphPath.empty()) {
            PatternBank candidate = patternBank_;
            if (candidate.loadFromFile(generatedGraphPath)) {
                patternBank_ = std::move(candidate);
                patternPlayer_.setBank(&patternBank_);
                graphVm_.reset(graphVmState_, config_.simulationSeed ^ tickIndex_);
                useCompiledPatternGraph_ = !patternBank_.compiledGraphs().empty();
                hotReloadErrorMessage_.clear();
            } else {
                hotReloadErrorMessage_ = "Hot reload failed, kept last-good content: " + generatedGraphPath;
                ErrorReport err = makeError(ErrorCode::HotReloadFailed, hotReloadErrorMessage_);
                addContext(err, "path", generatedGraphPath);
                pushStack(err, "Runtime::simTick/hotReload");
                logErrorReport(err);
            }
        }
        if (upgradeDebug.spawnUpgradeScreen && !traitSystem_.hasPendingChoices()) {
            (void)traitSystem_.rollChoices();
            upgradeScreenOpen_ = true;
        }
        if (upgradeDebug.forcedRarity >= 0 && traitSystem_.hasPendingChoices()) {
            traitSystem_.forcePendingRarity(static_cast<TraitRarity>(upgradeDebug.forcedRarity));
        }

        const ProjectileDebugOptions projectileDebug = toolSuite_.consumeProjectileDebugOptions();
        auto spawnDebugBehavior = [this](const ProjectileBehavior& behavior, const float speed, const float radius) {
            (void)projectiles_.spawn(ProjectileSpawn {
                .pos = {0.0F, 0.0F},
                .vel = {speed, 0.0F},
                .radius = radius,
                .behavior = behavior,
            });
        };
        if (projectileDebug.spawnHoming) {
            ProjectileBehavior b;
            b.homingTurnRateDegPerSec = 180.0F;
            b.homingMaxAngleStepDeg = 12.0F;
            spawnDebugBehavior(b, 130.0F, 3.0F);
        }
        if (projectileDebug.spawnCurved) {
            ProjectileBehavior b;
            b.curvedAngularVelocityDegPerSec = 140.0F;
            spawnDebugBehavior(b, 110.0F, 3.0F);
        }
        if (projectileDebug.spawnAccelDrag) {
            ProjectileBehavior b;
            b.accelerationPerSec = 20.0F;
            b.dragPerSec = 0.2F;
            spawnDebugBehavior(b, 80.0F, 3.0F);
        }
        if (projectileDebug.spawnBounce) {
            ProjectileBehavior b;
            b.maxBounces = 3;
            (void)projectiles_.spawn(ProjectileSpawn {.pos = {300.0F, 0.0F}, .vel = {220.0F, 30.0F}, .radius = 3.0F, .behavior = b});
        }
        if (projectileDebug.spawnSplit) {
            ProjectileBehavior b;
            b.splitCount = 5;
            b.splitAngleSpreadDeg = 70.0F;
            b.splitDelaySeconds = 0.6F;
            spawnDebugBehavior(b, 90.0F, 3.0F);
        }
        if (projectileDebug.spawnExplode) {
            ProjectileBehavior b;
            b.explodeRadius = 22.0F;
            b.explodeShards = 10;
            (void)projectiles_.spawn(ProjectileSpawn {.pos = {390.0F, 0.0F}, .vel = {90.0F, 0.0F}, .radius = 3.0F, .behavior = b});
        }
        if (projectileDebug.spawnBeam) {
            ProjectileBehavior b;
            b.beamSegmentSamples = 8;
            b.beamDurationSeconds = 1.1F;
            spawnDebugBehavior(b, 140.0F, 2.0F);
        }

        if (config_.headless && traitSystem_.hasPendingChoices()) {
            traitSystem_.choose(0);
            upgradeScreenOpen_ = false;
        }

        const TraitModifiers& tm = traitSystem_.modifiers();

        const ZoneDefinition* zone = runStructure_.currentZone();
        if (zone) {
            difficultyModel_.setStageZone(runStructure_.stageIndex(), zone->type);
        }
        const DifficultyScalars diffScalars = difficultyModel_.scalars();
        playerRadius_ = 10.0F + (archetype.stats.defense + mb.defenseBonus) * 0.45F + tm.playerRadiusAdd;
        const float fireRateScale = 0.80F + (archetype.stats.fireRate + mb.fireRateBonus) * 0.05F;
        patternPlayer_.setRuntimeModifiers(tm.patternCooldownScale / fireRateScale, tm.patternExtraBullets, tm.patternJitterAddDeg);

        auto emitWithRuntimeMods = [this, &tm, &archetype, &mb, &diffScalars](const ProjectileSpawn& spawn) {
            ProjectileSpawn mod = spawn;
            mod.allegiance = ProjectileAllegiance::Player;
            const float powerScale = 0.85F + (archetype.stats.power + mb.powerBonus) * 0.03F;
            mod.vel.x *= tm.projectileSpeedMul * powerScale * diffScalars.patternSpeed * tm.offensiveSpecialPowerMul * defensiveSpecial_.playerDamageMultiplier();
            mod.vel.y *= tm.projectileSpeedMul * powerScale * diffScalars.patternSpeed * tm.offensiveSpecialPowerMul * defensiveSpecial_.playerDamageMultiplier();
            mod.radius = std::max(0.5F, spawn.radius + tm.projectileRadiusAdd);
            projectiles_.spawn(mod);
        };

        if (useCompiledPatternGraph_ && !patternBank_.compiledGraphs().empty()) {
            struct GraphEmitContext {
                Runtime* self {nullptr};
                const TraitModifiers* tm {nullptr};
                const ArchetypeDefinition* archetype {nullptr};
                const MetaBonuses* mb {nullptr};
                const DifficultyScalars* diff {nullptr};
            } ctx {this, &tm, &archetype, &mb, &diffScalars};

            graphVmState_.difficultyScalar = diffScalars.patternSpeed;
            graphVm_.execute(
                patternBank_.compiledGraphs().front(),
                graphVmState_,
                static_cast<float>(dt),
                {0.0F, 0.0F},
                aimTarget_,
                &ctx,
                [](void* user, const ProjectileSpawn& spawn) {
                    auto* c = static_cast<GraphEmitContext*>(user);
                    ProjectileSpawn mod = spawn;
                    mod.allegiance = ProjectileAllegiance::Player;
                    const float powerScale = 0.85F + (c->archetype->stats.power + c->mb->powerBonus) * 0.03F;
                    mod.vel.x *= c->tm->projectileSpeedMul * powerScale * c->diff->patternSpeed * c->tm->offensiveSpecialPowerMul;
                    mod.vel.y *= c->tm->projectileSpeedMul * powerScale * c->diff->patternSpeed * c->tm->offensiveSpecialPowerMul;
                    mod.radius = std::max(0.5F, spawn.radius + c->tm->projectileRadiusAdd);
                    c->self->projectiles_.spawn(mod);
                }
            );
        } else {
            patternPlayer_.update(static_cast<float>(dt), {0.0F, 0.0F}, aimTarget_, emitWithRuntimeMods);
        }

        EntityRuntimeModifiers em;
        em.enemyFireRateScale = tm.enemyFireRateScale * diffScalars.spawnRate;
        em.enemyProjectileSpeedScale = tm.enemyProjectileSpeedScale * diffScalars.patternSpeed;
        em.harvestYieldMultiplier = tm.playerHarvestMultiplier * (0.70F + archetype.stats.resourceGain * 0.06F);
        em.enemyHealthScale = diffScalars.enemyHp;

        if (zone) {
            switch (zone->type) {
                case ZoneType::Combat: break;
                case ZoneType::Elite:
                    em.enemyFireRateScale *= 1.25F;
                    em.enemyProjectileSpeedScale *= 1.20F;
                    break;
                case ZoneType::Event:
                    em.enemyFireRateScale *= 0.80F;
                    em.harvestYieldMultiplier *= 1.50F;
                    break;
                case ZoneType::Boss:
                    em.enemyFireRateScale *= 1.35F;
                    em.enemyProjectileSpeedScale *= 1.25F;
                    break;
            }
        }
        profiler_.addZoneTime(PerfZone::Patterns, std::chrono::duration<double, std::milli>(Clock::now() - patternStart).count());

        const auto bulletStart = Clock::now();
        entitySystem_.update(static_cast<float>(dt) * dilation.enemyMovement, projectiles_, playerPos_, em);
        profiler_.addZoneTime(PerfZone::Bullets, std::chrono::duration<double, std::milli>(Clock::now() - bulletStart).count());
    }

    const auto collisionStart = Clock::now();
    if (bulletSimMode_ == BulletSimulationMode::CpuDeterministic) {
        projectiles_.update(static_cast<float>(dt), playerPos_, playerRadius_, dilation.enemyProjectiles, dilation.playerProjectiles);
    } else {
        DeterministicRng& g = rngStreams_.stream("gpu-bullets");
        const std::uint32_t target = 150000U;
        const std::uint32_t active = gpuBullets_.activeCount();
        const std::uint32_t budget = active < target ? std::min<std::uint32_t>(2000U, target - active) : 0U;
        for (std::uint32_t i = 0; i < budget; ++i) {
            const float angle = g.nextFloat01() * 6.2831853F;
            const float speed = 40.0F + g.nextFloat01() * 200.0F;
            const float radius = 1.5F + g.nextFloat01() * 2.5F;
            GpuBullet b;
            b.posX = (g.nextFloat01() * 2.0F - 1.0F) * 300.0F;
            b.posY = (g.nextFloat01() * 2.0F - 1.0F) * 300.0F;
            b.velX = std::cos(angle) * speed;
            b.velY = std::sin(angle) * speed;
            b.lifetime = 4.0F + g.nextFloat01() * 6.0F;
            b.radius = radius;
            b.colorRgba = 0xFFD8A0FFU;
            b.flags = 1U | (g.nextFloat01() > 0.85F ? 2U : 0U);
            b.angularVelocityDegPerSec = (g.nextFloat01() * 2.0F - 1.0F) * 45.0F;
            (void)gpuBullets_.emit(b);
        }
        const auto gpuStart = Clock::now();
        gpuBullets_.update(static_cast<float>(dt));
        gpuUpdateMsFrame_ = static_cast<float>(std::chrono::duration<double, std::milli>(Clock::now() - gpuStart).count());
    }
    profiler_.addZoneTime(PerfZone::Collisions, std::chrono::duration<double, std::milli>(Clock::now() - collisionStart).count());

    const ProjectileStats& ps = projectiles_.stats();
    const EntityStats& es = entitySystem_.stats();
    const std::uint32_t collisionDelta = ps.totalCollisions - prevTotalCollisions_;
    collisionRateWindowSeconds_ += static_cast<float>(dt);
    collisionRateAccumulator_ += static_cast<float>(collisionDelta);
    const float collisionsPerMinute = collisionRateWindowSeconds_ > 0.0F
        ? (collisionRateAccumulator_ / collisionRateWindowSeconds_) * 60.0F
        : 0.0F;
    DeterministicPerformanceMetrics perfMetrics;
    perfMetrics.playerHealth01 = std::clamp(playerHealth_ / 100.0F, 0.0F, 1.0F);
    perfMetrics.collisionsPerMinute = collisionsPerMinute;
    perfMetrics.defeatedBosses = entitySystem_.stats().defeatedBosses;
    perfMetrics.tick = tickIndex_;
    difficultyModel_.update(static_cast<float>(dt), perfMetrics);
    prevTotalCollisions_ = ps.totalCollisions;
    const float healDelta = std::max(0.0F, es.healthRecoveryAccum - prevHealthRecoveryAccum_);
    prevHealthRecoveryAccum_ = es.healthRecoveryAccum;
    playerHealth_ = std::clamp(playerHealth_ - static_cast<float>(collisionDelta) * 4.0F + healDelta * 0.35F, 0.0F, 100.0F);

    runStructure_.update(static_cast<float>(dt), es.defeatedBosses, playerHealth_ > 0.0F);
    if (runStructure_.stageIndex() != stageIndexMemo_ || runStructure_.zoneIndex() != zoneIndexMemo_) {
        stageIndexMemo_ = runStructure_.stageIndex();
        zoneIndexMemo_ = runStructure_.zoneIndex();
        if (!traitSystem_.hasPendingChoices()) {
            (void)traitSystem_.rollChoices();
        }
        upgradeScreenOpen_ = runStructure_.state() == RunState::InProgress;
        focusedUpgradeIndex_ = 0;
        cardAnim_ = {};
    }

    const std::uint32_t hashPeriod = replayPlaybackMode_ ? replayPlayer_.hashPeriodTicks() : std::max(1U, config_.replayHashPeriodTicks);
    if ((tickIndex_ % hashPeriod) == 0U) {
        const std::uint64_t bulletsHash = projectiles_.debugStateHash();
        const std::uint64_t entitiesHash = computeReplayStateHash(
            tickIndex_,
            static_cast<std::uint64_t>(es.aliveTotal) ^ (static_cast<std::uint64_t>(es.aliveEnemies) << 8U),
            static_cast<std::uint64_t>(es.aliveBosses) ^ (static_cast<std::uint64_t>(es.harvestedNodes) << 8U),
            static_cast<std::uint64_t>(ps.totalCollisions) ^ static_cast<std::uint64_t>(es.defeatedBosses)
        );
        const std::uint64_t runStateHash = computeReplayStateHash(
            tickIndex_,
            static_cast<std::uint64_t>(runStructure_.stageIndex()),
            static_cast<std::uint64_t>(runStructure_.zoneIndex()),
            static_cast<std::uint64_t>(runStructure_.state()) ^ static_cast<std::uint64_t>(std::llround(playerHealth_ * 100.0F)) ^ defensiveSpecial_.deterministicHash()
        );
        ReplayStateSample sample;
        sample.tick = tickIndex_;
        sample.bulletsHash = bulletsHash;
        sample.entitiesHash = entitiesHash;
        sample.runStateHash = runStateHash;
        sample.totalHash = computeReplayStateHash(tickIndex_, bulletsHash, entitiesHash, runStateHash);

        if (!config_.replayRecordPath.empty()) {
            replayRecorder_.recordStateSample(sample);
        }
        if (replayPlaybackMode_) {
            ReplayMismatch mismatch;
            if (!replayPlayer_.verifyStateSample(sample, &mismatch)) {
                logError(
                    "Replay verification mismatch at tick " + std::to_string(mismatch.tick)
                    + " subsystem=" + std::string(toString(mismatch.subsystem))
                    + " expected=" + std::to_string(mismatch.expected)
                    + " actual=" + std::to_string(mismatch.actual)
                );
                replayVerificationFailed_ = true;
                running_ = false;
            }
        }
    }

    if (runStructure_.state() == RunState::Completed || runStructure_.state() == RunState::Failed) {
        running_ = false;
    }

    auto futureNoise = jobSystem_.enqueue([this]() {
        return rngStreams_.stream("sim").nextFloat01();
    });

    const float noise = futureNoise.get();
    jobSystem_.waitIdle();

    simClock_ += dt;
    ++tickIndex_;

    if (tickIndex_ % 240 == 0) {
        camera_.setShake(2.0F + noise * 2.5F, 0.12F);
        std::ostringstream os;
        os << "Sim tick=" << tickIndex_ << " activeProjectiles=" << projectiles_.stats().activeCount << " collisionsTotal=" << projectiles_.stats().totalCollisions;
        logInfo(os.str());
    }

    profiler_.addZoneTime(PerfZone::Simulation, std::chrono::duration<double, std::milli>(Clock::now() - simStart).count());

    const float spawnsPerSec = static_cast<float>(ps.spawnedThisTick) / static_cast<float>(std::max(dt, 0.0001));
    profiler_.setCounters(PerfCounters {
                .activeBullets = bulletSimMode_ == BulletSimulationMode::CpuDeterministic ? ps.activeCount : 0U,
        .bulletSpawnsPerSecond = bulletSimMode_ == BulletSimulationMode::CpuDeterministic ? spawnsPerSec : 0.0F,
        .broadphaseChecks = ps.broadphaseChecksThisTick,
        .narrowphaseChecks = ps.narrowphaseChecksThisTick,
        .drawCalls = spriteBatch_.lastStats().drawCalls,
        .batches = spriteBatch_.lastStats().batchFlushes,
        .gpuActiveBullets = bulletSimMode_ == BulletSimulationMode::GpuMassHybrid ? gpuBullets_.activeCount() : 0U,
        .gpuUpdateMs = gpuUpdateMsFrame_,
        .gpuRenderMs = gpuRenderMsFrame_,
    });
}


void Runtime::buildSceneOverlay(const double frameDelta) {
    camera_.update(static_cast<float>(frameDelta));

    if (bulletSimMode_ == BulletSimulationMode::CpuDeterministic) {
        projectiles_.render(spriteBatch_, "projectile");
        projectiles_.debugDraw(debugDraw_, showHitboxes_, showGrid_);
    }
    entitySystem_.debugDraw(debugDraw_);

    debugDraw_.circle(playerPos_, playerRadius_, Color {120, 255, 170, 255}, 24);
    debugDraw_.circle(aimTarget_, 8.0F, Color {120, 180, 255, 220}, 18);

    const double fps = frameDelta > 0.0 ? 1.0 / frameDelta : 0.0;
    const ProjectileStats& s = projectiles_.stats();
    const PatternDefinition* activePattern = patternPlayer_.activePattern();
    const EntityStats& es = entitySystem_.stats();
    const TraitModifiers& tm = traitSystem_.modifiers();
    const ArchetypeDefinition& archetype = archetypeSystem_.selected();
    const StageDefinition* stage = runStructure_.currentStage();
    const ZoneDefinition* zone = runStructure_.currentZone();
    const MetaBonuses& mb = metaProgression_.bonuses();
    const Vec2 mouseWorld = currentMouseWorldPosition();

    if (debugHudOpen_) {
    std::ostringstream overlay;
    overlay << "FPS " << static_cast<int>(fps)
            << "  Tick " << tickIndex_
            << "  Projectiles " << s.activeCount
            << "  Collisions " << s.totalCollisions
            << "  Entities " << es.aliveTotal
            << " (E:" << es.aliveEnemies << " B:" << es.aliveBosses << " R:" << es.aliveResources << " H:" << es.aliveHazards << ")"
            << "\nHarvested " << es.harvestedNodes
            << "  Currency " << static_cast<int>(es.upgradeCurrency)
            << "  Health+ " << static_cast<int>(es.healthRecoveryAccum)
            << "  Buff " << es.buffTimeRemaining
            << "  BossDefeats " << es.defeatedBosses
            << "  BossTransitions " << es.bossPhaseTransitions
            << "\nRun " << toString(runStructure_.state())
            << "  MetaPts " << metaProgression_.progressionPoints()
            << "  MetaTier " << mb.archetypeUnlockTier
            << "  Replay " << (replayPlaybackMode_ ? "playback" : (!config_.replayRecordPath.empty() ? "record" : "off"))
            << "  HP " << static_cast<int>(playerHealth_)
            << "  Stage " << (stage ? stage->name : std::string("none"))
            << "  Zone " << (zone ? toString(zone->type) : std::string("none"))
            << "  ZoneT " << runStructure_.zoneTimeRemaining()
            << "  BulletMode " << (bulletSimMode_ == BulletSimulationMode::CpuDeterministic ? "CPU" : "GPU-Hybrid")
            << "\nTraits " << traitSystem_.activeTraits().size()
            << "  MetaPwr+ " << mb.powerBonus
            << "  MetaFR+ " << mb.fireRateBonus
            << "  TraitSpd " << tm.projectileSpeedMul
            << "  TraitCD " << tm.patternCooldownScale
            << "\nGraphVM " << (useCompiledPatternGraph_ ? "on" : "off")
            << "  GraphPrograms " << patternBank_.compiledGraphs().size()
            << "  GraphIssues " << patternBank_.graphDiagnostics().size()
            << "  ShaderPacks " << engine::public_api::shaderPackPlugins().size()
            << "  ContentPlugins " << engine::public_api::contentPackPlugins().size()
            << "  ToolPanels " << engine::public_api::toolPanelPlugins().size()
            << "\nArchetype " << archetype.name
            << "  Weapon " << archetype.primaryWeapon
            << "  Active " << archetype.activeAbility
            << "\nPattern " << (activePattern ? activePattern->name : std::string("none"))
            << "  Layers " << (activePattern ? activePattern->layers.size() : 0)
            << "  SeqStep " << patternPlayer_.activeSequenceStep()
            << "  ActiveLayers " << patternPlayer_.activeLayerCountForStep()
            << "  Speed " << patternPlayer_.playbackSpeed()
            << "  Paused " << (patternPlayer_.paused() ? "yes" : "no")
            << "\nStandards PF " << engineStandards().playfieldWidth << "x" << engineStandards().playfieldHeight
            << " RT " << engineStandards().renderTargetWidth << "x" << engineStandards().renderTargetHeight
            << " Density N/B/X " << engineStandards().densityNormal << "/" << engineStandards().densityBoss << "/" << engineStandards().densityExtreme
            << "\n[F1] archetypes  [F2-F9] buy meta node  [U] upgrades  [Arrows/Pad] navigate upgrades  [Enter/A] confirm  [X/R] reroll  [1-9/TAB] pattern  [SPACE] pause  [R] reset  [,/.] speed  [H] hitboxes  [G] grid  [`] debug HUD  [F10] perf HUD  [F11] graph VM  [F12] bullet mode";

    if (archetypeSelectionOpen_) {
        overlay << "\n\n=== Archetype Selection (press [1-8], [Enter] to close) ===";
        const auto& defs = archetypeSystem_.archetypes();
        for (std::size_t i = 0; i < defs.size(); ++i) {
            const ArchetypeDefinition& def = defs[i];
            const char* marker = i == archetypeSystem_.selectedIndex() ? ">" : " ";
            overlay << "\n" << marker << " [" << (i + 1) << "] " << def.name
                    << " Pwr:" << def.stats.power
                    << " FR:" << def.stats.fireRate
                    << " Mv:" << def.stats.moveSpeed
                    << " Def:" << def.stats.defense
                    << " Syn:" << def.stats.synergy
                    << " Res:" << def.stats.resourceGain
                    << " DoT:" << def.stats.dotPower
                    << " Rare:" << def.stats.rareChance
                    << " | " << def.primaryWeapon
                    << " | " << def.activeAbility
                    << (archetypeSystem_.isUnlocked(i) ? "" : " [LOCKED]");
        }
    }

    if (!hotReloadErrorMessage_.empty()) {
        overlay << "\nHotReloadError: " << hotReloadErrorMessage_;
    }

    debugText_.drawText(spriteBatch_, overlay.str(), Vec2 {-620.0F, -330.0F}, 0.85F * uiTextScale_, Color {255, 255, 255, 255});
    }

    if (perfHudOpen_) {
        const PerfSnapshot& perf = profiler_.snapshot();
        std::ostringstream perfHud;
        perfHud << "PERF " << perf.frameMs << "ms"
                << "\nSim " << perf.zoneMs[static_cast<std::size_t>(PerfZone::Simulation)]
                << "  Pat " << perf.zoneMs[static_cast<std::size_t>(PerfZone::Patterns)]
                << "  Bul " << perf.zoneMs[static_cast<std::size_t>(PerfZone::Bullets)]
                << "  Col " << perf.zoneMs[static_cast<std::size_t>(PerfZone::Collisions)]
                << "  Ren " << perf.zoneMs[static_cast<std::size_t>(PerfZone::Render)]
                << "\nBullets " << perf.counters.activeBullets
                << "  Spawn/s " << static_cast<int>(perf.counters.bulletSpawnsPerSecond)
                << "  BP " << perf.counters.broadphaseChecks
                << "  NP " << perf.counters.narrowphaseChecks;
        if (perf.warningSimulationBudget || perf.warningRenderBudget || perf.warningCollisionBudget || perf.counters.activeBullets > static_cast<std::uint32_t>(engineStandards().densityBoss)) {
            perfHud << "\nWARNING:";
            if (perf.warningSimulationBudget) perfHud << " Sim > 8ms";
            if (perf.warningRenderBudget) perfHud << " Render > 4ms";
            if (perf.warningCollisionBudget) perfHud << " Collisions > 2ms";
            if (perf.counters.activeBullets > static_cast<std::uint32_t>(engineStandards().densityBoss)) perfHud << " Density>BossTarget";
        }
        debugText_.drawText(spriteBatch_, perfHud.str(), Vec2 {320.0F, -330.0F}, 0.9F * uiTextScale_, Color {255, 240, 180, 255});
    }
}


UpgradeViewStats Runtime::buildCurrentViewStats() const {
    return buildUpgradeViewStats(archetypeSystem_.selected(), metaProgression_.bonuses(), traitSystem_.modifiers());
}

UpgradeViewStats Runtime::buildProjectedViewStats(const Trait& trait) const {
    return projectUpgradeViewStats(buildCurrentViewStats(), trait);
}

bool Runtime::hasSynergyWithActive(const Trait& trait) const {
    for (const Trait& active : traitSystem_.activeTraits()) {
        for (std::size_t i = 0; i < trait.synergyTagCount; ++i) {
            for (std::size_t j = 0; j < active.synergyTagCount; ++j) {
                if (!trait.synergyTags[i].empty() && trait.synergyTags[i] == active.synergyTags[j]) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Runtime::handleUpgradeNavigation(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_u) {
        upgradeScreenOpen_ = traitSystem_.hasPendingChoices() ? !upgradeScreenOpen_ : upgradeScreenOpen_;
    }
    if (!upgradeScreenOpen_ || !traitSystem_.hasPendingChoices()) return;

    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a) {
            focusedUpgradeIndex_ = (focusedUpgradeIndex_ + TraitSystem::choiceCount - 1) % TraitSystem::choiceCount;
        }
        if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d) {
            focusedUpgradeIndex_ = (focusedUpgradeIndex_ + 1) % TraitSystem::choiceCount;
        }
        if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_3) {
            focusedUpgradeIndex_ = static_cast<std::size_t>(event.key.keysym.sym - SDLK_1);
            if (traitSystem_.choose(focusedUpgradeIndex_)) upgradeScreenOpen_ = false;
        }
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
            if (traitSystem_.choose(focusedUpgradeIndex_)) upgradeScreenOpen_ = false;
        }
        if (event.key.keysym.sym == SDLK_x) {
            (void)traitSystem_.rerollChoices();
        }
    }
    if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
            focusedUpgradeIndex_ = (focusedUpgradeIndex_ + TraitSystem::choiceCount - 1) % TraitSystem::choiceCount;
        }
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
            focusedUpgradeIndex_ = (focusedUpgradeIndex_ + 1) % TraitSystem::choiceCount;
        }
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
            if (traitSystem_.choose(focusedUpgradeIndex_)) upgradeScreenOpen_ = false;
        }
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_X) {
            (void)traitSystem_.rerollChoices();
        }
    }
}

void Runtime::drawUpgradeSelectionUi(const double frameDelta) {
    if (!upgradeScreenOpen_ || !traitSystem_.hasPendingChoices()) return;

    const ImVec2 viewport = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowPos(ImVec2(20.0F, 40.0F), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport.x - 40.0F, viewport.y - 80.0F), ImGuiCond_Always);
    ImGui::Begin("Upgrade Selection", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    ImGui::TextUnformatted("Choose an upgrade");
    ImGui::SameLine();
    ImGui::Text("Rerolls: %d (cooldown: %llut)", traitSystem_.rerollCharges(), static_cast<unsigned long long>(traitSystem_.rerollCooldownRemainingTicks(tickIndex_)));

    const auto& choices = traitSystem_.pendingChoices();
    for (std::size_t i = 0; i < choices.size(); ++i) {
        UpgradeCardAnimState& anim = cardAnim_[i];
        anim.appearT = std::min(1.0F, anim.appearT + static_cast<float>(frameDelta) * 3.0F);
        anim.hoverT = (i == focusedUpgradeIndex_) ? std::min(1.0F, anim.hoverT + static_cast<float>(frameDelta) * 6.0F) : std::max(0.0F, anim.hoverT - static_cast<float>(frameDelta) * 6.0F);

        const Trait& t = choices[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::BeginGroup();

        const ImVec4 rarityColor = t.rarity == TraitRarity::Common ? ImVec4(0.55F, 0.56F, 0.58F, 1.0F) : (t.rarity == TraitRarity::Rare ? ImVec4(0.24F, 0.48F, 0.95F, 1.0F) : ImVec4(0.78F, 0.58F, 0.18F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_Border, rarityColor);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5F + anim.hoverT * 1.5F);
        ImGui::BeginChild("card", ImVec2((viewport.x - 120.0F) / 3.0F, 220.0F + anim.appearT * 20.0F), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("[%s] %s", t.iconToken.c_str(), t.name.c_str());
        ImGui::TextColored(rarityColor, "%s", rarityLabel(t.rarity));
        ImGui::Separator();
        ImGui::TextWrapped("%s", t.description.c_str());
        ImGui::Text("Stat Δ: spd %.2f  rad %.2f  cd %.2f  +bul %d", t.modifiers.projectileSpeedMul, t.modifiers.projectileRadiusAdd, t.modifiers.patternCooldownScale, t.modifiers.patternExtraBullets);
        if (hasSynergyWithActive(t)) {
            ImGui::TextColored(ImVec4(0.25F, 0.9F, 0.45F, 1.0F), "Synergy with current build");
        }
        if (i == focusedUpgradeIndex_) {
            ImGui::TextColored(ImVec4(1.0F, 0.9F, 0.3F, 1.0F), "Focused (Enter / A to select)");
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        if ((i + 1) < choices.size()) ImGui::SameLine();
        ImGui::EndGroup();
        ImGui::PopID();
    }

    const Trait& focused = choices[focusedUpgradeIndex_];
    const UpgradeViewStats current = buildCurrentViewStats();
    const UpgradeViewStats projected = buildProjectedViewStats(focused);

    ImGui::Separator();
    ImGui::Text("Stat Preview");
    auto statRow = [](const char* label, const float a, const float b) {
        ImGui::Text("%s: %.2f -> %.2f", label, a, b);
    };
    statRow("Power", current.power, projected.power);
    statRow("Fire Rate", current.fireRate, projected.fireRate);
    statRow("Move Speed", current.moveSpeed, projected.moveSpeed);
    statRow("Defense", current.defense, projected.defense);

    if (ImGui::Button("Reroll [X]")) {
        (void)traitSystem_.rerollChoices();
    }

    ImGui::SameLine();
    if (ImGui::Button("Select Focused [Enter]")) {
        if (traitSystem_.choose(focusedUpgradeIndex_)) upgradeScreenOpen_ = false;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Upgrade History");
    const auto& active = traitSystem_.activeTraits();
    const std::size_t start = active.size() > 6 ? active.size() - 6 : 0;
    for (std::size_t i = start; i < active.size(); ++i) {
        ImGui::BulletText("%s (%s)", active[i].name.c_str(), rarityLabel(active[i].rarity));
    }

    const auto& validation = traitSystem_.validationIssues();
    ImGui::Separator();
    if (validation.empty()) {
        ImGui::TextColored(ImVec4(0.2F, 0.95F, 0.3F, 1.0F), "Validation: PASS");
    } else {
        ImGui::TextColored(ImVec4(1.0F, 0.35F, 0.35F, 1.0F), "Validation: %d issue(s)", static_cast<int>(validation.size()));
        for (const TraitValidationIssue& issue : validation) {
            ImGui::BulletText("%s: %s", issue.traitId.c_str(), issue.message.c_str());
        }
    }

    ImGui::End();

}

void Runtime::renderFrame(const double frameDelta) {
    if (!renderContextReady_ || !renderer_ || !textures_) {
        logError("Render frame skipped because render context is unavailable.");
        running_ = false;
        return;
    }

    using Clock = std::chrono::steady_clock;
    const auto renderStart = Clock::now();

    constexpr std::array<Uint8, 4> clearColor {20, 28, 40, 255};
    if (useModernRenderer_) {
#if HELLEngine_ENABLE_MODERN_RENDERER
        int dw = 1;
        int dh = 1;
        SDL_GetRendererOutputSize(renderer_, &dw, &dh);
        std::string resizeError;
        if (!modernRenderer_.resize(dw, dh, &resizeError)) {
            useModernRenderer_ = false;
            logWarn("Modern renderer resize failed; falling back to legacy: " + resizeError);
        }
#endif
    }

    if (useModernRenderer_) {
        (void)modernRenderer_.beginScene(Color {clearColor[0], clearColor[1], clearColor[2], clearColor[3]});
    } else {
        SDL_SetRenderDrawColor(renderer_, clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        SDL_RenderClear(renderer_);
    }

    spriteBatch_.begin(camera_);
    buildSceneOverlay(frameDelta);
    spriteBatch_.flush(renderer_, *textures_);
    if (bulletSimMode_ == BulletSimulationMode::GpuMassHybrid) {
        const auto gpuRenderStart = Clock::now();
        gpuBullets_.render(renderer_, camera_, textures_->get("projectile"));
        gpuRenderMsFrame_ = static_cast<float>(std::chrono::duration<double, std::milli>(Clock::now() - gpuRenderStart).count());
    }
    debugDraw_.flush(renderer_, camera_);
    if (defensiveSpecial_.active()) {
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer_, 80, 180, 255, 36);
        SDL_Rect overlay {0, 0, std::max(1, static_cast<int>(config_.windowWidth * dpiScaleX_)), std::max(1, static_cast<int>(config_.windowHeight * dpiScaleY_))};
        SDL_RenderFillRect(renderer_, &overlay);
    }

    profiler_.addZoneTime(PerfZone::Render, std::chrono::duration<double, std::milli>(Clock::now() - renderStart).count());
    profiler_.endFrame();

    ToolRuntimeSnapshot snapshot;
    snapshot.tick = tickIndex_;
    snapshot.fps = frameDelta > 0.0 ? static_cast<int>(1.0 / frameDelta) : 0;
    snapshot.frameMs = static_cast<float>(frameDelta * 1000.0);
    snapshot.projectileCount = bulletSimMode_ == BulletSimulationMode::CpuDeterministic ? projectiles_.stats().activeCount : gpuBullets_.activeCount();
    snapshot.entityCount = entitySystem_.stats().aliveTotal;
    snapshot.collisionsTotal = projectiles_.stats().totalCollisions;
    snapshot.upgradeScreenOpen = upgradeScreenOpen_;
    const int forcedRarity = toolSuite_.upgradeDebugOptions().forcedRarity;
    snapshot.forceRarityLabel = forcedRarity < 0 ? "auto" : (forcedRarity == 0 ? "common" : (forcedRarity == 1 ? "rare" : "relic"));
    const PerfSnapshot& perf = profiler_.snapshot();
    snapshot.frameTimeMs = perf.frameMs;
    snapshot.simMs = perf.zoneMs[static_cast<std::size_t>(PerfZone::Simulation)];
    snapshot.patternMs = perf.zoneMs[static_cast<std::size_t>(PerfZone::Patterns)];
    snapshot.bulletMs = perf.zoneMs[static_cast<std::size_t>(PerfZone::Bullets)];
    snapshot.collisionMs = perf.zoneMs[static_cast<std::size_t>(PerfZone::Collisions)];
    snapshot.renderMs = perf.zoneMs[static_cast<std::size_t>(PerfZone::Render)];
    snapshot.activeBullets = perf.counters.activeBullets;
    snapshot.spawnsPerSecond = perf.counters.bulletSpawnsPerSecond;
    snapshot.broadphaseChecks = perf.counters.broadphaseChecks;
    snapshot.narrowphaseChecks = perf.counters.narrowphaseChecks;
    snapshot.drawCalls = perf.counters.drawCalls;
    snapshot.renderBatches = perf.counters.batches;
    snapshot.gpuActiveBullets = perf.counters.gpuActiveBullets;
    snapshot.gpuUpdateMs = perf.counters.gpuUpdateMs;
    snapshot.gpuRenderMs = perf.counters.gpuRenderMs;
    snapshot.perfWarnSimulation = perf.warningSimulationBudget;
    snapshot.perfWarnRender = perf.warningRenderBudget;
    snapshot.perfWarnCollisions = perf.warningCollisionBudget;
    difficultyProfileLabelCache_ = difficultyModel_.profileLabel();
    snapshot.difficultyProfileLabel = difficultyProfileLabelCache_.c_str();
    snapshot.difficultyOverall = difficultyModel_.currentDifficulty();
    snapshot.difficultyPatternSpeed = difficultyModel_.scalars().patternSpeed;
    snapshot.difficultySpawnRate = difficultyModel_.scalars().spawnRate;
    snapshot.difficultyEnemyHp = difficultyModel_.scalars().enemyHp;
    toolSuite_.beginFrame();
    ImGui::SetNextWindowBgAlpha(0.30F);
    if (ImGui::Begin("DefensiveSpecial HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
        ImGui::Text("DefensiveSpecial [Shift / Gamepad RB]");
        ImGui::Text("Active: %s  Remaining %.2fs", defensiveSpecial_.active() ? "yes" : "no", defensiveSpecial_.activeTimeRemaining());
        ImGui::Text("Charges: %d / %d", defensiveSpecial_.currentCharges(), defensiveSpecial_.capacity());
        for (int i = 0; i < defensiveSpecial_.capacity(); ++i) {
            ImGui::SameLine();
            ImGui::ProgressBar(defensiveSpecial_.cooldownProgressForCharge(i), ImVec2(52.0F, 8.0F), "");
        }
    }
    ImGui::End();
    drawUpgradeSelectionUi(frameDelta);
    toolSuite_.drawControlCenter(snapshot);
    toolSuite_.endFrame();

    if (useModernRenderer_) {
        modernRenderer_.endScene();
        modernRenderer_.present();
    }

    SDL_RenderPresent(renderer_);
}

} // namespace engine
