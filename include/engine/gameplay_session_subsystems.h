#pragma once

#include <engine/camera_shake.h>
#include <engine/config.h>
#include <engine/input_system.h>
#include <engine/projectiles.h>
#include <engine/standards.h>
#include <engine/traits.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

namespace engine {

struct PlayerCombatState;
struct ProgressionState;
struct PresentationState;
struct AudioEvent;
class DefensiveSpecialSystem;
class TraitSystem;
class ProjectileSystem;

class PlayerCombatSubsystem {
  public:
    void updateAimTarget(PlayerCombatState& playerState, double simClock) const;
    void applyMovement(PlayerCombatState& playerState, double dt, std::uint32_t inputMask) const;
    bool tryActivateDefensiveSpecial(std::uint32_t inputMask, DefensiveSpecialSystem& special) const;
    std::uint32_t collectGrazePoints(const PlayerCombatState& playerState, ProjectileSystem& projectiles, const DefensiveSpecialSystem& special, std::uint64_t tickIndex) const;
};

class ProgressionSubsystem {
  public:
    struct UpgradeSelectionContext {
        bool hasPendingChoices {false};
        std::size_t choiceCount {0};
        std::function<bool(std::size_t)> choose {};
        std::function<bool()> rerollChoices {};
    };

    void onUpgradeNavigation(ProgressionState& progression, UpgradeSelectionContext context, UpgradeNavAction action) const;
};

class PresentationSubsystem {
  public:
    void emitDefensiveSpecialActivation(std::vector<ShakeParams>& cameraShakes, std::vector<AudioEvent>& audioEvents, const Vec2& playerPos) const;
    void emitGrazeFeedback(std::vector<ShakeParams>& cameraShakes, std::vector<AudioEvent>& audioEvents, const Vec2& playerPos) const;
};

} // namespace engine
