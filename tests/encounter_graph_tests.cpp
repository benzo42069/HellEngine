#include <engine/encounter_graph.h>

#include <cstdlib>
#include <iostream>

int main() {
    engine::EncounterGraphAsset asset;
    asset.id = "stage-1";
    asset.nodes = {
        engine::EncounterNode {.id = "a", .type = engine::EncounterNodeType::Wave, .timeSeconds = 2.0F, .value = 40.0F, .payload = "drone"},
        engine::EncounterNode {.id = "b", .type = engine::EncounterNodeType::Delay, .timeSeconds = 4.0F, .durationSeconds = 3.0F},
        engine::EncounterNode {.id = "c", .type = engine::EncounterNodeType::EliteMarker, .timeSeconds = 6.0F, .value = 1.0F, .payload = "elite-lancer"},
        engine::EncounterNode {.id = "d", .type = engine::EncounterNodeType::DifficultyScalar, .timeSeconds = 8.0F, .value = 1.4F},
        engine::EncounterNode {.id = "e", .type = engine::EncounterNodeType::BossTrigger, .timeSeconds = 42.0F, .payload = "boss-alpha"},
        engine::EncounterNode {.id = "f", .type = engine::EncounterNodeType::Telegraph, .timeSeconds = 41.0F, .durationSeconds = 1.0F, .payload = "boss-alpha:phase1"},
        engine::EncounterNode {.id = "g", .type = engine::EncounterNodeType::HazardSync, .timeSeconds = 41.5F, .durationSeconds = 2.0F, .payload = "arena-lava"},
    };

    engine::EncounterCompiler compiler;
    engine::EncounterSchedule scheduleA;
    engine::EncounterSchedule scheduleB;

    if (!compiler.compile(asset, scheduleA) || !compiler.compile(asset, scheduleB)) {
        std::cerr << "encounter compile failed\n";
        return EXIT_FAILURE;
    }

    if (scheduleA.events.size() != scheduleB.events.size()) {
        std::cerr << "encounter compile not deterministic size\n";
        return EXIT_FAILURE;
    }

    bool foundTelegraph = false;
    bool foundHazardSync = false;
    for (std::size_t i = 0; i < scheduleA.events.size(); ++i) {
        if (scheduleA.events[i].atSeconds != scheduleB.events[i].atSeconds || scheduleA.events[i].type != scheduleB.events[i].type) {
            std::cerr << "encounter compile not deterministic order\n";
            return EXIT_FAILURE;
        }
        if (scheduleA.events[i].type == engine::EncounterNodeType::Telegraph) {
            foundTelegraph = true;
            if (scheduleA.events[i].owner != "boss" || scheduleA.events[i].durationSeconds <= 0.0F) {
                std::cerr << "telegraph owner or duration invalid\n";
                return EXIT_FAILURE;
            }
        }
        if (scheduleA.events[i].type == engine::EncounterNodeType::HazardSync) {
            foundHazardSync = true;
            if (scheduleA.events[i].owner != "hazards") {
                std::cerr << "hazard sync owner invalid\n";
                return EXIT_FAILURE;
            }
        }
    }

    if (!foundTelegraph || !foundHazardSync) {
        std::cerr << "missing telegraph or hazard sync events\n";
        return EXIT_FAILURE;
    }

    std::string err;
    const std::string testPath = "encounter_test.json";
    if (!engine::saveEncounterGraphJson(asset, testPath, &err)) {
        std::cerr << "save failed: " << err << "\n";
        return EXIT_FAILURE;
    }

    engine::EncounterGraphAsset loaded;
    if (!engine::loadEncounterGraphJson(testPath, loaded, &err)) {
        std::cerr << "load failed: " << err << "\n";
        return EXIT_FAILURE;
    }

    if (loaded.nodes.size() != asset.nodes.size()) {
        std::cerr << "load/save mismatch\n";
        return EXIT_FAILURE;
    }

    std::cout << "encounter_graph_tests passed\n";
    return EXIT_SUCCESS;
}
