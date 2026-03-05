#include <engine/projectiles.h>
#include <engine/render2d.h>

#include <SDL.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

using Clock = std::chrono::steady_clock;

double runBulletStress(const std::uint32_t bulletCount, const int ticks) {
    engine::ProjectileSystem ps;
    ps.initialize(bulletCount + 1024U, 420.0F, 32, 18);

    const auto t0 = Clock::now();
    for (int i = 0; i < ticks; ++i) {
        ps.clear();
        ps.spawnRadialBurst(bulletCount, 120.0F, 2.0F, static_cast<std::uint64_t>(i));
        ps.beginTick();
        ps.update(1.0F / 60.0F, {500.0F, 500.0F}, 8.0F);
    }
    const auto t1 = Clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

double runCollisionHeavy(const std::uint32_t bulletCount, const int ticks) {
    engine::ProjectileSystem ps;
    ps.initialize(bulletCount + 1024U, 420.0F, 32, 18);

    const auto t0 = Clock::now();
    for (int i = 0; i < ticks; ++i) {
        ps.clear();
        for (std::uint32_t b = 0; b < bulletCount; ++b) {
            (void)ps.spawn(engine::ProjectileSpawn {.pos = {0.0F, 0.0F}, .vel = {0.0F, 0.0F}, .radius = 2.0F});
        }
        ps.beginTick();
        ps.update(1.0F / 60.0F, {0.0F, 0.0F}, 12.0F);
    }
    const auto t1 = Clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

double runRenderHeavy(const int spritesPerFrame, const int frames) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return -1.0;
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    SDL_Window* window = SDL_CreateWindow("bench", 0, 0, 640, 360, SDL_WINDOW_HIDDEN);
    if (!window) {
        SDL_Quit();
        return -1.0;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1.0;
    }

    engine::TextureStore textures(renderer);
    (void)textures.createSolidTexture("white", 4, 4, engine::Color {255, 255, 255, 255});
    engine::Camera2D cam;
    cam.setViewport(640, 360);

    engine::SpriteBatch batch;
    batch.reserve(static_cast<std::size_t>(spritesPerFrame));

    const auto t0 = Clock::now();
    for (int f = 0; f < frames; ++f) {
        batch.begin(cam);
        for (int i = 0; i < spritesPerFrame; ++i) {
            const float x = static_cast<float>((i % 320) - 160);
            const float y = static_cast<float>((i / 320) - 90);
            batch.draw(engine::SpriteDrawCmd {.textureId = "white", .dest = SDL_FRect {x, y, 4.0F, 4.0F}});
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        batch.flush(renderer, textures);
        SDL_RenderPresent(renderer);
    }
    const auto t1 = Clock::now();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

} // namespace

int main(int argc, char** argv) {
    bool assertThresholds = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--assert-thresholds") assertThresholds = true;
    }

    const double ms10k = runBulletStress(10000U, 80);
    const double ms25k = runBulletStress(25000U, 60);
    const double ms50k = runBulletStress(50000U, 40);
    const double msCollision = runCollisionHeavy(25000U, 60);
    const double msRender = runRenderHeavy(50000, 30);

    std::cout << "bench_10k_ms=" << ms10k << "\n";
    std::cout << "bench_25k_ms=" << ms25k << "\n";
    std::cout << "bench_50k_ms=" << ms50k << "\n";
    std::cout << "bench_collision_ms=" << msCollision << "\n";
    std::cout << "bench_render_ms=" << msRender << "\n";

    if (assertThresholds) {
        if (ms10k <= 0.0 || ms10k > 250.0) return EXIT_FAILURE;
        if (ms25k <= 0.0 || ms25k > 450.0) return EXIT_FAILURE;
        if (ms50k <= 0.0 || ms50k > 700.0) return EXIT_FAILURE;
        if (msCollision <= 0.0 || msCollision > 500.0) return EXIT_FAILURE;
        if (msRender <= 0.0 || msRender > 2200.0) return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
