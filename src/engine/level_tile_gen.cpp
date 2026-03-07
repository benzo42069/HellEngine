#include <engine/level_tile_gen.h>

#include <SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace engine {
namespace {

[[nodiscard]] std::uint64_t mix64(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27U)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31U);
}

[[nodiscard]] std::uint64_t hashCombine(const std::uint64_t a, const std::uint64_t b, const std::uint64_t c) {
    return mix64(a ^ mix64(b + 0x9e3779b97f4a7c15ULL) ^ mix64(c + 0x243f6a8885a308d3ULL));
}

[[nodiscard]] float hash01(const std::uint64_t seed, const int x, const int y) {
    const std::uint64_t h = hashCombine(seed, static_cast<std::uint64_t>(x), static_cast<std::uint64_t>(y));
    constexpr double inv = 1.0 / static_cast<double>(std::numeric_limits<std::uint64_t>::max());
    return static_cast<float>(static_cast<double>(h) * inv);
}

[[nodiscard]] int wrapIndex(const int index, const int size) {
    const int m = index % size;
    return m < 0 ? m + size : m;
}

[[nodiscard]] std::uint8_t boolToByte(const bool v) { return static_cast<std::uint8_t>(v ? 1 : 0); }

[[nodiscard]] std::array<bool, 9> birthSurvivalMask(const TileGenRule rule) {
    std::array<bool, 9> mask {};
    switch (rule) {
        case TileGenRule::CaveFormation:
            mask[6] = mask[7] = mask[8] = true;
            break;
        case TileGenRule::CrystalGrowth:
        case TileGenRule::MazeCorridors:
            mask[3] = true;
            break;
        case TileGenRule::OrganicNoise:
            mask[4] = true;
            break;
        case TileGenRule::CircuitBoard:
            mask[2] = mask[3] = true;
            break;
        case TileGenRule::VoidScatter:
            mask[1] = true;
            break;
    }
    return mask;
}

[[nodiscard]] std::array<bool, 9> surviveMask(const TileGenRule rule) {
    std::array<bool, 9> mask {};
    switch (rule) {
        case TileGenRule::CaveFormation:
            mask[3] = mask[4] = mask[5] = mask[6] = mask[7] = mask[8] = true;
            break;
        case TileGenRule::CrystalGrowth:
            mask[1] = mask[2] = mask[3] = mask[4] = true;
            break;
        case TileGenRule::MazeCorridors:
            mask[1] = mask[2] = mask[3] = mask[4] = mask[5] = true;
            break;
        case TileGenRule::OrganicNoise:
            mask[2] = mask[3] = mask[4] = mask[5] = true;
            break;
        case TileGenRule::CircuitBoard:
            mask[1] = mask[2] = mask[3] = true;
            break;
        case TileGenRule::VoidScatter:
            mask[0] = mask[1] = true;
            break;
    }
    return mask;
}

[[nodiscard]] std::uint8_t sampleGrid(const std::vector<std::uint8_t>& grid, const int width, const int height, const int x, const int y) {
    const int wx = wrapIndex(x, width);
    const int wy = wrapIndex(y, height);
    return grid[static_cast<std::size_t>(wy * width + wx)];
}

void runCellularAutomata(std::vector<std::uint8_t>& grid, const TileGenParams& params) {
    if (params.gridWidth <= 0 || params.gridHeight <= 0) return;

    const auto birth = birthSurvivalMask(params.rule);
    const auto survive = surviveMask(params.rule);
    std::vector<std::uint8_t> scratch(static_cast<std::size_t>(params.gridWidth * params.gridHeight), 0U);

    for (int i = 0; i < std::max(params.iterations, 0); ++i) {
        for (int y = 0; y < params.gridHeight; ++y) {
            for (int x = 0; x < params.gridWidth; ++x) {
                int neighbors = 0;
                for (int oy = -1; oy <= 1; ++oy) {
                    for (int ox = -1; ox <= 1; ++ox) {
                        if (ox == 0 && oy == 0) continue;
                        neighbors += static_cast<int>(sampleGrid(grid, params.gridWidth, params.gridHeight, x + ox, y + oy));
                    }
                }

                const bool alive = sampleGrid(grid, params.gridWidth, params.gridHeight, x, y) != 0;
                const bool next = alive ? survive[neighbors] : birth[neighbors];
                scratch[static_cast<std::size_t>(y * params.gridWidth + x)] = boolToByte(next);
            }
        }
        grid.swap(scratch);
    }
}

[[nodiscard]] float cubicSmooth(const float t) {
    const float c = std::clamp(t, 0.0F, 1.0F);
    return c * c * (3.0F - 2.0F * c);
}

[[nodiscard]] float valueNoiseTileable(const int x, const int y, const int width, const int height, const std::uint64_t seed, const int period) {
    const int p = std::max(period, 1);
    const float fx = static_cast<float>(x) / static_cast<float>(width) * static_cast<float>(p);
    const float fy = static_cast<float>(y) / static_cast<float>(height) * static_cast<float>(p);

    const int x0 = static_cast<int>(std::floor(fx));
    const int y0 = static_cast<int>(std::floor(fy));
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;

    const float tx = cubicSmooth(fx - static_cast<float>(x0));
    const float ty = cubicSmooth(fy - static_cast<float>(y0));

    const float n00 = hash01(seed, wrapIndex(x0, p), wrapIndex(y0, p));
    const float n10 = hash01(seed, wrapIndex(x1, p), wrapIndex(y0, p));
    const float n01 = hash01(seed, wrapIndex(x0, p), wrapIndex(y1, p));
    const float n11 = hash01(seed, wrapIndex(x1, p), wrapIndex(y1, p));

    const float nx0 = n00 + (n10 - n00) * tx;
    const float nx1 = n01 + (n11 - n01) * tx;
    return nx0 + (nx1 - nx0) * ty;
}

[[nodiscard]] PaletteColor lerpColor(const PaletteColor& a, const PaletteColor& b, const float t) {
    const float u = std::clamp(t, 0.0F, 1.0F);
    return {
        .r = a.r + (b.r - a.r) * u,
        .g = a.g + (b.g - a.g) * u,
        .b = a.b + (b.b - a.b) * u,
        .a = a.a + (b.a - a.a) * u,
    };
}

[[nodiscard]] std::uint8_t toByte(const float v) {
    const int scaled = static_cast<int>(std::lround(std::clamp(v, 0.0F, 1.0F) * 255.0F));
    return static_cast<std::uint8_t>(std::clamp(scaled, 0, 255));
}

[[nodiscard]] SDL_Texture* createTextureFromPixels(SDL_Renderer* renderer, const int size, const std::vector<std::uint8_t>& pixels) {
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, size, size);
    if (!texture) return nullptr;
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    if (SDL_UpdateTexture(texture, nullptr, pixels.data(), size * 4) != 0) {
        SDL_DestroyTexture(texture);
        return nullptr;
    }
    return texture;
}

[[nodiscard]] float edgeFactor(const std::vector<std::uint8_t>& grid, const int w, const int h, const int cx, const int cy) {
    const std::uint8_t center = sampleGrid(grid, w, h, cx, cy);
    int diff = 0;
    diff += center != sampleGrid(grid, w, h, cx + 1, cy) ? 1 : 0;
    diff += center != sampleGrid(grid, w, h, cx - 1, cy) ? 1 : 0;
    diff += center != sampleGrid(grid, w, h, cx, cy + 1) ? 1 : 0;
    diff += center != sampleGrid(grid, w, h, cx, cy - 1) ? 1 : 0;
    return static_cast<float>(diff) * 0.25F;
}

[[nodiscard]] TileGenRule zoneRule(const ZoneType zone) {
    switch (zone) {
        case ZoneType::Combat: return TileGenRule::CaveFormation;
        case ZoneType::Elite: return TileGenRule::CrystalGrowth;
        case ZoneType::Event: return TileGenRule::OrganicNoise;
        case ZoneType::Boss: return TileGenRule::CircuitBoard;
    }
    return TileGenRule::OrganicNoise;
}

[[nodiscard]] PaletteColor accentFromSeed(const std::uint64_t seed) {
    const float r = 0.35F + hash01(seed, 1, 0) * 0.65F;
    const float g = 0.35F + hash01(seed, 2, 0) * 0.65F;
    const float b = 0.35F + hash01(seed, 3, 0) * 0.65F;
    return {r, g, b, 1.0F};
}

} // namespace

std::string LevelTileGenerator::generate(
    SDL_Renderer* renderer,
    TextureStore& store,
    const std::string& idPrefix,
    const TileGenParams& params
) {
    if (!renderer || params.gridWidth <= 0 || params.gridHeight <= 0 || params.textureSize <= 0) return {};

    std::vector<std::uint8_t> grid(static_cast<std::size_t>(params.gridWidth * params.gridHeight), 0U);

    for (int y = 0; y < params.gridHeight; ++y) {
        for (int x = 0; x < params.gridWidth; ++x) {
            const float n = hash01(params.seed, x, y);
            const bool alive = n < std::clamp(params.initialDensity, 0.0F, 1.0F);
            grid[static_cast<std::size_t>(y * params.gridWidth + x)] = boolToByte(alive);
        }
    }

    if (params.rule != TileGenRule::OrganicNoise) {
        runCellularAutomata(grid, params);
    }

    const int texSize = params.textureSize;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(texSize * texSize * 4), 0U);

    for (int py = 0; py < texSize; ++py) {
        for (int px = 0; px < texSize; ++px) {
            const float gx = (static_cast<float>(px) / static_cast<float>(texSize)) * static_cast<float>(params.gridWidth);
            const float gy = (static_cast<float>(py) / static_cast<float>(texSize)) * static_cast<float>(params.gridHeight);

            const int x0 = static_cast<int>(std::floor(gx));
            const int y0 = static_cast<int>(std::floor(gy));
            const int x1 = x0 + 1;
            const int y1 = y0 + 1;

            const float tx = gx - static_cast<float>(x0);
            const float ty = gy - static_cast<float>(y0);

            const float v00 = static_cast<float>(sampleGrid(grid, params.gridWidth, params.gridHeight, x0, y0));
            const float v10 = static_cast<float>(sampleGrid(grid, params.gridWidth, params.gridHeight, x1, y0));
            const float v01 = static_cast<float>(sampleGrid(grid, params.gridWidth, params.gridHeight, x0, y1));
            const float v11 = static_cast<float>(sampleGrid(grid, params.gridWidth, params.gridHeight, x1, y1));

            const float ix0 = v00 + (v10 - v00) * tx;
            const float ix1 = v01 + (v11 - v01) * tx;
            float field = ix0 + (ix1 - ix0) * ty;

            if (params.rule == TileGenRule::OrganicNoise) {
                field = valueNoiseTileable(px, py, texSize, texSize, params.seed ^ 0xBADC0FFEEULL, 7);
            } else if (params.rule == TileGenRule::CircuitBoard) {
                const float lineX = std::abs((tx - 0.5F) * 2.0F);
                const float lineY = std::abs((ty - 0.5F) * 2.0F);
                const float line = std::min(lineX, lineY);
                field = std::clamp(field * 0.6F + (1.0F - line) * 0.8F, 0.0F, 1.0F);
            } else if (params.rule == TileGenRule::VoidScatter) {
                field = hash01(params.seed ^ 0x5543ULL, px, py) > 0.985F ? 1.0F : 0.0F;
            }

            const int cx = wrapIndex(static_cast<int>(std::floor(gx)), params.gridWidth);
            const int cy = wrapIndex(static_cast<int>(std::floor(gy)), params.gridHeight);
            const float edge = edgeFactor(grid, params.gridWidth, params.gridHeight, cx, cy);

            PaletteColor color = lerpColor(params.secondary, params.primary, field);
            color = lerpColor(color, params.accent, std::clamp(edge * 0.35F, 0.0F, 0.35F));

            const float noise = (hash01(params.seed ^ 0xCAFEF00DULL, wrapIndex(px, texSize), wrapIndex(py, texSize)) - 0.5F) * 0.10F;
            color.r = std::clamp(color.r * (1.0F + noise), 0.0F, 1.0F);
            color.g = std::clamp(color.g * (1.0F + noise), 0.0F, 1.0F);
            color.b = std::clamp(color.b * (1.0F + noise), 0.0F, 1.0F);

            const std::size_t i = static_cast<std::size_t>((py * texSize + px) * 4);
            pixels[i + 0] = toByte(color.r);
            pixels[i + 1] = toByte(color.g);
            pixels[i + 2] = toByte(color.b);
            pixels[i + 3] = toByte(color.a);
        }
    }

    SDL_Texture* texture = createTextureFromPixels(renderer, texSize, pixels);
    if (!texture) return {};

    const std::string id = idPrefix + ".stage_" + std::to_string(params.seed & 0xFFFFFFFFULL);
    store.adoptTexture(id, texture, texSize, texSize);
    return id;
}

std::string LevelTileGenerator::generateForZone(
    SDL_Renderer* renderer,
    TextureStore& store,
    const ZoneType zone,
    const std::uint32_t stageIndex,
    const std::uint64_t runSeed
) {
    const std::uint64_t zoneSeed = hashCombine(runSeed, static_cast<std::uint64_t>(stageIndex), static_cast<std::uint64_t>(zone));
    const PaletteColor accent = accentFromSeed(zoneSeed ^ 0x9A1F5D43ULL);
    const PaletteFillResult fill = deriveBackgroundFillFromAccent(accent);

    TileGenParams params;
    params.rule = zoneRule(zone);
    params.seed = zoneSeed;
    params.gridWidth = 64;
    params.gridHeight = 64;
    params.iterations = 4 + static_cast<int>((zoneSeed >> 9U) & 0x3ULL);
    params.initialDensity = 0.35F + hash01(zoneSeed, 8, 13) * 0.25F;
    params.primary = fill.highlight;
    params.secondary = fill.core;
    params.accent = fill.glow;
    params.textureSize = 256;

    const std::string idPrefix = "bg.proc." + std::to_string(stageIndex) + "." + toString(zone);
    return generate(renderer, store, idPrefix, params);
}

} // namespace engine
