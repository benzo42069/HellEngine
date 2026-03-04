#include <engine/timing.h>

namespace engine {

StepResult consumeFixedSteps(double accumulator, const double fixedDt, const std::uint32_t maxSteps) {
    StepResult result;

    while (accumulator >= fixedDt && result.steps < maxSteps) {
        accumulator -= fixedDt;
        ++result.steps;
    }

    result.remainingAccumulator = accumulator;
    return result;
}

} // namespace engine
