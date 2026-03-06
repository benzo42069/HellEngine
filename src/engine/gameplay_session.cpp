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

GameplaySession::GameplaySession(EngineConfig& config)
    : frameAllocator_(1024 * 1024),
      jobSystem_(),
      rngStreams_(config.simulationSeed),
      config_(config) {
    collisionTargets_.reserve(1024);
    collisionEvents_.reserve(16384);
    particleFx_.initialize(4096);
}

void GameplaySession::initializeContent(PatternBank& patternBank, PatternPlayer& patternPlayer) {
    patternBank_ = patternBank;
    patternPlayer_ = patternPlayer;
}

void GameplaySession::onUpgradeNavigation(const UpgradeNavAction action) {
    if (action == UpgradeNavAction::ToggleScreen) {
        upgradeScreenOpen_ = traitSystem_.hasPendingChoices() ? !upgradeScreenOpen_ : upgradeScreenOpen_;
        return;
    }
    if (!upgradeScreenOpen_ || !traitSystem_.hasPendingChoices()) return;

    if (action == UpgradeNavAction::MoveLeft) focusedUpgradeIndex_ = (focusedUpgradeIndex_ + TraitSystem::choiceCount - 1) % TraitSystem::choiceCount;
    if (action == UpgradeNavAction::MoveRight) focusedUpgradeIndex_ = (focusedUpgradeIndex_ + 1) % TraitSystem::choiceCount;
    if (action == UpgradeNavAction::SelectSlot1) focusedUpgradeIndex_ = 0;
    if (action == UpgradeNavAction::SelectSlot2) focusedUpgradeIndex_ = 1;
    if (action == UpgradeNavAction::SelectSlot3) focusedUpgradeIndex_ = 2;
    if (action == UpgradeNavAction::SelectSlot1 || action == UpgradeNavAction::SelectSlot2 || action == UpgradeNavAction::SelectSlot3 || action == UpgradeNavAction::Confirm) {
        if (traitSystem_.choose(focusedUpgradeIndex_)) upgradeScreenOpen_ = false;
    }
    if (action == UpgradeNavAction::Reroll) (void)traitSystem_.rerollChoices();
}

void GameplaySession::updateGameplay(const double dt, const std::uint32_t inputMask) {
    using Clock = std::chrono::steady_clock;
    const auto simStart = Clock::now();
    gpuUpdateMsFrame_ = 0.0F;

    frameAllocator_.reset();
    auto* scratch = static_cast<std::uint64_t*>(frameAllocator_.allocate(sizeof(std::uint64_t), alignof(std::uint64_t)));
    *scratch = tickIndex_;

    projectiles_.beginTick();
    traitSystem_.onTick(tickIndex_);

    auto emitDespawnParticles = [this]() {
        for (const ProjectileDespawnEvent& event : projectiles_.despawnEvents()) {
            Color impactColor = event.paletteIndex == 0
                ? (event.allegiance == ProjectileAllegiance::Enemy ? Color {255, 220, 120, 220} : Color {120, 220, 255, 220})
                : bulletPaletteTable_.get(event.paletteIndex).impact;
            if (event.paletteIndex != 0 && impactColor.a == 0) {
                impactColor = bulletPaletteTable_.get(event.paletteIndex).core;
            }
            particleFx_.burst(event.pos, impactColor);
        }
    };

    aimTarget_.x = 180.0F * std::cos(static_cast<float>(simClock_) * 0.9F);
    aimTarget_.y = 120.0F * std::sin(static_cast<float>(simClock_) * 1.3F);

    if ((inputMask & InputDefensiveSpecial) != 0U) (void)defensiveSpecial_.tryActivate();

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
        if (upgradeDebug.spawnUpgradeScreen && !traitSystem_.hasPendingChoices()) {
            (void)traitSystem_.rollChoices();
            upgradeScreenOpen_ = true;
        }
        if (upgradeDebug.forcedRarity >= 0 && traitSystem_.hasPendingChoices()) {
            traitSystem_.forcePendingRarity(static_cast<TraitRarity>(upgradeDebug.forcedRarity));
        }

        const TraitModifiers& tm = traitSystem_.modifiers();
        const ZoneDefinition* zone = runStructure_.currentZone();
        if (zone) difficultyModel_.setStageZone(runStructure_.stageIndex(), zone->type);
        const DifficultyScalars diffScalars = difficultyModel_.scalars();
        playerRadius_ = 10.0F + (archetype.stats.defense + mb.defenseBonus) * 0.45F + tm.playerRadiusAdd;
        const float fireRateScale = 0.80F + (archetype.stats.fireRate + mb.fireRateBonus) * 0.05F;
        patternPlayer_.setRuntimeModifiers(tm.patternCooldownScale / fireRateScale, tm.patternExtraBullets, tm.patternJitterAddDeg);

        auto emitWithRuntimeMods = [this, &tm, &archetype, &mb, &diffScalars](const ProjectileSpawn& spawn) {
            ProjectileSpawn mod = spawn;
            mod.allegiance = ProjectileAllegiance::Player;
            mod.paletteIndex = static_cast<std::uint8_t>(std::min<std::size_t>(BulletPaletteTable::kMaxPalettes - 1U, archetypeSystem_.selectedIndex() + 1U));
            const float powerScale = 0.85F + (archetype.stats.power + mb.powerBonus) * 0.03F;
            mod.vel.x *= tm.projectileSpeedMul * powerScale * diffScalars.patternSpeed * tm.offensiveSpecialPowerMul * defensiveSpecial_.playerDamageMultiplier();
            mod.vel.y *= tm.projectileSpeedMul * powerScale * diffScalars.patternSpeed * tm.offensiveSpecialPowerMul * defensiveSpecial_.playerDamageMultiplier();
            mod.radius = std::max(0.5F, spawn.radius + tm.projectileRadiusAdd);
            projectiles_.spawn(mod);
        };
        if (useCompiledPatternGraph_ && !patternBank_.compiledGraphs().empty()) {
            graphVmState_.difficultyScalar = diffScalars.patternSpeed;
            graphVm_.execute(patternBank_.compiledGraphs().front(), graphVmState_, static_cast<float>(dt), {0.0F, 0.0F}, aimTarget_,
                [this, &tm, &archetype, &mb, &diffScalars](const ProjectileSpawn& spawn) {
                    ProjectileSpawn mod = spawn;
                    mod.allegiance = ProjectileAllegiance::Player;
                    mod.paletteIndex = static_cast<std::uint8_t>(std::min<std::size_t>(BulletPaletteTable::kMaxPalettes - 1U, archetypeSystem_.selectedIndex() + 1U));
                    const float powerScale = 0.85F + (archetype.stats.power + mb.powerBonus) * 0.03F;
                    mod.vel.x *= tm.projectileSpeedMul * powerScale * diffScalars.patternSpeed * tm.offensiveSpecialPowerMul;
                    mod.vel.y *= tm.projectileSpeedMul * powerScale * diffScalars.patternSpeed * tm.offensiveSpecialPowerMul;
                    mod.radius = std::max(0.5F, spawn.radius + tm.projectileRadiusAdd);
                    projectiles_.spawn(mod);
                });
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
                case ZoneType::Elite: em.enemyFireRateScale *= 1.25F; em.enemyProjectileSpeedScale *= 1.20F; break;
                case ZoneType::Event: em.enemyFireRateScale *= 0.80F; em.harvestYieldMultiplier *= 1.50F; break;
                case ZoneType::Boss: em.enemyFireRateScale *= 1.35F; em.enemyProjectileSpeedScale *= 1.25F; break;
            }
        }
        profiler_.addZoneTime(PerfZone::Patterns, std::chrono::duration<double, std::milli>(Clock::now() - patternStart).count());
        const auto bulletStart = Clock::now();
        entitySystem_.update(static_cast<float>(dt) * dilation.enemyMovement, projectiles_, playerPos_, em);
        profiler_.addZoneTime(PerfZone::Bullets, std::chrono::duration<double, std::milli>(Clock::now() - bulletStart).count());
    }

    if (bulletSimMode_ == BulletSimulationMode::CpuDeterministic) {
        projectiles_.updateMotion(static_cast<float>(dt), dilation.enemyProjectiles, dilation.playerProjectiles);
        emitDespawnParticles();
        projectiles_.buildGrid();
        collisionTargets_.clear();
        collisionTargets_.push_back(CollisionTarget {.pos = playerPos_, .radius = playerRadius_, .id = 0U, .team = 0U});
        entitySystem_.appendCollisionTargets(collisionTargets_);
        collisionEvents_.clear();
        projectiles_.resolveCollisions(std::span<const CollisionTarget>(collisionTargets_.data(), collisionTargets_.size()), collisionEvents_);
        emitDespawnParticles();
        entitySystem_.processCollisionEvents(std::span<const CollisionEvent>(collisionEvents_.data(), collisionEvents_.size()));
    }

    particleFx_.update(static_cast<float>(dt));
    runStructure_.update(static_cast<float>(dt), entitySystem_.stats().defeatedBosses, playerHealth_ > 0.0F);
    simClock_ += dt;
    ++tickIndex_;
    profiler_.addZoneTime(PerfZone::Simulation, std::chrono::duration<double, std::milli>(Clock::now() - simStart).count());
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

void GameplaySession::drawUpgradeSelectionUi(const double frameDelta) {
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
        if (hasSynergyWithActive(t)) ImGui::TextColored(ImVec4(0.25F, 0.9F, 0.45F, 1.0F), "Synergy with current build");
        if (i == focusedUpgradeIndex_) ImGui::TextColored(ImVec4(1.0F, 0.9F, 0.3F, 1.0F), "Focused (Enter / A to select)");
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
    ImGui::Text("Power: %.2f -> %.2f", current.power, projected.power);
    ImGui::Text("Fire Rate: %.2f -> %.2f", current.fireRate, projected.fireRate);
    ImGui::Text("Move Speed: %.2f -> %.2f", current.moveSpeed, projected.moveSpeed);
    ImGui::Text("Defense: %.2f -> %.2f", current.defense, projected.defense);
    if (ImGui::Button("Reroll [X]")) (void)traitSystem_.rerollChoices();
    ImGui::SameLine();
    if (ImGui::Button("Select Focused [Enter]")) {
        if (traitSystem_.choose(focusedUpgradeIndex_)) upgradeScreenOpen_ = false;
    }
    ImGui::End();
}

} // namespace engine
