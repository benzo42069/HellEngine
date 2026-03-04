#include <engine/entities.h>
#include <engine/patterns.h>

#include <cstdlib>
#include <iostream>

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

    std::cout << "content_packer_tests passed\n";
    return EXIT_SUCCESS;
}
