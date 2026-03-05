#pragma once

#include <engine/archetypes.h>
#include <engine/meta_progression.h>
#include <engine/traits.h>

namespace engine {

struct UpgradeViewStats {
    float power {0.0F};
    float fireRate {0.0F};
    float moveSpeed {0.0F};
    float defense {0.0F};
};

UpgradeViewStats buildUpgradeViewStats(const ArchetypeDefinition& archetype, const MetaBonuses& mb, const TraitModifiers& tm);
UpgradeViewStats projectUpgradeViewStats(const UpgradeViewStats& current, const Trait& trait);
const char* rarityLabel(TraitRarity rarity);

} // namespace engine
