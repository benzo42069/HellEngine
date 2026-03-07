#include <engine/patterns.h>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

TEST_CASE("Malformed JSON does not crash pattern loader", "[fuzz]") {
    const std::string oneMegGarbage(1024 * 1024, 'x');

    const std::vector<std::string> malformedInputs = {
        "",
        "{}",
        "null",
        R"({"patterns": 1})",
        R"({"patterns": [{"name": 42, "layers": "oops", "sequence": -1}]})",
        R"({"packId": -1, "patterns": [{"name": "n", "bulletCount": -5}]})",
        oneMegGarbage,
    };

    for (const std::string& input : malformedInputs) {
        engine::PatternBank bank;
        REQUIRE_NOTHROW((void)bank.loadFromString(input));
    }
}
