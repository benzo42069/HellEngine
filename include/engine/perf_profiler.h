#pragma once

#include <array>
#include <cstdint>

namespace engine {

enum class PerfZone : std::uint8_t {
    Simulation,
    Patterns,
    Bullets,
    Collisions,
    Render,
    Count,
};

struct PerfCounters {
    std::uint32_t activeBullets {0};
    float bulletSpawnsPerSecond {0.0F};
    std::uint32_t broadphaseChecks {0};
    std::uint32_t narrowphaseChecks {0};
    std::uint32_t drawCalls {0};
    std::uint32_t batches {0};
    std::uint32_t gpuActiveBullets {0};
    float gpuUpdateMs {0.0F};
    float gpuRenderMs {0.0F};
};

struct PerfSnapshot {
    float frameMs {0.0F};
    std::array<float, static_cast<std::size_t>(PerfZone::Count)> zoneMs {};
    PerfCounters counters {};
    bool warningSimulationBudget {false};
    bool warningRenderBudget {false};
    bool warningCollisionBudget {false};
};

class PerfProfiler {
  public:
    void setEnabled(bool enabled);
    [[nodiscard]] bool enabled() const;

    void beginFrame();
    void addZoneTime(PerfZone zone, double ms);
    void endFrame();

    void setCounters(const PerfCounters& counters);
    [[nodiscard]] const PerfSnapshot& snapshot() const;

  private:
    bool enabled_ {true};
    std::array<double, static_cast<std::size_t>(PerfZone::Count)> accumMs_ {};
    PerfSnapshot snapshot_ {};
};

} // namespace engine
