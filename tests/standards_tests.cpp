#include <engine/standards.h>

#include <cstdlib>
#include <iostream>

int main() {
    const engine::EngineStandards d = engine::specDefaultStandards();
    if (d.playfieldWidth != 1080 || d.playfieldHeight != 1440 || d.renderTargetWidth != 1440 || d.renderTargetHeight != 1920) {
        std::cerr << "standards defaults mismatch\n";
        return EXIT_FAILURE;
    }

    engine::EngineStandards o = d;
    o.playfieldWidth = 12;
    o.playfieldHeight = 12;
    o.renderTargetWidth = 1;
    o.renderTargetHeight = 1;
    o.backgroundBrightnessMin = 0.9F;
    o.backgroundBrightnessMax = 0.2F;
    engine::clampStandards(o);
    if (o.playfieldWidth < 480 || o.playfieldHeight < 640 || o.renderTargetWidth < 640 || o.renderTargetHeight < 640 || o.backgroundBrightnessMax < o.backgroundBrightnessMin) {
        std::cerr << "standards clamp failed\n";
        return EXIT_FAILURE;
    }

    if (engine::getRadialAngleStep(6) != 60.0F || engine::getRadialAngleStep(8) != 45.0F || engine::getRadialAngleStep(12) != 30.0F
        || engine::getRadialAngleStep(16) != 22.5F || engine::getRadialAngleStep(24) != 15.0F || engine::getRadialAngleStep(36) != 10.0F) {
        std::cerr << "radial angle lookup mismatch\n";
        return EXIT_FAILURE;
    }

    if (d.renderOrder[0] != engine::RenderLayer::Player || d.renderOrder[1] != engine::RenderLayer::EnemyProjectiles
        || d.renderOrder[2] != engine::RenderLayer::PlayerProjectiles || d.renderOrder[3] != engine::RenderLayer::Bosses
        || d.renderOrder[4] != engine::RenderLayer::Enemies || d.renderOrder[5] != engine::RenderLayer::Background) {
        std::cerr << "render order default mismatch\n";
        return EXIT_FAILURE;
    }

    std::cout << "standards_tests passed\n";
    return EXIT_SUCCESS;
}
