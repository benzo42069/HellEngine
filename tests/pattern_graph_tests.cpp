#include <engine/pattern_graph.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
    engine::PatternGraphAsset asset;
    asset.id = "determinism-graph";
    asset.nodes = {
        engine::PatternGraphNode {.id = "001-random", .type = engine::PatternGraphNodeType::RandomRange, .params = {{"min", -10.0F}, {"max", 10.0F}, {"stream", 2.0F}}},
        engine::PatternGraphNode {.id = "010-emit", .type = engine::PatternGraphNodeType::EmitRing, .params = {{"count", 8.0F}, {"speed", 120.0F}, {"radius", 2.5F}, {"angle", 0.0F}}},
        engine::PatternGraphNode {.id = "020-wait", .type = engine::PatternGraphNodeType::Wait, .params = {{"seconds", 0.05F}}},
        engine::PatternGraphNode {.id = "030-loop", .type = engine::PatternGraphNodeType::Loop, .params = {{"count", 4.0F}, {"target", 0.0F}}},
    };

    engine::CompiledPatternGraph compiled;
    engine::PatternGraphCompiler compiler;
    if (!compiler.compile(asset, compiled)) {
        std::cerr << "compile failed\n";
        return EXIT_FAILURE;
    }

    engine::PatternGraphVm vm;
    engine::PatternGraphVm::RuntimeState a;
    engine::PatternGraphVm::RuntimeState b;
    vm.reset(a, 1337);
    vm.reset(b, 1337);

    std::vector<engine::ProjectileSpawn> outA;
    std::vector<engine::ProjectileSpawn> outB;

    for (int i = 0; i < 40; ++i) {
        vm.execute(compiled, a, 1.0F / 60.0F, {0.0F, 0.0F}, {50.0F, -10.0F}, [&](const engine::ProjectileSpawn& p) { outA.push_back(p); });
        vm.execute(compiled, b, 1.0F / 60.0F, {0.0F, 0.0F}, {50.0F, -10.0F}, [&](const engine::ProjectileSpawn& p) { outB.push_back(p); });
    }

    if (outA.size() != outB.size()) {
        std::cerr << "vm emit count mismatch\n";
        return EXIT_FAILURE;
    }
    for (std::size_t i = 0; i < outA.size(); ++i) {
        if (std::abs(outA[i].vel.x - outB[i].vel.x) > 0.0001F || std::abs(outA[i].vel.y - outB[i].vel.y) > 0.0001F) {
            std::cerr << "vm deterministic output mismatch\n";
            return EXIT_FAILURE;
        }
    }

    std::cout << "pattern_graph_tests passed\n";
    return EXIT_SUCCESS;
}
