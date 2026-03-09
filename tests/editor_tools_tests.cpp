#include <engine/editor_tools.h>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

TEST_CASE("editor tools demo content generation and validation", "[editor_tools]") {
    const std::string outDir = "generated_editor_demo_test";
    std::string error;
    if (!engine::generateDemoContent(outDir, &error)) {
        FAIL("generateDemoContent failed: " << error);
    }

    const std::filesystem::path root(outDir);
    const std::filesystem::path patterns = root / "demo_patterns.json";
    const std::filesystem::path entities = root / "demo_entities.json";
    const std::filesystem::path traits = root / "demo_traits.json";
    const std::filesystem::path waves = root / "demo_waves.json";

    if (!std::filesystem::exists(patterns) || !std::filesystem::exists(entities)
        || !std::filesystem::exists(traits) || !std::filesystem::exists(waves)) {
        std::filesystem::remove_all(root);
        FAIL("generated files missing");
    }


    engine::ToolRuntimeSnapshot snapshot;
    snapshot.spawnsPerSecond = 300.0F;
    const engine::ControlCenterValidationReport report = engine::runControlCenterValidation("missing_test_root", snapshot);
    if (report.issues.empty()) {
        std::filesystem::remove_all(root);
        FAIL("validator should report missing content/perf hazards");
    }

    engine::ToolRuntimeSnapshot okSnapshot;
    const engine::ControlCenterValidationReport generatedReport = engine::runControlCenterValidation(outDir, okSnapshot);
    if (generatedReport.filesScanned == 0) {
        std::filesystem::remove_all(root);
        FAIL("validator should scan generated demo files");
    }

    std::filesystem::remove_all(root);
    SUCCEED("editor_tools_tests passed");
}
