#include <engine/timing.h>

#include <cmath>
#include <cstdlib>
#include <iostream>

int main() {
    const engine::StepResult r1 = engine::consumeFixedSteps(0.050, 1.0 / 60.0, 8);
    if (r1.steps != 3) {
        std::cerr << "Expected 3 steps, got " << r1.steps << '\n';
        return EXIT_FAILURE;
    }

    const double expectedRem = 0.050 - 3.0 * (1.0 / 60.0);
    if (std::fabs(r1.remainingAccumulator - expectedRem) > 1e-9) {
        std::cerr << "Unexpected remaining accumulator\n";
        return EXIT_FAILURE;
    }

    const engine::StepResult r2 = engine::consumeFixedSteps(1.0, 1.0 / 60.0, 4);
    if (r2.steps != 4) {
        std::cerr << "Expected max step cap of 4\n";
        return EXIT_FAILURE;
    }

    std::cout << "timing_tests passed\n";
    return EXIT_SUCCESS;
}
