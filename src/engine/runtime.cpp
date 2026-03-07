#include <engine/runtime.h>

#include <engine/logging.h>
#include <engine/persistence.h>
#include <engine/public/plugins.h>
#include <engine/standards.h>
#include <engine/timing.h>

#include <algorithm>
#include <chrono>

namespace engine {

Runtime::Runtime(EngineConfig config)
    : config_(std::move(config)),
      session_(config_) {}

Runtime::~Runtime() {
    renderPipeline_.shutdown(session_.debugTools_.toolSuite);
    audio_.shutdown();
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void Runtime::dispatchAudioEvents() {
    if (!audio_.available()) return;
    const Vec2 listener = session_.playerPos();
    for (const AudioEvent& event : session_.consumeAudioEvents()) {
        switch (event.type) {
        case AudioEventType::Hit:
            audio_.playSoundPositional(hitSoundId_, event.position, listener);
            break;
        case AudioEventType::Graze:
            audio_.playSound(grazeSoundId_, 0.85F);
            break;
        case AudioEventType::BossPhaseTransition:
            audio_.playSound(phaseWarnSoundId_, 1.0F);
            break;
        case AudioEventType::DefensiveSpecialActivated:
            audio_.playSound(specialSoundId_, 0.95F);
            break;
        }
    }
    audio_.update();
}

void Runtime::simTick(const double dt) {
    const std::uint32_t inputMask = input_.effectiveInputMask(session_.simulation_.tickIndex);
    if (!config_.replayRecordPath.empty()) {
        session_.replayRecorder_.recordTickInput(inputMask);
    }
    session_.updateGameplay(dt, inputMask);
    const std::uint32_t hashPeriod = session_.replayPlaybackMode_ ? session_.replayPlayer_.hashPeriodTicks() : std::max(1U, config_.replayHashPeriodTicks);
    if ((session_.simulation_.tickIndex % hashPeriod) == 0U) {
        ReplayStateSample sample;
        sample.tick = session_.simulation_.tickIndex;
        sample.bulletsHash = session_.projectiles_.debugStateHash();
        sample.entitiesHash = static_cast<std::uint64_t>(session_.entitySystem_.stats().aliveTotal);
        sample.runStateHash = static_cast<std::uint64_t>(session_.runStructure_.stageIndex());
        sample.totalHash = computeReplayStateHash(sample.tick, sample.bulletsHash, sample.entitiesHash, sample.runStateHash);
        if (!config_.replayRecordPath.empty()) session_.replayRecorder_.recordStateSample(sample);
        if (session_.replayPlaybackMode_) {
            ReplayMismatch mismatch;
            if (!session_.replayPlayer_.verifyStateSample(sample, &mismatch)) {
                logError("Replay verification mismatch at tick " + std::to_string(mismatch.tick));
                session_.replayVerificationFailed_ = true;
                running_ = false;
            }
        }
    }
    if (session_.runStructure_.state() == RunState::Completed || session_.runStructure_.state() == RunState::Failed) {
        running_ = false;
    }
}

int Runtime::run() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
        logError("SDL_Init failed: " + std::string(SDL_GetError()));
        return 1;
    }

    UserSettings userSettings;
    const PersistenceLoadResult settingsLoad = loadUserSettingsFromFile("user_settings.json", userSettings);
    if (settingsLoad.ok) {
        config_.audioMasterVolume = userSettings.audioMasterVolume;
        config_.audioMusicVolume = userSettings.audioMusicVolume;
        config_.audioSfxVolume = userSettings.audioSfxVolume;
        config_.enableModernRenderer = userSettings.enableModernRenderer;
        if (!userSettings.difficultyProfile.empty()) config_.difficultyProfile = userSettings.difficultyProfile;
    } else if (settingsLoad.usedFallback) {
        logWarn("User settings fallback applied: " + settingsLoad.error);
    }

    SaveProfilesFile saveProfiles;
    const PersistenceLoadResult profilesLoad = loadSaveProfilesFromFile("profiles.json", saveProfiles);
    if (!profilesLoad.ok && profilesLoad.usedFallback) {
        logWarn("Profile fallback applied: " + profilesLoad.error);
    }

    (void)audio_.initialize();
    hitSoundId_ = audio_.loadSound("data/audio/hit.wav");
    grazeSoundId_ = audio_.loadSound("data/audio/graze.wav");
    phaseWarnSoundId_ = audio_.loadSound("data/audio/phase_warn.wav");
    specialSoundId_ = audio_.loadSound("data/audio/special.wav");

    session_.projectiles_.initialize(config_.projectileCapacity, 420.0F, 32, 18);
    session_.cpuMassBullets_.initialize(std::max<std::uint32_t>(config_.projectileCapacity, 500000U), 420.0F);

    std::string packSearchPath = config_.contentPackPath;
    for (const auto* plugin : engine::public_api::contentPackPlugins()) {
        for (const std::string& extraPath : plugin->contentPackPaths()) {
            if (!extraPath.empty()) {
                if (!packSearchPath.empty()) packSearchPath += ";";
                packSearchPath += extraPath;
            }
        }
    }

    const bool loadedPack = session_.patternBank_.loadFromFile(packSearchPath)
        || session_.patternBank_.loadFromFile("assets/patterns/sandbox_patterns.json");
    if (!loadedPack) session_.patternBank_.loadFallbackDefaults();
    session_.patternPlayer_.setBank(&session_.patternBank_);
    session_.patternPlayer_.setRunSeed(config_.simulationSeed);
    session_.patternPlayer_.setPatternIndex(0);
    session_.graphVm_.reset(session_.graphVmState_, config_.simulationSeed);

    const bool loadedEntities = session_.entityDatabase_.loadFromFile(packSearchPath)
        || session_.entityDatabase_.loadFromFile("data/entities.json");
    if (!loadedEntities) session_.entityDatabase_.loadFallbackDefaults();
    session_.entitySystem_.setTemplates(&session_.entityDatabase_.templates());
    session_.entitySystem_.setPatternBank(&session_.patternBank_);
    session_.entitySystem_.setRunSeed(config_.simulationSeed ^ 0xE7717U);
    session_.entitySystem_.reset();

    session_.traitSystem_.initialize(config_.simulationSeed);
    session_.archetypeSystem_.initializeDefaults();
    session_.metaProgression_.initializeDefaults();
    if (!saveProfiles.profiles.empty()) {
        const auto it = std::find_if(saveProfiles.profiles.begin(), saveProfiles.profiles.end(), [this, &saveProfiles](const SaveProfile& p) {
            if (saveProfiles.activeProfileId.empty()) return true;
            return p.id == saveProfiles.activeProfileId;
        });
        if (it != saveProfiles.profiles.end()) {
            session_.metaProgression_.applyProgressSnapshot(it->progression);
        }
    } else {
        (void)session_.metaProgression_.loadFromFile("meta_progression.json");
    }
    session_.archetypeSystem_.setUnlockTier(session_.metaProgression_.bonuses().archetypeUnlockTier);
    session_.runStructure_.initializeDefaults();
    session_.difficultyModel_.initializeDefaults();
    (void)session_.difficultyModel_.loadProfilesFromFile("data/difficulty_profiles.json");
    session_.difficultyModel_.setProfile(difficultyProfileFromString(config_.difficultyProfile));

    std::string paletteError;
    if (session_.bulletPaletteRegistry_.loadFromJsonFile("data/palettes/palette_fx_templates.json", &paletteError)) {
        session_.bulletPaletteTable_.buildFromRegistry(session_.bulletPaletteRegistry_);
        session_.entityDatabase_.resolveProjectilePaletteIndices(session_.bulletPaletteRegistry_);
    } else {
        logWarn("Bullet palette template load failed: " + paletteError);
    }

    DefensiveSpecialConfig defensiveCfg;
    defensiveCfg.durationSeconds = config_.defensiveDurationSeconds;
    defensiveCfg.cooldownPerChargeSeconds = config_.defensiveCooldownSeconds;
    session_.defensiveSpecial_.initialize(defensiveCfg, presetFromLabel(config_.defensiveSpecialPreset.c_str()));

    session_.replayContentVersion_ = buildContentVersionTag(config_.contentPackPath);
    session_.replayContentHash_ = buildContentHashTag(config_.contentPackPath);
    session_.replayPlaybackMode_ = !config_.replayPlaybackPath.empty();
    if (session_.replayPlaybackMode_) {
        if (!session_.replayPlayer_.load(config_.replayPlaybackPath) || !session_.replayPlayer_.validFor(config_.simulationSeed, session_.replayContentVersion_, session_.replayContentHash_)) {
            return 1;
        }
        input_.setReplayPlayback(&session_.replayPlayer_);
    }
    if (!config_.replayRecordPath.empty()) {
        session_.replayRecorder_.begin(config_.simulationSeed, session_.replayContentVersion_, session_.replayContentHash_, config_.replayHashPeriodTicks);
    }

    (void)session_.traitSystem_.rollChoices();

    if (!config_.headless) {
        window_ = SDL_CreateWindow(config_.windowTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config_.windowWidth, config_.windowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL);
        if (!window_) return 1;
        if (!renderPipeline_.initialize(window_, config_, session_.debugTools_.toolSuite)) return 1;
        if (renderPipeline_.renderer() && renderPipeline_.textures()) {
            session_.projectiles_.configurePaletteAnimations(renderPipeline_.renderer(), *renderPipeline_.textures(), session_.bulletPaletteRegistry_);
        }
        renderPipeline_.generateBulletSprites(session_.bulletPaletteRegistry_, session_.bulletPaletteTable_);
        renderPipeline_.generatePatternSignatures(session_.patternBank_, config_.simulationSeed);
        if (config_.rendererSmokeTest) {
            SDL_SetRenderDrawColor(renderPipeline_.renderer(), 12, 12, 16, 255);
            SDL_RenderClear(renderPipeline_.renderer());
            SDL_RenderPresent(renderPipeline_.renderer());
            return 0;
        }
    }

    input_.setUpgradeNavCallback([this](const UpgradeNavAction a) { session_.onUpgradeNavigation(a); });

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
                session_.debugTools_.toolSuite.processEvent(event);
                input_.processEvent(event);
                if (event.type == SDL_QUIT) running_ = false;
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_v && event.key.repeat == 0) {
                    UpgradeDebugOptions& upgradeDebug = session_.debugTools_.toolSuite.upgradeDebugOptionsMutable();
                    upgradeDebug.showDangerField = !upgradeDebug.showDangerField;
                    session_.setDangerFieldEnabled(upgradeDebug.showDangerField);
                }
                if (event.type == SDL_WINDOWEVENT && (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED)) {
                    renderPipeline_.refreshDisplayMetrics(window_);
                }
            }
            input_.pollInput(false);
        }

        session_.profiler_.beginFrame();
        const StepResult stepResult = consumeFixedSteps(accumulator, config_.fixedDeltaSeconds, config_.maxFrameSteps);
        for (std::uint32_t i = 0; i < stepResult.steps; ++i) {
            simTick(config_.fixedDeltaSeconds);
            dispatchAudioEvents();
        }
        accumulator = stepResult.remainingAccumulator;

        if (config_.targetTicks > 0 && static_cast<int>(session_.simulation_.tickIndex) >= config_.targetTicks) running_ = false;
        if (!config_.headless) {
            renderPipeline_.renderFrame({session_}, frameDelta, session_.debugTools_.toolSuite);
            session_.drawUpgradeSelectionUi(frameDelta);
        }
    }

    if (!config_.replayRecordPath.empty()) (void)session_.replayRecorder_.save(config_.replayRecordPath);

    UserSettings toSaveSettings;
    toSaveSettings.audioMasterVolume = config_.audioMasterVolume;
    toSaveSettings.audioMusicVolume = config_.audioMusicVolume;
    toSaveSettings.audioSfxVolume = config_.audioSfxVolume;
    toSaveSettings.enableModernRenderer = config_.enableModernRenderer;
    toSaveSettings.difficultyProfile = config_.difficultyProfile;
    std::string settingsSaveError;
    if (!saveUserSettingsToFile("user_settings.json", toSaveSettings, &settingsSaveError)) {
        logWarn("Failed to persist user settings: " + settingsSaveError);
    }

    if (saveProfiles.profiles.empty()) {
        SaveProfile profile;
        profile.id = "profile-1";
        profile.displayName = "Profile 1";
        saveProfiles.activeProfileId = profile.id;
        saveProfiles.profiles.push_back(profile);
    }
    for (auto& profile : saveProfiles.profiles) {
        if (profile.id == saveProfiles.activeProfileId) {
            profile.progression = session_.metaProgression_.makeProgressSnapshot();
            profile.lastPlayedUtc = "runtime-local";
        }
    }

    std::string profilesSaveError;
    if (!saveSaveProfilesToFile("profiles.json", saveProfiles, &profilesSaveError)) {
        logWarn("Failed to persist save profiles: " + profilesSaveError);
    }

    return session_.replayVerificationFailed_ ? 2 : 0;
}

} // namespace engine
