#include <engine/diagnostics.h>
#include <engine/logging.h>
#include <engine/patterns.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

int main() {
    engine::ErrorReport report = engine::makeError(engine::ErrorCode::HotReloadFailed, "reload failed");
    engine::addContext(report, "path", "broken.json");
    engine::pushStack(report, "Runtime::simTick/hotReload");
    const std::string payload = engine::toJson(report);
    if (payload.find("hot_reload_failed") == std::string::npos || payload.find("broken.json") == std::string::npos) {
        std::cerr << "error json payload missing expected fields\n";
        return EXIT_FAILURE;
    }

    engine::logInfo("diagnostics_tests_line_1");
    engine::logWarn("diagnostics_tests_line_2");
    const auto tail = engine::Logger::instance().recentLines(8);
    if (tail.empty()) {
        std::cerr << "logger tail unexpectedly empty\n";
        return EXIT_FAILURE;
    }

    {
        std::ofstream out("broken_patterns.json");
        out << "{ this is not valid json";
    }
    engine::PatternBank bank;
    if (bank.loadFromFile("broken_patterns.json")) {
        std::cerr << "invalid pattern json should not load\n";
        return EXIT_FAILURE;
    }

    std::cout << "diagnostics_tests passed\n";
    return EXIT_SUCCESS;
}
