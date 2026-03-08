#include <engine/gameplay_session_subsystems.h>

#include <engine/defensive_special.h>
#include <engine/gameplay_session.h>

#include <algorithm>

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
