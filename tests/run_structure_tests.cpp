#include <engine/run_structure.h>

#include <cstdlib>
#include <iostream>

int main() {
    engine::RunStructure run;
    run.initializeDefaults();

    if (run.stageCount() != 2) {
        std::cerr << "expected 2 stages\n";
        return EXIT_FAILURE;
    }
    const auto& stages = run.stages();
    if (stages[0].zones.size() != 6) {
        std::cerr << "expected 6 zones in stage\n";
        return EXIT_FAILURE;
    }

    int combatCount = 0;
    int eliteCount = 0;
    int eventCount = 0;
    int bossCount = 0;
    for (const auto& z : stages[0].zones) {
        if (z.type == engine::ZoneType::Combat) ++combatCount;
        if (z.type == engine::ZoneType::Elite) ++eliteCount;
        if (z.type == engine::ZoneType::Event) ++eventCount;
        if (z.type == engine::ZoneType::Boss) ++bossCount;
    }
    if (combatCount != 3 || eliteCount != 1 || eventCount != 1 || bossCount != 1) {
        std::cerr << "stage zone composition mismatch\n";
        return EXIT_FAILURE;
    }


    engine::StageDefinition authoredStage;
    authoredStage.name = "Vertical Slice - Ember Crossing";
    authoredStage.zones = {
        engine::ZoneDefinition {.type = engine::ZoneType::Combat, .durationSeconds = 45.0F},
        engine::ZoneDefinition {.type = engine::ZoneType::Elite, .durationSeconds = 55.0F},
        engine::ZoneDefinition {.type = engine::ZoneType::Event, .durationSeconds = 35.0F},
        engine::ZoneDefinition {.type = engine::ZoneType::Boss, .durationSeconds = 120.0F},
    };
    run.setStages({authoredStage});

    if (run.stageCount() != 1 || run.stages()[0].zones.size() != 4) {
        std::cerr << "authored stage load mismatch\n";
        return EXIT_FAILURE;
    }
    if (run.currentZone()->type != engine::ZoneType::Combat || run.zoneTimeRemaining() != 45.0F) {
        std::cerr << "authored stage initial zone mismatch\n";
        return EXIT_FAILURE;
    }

    run.reset();
    for (int i = 0; i < 8000 && run.state() == engine::RunState::InProgress; ++i) {
        const auto* zone = run.currentZone();
        const unsigned bossDefeats = (zone && zone->type == engine::ZoneType::Boss) ? 99U : 0U;
        run.update(1.0F / 60.0F, bossDefeats, true);
    }

    if (run.state() != engine::RunState::Completed) {
        std::cerr << "run did not complete when boss zones were cleared\n";
        return EXIT_FAILURE;
    }

    run.reset();
    run.update(0.016F, 0, false);
    if (run.state() != engine::RunState::Failed) {
        std::cerr << "run failure state not triggered\n";
        return EXIT_FAILURE;
    }

    std::cout << "run_structure_tests passed\n";
    return EXIT_SUCCESS;
}
