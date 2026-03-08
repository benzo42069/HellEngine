#include <engine/editor_tools.h>

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <iostream>

int main() {
    const std::string outDir = "generated_editor_demo_test";
    std::string error;
    if (!engine::generateDemoContent(outDir, &error)) {
        std::cerr << "generateDemoContent failed: " << error << "\n";
        return EXIT_FAILURE;
    }

    const std::filesystem::path root(outDir);
    const std::filesystem::path patterns = root / "demo_patterns.json";
    const std::filesystem::path entities = root / "demo_entities.json";
    const std::filesystem::path traits = root / "demo_traits.json";
    const std::filesystem::path waves = root / "demo_waves.json";

    if (!std::filesystem::exists(patterns) || !std::filesystem::exists(entities)
        || !std::filesystem::exists(traits) || !std::filesystem::exists(waves)) {
        std::cerr << "generated files missing\n";
        return EXIT_FAILURE;
    }


    engine::ToolRuntimeSnapshot snapshot;
    snapshot.spawnsPerSecond = 300.0F;
    const engine::ControlCenterValidationReport report = engine::runControlCenterValidation("missing_test_root", snapshot);
    if (report.issues.empty()) {
        std::cerr << "validator should report missing content/perf hazards\n";
        return EXIT_FAILURE;
    }

    engine::ToolRuntimeSnapshot okSnapshot;
    const engine::ControlCenterValidationReport generatedReport = engine::runControlCenterValidation(outDir, okSnapshot);
    if (generatedReport.filesScanned == 0) {
        std::cerr << "validator should scan generated demo files\n";
        return EXIT_FAILURE;
    }

    std::filesystem::remove_all(root);
    std::cout << "editor_tools_tests passed\n";
    return EXIT_SUCCESS;
}
