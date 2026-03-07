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
}

void GameplaySession::initializeContent(PatternBank& patternBank, PatternPlayer& patternPlayer) {
    patternBank_ = patternBank;
    patternPlayer_ = patternPlayer;
}

void GameplaySession::onUpgradeNavigation(const UpgradeNavAction action) {
    if (action == UpgradeNavAction::ToggleScreen) {
        progression_.upgradeScreenOpen = traitSystem_.hasPendingChoices() ? !progression_.upgradeScreenOpen : progression_.upgradeScreenOpen;
        return;
    }
    if (!progression_.upgradeScreenOpen || !traitSystem_.hasPendingChoices()) return;

    if (action == UpgradeNavAction::MoveLeft) { progression_.focusedUpgradeIndex = (progression_.focusedUpgradeIndex + TraitSystem::choiceCount - 1) % TraitSystem::choiceCount; presentation_.pendingAudioEvents.push_back(AudioEventId::UiClick); }
    if (action == UpgradeNavAction::MoveRight) { progression_.focusedUpgradeIndex = (progression_.focusedUpgradeIndex + 1) % TraitSystem::choiceCount; presentation_.pendingAudioEvents.push_back(AudioEventId::UiClick); }
    if (action == UpgradeNavAction::SelectSlot1) progression_.focusedUpgradeIndex = 0;
    if (action == UpgradeNavAction::SelectSlot2) progression_.focusedUpgradeIndex = 1;
    if (action == UpgradeNavAction::SelectSlot3) progression_.focusedUpgradeIndex = 2;
    if (action == UpgradeNavAction::SelectSlot1 || action == UpgradeNavAction::SelectSlot2 || action == UpgradeNavAction::SelectSlot3 || action == UpgradeNavAction::Confirm) {
        if (traitSystem_.choose(progression_.focusedUpgradeIndex)) progression_.upgradeScreenOpen = false;
        presentation_.pendingAudioEvents.push_back(AudioEventId::UiConfirm);
    }
    if (action == UpgradeNavAction::Reroll) (void)traitSystem_.rerollChoices();
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

    auto emitDespawnParticles = [this]() {
        for (const ProjectileDespawnEvent& event : projectiles_.despawnEvents()) {
            Color impactColor = event.paletteIndex == 0
                ? (event.allegiance == ProjectileAllegiance::Enemy ? Color {255, 220, 120, 220} : Color {120, 220, 255, 220})
                : bulletPaletteTable_.get(event.paletteIndex).impact;
            if (event.paletteIndex != 0 && impactColor.a == 0) {
                impactColor = bulletPaletteTable_.get(event.paletteIndex).core;
            }
            presentation_.particleFx.burst(event.pos, impactColor);
            if (event.explodeShards > 0) {
                const float len = std::sqrt(event.pos.x * event.pos.x + event.pos.y * event.pos.y);
                const Vec2 dir = len > 0.0001F ? Vec2 {event.pos.x / len, event.pos.y / len} : Vec2 {0.0F, -1.0F};
                presentation_.cameraShakeEvents.push_back(ShakeParams {
                    .profile = ShakeProfile::Explosion,
                    .amplitude = 4.0F,
                    .duration = 0.18F,
                    .direction = dir,
                    .frequency = 36.0F,
                    .damping = 11.0F,
                });
            }
        }
    };

    playerState_.aimTarget.x = 180.0F * std::cos(static_cast<float>(simulation_.simClock) * 0.9F);
    playerState_.aimTarget.y = 120.0F * std::sin(static_cast<float>(simulation_.simClock) * 1.3F);

    if ((inputMask & InputDefensiveSpecial) != 0U && defensiveSpecial_.tryActivate()) {
        presentation_.cameraShakeEvents.push_back(ShakeParams {
            .profile = ShakeProfile::SpecialPulse,
            .amplitude = 5.0F,
            .duration = 0.45F,
            .direction = {0.0F, 0.0F},
            .frequency = 12.0F,
            .damping = 3.5F,
        });
        presentation_.pendingAudioEvents.push_back(AudioEventId::Graze);
    }

    const TraitModifiers& traitModsForSpecial = traitSystem_.modifiers();
    defensiveSpecial_.update(static_cast<float>(dt), traitModsForSpecial);
    const TimeDilationScales dilation = defensiveSpecial_.dilationScales();

    const ZoneDefinition* zoneBeforeUpdate = runStructure_.currentZone();

    const std::uint32_t grazePoints = projectiles_.collectGrazePoints(playerState_.playerPos, playerState_.playerRadius, defensiveSpecial_.config().grazeBandInnerPadding, defensiveSpecial_.config().grazeBandOuterPadding, simulation_.tickIndex, defensiveSpecial_.config().grazeCooldownTicks);
    if (grazePoints > 0) {
        defensiveSpecial_.addGrazePoints(grazePoints);
        presentation_.cameraShakeEvents.push_back(ShakeParams {
            .profile = ShakeProfile::GrazeTremor,
            .amplitude = 0.8F,
            .duration = 0.10F,
            .direction = {0.0F, 0.0F},
            .frequency = 110.0F,
            .damping = 22.0F,
        });
    }

    const float moveStep = static_cast<float>(dt) * 120.0F;
    if ((inputMask & InputMoveLeft) != 0U) playerState_.playerPos.x -= moveStep;
    if ((inputMask & InputMoveRight) != 0U) playerState_.playerPos.x += moveStep;
    if ((inputMask & InputMoveUp) != 0U) playerState_.playerPos.y -= moveStep;
    if ((inputMask & InputMoveDown) != 0U) playerState_.playerPos.y += moveStep;
    playerState_.playerPos.x = std::clamp(playerState_.playerPos.x, 0.0F, static_cast<float>(engineStandards().playfieldWidth));
    playerState_.playerPos.y = std::clamp(playerState_.playerPos.y, 0.0F, static_cast<float>(engineStandards().playfieldHeight));

    const auto patternStart = Clock::now();
    if (!config_.stress10k) {
        const ArchetypeDefinition& archetype = archetypeSystem_.selected();
        const MetaBonuses& mb = metaProgression_.bonuses();
        const auto rareChanceMultiplier = 1.0F + (archetype.stats.rareChance + mb.rarityBonus) * 0.06F;
        traitSystem_.setRareChanceMultiplier(rareChanceMultiplier);
        if (simulation_.tickIndex > 0 && simulation_.tickIndex % 300 == 0 && !traitSystem_.hasPendingChoices()) {
            (void)traitSystem_.rollChoices();
            progression_.upgradeScreenOpen = true;
        }
        const UpgradeDebugOptions& upgradeDebug = debugTools_.toolSuite.upgradeDebugOptions();
        debugTools_.perfHudOpen = upgradeDebug.showPerfHud;
        presentation_.dangerFieldEnabled = upgradeDebug.showDangerField;
        if (upgradeDebug.spawnUpgradeScreen && !traitSystem_.hasPendingChoices()) {
            (void)traitSystem_.rollChoices();
            progression_.upgradeScreenOpen = true;
        }
        if (upgradeDebug.forcedRarity >= 0 && traitSystem_.hasPendingChoices()) {
            traitSystem_.forcePendingRarity(static_cast<TraitRarity>(upgradeDebug.forcedRarity));
        }

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
        profiler_.addZoneTime(PerfZone::Bullets, std::chrono::duration<double, std::milli>(Clock::now() - bulletStart).count());
    }

    if (bulletSimMode_ == BulletSimulationMode::CpuDeterministic) {
        projectiles_.updateMotion(static_cast<float>(dt), dilation.enemyProjectiles, dilation.playerProjectiles);
        emitDespawnParticles();
        projectiles_.buildGrid();
        if (presentation_.dangerFieldEnabled) {
            presentation_.dangerField.buildFromGrid(
                projectiles_.gridHead(),
                projectiles_.gridNext(),
                projectiles_.posX(),
                projectiles_.posY(),
                projectiles_.active(),
                projectiles_.gridX(),
                projectiles_.gridY(),
                projectiles_.worldHalfExtent()
            );
        }
        encounter_.collisionTargetCount = 0;
        encounter_.collisionTargets[encounter_.collisionTargetCount++] = CollisionTarget {.pos = playerState_.playerPos, .radius = playerState_.playerRadius, .id = 0U, .team = 0U};
        entitySystem_.appendCollisionTargets(encounter_.collisionTargets, encounter_.collisionTargetCount);
        encounter_.collisionEventCount = 0;
        projectiles_.resolveCollisions(
            std::span<const CollisionTarget>(encounter_.collisionTargets.data(), encounter_.collisionTargetCount),
            encounter_.collisionEvents,
            encounter_.collisionEventCount
        );
        emitDespawnParticles();
        for (std::uint32_t collisionIndex = 0; collisionIndex < encounter_.collisionEventCount; ++collisionIndex) {
            const CollisionEvent& e = encounter_.collisionEvents[collisionIndex];
            if (e.targetId != 0U || e.bulletIndex >= projectiles_.capacity()) continue;
            const Vec2 hitPos {projectiles_.posX()[e.bulletIndex], projectiles_.posY()[e.bulletIndex]};
            const Vec2 dir = {playerState_.playerPos.x - hitPos.x, playerState_.playerPos.y - hitPos.y};
            presentation_.cameraShakeEvents.push_back(ShakeParams {
                .profile = ShakeProfile::Impact,
                .amplitude = 3.5F,
                .duration = 0.16F,
                .direction = dir,
                .frequency = 32.0F,
                .damping = 12.0F,
            });
            presentation_.pendingAudioEvents.push_back(AudioEventId::PlayerDamage);
        }
        bool enemyHit = false;
        for (std::uint32_t i = 0; i < encounter_.collisionEventCount; ++i) {
            if (encounter_.collisionEvents[i].targetId >= 1000U) { enemyHit = true; break; }
        }
        entitySystem_.processCollisionEvents(std::span<const CollisionEvent>(encounter_.collisionEvents.data(), encounter_.collisionEventCount));
        if (encounter_.collisionEventCount > 0) presentation_.pendingAudioEvents.push_back(AudioEventId::Hit);
        if (enemyHit) presentation_.pendingAudioEvents.push_back(AudioEventId::EnemyDeath);
    }

    presentation_.particleFx.update(static_cast<float>(dt));
    runStructure_.update(static_cast<float>(dt), entitySystem_.stats().defeatedBosses, playerState_.playerHealth > 0.0F);
    const ZoneDefinition* zoneAfterUpdate = runStructure_.currentZone();
    if (zoneBeforeUpdate && zoneAfterUpdate && zoneBeforeUpdate->type != zoneAfterUpdate->type && zoneAfterUpdate->type == ZoneType::Boss) {
        presentation_.cameraShakeEvents.push_back(ShakeParams {
            .profile = ShakeProfile::BossRumble,
            .amplitude = 7.5F,
            .duration = 1.2F,
            .direction = {0.0F, 0.0F},
            .frequency = 8.0F,
            .damping = 2.0F,
        });
        presentation_.pendingAudioEvents.push_back(AudioEventId::BossWarning);
    }
    if (zoneAfterUpdate && (zoneAfterUpdate->type == ZoneType::Combat || zoneAfterUpdate->type == ZoneType::Elite || zoneAfterUpdate->type == ZoneType::Boss)) {
        presentation_.cameraShakeEvents.push_back(ShakeParams {
            .profile = ShakeProfile::Ambient,
            .amplitude = 0.18F,
            .duration = 0.0F,
            .direction = {0.0F, 0.0F},
            .frequency = 2.0F,
            .damping = 0.0F,
        });
    }
    simulation_.simClock += dt;
    ++simulation_.tickIndex;
    profiler_.addZoneTime(PerfZone::Simulation, std::chrono::duration<double, std::milli>(Clock::now() - simStart).count());
}

std::vector<ShakeParams> GameplaySession::consumeCameraShakeEvents() const {
    std::vector<ShakeParams> out;
    out.swap(presentation_.cameraShakeEvents);
    return out;
}

std::vector<AudioEventId> GameplaySession::consumeAudioEvents() const {
    std::vector<AudioEventId> out;
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
        presentation_.pendingAudioEvents.push_back(AudioEventId::UiConfirm);
    }
    ImGui::End();
}

} // namespace engine
