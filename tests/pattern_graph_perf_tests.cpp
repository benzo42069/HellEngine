#include <engine/pattern_graph.h>
#include <engine/patterns.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>

int main() {
    engine::PatternGraphAsset asset;
    asset.id = "perf-graph";
    asset.nodes = {
        engine::PatternGraphNode {.id = "010-emit", .type = engine::PatternGraphNodeType::EmitRing, .params = {{"count", 24.0F}, {"speed", 180.0F}, {"radius", 3.0F}, {"angle", 0.0F}}},
        engine::PatternGraphNode {.id = "020-wait", .type = engine::PatternGraphNodeType::Wait, .params = {{"seconds", 0.01F}}},
        engine::PatternGraphNode {.id = "030-loop", .type = engine::PatternGraphNodeType::Loop, .params = {{"count", 64.0F}}, .targetNodeId = "010-emit"},
    };

    engine::PatternGraphCompiler compiler;
    engine::CompiledPatternGraph compiled;
    if (!compiler.compile(asset, compiled)) {
        std::cerr << "graph compile failed\n";
        return EXIT_FAILURE;
    }

    std::ofstream pack("pattern_perf_pack.json");
    pack << R"({"schemaVersion":2,"packVersion":3,"patterns":[{"name":"PerfLegacy","seedOffset":1,"loopSequence":true,"layers":[{"name":"ring","type":"radial","bulletCount":24,"bulletSpeed":180.0,"projectileRadius":3.0,"cooldownSeconds":0.01}],"sequence":[{"durationSeconds":1.0,"activeLayers":[0]}]}]})";
    pack.close();

    engine::PatternBank bank;
    if (!bank.loadFromFile("pattern_perf_pack.json")) {
        std::cerr << "legacy pattern load failed\n";
        return EXIT_FAILURE;
    }

    engine::PatternPlayer player;
    player.setBank(&bank);
    player.setRunSeed(42);

    engine::PatternGraphVm vm;
    engine::PatternGraphVm::RuntimeState vmState;
    vm.reset(vmState, 42);

    constexpr int kTicks = 4000;
    std::uint64_t vmCount = 0;
    std::uint64_t legacyCount = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kTicks; ++i) {
        vm.execute(compiled, vmState, 1.0F / 60.0F, {0.0F, 0.0F}, {50.0F, 0.0F}, &vmCount, [](void* user, const engine::ProjectileSpawn&) {
            ++(*static_cast<std::uint64_t*>(user));
        });
    }
    auto t1 = std::chrono::steady_clock::now();

    auto t2 = std::chrono::steady_clock::now();
    for (int i = 0; i < kTicks; ++i) {
        player.update(1.0F / 60.0F, {0.0F, 0.0F}, {50.0F, 0.0F}, [&](const engine::ProjectileSpawn&) {
            ++legacyCount;
        });
    }
    auto t3 = std::chrono::steady_clock::now();

    if (vmCount == 0 || legacyCount == 0) {
        std::cerr << "invalid benchmark counts\n";
        return EXIT_FAILURE;
    }

    const auto vmNs = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    const auto legacyNs = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();
    std::cout << "vm_ns=" << vmNs << " legacy_ns=" << legacyNs << " vm_emits=" << vmCount << " legacy_emits=" << legacyCount << "\n";

    if (vmNs >= legacyNs) {
        std::cerr << "compiled graph VM did not outperform legacy interpreted pattern\n";
        return EXIT_FAILURE;
    }

    std::cout << "pattern_graph_perf_tests passed\n";
    return EXIT_SUCCESS;
}
