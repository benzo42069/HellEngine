#include <engine/runtime.h>

#include <engine/logging.h>
#include <engine/timing.h>

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
    if (!config_.headless) {
        toolSuite_.shutdown();
    }

    textures_.reset();

    if (!config_.headless) {
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
    }

    SDL_Quit();
}

int Runtime::run() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        logError("SDL_Init failed: " + std::string(SDL_GetError()));
        return 1;
    }

    projectiles_.initialize(config_.projectileCapacity, 420.0F, 32, 18);

    const bool loadedPack = patternBank_.loadFromFile(config_.contentPackPath)
        || patternBank_.loadFromFile("assets/patterns/sandbox_patterns.json");
    if (!loadedPack) {
        patternBank_.loadFallbackDefaults();
    }
    patternPlayer_.setBank(&patternBank_);
    patternPlayer_.setRunSeed(config_.simulationSeed);
    patternPlayer_.setPatternIndex(0);

    const bool loadedEntities = entityDatabase_.loadFromFile(config_.contentPackPath)
        || entityDatabase_.loadFromFile("data/entities.json");
    if (!loadedEntities) {
        entityDatabase_.loadFallbackDefaults();
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
    playerHealth_ = 100.0F;
    prevTotalCollisions_ = 0;
    prevHealthRecoveryAccum_ = 0.0F;
    replayContentVersion_ = buildContentVersionTag(config_.contentPackPath);
    replayPlaybackMode_ = !config_.replayPlaybackPath.empty();
    if (replayPlaybackMode_) {
        if (!replayPlayer_.load(config_.replayPlaybackPath) || !replayPlayer_.validFor(config_.simulationSeed, replayContentVersion_)) {
            logError("Replay load/validation failed: " + config_.replayPlaybackPath);
            return 1;
        }
    }
    if (!config_.replayRecordPath.empty()) {
        replayRecorder_.begin(config_.simulationSeed, replayContentVersion_);
    }

    (void)traitSystem_.rollChoices();

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
            SDL_WINDOW_RESIZABLE
        );

        if (!window_) {
            logError("SDL_CreateWindow failed: " + std::string(SDL_GetError()));
            return 1;
        }

        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer_) {
            logError("SDL_CreateRenderer failed: " + std::string(SDL_GetError()));
            return 1;
        }

        textures_ = std::make_unique<TextureStore>(renderer_);
        if (!textures_->loadTexture("projectile", "assets/sprites/sample.bmp")) {
            textures_->createSolidTexture("projectile", 8, 8, Color {250, 200, 120, 255});
        }

        toolSuite_.initialize(window_, renderer_);

        debugText_.init(renderer_);
        debugText_.registerTexture(*textures_, "debug_font");

        int winW = 0;
        int winH = 0;
        SDL_GetWindowSize(window_, &winW, &winH);
        camera_.setViewport(winW, winH);
        camera_.setCenter({0.0F, 0.0F});
        spriteBatch_.reserve(config_.projectileCapacity + 2048);
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
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) running_ = false;
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    camera_.setViewport(event.window.data1, event.window.data2);
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
                if (!archetypeSelectionOpen_ && event.type == SDL_KEYDOWN && event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_3 && traitSystem_.hasPendingChoices()) {
                    const std::size_t idx = static_cast<std::size_t>(event.key.keysym.sym - SDLK_1);
                    traitSystem_.choose(idx);
                } else if (!archetypeSelectionOpen_ && event.type == SDL_KEYDOWN && event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_9) {
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
            }

            const float cameraSpeed = 220.0F * static_cast<float>(frameDelta) / camera_.zoom();
            if (keyboard[SDL_SCANCODE_A] || keyboard[SDL_SCANCODE_LEFT]) camera_.pan({-cameraSpeed, 0.0F});
            if (keyboard[SDL_SCANCODE_D] || keyboard[SDL_SCANCODE_RIGHT]) camera_.pan({cameraSpeed, 0.0F});
            if (keyboard[SDL_SCANCODE_W] || keyboard[SDL_SCANCODE_UP]) camera_.pan({0.0F, -cameraSpeed});
            if (keyboard[SDL_SCANCODE_S] || keyboard[SDL_SCANCODE_DOWN]) camera_.pan({0.0F, cameraSpeed});
            if (keyboard[SDL_SCANCODE_Q]) camera_.addZoom(-0.6F * static_cast<float>(frameDelta));
            if (keyboard[SDL_SCANCODE_E]) camera_.addZoom(0.6F * static_cast<float>(frameDelta));
        }

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

    return 0;
}

void Runtime::simTick(const double dt) {
    frameAllocator_.reset();
    auto* scratch = static_cast<std::uint64_t*>(frameAllocator_.allocate(sizeof(std::uint64_t), alignof(std::uint64_t)));
    *scratch = tickIndex_;

    aimTarget_.x = 180.0F * std::cos(static_cast<float>(simClock_) * 0.9F);
    aimTarget_.y = 120.0F * std::sin(static_cast<float>(simClock_) * 1.3F);

    std::uint32_t inputMask = currentInputMask_;
    if (replayPlaybackMode_) {
        inputMask = replayPlayer_.inputForTick(tickIndex_);
    }
    const float moveStep = static_cast<float>(dt) * 120.0F;
    if ((inputMask & InputMoveLeft) != 0U) playerPos_.x -= moveStep;
    if ((inputMask & InputMoveRight) != 0U) playerPos_.x += moveStep;
    if ((inputMask & InputMoveUp) != 0U) playerPos_.y -= moveStep;
    if ((inputMask & InputMoveDown) != 0U) playerPos_.y += moveStep;
    playerPos_.x = std::clamp(playerPos_.x, -320.0F, 320.0F);
    playerPos_.y = std::clamp(playerPos_.y, -180.0F, 180.0F);

    if (!config_.stress10k) {
        const ArchetypeDefinition& archetype = archetypeSystem_.selected();
        const MetaBonuses& mb = metaProgression_.bonuses();
        const auto rareChanceMultiplier = 1.0F + (archetype.stats.rareChance + mb.rarityBonus) * 0.06F;
        traitSystem_.setRareChanceMultiplier(rareChanceMultiplier);

        if (tickIndex_ > 0 && tickIndex_ % 300 == 0 && !traitSystem_.hasPendingChoices()) {
            (void)traitSystem_.rollChoices();
        }
        if (config_.headless && traitSystem_.hasPendingChoices()) {
            traitSystem_.choose(0);
        }

        const TraitModifiers& tm = traitSystem_.modifiers();
        playerRadius_ = 10.0F + (archetype.stats.defense + mb.defenseBonus) * 0.45F + tm.playerRadiusAdd;
        const float fireRateScale = 0.80F + (archetype.stats.fireRate + mb.fireRateBonus) * 0.05F;
        patternPlayer_.setRuntimeModifiers(tm.patternCooldownScale / fireRateScale, tm.patternExtraBullets, tm.patternJitterAddDeg);

        patternPlayer_.update(static_cast<float>(dt), {0.0F, 0.0F}, aimTarget_, [this, &tm, &archetype, &mb](const ProjectileSpawn& spawn) {
            ProjectileSpawn mod = spawn;
            const float powerScale = 0.85F + (archetype.stats.power + mb.powerBonus) * 0.03F;
            mod.vel.x *= tm.projectileSpeedMul * powerScale;
            mod.vel.y *= tm.projectileSpeedMul * powerScale;
            mod.radius = std::max(0.5F, spawn.radius + tm.projectileRadiusAdd);
            projectiles_.spawn(mod);
        });

        EntityRuntimeModifiers em;
        em.enemyFireRateScale = tm.enemyFireRateScale;
        em.enemyProjectileSpeedScale = tm.enemyProjectileSpeedScale;
        em.harvestYieldMultiplier = tm.playerHarvestMultiplier * (0.70F + archetype.stats.resourceGain * 0.06F);

        const ZoneDefinition* zone = runStructure_.currentZone();
        if (zone) {
            switch (zone->type) {
                case ZoneType::Combat:
                    break;
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

        entitySystem_.update(static_cast<float>(dt), projectiles_, playerPos_, em);
    }

    projectiles_.update(static_cast<float>(dt), playerPos_, playerRadius_);

    const ProjectileStats& ps = projectiles_.stats();
    const EntityStats& es = entitySystem_.stats();
    const std::uint32_t collisionDelta = ps.totalCollisions - prevTotalCollisions_;
    prevTotalCollisions_ = ps.totalCollisions;
    const float healDelta = std::max(0.0F, es.healthRecoveryAccum - prevHealthRecoveryAccum_);
    prevHealthRecoveryAccum_ = es.healthRecoveryAccum;
    playerHealth_ = std::clamp(playerHealth_ - static_cast<float>(collisionDelta) * 4.0F + healDelta * 0.35F, 0.0F, 100.0F);

    runStructure_.update(static_cast<float>(dt), es.defeatedBosses, playerHealth_ > 0.0F);

    const std::uint64_t stateHash = computeReplayStateHash(
        tickIndex_,
        ps.activeCount,
        es.aliveTotal,
        ps.totalCollisions,
        es.upgradeCurrency
    );
    if (!config_.replayRecordPath.empty()) {
        replayRecorder_.recordTick(inputMask, stateHash);
    }
    if (replayPlaybackMode_ && !replayPlayer_.verifyTick(tickIndex_, stateHash)) {
        logError("Replay verification mismatch at tick " + std::to_string(tickIndex_));
        running_ = false;
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
        os << "Sim tick=" << tickIndex_
           << " activeProjectiles=" << projectiles_.stats().activeCount
           << " collisionsTotal=" << projectiles_.stats().totalCollisions;
        logInfo(os.str());
    }
}

void Runtime::buildSceneOverlay(const double frameDelta) {
    camera_.update(static_cast<float>(frameDelta));

    projectiles_.render(spriteBatch_, "projectile");
    projectiles_.debugDraw(debugDraw_, showHitboxes_, showGrid_);
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
            << "\nTraits " << traitSystem_.activeTraits().size()
            << "  MetaPwr+ " << mb.powerBonus
            << "  MetaFR+ " << mb.fireRateBonus
            << "  TraitSpd " << tm.projectileSpeedMul
            << "  TraitCD " << tm.patternCooldownScale
            << "\nArchetype " << archetype.name
            << "  Weapon " << archetype.primaryWeapon
            << "  Active " << archetype.activeAbility
            << "\nPattern " << (activePattern ? activePattern->name : std::string("none"))
            << "  Layers " << (activePattern ? activePattern->layers.size() : 0)
            << "  SeqStep " << patternPlayer_.activeSequenceStep()
            << "  ActiveLayers " << patternPlayer_.activeLayerCountForStep()
            << "  Speed " << patternPlayer_.playbackSpeed()
            << "  Paused " << (patternPlayer_.paused() ? "yes" : "no")
            << "\n[F1] archetypes  [F2-F9] buy meta node  [1-3] choose trait  [1-9/TAB] pattern  [SPACE] pause  [R] reset  [,/.] speed  [H] hitboxes  [G] grid";

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

    debugText_.drawText(spriteBatch_, overlay.str(), Vec2 {-620.0F, -330.0F}, 0.85F, Color {255, 255, 255, 255});
}

void Runtime::renderFrame(const double frameDelta) {
    constexpr std::array<Uint8, 4> clearColor {20, 28, 40, 255};
    SDL_SetRenderDrawColor(renderer_, clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    SDL_RenderClear(renderer_);

    spriteBatch_.begin(camera_);
    buildSceneOverlay(frameDelta);
    spriteBatch_.flush(renderer_, *textures_);
    debugDraw_.flush(renderer_, camera_);

    ToolRuntimeSnapshot snapshot;
    snapshot.tick = tickIndex_;
    snapshot.fps = frameDelta > 0.0 ? static_cast<int>(1.0 / frameDelta) : 0;
    snapshot.frameMs = static_cast<float>(frameDelta * 1000.0);
    snapshot.projectileCount = projectiles_.stats().activeCount;
    snapshot.entityCount = entitySystem_.stats().aliveTotal;
    snapshot.collisionsTotal = projectiles_.stats().totalCollisions;
    toolSuite_.beginFrame();
    toolSuite_.drawControlCenter(snapshot);
    toolSuite_.endFrame();

    SDL_RenderPresent(renderer_);
}

} // namespace engine
