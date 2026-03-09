#include <engine/gameplay_session_subsystems.h>

#include <engine/defensive_special.h>
#include <engine/gameplay_session.h>

#include <algorithm>
#include <cmath>
#include <span>

namespace engine {

void PlayerCombatSubsystem::updateAimTarget(PlayerCombatState& playerState, const double simClock) const {
    playerState.aimTarget.x = 180.0F * std::cos(static_cast<float>(simClock) * 0.9F);
    playerState.aimTarget.y = 120.0F * std::sin(static_cast<float>(simClock) * 1.3F);
}

void PlayerCombatSubsystem::applyMovement(PlayerCombatState& playerState, const double dt, const std::uint32_t inputMask) const {
    const float moveStep = static_cast<float>(dt) * 120.0F;
    if ((inputMask & InputMoveLeft) != 0U) playerState.playerPos.x -= moveStep;
    if ((inputMask & InputMoveRight) != 0U) playerState.playerPos.x += moveStep;
    if ((inputMask & InputMoveUp) != 0U) playerState.playerPos.y -= moveStep;
    if ((inputMask & InputMoveDown) != 0U) playerState.playerPos.y += moveStep;
    playerState.playerPos.x = std::clamp(playerState.playerPos.x, 0.0F, static_cast<float>(engineStandards().playfieldWidth));
    playerState.playerPos.y = std::clamp(playerState.playerPos.y, 0.0F, static_cast<float>(engineStandards().playfieldHeight));
}

bool PlayerCombatSubsystem::tryActivateDefensiveSpecial(const std::uint32_t inputMask, DefensiveSpecialSystem& special) const {
    return (inputMask & InputDefensiveSpecial) != 0U && special.tryActivate();
}

std::uint32_t PlayerCombatSubsystem::collectGrazePoints(const PlayerCombatState& playerState, ProjectileSystem& projectiles, const DefensiveSpecialSystem& special, const std::uint64_t tickIndex) const {
    return projectiles.collectGrazePoints(
        playerState.playerPos,
        playerState.playerRadius,
        special.config().grazeBandInnerPadding,
        special.config().grazeBandOuterPadding,
        tickIndex,
        special.config().grazeCooldownTicks
    );
}

void ProgressionSubsystem::onUpgradeNavigation(ProgressionState& progression, const UpgradeSelectionContext context, const UpgradeNavAction action) const {
    if (action == UpgradeNavAction::ToggleScreen) {
        progression.upgradeScreenOpen = context.hasPendingChoices ? !progression.upgradeScreenOpen : progression.upgradeScreenOpen;
        return;
    }
    if (!progression.upgradeScreenOpen || !context.hasPendingChoices || context.choiceCount == 0) return;

    if (action == UpgradeNavAction::MoveLeft) { progression.focusedUpgradeIndex = (progression.focusedUpgradeIndex + context.choiceCount - 1) % context.choiceCount; }
    if (action == UpgradeNavAction::MoveRight) { progression.focusedUpgradeIndex = (progression.focusedUpgradeIndex + 1) % context.choiceCount; }
    if (action == UpgradeNavAction::SelectSlot1) progression.focusedUpgradeIndex = 0;
    if (action == UpgradeNavAction::SelectSlot2) progression.focusedUpgradeIndex = 1;
    if (action == UpgradeNavAction::SelectSlot3) progression.focusedUpgradeIndex = 2;
    if (action == UpgradeNavAction::SelectSlot1 || action == UpgradeNavAction::SelectSlot2 || action == UpgradeNavAction::SelectSlot3 || action == UpgradeNavAction::Confirm) {
        if (context.choose && context.choose(progression.focusedUpgradeIndex)) progression.upgradeScreenOpen = false;
    }
    if (action == UpgradeNavAction::Reroll && context.rerollChoices) (void)context.rerollChoices();
}


void EncounterSimulationSubsystem::emitDespawnParticles(ProjectileSystem& projectiles, const BulletPaletteTable& paletteTable, PresentationState& presentation) const {
    for (const ProjectileDespawnEvent& event : projectiles.despawnEvents()) {
        Color impactColor = event.paletteIndex == 0
            ? (event.allegiance == ProjectileAllegiance::Enemy ? Color {255, 220, 120, 220} : Color {120, 220, 255, 220})
            : paletteTable.get(event.paletteIndex).impact;
        if (event.paletteIndex != 0 && impactColor.a == 0) {
            impactColor = paletteTable.get(event.paletteIndex).core;
        }
        presentation.particleFx.burst(event.pos, impactColor);
        if (event.explodeShards > 0) {
            const float len = std::sqrt(event.pos.x * event.pos.x + event.pos.y * event.pos.y);
            const Vec2 dir = len > 0.0001F ? Vec2 {event.pos.x / len, event.pos.y / len} : Vec2 {0.0F, -1.0F};
            presentation.cameraShakeEvents.push_back(ShakeParams {
                .profile = ShakeProfile::Explosion,
                .amplitude = 3.0F,
                .duration = 0.18F,
                .direction = dir,
                .frequency = 36.0F,
                .damping = 11.0F,
            });
        }
    }
}

void EncounterSimulationSubsystem::processRuntimeEvents(EntitySystem& entities, EncounterRuntimeState& encounter, std::vector<ShakeParams>& cameraShakes) const {
    for (const EntityRuntimeEvent& runtimeEvent : entities.runtimeEvents()) {
        if (runtimeEvent.type == EntityRuntimeEventType::Telegraph) {
            ++encounter.telegraphCount;
            cameraShakes.push_back(ShakeParams {
                .profile = ShakeProfile::Ambient,
                .amplitude = 0.85F,
                .duration = 0.15F,
                .direction = {0.0F, 0.0F},
                .frequency = 12.0F,
                .damping = 6.0F,
            });
        }
        if (runtimeEvent.type == EntityRuntimeEventType::HazardSync) {
            ++encounter.hazardSyncCount;
        }
        if (runtimeEvent.type == EntityRuntimeEventType::BossDefeated) {
        }
    }
}

void EncounterSimulationSubsystem::resolveCpuDeterministicCollisions(ProjectileSystem& projectiles, EntitySystem& entities, const PlayerCombatState& playerState, EncounterRuntimeState& encounter, PresentationState& presentation) const {
    projectiles.buildGrid();
    if (presentation.dangerFieldEnabled) {
        presentation.dangerField.buildFromGrid(
            projectiles.gridHead(),
            projectiles.gridNext(),
            projectiles.posX(),
            projectiles.posY(),
            projectiles.active(),
            projectiles.gridX(),
            projectiles.gridY(),
            projectiles.worldHalfExtent()
        );
    }
    encounter.collisionTargetCount = 0;
    encounter.collisionTargets[encounter.collisionTargetCount++] = CollisionTarget {.pos = playerState.playerPos, .radius = playerState.playerRadius, .id = 0U, .team = 0U};
    entities.appendCollisionTargets(encounter.collisionTargets, encounter.collisionTargetCount);
    encounter.collisionEventCount = 0;
    projectiles.resolveCollisions(
        std::span<const CollisionTarget>(encounter.collisionTargets.data(), encounter.collisionTargetCount),
        encounter.collisionEvents,
        encounter.collisionEventCount
    );

    for (std::uint32_t collisionIndex = 0; collisionIndex < encounter.collisionEventCount; ++collisionIndex) {
        const CollisionEvent& e = encounter.collisionEvents[collisionIndex];
        if (e.targetId != 0U || e.bulletIndex >= projectiles.capacity()) continue;
        const Vec2 hitPos {projectiles.posX()[e.bulletIndex], projectiles.posY()[e.bulletIndex]};
        const Vec2 dir = {playerState.playerPos.x - hitPos.x, playerState.playerPos.y - hitPos.y};
        presentation.cameraShakeEvents.push_back(ShakeParams {
            .profile = ShakeProfile::Impact,
            .amplitude = 3.5F,
            .duration = 0.16F,
            .direction = dir,
            .frequency = 32.0F,
            .damping = 12.0F,
        });
    }

    entities.processCollisionEvents(std::span<const CollisionEvent>(encounter.collisionEvents.data(), encounter.collisionEventCount));
    for (std::uint32_t i = 0; i < encounter.collisionEventCount; ++i) {
        const CollisionEvent& e = encounter.collisionEvents[i];
        if (e.bulletIndex >= projectiles.capacity()) continue;
        const Vec2 hitPos {projectiles.posX()[e.bulletIndex], projectiles.posY()[e.bulletIndex]};
        presentation.pendingAudioEvents.push_back(AudioEvent {.type = AudioEventType::Hit, .position = hitPos});
    }
}

void EncounterSimulationSubsystem::emitZoneTransitionFeedback(const ZoneDefinition* zoneBeforeUpdate, const ZoneDefinition* zoneAfterUpdate, const Vec2& playerPos, std::vector<ShakeParams>& cameraShakes, std::vector<AudioEvent>& audioEvents) const {
    if (zoneBeforeUpdate && zoneAfterUpdate && zoneBeforeUpdate->type != zoneAfterUpdate->type && zoneAfterUpdate->type == ZoneType::Boss) {
        cameraShakes.push_back(ShakeParams {
            .profile = ShakeProfile::BossRumble,
            .amplitude = 4.0F,
            .duration = 0.4F,
            .direction = {0.0F, 0.0F},
            .frequency = 8.0F,
            .damping = 2.0F,
        });
        audioEvents.push_back(AudioEvent {.type = AudioEventType::BossPhaseTransition, .position = playerPos});
    }
}


void SessionOrchestrationSubsystem::pollContentHotReloads(ContentWatcher& watcher, const std::uint64_t tickIndex, const std::uint64_t pollIntervalTicks, std::uint64_t& nextPollTick, const HotReloadCallbacks& callbacks) const {
    if (tickIndex < nextPollTick) return;
    nextPollTick = tickIndex + pollIntervalTicks;

    for (const ContentChange& change : watcher.pollChanges()) {
        switch (change.type) {
            case ContentType::Patterns:
                if (callbacks.reloadPatterns) callbacks.reloadPatterns(change.path);
                break;
            case ContentType::Entities:
                if (callbacks.reloadEntities) callbacks.reloadEntities(change.path);
                break;
            case ContentType::Traits:
                if (callbacks.reloadTraits) callbacks.reloadTraits(change.path);
                break;
            case ContentType::Difficulty:
                if (callbacks.reloadDifficulty) callbacks.reloadDifficulty(change.path);
                break;
            case ContentType::Palettes:
                if (callbacks.reloadPalettes) callbacks.reloadPalettes(change.path);
                break;
        }
    }
}

void SessionOrchestrationSubsystem::updateUpgradeCadence(const std::uint64_t tickIndex, const bool hasPendingChoices, bool& upgradeScreenOpen, const std::function<bool()>& rollChoices) const {
    if (tickIndex == 0 || tickIndex % 300 != 0 || hasPendingChoices) return;
    if (rollChoices && rollChoices()) upgradeScreenOpen = true;
}

void SessionOrchestrationSubsystem::applyUpgradeDebugOptions(const UpgradeDebugOptions& options,
                                                             const bool hasPendingChoices,
                                                             bool& perfHudOpen,
                                                             bool& dangerFieldEnabled,
                                                             bool& upgradeScreenOpen,
                                                             const std::function<bool()>& rollChoices,
                                                             const std::function<void(TraitRarity)>& forcePendingRarity) const {
    perfHudOpen = options.showPerfHud;
    dangerFieldEnabled = options.showDangerField;

    if (options.spawnUpgradeScreen && !hasPendingChoices) {
        if (rollChoices && rollChoices()) upgradeScreenOpen = true;
    }

    if (options.forcedRarity >= 0 && hasPendingChoices && forcePendingRarity) {
        forcePendingRarity(static_cast<TraitRarity>(options.forcedRarity));
    }
}

void EncounterSimulationSubsystem::emitAmbientZoneFeedback(const ZoneDefinition* zoneAfterUpdate, std::vector<ShakeParams>& cameraShakes) const {
    if (zoneAfterUpdate && (zoneAfterUpdate->type == ZoneType::Combat || zoneAfterUpdate->type == ZoneType::Elite || zoneAfterUpdate->type == ZoneType::Boss)) {
        cameraShakes.push_back(ShakeParams {
            .profile = ShakeProfile::Ambient,
            .amplitude = 0.12F,
            .duration = 0.0F,
            .direction = {0.0F, 0.0F},
            .frequency = 2.0F,
            .damping = 0.0F,
        });
    }
}

void PresentationSubsystem::emitDefensiveSpecialActivation(std::vector<ShakeParams>& cameraShakes, std::vector<AudioEvent>& audioEvents, const Vec2& playerPos) const {
    cameraShakes.push_back(ShakeParams {
        .profile = ShakeProfile::SpecialPulse,
        .amplitude = 2.0F,
        .duration = 0.45F,
        .direction = {0.0F, 0.0F},
        .frequency = 12.0F,
        .damping = 3.5F,
    });
    audioEvents.push_back(AudioEvent {.type = AudioEventType::DefensiveSpecialActivated, .position = playerPos});
}

void PresentationSubsystem::emitGrazeFeedback(std::vector<ShakeParams>& cameraShakes, std::vector<AudioEvent>& audioEvents, const Vec2& playerPos) const {
    cameraShakes.push_back(ShakeParams {
        .profile = ShakeProfile::GrazeTremor,
        .amplitude = 0.5F,
        .duration = 0.10F,
        .direction = {0.0F, 0.0F},
        .frequency = 110.0F,
        .damping = 22.0F,
    });
    audioEvents.push_back(AudioEvent {.type = AudioEventType::Graze, .position = playerPos});
}

} // namespace engine
