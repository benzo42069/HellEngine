#include <engine/perf_profiler.h>

namespace engine {

void PerfProfiler::setEnabled(const bool enabled) { enabled_ = enabled; }

bool PerfProfiler::enabled() const { return enabled_; }

void PerfProfiler::beginFrame() {
    if (!enabled_) return;
    accumMs_.fill(0.0);
}

void PerfProfiler::addZoneTime(const PerfZone zone, const double ms) {
    if (!enabled_) return;
    accumMs_[static_cast<std::size_t>(zone)] += ms;
}

void PerfProfiler::endFrame() {
    if (!enabled_) return;

    snapshot_.frameMs = 0.0F;
    for (std::size_t i = 0; i < accumMs_.size(); ++i) {
        snapshot_.zoneMs[i] = static_cast<float>(accumMs_[i]);
        snapshot_.frameMs += snapshot_.zoneMs[i];
    }

    snapshot_.warningSimulationBudget = snapshot_.zoneMs[static_cast<std::size_t>(PerfZone::Simulation)] > 8.0F;
    snapshot_.warningRenderBudget = snapshot_.zoneMs[static_cast<std::size_t>(PerfZone::Render)] > 4.0F;
    snapshot_.warningCollisionBudget = snapshot_.zoneMs[static_cast<std::size_t>(PerfZone::Collisions)] > 2.0F;
}

void PerfProfiler::setCounters(const PerfCounters& counters) {
    if (!enabled_) return;
    snapshot_.counters = counters;
}

const PerfSnapshot& PerfProfiler::snapshot() const { return snapshot_; }

} // namespace engine
