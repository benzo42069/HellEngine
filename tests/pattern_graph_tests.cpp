#include <engine/pattern_graph.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

TEST_CASE("Pattern graph compile execute and roundtrip", "[pattern_graph]") {
    engine::PatternGraphAsset asset;
    asset.id = "determinism-graph";
    asset.nodes = {
        engine::PatternGraphNode {.id = "001-random", .type = engine::PatternGraphNodeType::RandomRange, .params = {{"min", -10.0F}, {"max", 10.0F}, {"stream", 2.0F}}},
        engine::PatternGraphNode {.id = "010-emit", .type = engine::PatternGraphNodeType::EmitRing, .params = {{"count", 8.0F}, {"speed", 120.0F}, {"radius", 2.5F}, {"angle", 0.0F}}},
        engine::PatternGraphNode {.id = "020-wait", .type = engine::PatternGraphNodeType::Wait, .params = {{"seconds", 0.05F}}},
        engine::PatternGraphNode {.id = "030-loop", .type = engine::PatternGraphNodeType::Loop, .params = {{"count", 4.0F}}, .targetNodeId = "010-emit"},
    };

    engine::CompiledPatternGraph compiled;
    engine::PatternGraphCompiler compiler;
    REQUIRE(compiler.compile(asset, compiled));

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

    REQUIRE(outA.size() == outB.size());
    for (std::size_t i = 0; i < outA.size(); ++i) {
        REQUIRE(std::abs(outA[i].vel.x - outB[i].vel.x) <= 0.0001F);
        REQUIRE(std::abs(outA[i].vel.y - outB[i].vel.y) <= 0.0001F);
    }

    std::string err;
    REQUIRE(engine::savePatternGraphsToFile("pattern_graph_roundtrip.json", {asset}, &err));
    std::vector<engine::PatternGraphAsset> loadedAssets;
    std::vector<engine::PatternGraphDiagnostic> loadDiag;
    REQUIRE(engine::loadPatternGraphsFromFile("pattern_graph_roundtrip.json", loadedAssets, loadDiag));
    REQUIRE(!loadedAssets.empty());

    const engine::PatternGraphAsset migrated = engine::migrateLegacyPatternToGraph("migrated", 0.12F, 12, 150.0F);
    engine::CompiledPatternGraph migratedCompiled;
    REQUIRE(compiler.compile(migrated, migratedCompiled));
}
