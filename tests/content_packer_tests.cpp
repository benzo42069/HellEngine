#include <engine/entities.h>
#include <engine/patterns.h>

#include <catch2/catch_test_macros.hpp>

#include <fstream>

#include <nlohmann/json.hpp>

TEST_CASE("Content packer output includes required metadata and content", "[content_packer]") {
    const char* pakCandidates[] = {
        "content_test.pak",
        "../content_test.pak",
    };

    const char* pakPath = nullptr;
    for (const char* candidate : pakCandidates) {
        std::ifstream probe(candidate);
        if (probe.good()) {
            pakPath = candidate;
            break;
        }
    }

    REQUIRE(pakPath != nullptr);

    engine::PatternBank patternBank;
    REQUIRE(patternBank.loadFromFile(pakPath));

    engine::EntityDatabase entityDb;
    REQUIRE(entityDb.loadFromFile(pakPath));

    REQUIRE_FALSE(patternBank.patterns().empty());
    REQUIRE_FALSE(entityDb.templates().empty());

    std::ifstream in(pakPath);
    REQUIRE(in.good());

    nlohmann::json pakJson;
    in >> pakJson;

    REQUIRE(pakJson.contains("importRegistry"));
    REQUIRE(pakJson["importRegistry"].is_array());

    REQUIRE(pakJson.contains("schemaVersion"));
    REQUIRE(pakJson["schemaVersion"].get<int>() == 2);

    REQUIRE(pakJson.contains("packVersion"));
    REQUIRE(pakJson["packVersion"].get<int>() == 4);

    REQUIRE(pakJson.contains("compatibility"));
    REQUIRE(pakJson["compatibility"].is_object());

    const auto& compatibility = pakJson["compatibility"];
    REQUIRE(compatibility.contains("minRuntimePackVersion"));
    REQUIRE(compatibility["minRuntimePackVersion"].get<int>() == 2);
    REQUIRE(compatibility.contains("maxRuntimePackVersion"));
    REQUIRE(compatibility["maxRuntimePackVersion"].get<int>() == 4);

    REQUIRE(pakJson.contains("atlasBuild"));
    REQUIRE(pakJson["atlasBuild"].is_array());

    REQUIRE(pakJson.contains("animationBuild"));
    REQUIRE(pakJson["animationBuild"].is_array());

    REQUIRE(pakJson.contains("variantBuild"));
    REQUIRE(pakJson["variantBuild"].is_array());
}
