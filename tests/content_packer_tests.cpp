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
    if (!pakJson.contains("atlasBuild") || !pakJson["atlasBuild"].is_array()) {
        std::cerr << "generated pak missing atlasBuild metadata\n";
        return EXIT_FAILURE;
    }

    std::cout << "content_packer_tests passed\n";
    return EXIT_SUCCESS;
}
