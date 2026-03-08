#include <engine/render2d.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>

TEST_CASE("Camera2D world-space projection remains stable", "[render2d]") {
    engine::Camera2D camera;
    camera.setViewport(800, 600);
    camera.setCenter({100.0F, 50.0F});
    camera.setZoom(2.0F);

    const engine::Vec2 s = camera.worldToScreen({100.0F, 50.0F});
    REQUIRE(std::fabs(s.x - 400.0F) <= 0.001F);
    REQUIRE(std::fabs(s.y - 300.0F) <= 0.001F);

    camera.pan({10.0F, -20.0F});
    const engine::Vec2 s2 = camera.worldToScreen({110.0F, 30.0F});
    REQUIRE(std::fabs(s2.x - 400.0F) <= 0.001F);
    REQUIRE(std::fabs(s2.y - 300.0F) <= 0.001F);
}
