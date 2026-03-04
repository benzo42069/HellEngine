#include <engine/render2d.h>

#include <cmath>
#include <cstdlib>
#include <iostream>

int main() {
    engine::Camera2D camera;
    camera.setViewport(800, 600);
    camera.setCenter({100.0F, 50.0F});
    camera.setZoom(2.0F);

    const engine::Vec2 s = camera.worldToScreen({100.0F, 50.0F});
    if (std::fabs(s.x - 400.0F) > 0.001F || std::fabs(s.y - 300.0F) > 0.001F) {
        std::cerr << "camera center projection failed\n";
        return EXIT_FAILURE;
    }

    camera.pan({10.0F, -20.0F});
    const engine::Vec2 s2 = camera.worldToScreen({110.0F, 30.0F});
    if (std::fabs(s2.x - 400.0F) > 0.001F || std::fabs(s2.y - 300.0F) > 0.001F) {
        std::cerr << "camera pan projection failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "render2d_tests passed\n";
    return EXIT_SUCCESS;
}
