#include <engine/content_pipeline.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
    const std::string pngPath0 = "hero_walk_down_000.png";
    const std::string pngPath1 = "hero_walk_down_001.png";
    const unsigned char bytes[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x04, 0x00, 0x00, 0x00, 0xB5, 0x1C, 0x0C,
        0x02, 0x00, 0x00, 0x00, 0x0B, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0xFC, 0xFF, 0x1F, 0x00,
        0x03, 0x03, 0x02, 0x00, 0xED, 0x9C, 0xE9, 0x74, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44,
        0xAE, 0x42, 0x60, 0x82,
    };
    {
        std::ofstream out(pngPath0, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
    }
    {
        std::ofstream out(pngPath1, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
    }

    nlohmann::json manifest = {
        {"assetManifestType", "art-import"},
        {"assets", nlohmann::json::array({
            {
                {"name", "hero_walk_down_000"},
                {"kind", "sprite"},
                {"source", pngPath0},
                {"settings",
                 {
                     {"colorWorkflow", "grayscale"},
                     {"pivotX", 0.5},
                     {"pivotY", 0.5},
                     {"collisionBoundsPolicy", "tight"},
                     {"atlasGroup", "test_atlas"},
                     {"filter", "linear"},
                     {"mipPreference", "none"},
                     {"variantGroup", "slime_theme"},
                     {"variantName", "base"},
                     {"variantWeight", 1.0},
                     {"paletteTemplate", "enemy_slime"},
                     {"animationSequenceFromFilename", true},
                     {"animationNamingRegex", "^([A-Za-z0-9_-]+)_([A-Za-z0-9_-]+)_([A-Za-z0-9_-]+)_([0-9]+)$"},
                     {"animationFps", 10.0},
                 }},
            },
            {
                {"name", "hero_walk_down_001"},
                {"kind", "sprite"},
                {"source", pngPath1},
                {"guid", "sprite-custom-frame-1"},
                {"settings",
                 {
                     {"colorWorkflow", "grayscale"},
                     {"pivotX", 0.5},
                     {"pivotY", 0.5},
                     {"collisionBoundsPolicy", "tight"},
                     {"atlasGroup", "test_atlas"},
                     {"filter", "linear"},
                     {"mipPreference", "none"},
                     {"variantGroup", "slime_theme"},
                     {"variantName", "forest"},
                     {"variantWeight", 2.0},
                     {"paletteTemplate", "enemy_slime_forest"},
                     {"animationSet", "hero"},
                     {"animationState", "walk"},
                     {"animationDirection", "down"},
                     {"animationFrame", 1},
                     {"animationFps", 10.0},
                 }},
            },
        })},
    };

    nlohmann::json invalidManifestType = manifest;
    invalidManifestType["assetManifestType"] = "wrong-type";
    std::vector<engine::SourceArtAssetRecord> invalidParsed;
    std::vector<engine::ArtImportValidationError> invalidErrors;
    if (engine::parseSourceArtManifest(invalidManifestType, "inline", invalidParsed, invalidErrors) || invalidErrors.empty()) {
        std::cerr << "manifest type validation failed\n";
        return EXIT_FAILURE;
    }

    nlohmann::json duplicateGuidManifest = manifest;
    duplicateGuidManifest["assets"][0]["guid"] = "duplicate-guid";
    duplicateGuidManifest["assets"][1]["guid"] = "duplicate-guid";
    invalidParsed.clear();
    invalidErrors.clear();
    if (engine::parseSourceArtManifest(duplicateGuidManifest, "inline", invalidParsed, invalidErrors) || invalidErrors.empty()) {
        std::cerr << "duplicate guid validation failed\n";
        return EXIT_FAILURE;
    }

    std::vector<engine::SourceArtAssetRecord> parsed;
    std::vector<engine::ArtImportValidationError> errors;
    if (!engine::parseSourceArtManifest(manifest, "inline", parsed, errors) || !errors.empty() || parsed.size() != 2) {
        std::cerr << "parseSourceArtManifest failed\n";
        return EXIT_FAILURE;
    }

    std::vector<engine::ImportedArtAssetRecord> imported;
    if (!engine::importSourceArtAssets(parsed, imported, errors) || !errors.empty() || imported.size() != 2) {
        std::cerr << "importSourceArtAssets failed\n";
        return EXIT_FAILURE;
    }

    if (imported[0].importFingerprint.empty() || imported[0].settingsFingerprint.empty() || imported[0].sourceFingerprint.empty()) {
        std::cerr << "import fingerprints must be populated\n";
        return EXIT_FAILURE;
    }
    if (imported[0].dependencies.size() < 3 || imported[0].dependencies[2] != "paletteTemplate:enemy_slime") {
        std::cerr << "palette template dependency must be tracked\n";
        return EXIT_FAILURE;
    }

    const auto atlasPlans = engine::buildAtlasPlans(imported);
    if (atlasPlans.size() != 1 || atlasPlans[0].group != "test_atlas" || atlasPlans[0].assetGuids.size() != 2) {
        std::cerr << "buildAtlasPlans returned unexpected grouping\n";
        return EXIT_FAILURE;
    }

    std::vector<engine::ArtImportValidationError> groupingErrors;
    const auto animationPlans = engine::buildAnimationClipPlans(imported, groupingErrors);
    const auto variantPlans = engine::buildVariantGroupPlans(imported, groupingErrors);
    if (!groupingErrors.empty()) {
        std::cerr << "grouping plan generation failed\n";
        return EXIT_FAILURE;
    }
    if (animationPlans.size() != 1 || animationPlans[0].frameAssetGuids.size() != 2 || animationPlans[0].fps != 10.0F) {
        std::cerr << "animation plan generation failed\n";
        return EXIT_FAILURE;
    }
    if (variantPlans.size() != 1 || variantPlans[0].group != "slime_theme" || variantPlans[0].options.size() != 2) {
        std::cerr << "variant plan generation failed\n";
        return EXIT_FAILURE;
    }


    nlohmann::json audioDoc = {
        {"audio", {
            {"music", "bgm_core"},
            {"clips", nlohmann::json::array({
                {{"id", "bgm_core"}, {"path", "music/core.wav"}, {"bus", "music"}, {"loop", true}, {"baseGain", 0.5}},
                {{"id", "sfx_hit"}, {"path", "sfx/hit.wav"}, {"bus", "sfx"}, {"loop", false}, {"baseGain", 1.0}}
            })},
            {"events", nlohmann::json::array({
                {{"name", "hit"}, {"clip", "sfx_hit"}, {"gain", 1.0}},
                {{"name", "boss_phase_shift"}, {"clip", "sfx_hit"}, {"gain", 0.8}},
                {{"name", "defensive_special"}, {"clip", "sfx_hit"}, {"gain", 0.6}}
            })}
        }}
    };
    engine::AudioContentDatabase audioDb;
    std::string audioError;
    if (!engine::parseAudioContentDatabase(audioDoc, audioDb, audioError) || !audioError.empty() || audioDb.clips.size() != 2 || audioDb.events.size() != 3 || audioDb.musicClipId != "bgm_core") {
        std::cerr << "parseAudioContentDatabase failed\n";
        return EXIT_FAILURE;
    }

    nlohmann::json pack = {
        {"importRegistry", nlohmann::json::array({
            {{"guid", imported[0].source.guid}, {"importFingerprint", imported[0].importFingerprint}},
            {{"guid", imported[1].source.guid}, {"importFingerprint", imported[1].importFingerprint}},
        })},
    };
    const auto fingerprints = engine::extractImportFingerprintByGuid(pack);
    if (fingerprints.size() != 2 || fingerprints.at(imported[0].source.guid) != imported[0].importFingerprint ||
        fingerprints.at(imported[1].source.guid) != imported[1].importFingerprint) {
        std::cerr << "extractImportFingerprintByGuid failed\n";
        return EXIT_FAILURE;
    }

    std::remove(pngPath0.c_str());
    std::remove(pngPath1.c_str());
    std::cout << "content_import_pipeline_tests passed\n";
    return EXIT_SUCCESS;
}
