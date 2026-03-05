#include <engine/gpu_bullets.h>

#include <cstdlib>
#include <iostream>

int main() {
    engine::GpuBulletSystem gpu;
    gpu.initialize(1000, 400.0F);

    engine::GpuBullet b;
    b.flags = 1U;
    b.lifetime = 1.0F;
    b.velX = 10.0F;
    b.velY = 0.0F;
    b.radius = 2.0F;

    if (!gpu.emit(b)) {
        std::cerr << "emit failed\n";
        return EXIT_FAILURE;
    }

    gpu.update(0.5F);
    if (gpu.activeCount() != 1) {
        std::cerr << "bullet should still be active\n";
        return EXIT_FAILURE;
    }

    gpu.update(0.6F);
    if (gpu.activeCount() != 0) {
        std::cerr << "bullet should be culled after lifetime expiry\n";
        return EXIT_FAILURE;
    }

    std::cout << "gpu_bullets_tests passed\n";
    return EXIT_SUCCESS;
}
