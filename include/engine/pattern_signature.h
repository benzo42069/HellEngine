#pragma once

#include <engine/pattern_graph.h>
#include <engine/render2d.h>

#include <cstdint>
#include <string>

namespace engine {

[[nodiscard]] std::string patternSignatureTextureId(const std::string& graphId);

class PatternSignatureGenerator {
  public:
    std::string generate(
        SDL_Renderer* renderer,
        TextureStore& store,
        const CompiledPatternGraph& graph,
        std::uint64_t seed,
        int textureSize = 64,
        int simTicks = 80
    );
};

} // namespace engine
