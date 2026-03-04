#pragma once

#include <cstdint>

namespace engine {

struct StepResult {
    std::uint32_t steps {0};
    double remainingAccumulator {0.0};
};

StepResult consumeFixedSteps(double accumulator, double fixedDt, std::uint32_t maxSteps);

} // namespace engine
