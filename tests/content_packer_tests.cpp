#include <engine/entities.h>
#include <engine/patterns.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: content_packer_tests <pak_path>\n";
        return EXIT_FAILURE;
    }

    engine::PatternBank patternBank;
    if (!patternBank.loadFromFile(argv[1])) {
        std::cerr << "failed to load generated pak patterns\n";
        return EXIT_FAILURE;
    }

    engine::EntityDatabase entityDb;
    if (!entityDb.loadFromFile(argv[1])) {
        std::cerr << "failed to load generated pak entities\n";
        return EXIT_FAILURE;
    }

    if (patternBank.patterns().empty() || entityDb.templates().empty()) {
        std::cerr << "generated pak missing expected content\n";
        return EXIT_FAILURE;
    }

    std::ifstream in(argv[1]);
    if (!in.good()) {
        std::cerr << "failed to open generated pak json\n";
        return EXIT_FAILURE;
    }

    nlohmann::json pakJson;
    in >> pakJson;

    if (!pakJson.contains("importRegistry") || !pakJson["importRegistry"].is_array()) {
        std::cerr << "generated pak missing importRegistry\n";
        return EXIT_FAILURE;
    }
    if (!pakJson.contains("schemaVersion") || pakJson["schemaVersion"].get<int>() != 2) {
        std::cerr << "generated pak missing expected schemaVersion=2\n";
        return EXIT_FAILURE;
    }
    if (!pakJson.contains("packVersion") || pakJson["packVersion"].get<int>() != 4) {
        std::cerr << "generated pak missing expected packVersion=4\n";
        return EXIT_FAILURE;
    }
    if (!pakJson.contains("compatibility") || !pakJson["compatibility"].is_object()) {
        std::cerr << "generated pak missing compatibility metadata\n";
        return EXIT_FAILURE;
    }
    const auto& compatibility = pakJson["compatibility"];
    if (!compatibility.contains("minRuntimePackVersion") || compatibility["minRuntimePackVersion"].get<int>() != 2 ||
        !compatibility.contains("maxRuntimePackVersion") || compatibility["maxRuntimePackVersion"].get<int>() != 4) {
        std::cerr << "generated pak compatibility range mismatch\n";
        return EXIT_FAILURE;
    }
    if (!pakJson.contains("atlasBuild") || !pakJson["atlasBuild"].is_array()) {
        std::cerr << "generated pak missing atlasBuild metadata\n";
        return EXIT_FAILURE;
    }
    if (!pakJson.contains("animationBuild") || !pakJson["animationBuild"].is_array()) {
        std::cerr << "generated pak missing animationBuild metadata\n";
        return EXIT_FAILURE;
    }
    if (!pakJson.contains("variantBuild") || !pakJson["variantBuild"].is_array()) {
        std::cerr << "generated pak missing variantBuild metadata\n";
        return EXIT_FAILURE;
    }

    std::cout << "content_packer_tests passed\n";
    return EXIT_SUCCESS;
}
