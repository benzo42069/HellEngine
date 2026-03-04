#include <engine/archetypes.h>

namespace engine {

namespace {
ArchetypeDefinition makeArchetype(
    const char* id,
    const char* name,
    const ArchetypeStats& stats,
    const char* primaryWeapon,
    const char* activeAbility,
    const char* passiveEffect,
    const std::uint32_t requiredUnlockTier
) {
    return ArchetypeDefinition {
        .id = id,
        .name = name,
        .stats = stats,
        .primaryWeapon = primaryWeapon,
        .activeAbility = activeAbility,
        .passiveEffect = passiveEffect,
        .requiredUnlockTier = requiredUnlockTier,
    };
}
} // namespace

void ArchetypeSystem::initializeDefaults() {
    archetypes_.clear();

    archetypes_.push_back(makeArchetype(
        "vanguard",
        "Vanguard",
        {.power = 8.0F, .fireRate = 6.0F, .moveSpeed = 5.0F, .defense = 8.0F, .synergy = 4.0F, .resourceGain = 4.0F, .dotPower = 3.0F, .rareChance = 2.0F},
        "Breaker Cannon",
        "Aegis Dash",
        "Bulwark: periodic barrier reduces incoming collision impact",
        0
    ));

    archetypes_.push_back(makeArchetype(
        "gunslinger",
        "Gunslinger",
        {.power = 6.0F, .fireRate = 9.0F, .moveSpeed = 7.0F, .defense = 4.0F, .synergy = 6.0F, .resourceGain = 4.0F, .dotPower = 3.0F, .rareChance = 5.0F},
        "Twin Repeaters",
        "Fan Volley",
        "Hot Hands: sustained fire gradually boosts shot tempo",
        0
    ));

    archetypes_.push_back(makeArchetype(
        "arcanist",
        "Arcanist",
        {.power = 7.0F, .fireRate = 5.0F, .moveSpeed = 5.0F, .defense = 4.0F, .synergy = 9.0F, .resourceGain = 6.0F, .dotPower = 8.0F, .rareChance = 7.0F},
        "Sigil Launcher",
        "Chrono Fracture",
        "Runic Echo: effects with status damage linger longer",
        0
    ));

    archetypes_.push_back(makeArchetype(
        "reaper",
        "Reaper",
        {.power = 9.0F, .fireRate = 4.0F, .moveSpeed = 6.0F, .defense = 5.0F, .synergy = 5.0F, .resourceGain = 5.0F, .dotPower = 9.0F, .rareChance = 3.0F},
        "Scythe Rail",
        "Soul Sever",
        "Mortal Wound: defeated foes pulse damage over time",
        1
    ));

    archetypes_.push_back(makeArchetype(
        "engineer",
        "Engineer",
        {.power = 5.0F, .fireRate = 6.0F, .moveSpeed = 4.0F, .defense = 7.0F, .synergy = 8.0F, .resourceGain = 8.0F, .dotPower = 4.0F, .rareChance = 4.0F},
        "Drone Carbine",
        "Deploy Turret",
        "Scrapper: extra resource drops from harvest nodes",
        1
    ));

    archetypes_.push_back(makeArchetype(
        "assassin",
        "Assassin",
        {.power = 8.0F, .fireRate = 7.0F, .moveSpeed = 9.0F, .defense = 3.0F, .synergy = 6.0F, .resourceGain = 3.0F, .dotPower = 6.0F, .rareChance = 6.0F},
        "Needle Burst",
        "Shadowstep",
        "Executioner: bonus damage when striking isolated targets",
        2
    ));

    archetypes_.push_back(makeArchetype(
        "alchemist",
        "Alchemist",
        {.power = 6.0F, .fireRate = 6.0F, .moveSpeed = 6.0F, .defense = 5.0F, .synergy = 7.0F, .resourceGain = 7.0F, .dotPower = 8.0F, .rareChance = 8.0F},
        "Catalyst Sprayer",
        "Volatile Mix",
        "Transmute: status kills convert into bonus currency",
        2
    ));

    archetypes_.push_back(makeArchetype(
        "oracle",
        "Oracle",
        {.power = 5.0F, .fireRate = 7.0F, .moveSpeed = 7.0F, .defense = 4.0F, .synergy = 9.0F, .resourceGain = 6.0F, .dotPower = 5.0F, .rareChance = 9.0F},
        "Prism Array",
        "Fate Weave",
        "Foretell: increased odds for rare and epic trait rolls",
        2
    ));

    selectedIndex_ = 0;
    unlockTier_ = 0;
}

void ArchetypeSystem::setUnlockTier(const std::uint32_t tier) {
    unlockTier_ = tier;
    if (!isUnlocked(selectedIndex_)) {
        for (std::size_t i = 0; i < archetypes_.size(); ++i) {
            if (isUnlocked(i)) {
                selectedIndex_ = i;
                break;
            }
        }
    }
}

const std::vector<ArchetypeDefinition>& ArchetypeSystem::archetypes() const { return archetypes_; }

std::size_t ArchetypeSystem::selectedIndex() const { return selectedIndex_; }

const ArchetypeDefinition& ArchetypeSystem::selected() const { return archetypes_[selectedIndex_]; }

bool ArchetypeSystem::isUnlocked(const std::size_t index) const {
    if (index >= archetypes_.size()) return false;
    return archetypes_[index].requiredUnlockTier <= unlockTier_;
}

bool ArchetypeSystem::select(const std::size_t index) {
    if (!isUnlocked(index)) return false;
    selectedIndex_ = index;
    return true;
}

} // namespace engine
