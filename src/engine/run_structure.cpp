#include <engine/run_structure.h>

#include <algorithm>
#include <utility>

namespace engine {

void RunStructure::initializeDefaults() {
    stages_.clear();

    for (int i = 0; i < 2; ++i) {
        StageDefinition stage;
        stage.name = i == 0 ? "Stage 1: Frontier" : "Stage 2: Citadel";
        stage.zones = {
            ZoneDefinition {.type = ZoneType::Combat, .durationSeconds = 8.0F},
            ZoneDefinition {.type = ZoneType::Combat, .durationSeconds = 8.0F},
            ZoneDefinition {.type = ZoneType::Combat, .durationSeconds = 8.0F},
            ZoneDefinition {.type = ZoneType::Elite, .durationSeconds = 10.0F},
            ZoneDefinition {.type = ZoneType::Event, .durationSeconds = 9.0F},
            ZoneDefinition {.type = ZoneType::Boss, .durationSeconds = 9999.0F},
        };
        stages_.push_back(stage);
    }

    reset();
}

void RunStructure::setStages(std::vector<StageDefinition> stages) {
    stages_ = std::move(stages);
    reset();
}

void RunStructure::reset() {
    state_ = stages_.empty() ? RunState::Completed : RunState::InProgress;
    stageIndex_ = 0;
    zoneIndex_ = 0;
    bossDefeatsAtZoneStart_ = 0;
    if (const ZoneDefinition* zone = currentZone()) {
        zoneTimeRemaining_ = zone->durationSeconds;
    } else {
        zoneTimeRemaining_ = 0.0F;
    }
}

void RunStructure::advanceZone(const std::uint32_t defeatedBossesTotal) {
    if (state_ != RunState::InProgress) return;

    ++zoneIndex_;
    const StageDefinition* stage = currentStage();
    if (!stage || zoneIndex_ >= stage->zones.size()) {
        ++stageIndex_;
        zoneIndex_ = 0;
        stage = currentStage();
    }

    if (!stage) {
        state_ = RunState::Completed;
        zoneTimeRemaining_ = 0.0F;
        return;
    }

    const ZoneDefinition& zone = stage->zones[zoneIndex_];
    zoneTimeRemaining_ = zone.durationSeconds;
    bossDefeatsAtZoneStart_ = defeatedBossesTotal;
}

void RunStructure::update(const float dt, const std::uint32_t defeatedBossesTotal, const bool playerAlive) {
    if (state_ != RunState::InProgress) return;
    if (!playerAlive) {
        state_ = RunState::Failed;
        return;
    }

    const ZoneDefinition* zone = currentZone();
    if (!zone) {
        state_ = RunState::Completed;
        return;
    }

    if (zone->type == ZoneType::Boss) {
        if (defeatedBossesTotal > bossDefeatsAtZoneStart_) {
            advanceZone(defeatedBossesTotal);
        }
        return;
    }

    zoneTimeRemaining_ = std::max(0.0F, zoneTimeRemaining_ - dt);
    if (zoneTimeRemaining_ <= 0.0F) {
        advanceZone(defeatedBossesTotal);
    }
}

RunState RunStructure::state() const { return state_; }

std::size_t RunStructure::stageIndex() const { return stageIndex_; }

std::size_t RunStructure::zoneIndex() const { return zoneIndex_; }

float RunStructure::zoneTimeRemaining() const { return zoneTimeRemaining_; }

std::size_t RunStructure::stageCount() const { return stages_.size(); }

const std::vector<StageDefinition>& RunStructure::stages() const { return stages_; }

const StageDefinition* RunStructure::currentStage() const {
    if (stageIndex_ >= stages_.size()) return nullptr;
    return &stages_[stageIndex_];
}

const ZoneDefinition* RunStructure::currentZone() const {
    const StageDefinition* stage = currentStage();
    if (!stage || zoneIndex_ >= stage->zones.size()) return nullptr;
    return &stage->zones[zoneIndex_];
}

std::string toString(const ZoneType type) {
    switch (type) {
        case ZoneType::Combat: return "Combat";
        case ZoneType::Elite: return "Elite";
        case ZoneType::Event: return "Event";
        case ZoneType::Boss: return "Boss";
    }
    return "Unknown";
}

std::string toString(const RunState state) {
    switch (state) {
        case RunState::InProgress: return "In Progress";
        case RunState::Completed: return "Completed";
        case RunState::Failed: return "Failed";
    }
    return "Unknown";
}

} // namespace engine
