#include <engine/pattern_signature.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace engine {

namespace {
constexpr float kSimDt = 1.0F / 60.0F;

float hash01(const std::string& text) {
    std::uint32_t h = 2166136261U;
    for (const unsigned char ch : text) {
        h ^= ch;
        h *= 16777619U;
    }
    return static_cast<float>(h & 0x00FFFFFFU) / static_cast<float>(0x00FFFFFFU);
}

void blur3x3(std::vector<float>& buffer, const int size) {
    if (size <= 2) return;
    std::vector<float> scratch(buffer.size(), 0.0F);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float accum = 0.0F;
            float weightSum = 0.0F;
            for (int oy = -1; oy <= 1; ++oy) {
                for (int ox = -1; ox <= 1; ++ox) {
                    const int sx = std::clamp(x + ox, 0, size - 1);
                    const int sy = std::clamp(y + oy, 0, size - 1);
                    const float w = (ox == 0 && oy == 0) ? 4.0F : ((ox == 0 || oy == 0) ? 2.0F : 1.0F);
                    accum += buffer[static_cast<std::size_t>(sy * size + sx)] * w;
                    weightSum += w;
                }
            }
            scratch[static_cast<std::size_t>(y * size + x)] = accum / weightSum;
        }
    }
    buffer.swap(scratch);
}
} // namespace

std::string patternSignatureTextureId(const std::string& graphId) {
    return "sig_" + graphId;
}

std::string PatternSignatureGenerator::generate(
    SDL_Renderer* renderer,
    TextureStore& store,
    const CompiledPatternGraph& graph,
    const std::uint64_t seed,
    int textureSize,
    int simTicks
) {
    if (!renderer || textureSize <= 0 || simTicks <= 0) return {};
    textureSize = std::clamp(textureSize, 8, 64);
    simTicks = std::clamp(simTicks, 8, 360);

    const std::size_t pixelCount = static_cast<std::size_t>(textureSize * textureSize);
    std::vector<float> density(pixelCount, 0.0F);

    PatternGraphVm vm;
    PatternGraphVm::RuntimeState vmState;
    vm.reset(vmState, seed);

    struct EmittedSample {
        ProjectileSpawn spawn;
        int emitTick {0};
    };
    std::vector<EmittedSample> emitted;
    emitted.reserve(static_cast<std::size_t>(simTicks) * 16U);

    const Vec2 origin {0.0F, 0.0F};
    const Vec2 aimTarget {0.0F, -180.0F};
    for (int tick = 0; tick < simTicks; ++tick) {
        vm.execute(graph, vmState, kSimDt, origin, aimTarget, [&](const ProjectileSpawn& spawn) {
            emitted.push_back(EmittedSample {.spawn = spawn, .emitTick = tick});
        });
    }

    float maxRadius = 40.0F;
    for (std::size_t i = 0; i < emitted.size(); ++i) {
        const ProjectileSpawn& spawn = emitted[i].spawn;
        const int remainingTicks = std::max(0, simTicks - emitted[i].emitTick);
        const float forwardT = static_cast<float>(remainingTicks) * kSimDt;
        const Vec2 finalPos {
            spawn.pos.x + spawn.vel.x * forwardT,
            spawn.pos.y + spawn.vel.y * forwardT,
        };
        const float radial = std::sqrt(finalPos.x * finalPos.x + finalPos.y * finalPos.y);
        maxRadius = std::max(maxRadius, radial);
    }

    const float half = static_cast<float>(textureSize - 1) * 0.5F;
    const float scale = half / std::max(1.0F, maxRadius * 1.1F);
    for (std::size_t i = 0; i < emitted.size(); ++i) {
        const ProjectileSpawn& spawn = emitted[i].spawn;
        const int remainingTicks = std::max(0, simTicks - emitted[i].emitTick);
        const float forwardT = static_cast<float>(remainingTicks) * kSimDt;
        const float px = spawn.pos.x + spawn.vel.x * forwardT;
        const float py = spawn.pos.y + spawn.vel.y * forwardT;
        const int x = static_cast<int>(std::lround(half + px * scale));
        const int y = static_cast<int>(std::lround(half + py * scale));
        if (x < 0 || y < 0 || x >= textureSize || y >= textureSize) continue;
        float& cell = density[static_cast<std::size_t>(y * textureSize + x)];
        cell = std::min(1.0F, cell + 0.18F);
    }

    blur3x3(density, textureSize);

    const float hueJitter = hash01(graph.id);
    std::vector<std::uint8_t> rgba(pixelCount * 4U, 0);
    for (std::size_t i = 0; i < pixelCount; ++i) {
        const float t = std::clamp(density[i], 0.0F, 1.0F);
        const float rF = t * (0.8F + 0.2F * hueJitter);
        const float gF = t * (0.95F + 0.05F * hueJitter);
        const float bF = t;
        const float aF = std::pow(t, 0.7F);
        rgba[i * 4U + 0U] = static_cast<std::uint8_t>(std::clamp(rF, 0.0F, 1.0F) * 255.0F);
        rgba[i * 4U + 1U] = static_cast<std::uint8_t>(std::clamp(gF, 0.0F, 1.0F) * 255.0F);
        rgba[i * 4U + 2U] = static_cast<std::uint8_t>(std::clamp(bF, 0.0F, 1.0F) * 255.0F);
        rgba[i * 4U + 3U] = static_cast<std::uint8_t>(std::clamp(aF, 0.0F, 1.0F) * 255.0F);
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, textureSize, textureSize);
    if (!texture) return {};
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    if (SDL_UpdateTexture(texture, nullptr, rgba.data(), textureSize * 4) != 0) {
        SDL_DestroyTexture(texture);
        return {};
    }

    const std::string textureId = patternSignatureTextureId(graph.id);
    store.adoptTexture(textureId, texture, textureSize, textureSize);
    return textureId;
}

} // namespace engine
