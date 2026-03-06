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


    engine::GpuBulletSystem mass;
    mass.initialize(100000, 400.0F);
    engine::GpuBullet m;
    m.flags = 1U;
    m.lifetime = 0.1F;
    m.velX = 1.0F;
    m.velY = 0.0F;
    m.radius = 1.0F;

    for (int i = 0; i < 100000; ++i) {
        if (!mass.emit(m)) {
            std::cerr << "100k emit failed\n";
            return EXIT_FAILURE;
        }
    }
    if (mass.activeCount() != 100000U) {
        std::cerr << "100k active count mismatch\n";
        return EXIT_FAILURE;
    }
    mass.update(0.11F);
    if (mass.activeCount() != 0U) {
        std::cerr << "expire cycle failed\n";
        return EXIT_FAILURE;
    }
    for (int i = 0; i < 100000; ++i) {
        if (!mass.emit(m)) {
            std::cerr << "free list refill failed\n";
            return EXIT_FAILURE;
        }
    }

    std::cout << "gpu_bullets_tests passed\n";
    return EXIT_SUCCESS;
}
