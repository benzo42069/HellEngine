#include <engine/gameplay_session.h>

#include <engine/input_system.h>
#include <engine/logging.h>
#include <engine/public/plugins.h>
#include <engine/standards.h>

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

namespace engine {

namespace {

std::uint8_t paletteIndexFromName(const PaletteFxTemplateRegistry& registry, const std::string& paletteName, const std::uint8_t fallbackIndex) {
    if (paletteName.empty()) return fallbackIndex;
    const auto& palettes = registry.database().palettes;
    const std::size_t maxRows = static_cast<std::size_t>(BulletPaletteTable::kMaxPalettes - 1);
    const std::size_t count = std::min(palettes.size(), maxRows);
    for (std::size_t i = 0; i < count; ++i) {
        if (palettes[i].name == paletteName) return static_cast<std::uint8_t>(i + 1);
    }
    return fallbackIndex;
}

} // namespace

GameplaySession::GameplaySession(EngineConfig& config)
    : simulation_(config.simulationSeed),
      config_(config) {
    presentation_.particleFx.initialize(4096);
    initializeContentWatcher();
}

void GameplaySession::initializeContent(PatternBank& patternBank, PatternPlayer& patternPlayer) {
    patternBank_ = patternBank;
    patternPlayer_ = patternPlayer;
}

void GameplaySession::onUpgradeNavigation(const UpgradeNavAction action) {
    ProgressionSubsystem::UpgradeSelectionContext ctx;
    ctx.hasPendingChoices = traitSystem_.hasPendingChoices();
    ctx.choiceCount = TraitSystem::choiceCount;
    ctx.choose = [this](const std::size_t index) { return traitSystem_.choose(index); };
    ctx.rerollChoices = [this]() { return traitSystem_.rerollChoices(); };
    progressionRuntime_.onUpgradeNavigation(progression_, ctx, action);
}

void GameplaySession::updateGameplay(const double dt, const std::uint32_t inputMask) {
    using Clock = std::chrono::steady_clock;
    const auto simStart = Clock::now();
    debugTools_.gpuUpdateMsFrame = 0.0F;

    simulation_.frameAllocator.reset();
    auto* scratch = static_cast<std::uint64_t*>(simulation_.frameAllocator.allocate(sizeof(std::uint64_t), alignof(std::uint64_t)));
    *scratch = simulation_.tickIndex;

    projectiles_.beginTick();
    presentation_.cameraShakeEvents.clear();
    presentation_.pendingAudioEvents.clear();
    traitSystem_.onTick(simulation_.tickIndex);

    SessionOrchestrationSubsystem::HotReloadCallbacks reloadCallbacks;
    reloadCallbacks.reloadPatterns = [this](const std::string& path) { reloadPatterns(path); };
    reloadCallbacks.reloadEntities = [this](const std::string& path) { reloadEntities(path); };
    reloadCallbacks.reloadTraits = [this](const std::string& path) { reloadTraits(path); };
    reloadCallbacks.reloadDifficulty = [this](const std::string& path) { reloadDifficulty(path); };
    reloadCallbacks.reloadPalettes = [this](const std::string& path) { reloadPalettes(path); };
    sessionOrchestration_.pollContentHotReloads(contentWatcher_, simulation_.tickIndex, kHotReloadPollTicks, nextHotReloadPollTick_, reloadCallbacks);

    playerCombat_.updateAimTarget(playerState_, simulation_.simClock);

    if (playerCombat_.tryActivateDefensiveSpecial(inputMask, defensiveSpecial_)) {
        presentationRuntime_.emitDefensiveSpecialActivation(presentation_.cameraShakeEvents, presentation_.pendingAudioEvents, playerState_.playerPos);
    }

    const TraitModifiers& traitModsForSpecial = traitSystem_.modifiers();
    defensiveSpecial_.update(static_cast<float>(dt), traitModsForSpecial);
    const TimeDilationScales dilation = defensiveSpecial_.dilationScales();

    const ZoneDefinition* zoneBeforeUpdate = runStructure_.currentZone();

    const std::uint32_t grazePoints = playerCombat_.collectGrazePoints(playerState_, projectiles_, defensiveSpecial_, simulation_.tickIndex);
    if (grazePoints > 0) {
        defensiveSpecial_.addGrazePoints(grazePoints);
        presentationRuntime_.emitGrazeFeedback(presentation_.cameraShakeEvents, presentation_.pendingAudioEvents, playerState_.playerPos);
    }

    playerCombat_.applyMovement(playerState_, dt, inputMask);

    const auto patternStart = Clock::now();
    if (!config_.stress10k) {
        const ArchetypeDefinition& archetype = archetypeSystem_.selected();
        const MetaBonuses& mb = metaProgression_.bonuses();
        const auto rareChanceMultiplier = 1.0F + (archetype.stats.rareChance + mb.rarityBonus) * 0.06F;
        traitSystem_.setRareChanceMultiplier(rareChanceMultiplier);
        sessionOrchestration_.updateUpgradeCadence(
            simulation_.tickIndex,
            traitSystem_.hasPendingChoices(),
            progression_.upgradeScreenOpen,
            [this]() -> bool {
                (void)traitSystem_.rollChoices();
                return true;
            }
        );
        const UpgradeDebugOptions& upgradeDebug = debugTools_.toolSuite.upgradeDebugOptions();
        sessionOrchestration_.applyUpgradeDebugOptions(
            upgradeDebug,
            traitSystem_.hasPendingChoices(),
            debugTools_.perfHudOpen,
            presentation_.dangerFieldEnabled,
            progression_.upgradeScreenOpen,
            [this]() -> bool {
                (void)traitSystem_.rollChoices();
                return true;
            },
            [this](const TraitRarity rarity) { traitSystem_.forcePendingRarity(rarity); }
        );

        const TraitModifiers& tm = traitSystem_.modifiers();
        if (zoneBeforeUpdate) difficultyModel_.setStageZone(runStructure_.stageIndex(), zoneBeforeUpdate->type);
        const DifficultyScalars diffScalars = difficultyModel_.scalars();
        playerState_.playerRadius = 10.0F + (archetype.stats.defense + mb.defenseBonus) * 0.45F + tm.playerRadiusAdd;
        const float fireRateScale = 0.80F + (archetype.stats.fireRate + mb.fireRateBonus) * 0.05F;
        patternPlayer_.setRuntimeModifiers(tm.patternCooldownScale / fireRateScale, tm.patternExtraBullets, tm.patternJitterAddDeg);

        const std::uint8_t fallbackPaletteIndex = static_cast<std::uint8_t>(std::min<std::size_t>(BulletPaletteTable::kMaxPalettes - 1U, archetypeSystem_.selectedIndex() + 1U));
        const std::uint8_t playerPaletteIndex = paletteIndexFromName(bulletPaletteRegistry_, archetype.projectilePaletteName, fallbackPaletteIndex);

        auto emitWithRuntimeMods = [this, &tm, &archetype, &mb, &diffScalars, playerPaletteIndex](const ProjectileSpawn& spawn) {
            ProjectileSpawn mod = spawn;
            mod.allegiance = ProjectileAllegiance::Player;
            mod.paletteIndex = playerPaletteIndex;
            const float powerScale = 0.85F + (archetype.stats.power + mb.powerBonus) * 0.03F;
            mod.vel.x *= tm.projectileSpeedMul * powerScale * diffScalars.patternSpeed * tm.offensiveSpecialPowerMul * defensiveSpecial_.playerDamageMultiplier();
            mod.vel.y *= tm.projectileSpeedMul * powerScale * diffScalars.patternSpeed * tm.offensiveSpecialPowerMul * defensiveSpecial_.playerDamageMultiplier();
            mod.radius = std::max(0.5F, spawn.radius + tm.projectileRadiusAdd);
            projectiles_.spawn(mod);
        };
        if (useCompiledPatternGraph_ && !patternBank_.compiledGraphs().empty()) {
            graphVmState_.difficultyScalar = diffScalars.patternSpeed;
            graphVm_.execute(patternBank_.compiledGraphs().front(), graphVmState_, static_cast<float>(dt), {0.0F, 0.0F}, playerState_.aimTarget,
                [this, &tm, &archetype, &mb, &diffScalars, playerPaletteIndex](const ProjectileSpawn& spawn) {
                    ProjectileSpawn mod = spawn;
                    mod.allegiance = ProjectileAllegiance::Player;
                    mod.paletteIndex = playerPaletteIndex;
                    const float powerScale = 0.85F + (archetype.stats.power + mb.powerBonus) * 0.03F;
                    mod.vel.x *= tm.projectileSpeedMul * powerScale * diffScalars.patternSpeed * tm.offensiveSpecialPowerMul;
                    mod.vel.y *= tm.projectileSpeedMul * powerScale * diffScalars.patternSpeed * tm.offensiveSpecialPowerMul;
                    mod.radius = std::max(0.5F, spawn.radius + tm.projectileRadiusAdd);
                    projectiles_.spawn(mod);
                });
        } else {
            patternPlayer_.update(static_cast<float>(dt), {0.0F, 0.0F}, playerState_.aimTarget, emitWithRuntimeMods);
        }

        EntityRuntimeModifiers em;
        em.enemyFireRateScale = tm.enemyFireRateScale * diffScalars.spawnRate;
        em.enemyProjectileSpeedScale = tm.enemyProjectileSpeedScale * diffScalars.patternSpeed;
        em.harvestYieldMultiplier = tm.playerHarvestMultiplier * (0.70F + archetype.stats.resourceGain * 0.06F);
        em.enemyHealthScale = diffScalars.enemyHp;

        if (zoneBeforeUpdate) {
            switch (zoneBeforeUpdate->type) {
                case ZoneType::Combat: break;
                case ZoneType::Elite: em.enemyFireRateScale *= 1.25F; em.enemyProjectileSpeedScale *= 1.20F; break;
                case ZoneType::Event: em.enemyFireRateScale *= 0.80F; em.harvestYieldMultiplier *= 1.50F; break;
                case ZoneType::Boss: em.enemyFireRateScale *= 1.35F; em.enemyProjectileSpeedScale *= 1.25F; break;
            }
        }
        profiler_.addZoneTime(PerfZone::Patterns, std::chrono::duration<double, std::milli>(Clock::now() - patternStart).count());
        const auto bulletStart = Clock::now();
        entitySystem_.update(static_cast<float>(dt) * dilation.enemyMovement, projectiles_, playerState_.playerPos, em);
        encounterRuntime_.processRuntimeEvents(entitySystem_, encounter_, presentation_.cameraShakeEvents);
        profiler_.addZoneTime(PerfZone::Bullets, std::chrono::duration<double, std::milli>(Clock::now() - bulletStart).count());
    }

    if (bulletSimMode_ == BulletSimulationMode::CpuCollisionDeterministic) {
        projectiles_.updateMotion(static_cast<float>(dt), dilation.enemyProjectiles, dilation.playerProjectiles);
        encounterRuntime_.emitDespawnParticles(projectiles_, bulletPaletteTable_, presentation_);
        encounterRuntime_.resolveCpuDeterministicCollisions(projectiles_, entitySystem_, playerState_, encounter_, presentation_);
        encounterRuntime_.emitDespawnParticles(projectiles_, bulletPaletteTable_, presentation_);
    }

    presentation_.particleFx.update(static_cast<float>(dt));
    runStructure_.update(static_cast<float>(dt), entitySystem_.stats().defeatedBosses, playerState_.playerHealth > 0.0F);
    const ZoneDefinition* zoneAfterUpdate = runStructure_.currentZone();
    encounterRuntime_.emitZoneTransitionFeedback(zoneBeforeUpdate, zoneAfterUpdate, playerState_.playerPos, presentation_.cameraShakeEvents, presentation_.pendingAudioEvents);
    encounterRuntime_.emitAmbientZoneFeedback(zoneAfterUpdate, presentation_.cameraShakeEvents);
    simulation_.simClock += dt;
    encounter_.encounterClockSeconds += static_cast<float>(dt);
    ++simulation_.tickIndex;
    profiler_.addZoneTime(PerfZone::Simulation, std::chrono::duration<double, std::milli>(Clock::now() - simStart).count());
}

void GameplaySession::initializeContentWatcher() {
    contentWatcher_.addWatchPath("data/patterns.json", ContentType::Patterns);
    contentWatcher_.addWatchPath("assets/patterns/sandbox_patterns.json", ContentType::Patterns);
    contentWatcher_.addWatchPath("data/entities.json", ContentType::Entities);
    contentWatcher_.addWatchPath("data/traits.json", ContentType::Traits);
    contentWatcher_.addWatchPath("data/difficulty_profiles.json", ContentType::Difficulty);
    contentWatcher_.addWatchPath("data/palettes/palette_fx_templates.json", ContentType::Palettes);
}

void GameplaySession::reloadPatterns(const std::string& path) {
    PatternBank nextBank;
    if (!nextBank.loadFromFile(path)) {
        debugTools_.hotReloadErrorMessage = "Pattern reload failed: " + path;
        logError(debugTools_.hotReloadErrorMessage);
        return;
    }

    const std::size_t prevPatternIndex = patternPlayer_.patternIndex();
    patternBank_ = std::move(nextBank);
    patternPlayer_.setBank(&patternBank_);
    const std::size_t availablePatterns = patternBank_.patterns().size();
    patternPlayer_.setPatternIndex(availablePatterns == 0 ? 0 : std::min(prevPatternIndex, availablePatterns - 1));
    debugTools_.hotReloadErrorMessage.clear();
    logInfo("Pattern hot reload succeeded: " + path);
}

void GameplaySession::reloadEntities(const std::string& path) {
    EntityDatabase nextDatabase;
    if (!nextDatabase.loadFromFile(path)) {
        debugTools_.hotReloadErrorMessage = "Entity reload failed: " + path;
        logError(debugTools_.hotReloadErrorMessage);
        return;
    }

    nextDatabase.resolveProjectilePaletteIndices(bulletPaletteRegistry_);
    entityDatabase_ = std::move(nextDatabase);
    entitySystem_.setTemplates(&entityDatabase_.templates());
    debugTools_.hotReloadErrorMessage.clear();
    logInfo("Entity hot reload succeeded: " + path);
}

void GameplaySession::reloadTraits(const std::string& path) {
    TraitSystem nextTraits;
    nextTraits.initialize(config_.simulationSeed);
    if (!nextTraits.loadFromFile(path)) {
        debugTools_.hotReloadErrorMessage = "Trait reload failed: " + path;
        logError(debugTools_.hotReloadErrorMessage);
        return;
    }

    traitSystem_ = std::move(nextTraits);
    debugTools_.hotReloadErrorMessage.clear();
    logInfo("Trait hot reload succeeded: " + path);
}

void GameplaySession::reloadDifficulty(const std::string& path) {
    DifficultyModel nextDifficulty;
    nextDifficulty.initializeDefaults();
    if (!nextDifficulty.loadProfilesFromFile(path)) {
        debugTools_.hotReloadErrorMessage = "Difficulty reload failed: " + path;
        logError(debugTools_.hotReloadErrorMessage);
        return;
    }

    nextDifficulty.setProfile(difficultyModel_.profile());
    difficultyModel_ = std::move(nextDifficulty);
    debugTools_.difficultyProfileLabelCache = difficultyModel_.profileLabel();
    debugTools_.hotReloadErrorMessage.clear();
    logInfo("Difficulty hot reload succeeded: " + path);
}

void GameplaySession::reloadPalettes(const std::string& path) {
    PaletteFxTemplateRegistry nextRegistry;
    std::string error;
    if (!nextRegistry.loadFromJsonFile(path, &error)) {
        debugTools_.hotReloadErrorMessage = "Palette reload failed: " + error;
        logError(debugTools_.hotReloadErrorMessage);
        return;
    }

    BulletPaletteTable nextTable;
    nextTable.buildFromRegistry(nextRegistry);

    bulletPaletteRegistry_ = std::move(nextRegistry);
    bulletPaletteTable_ = std::move(nextTable);
    entityDatabase_.resolveProjectilePaletteIndices(bulletPaletteRegistry_);
    debugTools_.hotReloadErrorMessage.clear();
    logInfo("Palette hot reload succeeded: " + path);
}

std::vector<ShakeParams> GameplaySession::consumeCameraShakeEvents() const {
    std::vector<ShakeParams> out;
    out.swap(presentation_.cameraShakeEvents);
    return out;
}

std::vector<AudioEvent> GameplaySession::consumeAudioEvents() const {
    std::vector<AudioEvent> out;
    out.swap(presentation_.pendingAudioEvents);
    return out;
}


UpgradeViewStats GameplaySession::buildCurrentViewStats() const {
    return buildUpgradeViewStats(archetypeSystem_.selected(), metaProgression_.bonuses(), traitSystem_.modifiers());
}

UpgradeViewStats GameplaySession::buildProjectedViewStats(const Trait& trait) const {
    return projectUpgradeViewStats(buildCurrentViewStats(), trait);
}

bool GameplaySession::hasSynergyWithActive(const Trait& trait) const {
    for (const Trait& active : traitSystem_.activeTraits()) {
        for (std::size_t i = 0; i < trait.synergyTagCount; ++i) {
            for (std::size_t j = 0; j < active.synergyTagCount; ++j) {
                if (!trait.synergyTags[i].empty() && trait.synergyTags[i] == active.synergyTags[j]) return true;
            }
        }
    }
    return false;
}


void GameplaySession::renderDangerFieldOverlay(SDL_Renderer* renderer, const Camera2D& camera, const float opacity) const {
    if (!presentation_.dangerFieldEnabled) return;
    presentation_.dangerField.render(renderer, camera, opacity);
}
void GameplaySession::drawUpgradeSelectionUi(const double frameDelta) {
    if (!progression_.upgradeScreenOpen || !traitSystem_.hasPendingChoices()) return;
    const ImVec2 viewport = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowPos(ImVec2(20.0F, 40.0F), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport.x - 40.0F, viewport.y - 80.0F), ImGuiCond_Always);
    ImGui::Begin("Upgrade Selection", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    ImGui::TextUnformatted("Choose an upgrade");
    ImGui::SameLine();
    ImGui::Text("Rerolls: %d (cooldown: %llut)", traitSystem_.rerollCharges(), static_cast<unsigned long long>(traitSystem_.rerollCooldownRemainingTicks(simulation_.tickIndex)));
    const auto& choices = traitSystem_.pendingChoices();
    for (std::size_t i = 0; i < choices.size(); ++i) {
        UpgradeCardAnimState& anim = cardAnim_[i];
        anim.appearT = std::min(1.0F, anim.appearT + static_cast<float>(frameDelta) * 3.0F);
        anim.hoverT = (i == progression_.focusedUpgradeIndex) ? std::min(1.0F, anim.hoverT + static_cast<float>(frameDelta) * 6.0F) : std::max(0.0F, anim.hoverT - static_cast<float>(frameDelta) * 6.0F);
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
        if (hasSynergyWithActive(t)) ImGui::TextColored(ImVec4(0.25F, 0.9F, 0.45F, 1.0F), "Synergy with current build");
        if (i == progression_.focusedUpgradeIndex) ImGui::TextColored(ImVec4(1.0F, 0.9F, 0.3F, 1.0F), "Focused (Enter / A to select)");
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        if ((i + 1) < choices.size()) ImGui::SameLine();
        ImGui::EndGroup();
        ImGui::PopID();
    }
    const Trait& focused = choices[progression_.focusedUpgradeIndex];
    const UpgradeViewStats current = buildCurrentViewStats();
    const UpgradeViewStats projected = buildProjectedViewStats(focused);
    ImGui::Separator();
    ImGui::Text("Stat Preview");
    ImGui::Text("Power: %.2f -> %.2f", current.power, projected.power);
    ImGui::Text("Fire Rate: %.2f -> %.2f", current.fireRate, projected.fireRate);
    ImGui::Text("Move Speed: %.2f -> %.2f", current.moveSpeed, projected.moveSpeed);
    ImGui::Text("Defense: %.2f -> %.2f", current.defense, projected.defense);
    if (ImGui::Button("Reroll [X]")) (void)traitSystem_.rerollChoices();
    ImGui::SameLine();
    if (ImGui::Button("Select Focused [Enter]")) {
        if (traitSystem_.choose(progression_.focusedUpgradeIndex)) progression_.upgradeScreenOpen = false;
    }
    ImGui::End();
}

} // namespace engine
