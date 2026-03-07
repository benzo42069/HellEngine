#pragma once

#include <engine/palette_fx_templates.h>
#include <engine/render2d.h>
#include <engine/run_structure.h>

#include <SDL.h>

#include <cstdint>
#include <string>

namespace engine {

enum class TileGenRule : std::uint8_t {
    CaveFormation,
    CrystalGrowth,
    MazeCorridors,
    OrganicNoise,
    CircuitBoard,
    VoidScatter,
};

struct TileGenParams {
    TileGenRule rule {TileGenRule::OrganicNoise};
    std::uint64_t seed {0};
    int gridWidth {64};
    int gridHeight {64};
    int iterations {4};
    float initialDensity {0.45F};
    PaletteColor primary {};
    PaletteColor secondary {};
    PaletteColor accent {};
    int textureSize {256};
};

class LevelTileGenerator {
  public:
    std::string generate(SDL_Renderer* renderer, TextureStore& store, const std::string& idPrefix, const TileGenParams& params);

    std::string generateForZone(
        SDL_Renderer* renderer,
        TextureStore& store,
        ZoneType zone,
        std::uint32_t stageIndex,
        std::uint64_t runSeed
    );
};

} // namespace engine
