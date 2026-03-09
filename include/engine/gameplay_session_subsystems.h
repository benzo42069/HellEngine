#pragma once

#include <engine/camera_shake.h>
#include <engine/content_watcher.h>
#include <engine/config.h>
#include <engine/editor_tools.h>
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
struct EncounterRuntimeState;
struct ProgressionState;
struct PresentationState;
struct AudioEvent;
struct ZoneDefinition;
class DefensiveSpecialSystem;
class TraitSystem;
class ProjectileSystem;
class EntitySystem;
class BulletPaletteTable;

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

class EncounterSimulationSubsystem {
  public:
    void emitDespawnParticles(ProjectileSystem& projectiles, const BulletPaletteTable& paletteTable, PresentationState& presentation) const;
    void processRuntimeEvents(EntitySystem& entities, EncounterRuntimeState& encounter, std::vector<ShakeParams>& cameraShakes) const;
    void resolveCpuDeterministicCollisions(ProjectileSystem& projectiles, EntitySystem& entities, const PlayerCombatState& playerState, EncounterRuntimeState& encounter, PresentationState& presentation) const;
    void emitZoneTransitionFeedback(const ZoneDefinition* zoneBeforeUpdate, const ZoneDefinition* zoneAfterUpdate, const Vec2& playerPos, std::vector<ShakeParams>& cameraShakes, std::vector<AudioEvent>& audioEvents) const;
    void emitAmbientZoneFeedback(const ZoneDefinition* zoneAfterUpdate, std::vector<ShakeParams>& cameraShakes) const;
};

class SessionOrchestrationSubsystem {
  public:
    struct HotReloadCallbacks {
        std::function<void(const std::string&)> reloadPatterns {};
        std::function<void(const std::string&)> reloadEntities {};
        std::function<void(const std::string&)> reloadTraits {};
        std::function<void(const std::string&)> reloadDifficulty {};
        std::function<void(const std::string&)> reloadPalettes {};
    };

    void pollContentHotReloads(ContentWatcher& watcher, std::uint64_t tickIndex, std::uint64_t pollIntervalTicks, std::uint64_t& nextPollTick, const HotReloadCallbacks& callbacks) const;
    void updateUpgradeCadence(std::uint64_t tickIndex, bool hasPendingChoices, bool& upgradeScreenOpen, const std::function<bool()>& rollChoices) const;
    void applyUpgradeDebugOptions(const UpgradeDebugOptions& options, bool hasPendingChoices, bool& perfHudOpen, bool& dangerFieldEnabled, bool& upgradeScreenOpen, const std::function<bool()>& rollChoices, const std::function<void(TraitRarity)>& forcePendingRarity) const;
};

} // namespace engine
