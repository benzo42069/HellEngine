#include <engine/content_pipeline.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
    const std::string pngPath = "content_import_pipeline_test.png";
    {
        std::ofstream out(pngPath, std::ios::binary);
        // 1x1 PNG.
        const unsigned char bytes[] = {
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x04, 0x00, 0x00, 0x00, 0xB5, 0x1C, 0x0C,
            0x02, 0x00, 0x00, 0x00, 0x0B, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0xFC, 0xFF, 0x1F, 0x00,
            0x03, 0x03, 0x02, 0x00, 0xED, 0x9C, 0xE9, 0x74, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44,
            0xAE, 0x42, 0x60, 0x82,
        };
        out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
    }

    nlohmann::json manifest = {
        {"assetManifestType", "art-import"},
        {"assets", nlohmann::json::array({
            {
                {"name", "import_test_sprite"},
                {"kind", "sprite"},
                {"source", pngPath},
                {"settings",
                 {
                     {"colorWorkflow", "grayscale"},
                     {"pivotX", 0.5},
                     {"pivotY", 0.5},
                     {"collisionBoundsPolicy", "tight"},
                     {"atlasGroup", "test_atlas"},
                     {"filter", "linear"},
                     {"mipPreference", "none"},
                     {"variantGroup", "test_variants"},
                 }},
            },
        })},
    };

    std::vector<engine::SourceArtAssetRecord> parsed;
    std::vector<engine::ArtImportValidationError> errors;
    if (!engine::parseSourceArtManifest(manifest, "inline", parsed, errors) || !errors.empty() || parsed.size() != 1) {
        std::cerr << "parseSourceArtManifest failed\n";
        return EXIT_FAILURE;
    }

    std::vector<engine::ImportedArtAssetRecord> imported;
    if (!engine::importSourceArtAssets(parsed, imported, errors) || !errors.empty() || imported.size() != 1) {
        std::cerr << "importSourceArtAssets failed\n";
        return EXIT_FAILURE;
    }

    if (imported[0].importFingerprint.empty() || imported[0].settingsFingerprint.empty() || imported[0].sourceFingerprint.empty()) {
        std::cerr << "import fingerprints must be populated\n";
        return EXIT_FAILURE;
    }

    const auto plans = engine::buildAtlasPlans(imported);
    if (plans.size() != 1 || plans[0].group != "test_atlas" || plans[0].assetGuids.size() != 1) {
        std::cerr << "buildAtlasPlans returned unexpected grouping\n";
        return EXIT_FAILURE;
    }

    nlohmann::json pack = {
        {"importRegistry", nlohmann::json::array({
            {{"guid", imported[0].source.guid}, {"importFingerprint", imported[0].importFingerprint}},
        })},
    };
    const auto fingerprints = engine::extractImportFingerprintByGuid(pack);
    if (fingerprints.size() != 1 || fingerprints.begin()->second != imported[0].importFingerprint) {
        std::cerr << "extractImportFingerprintByGuid failed\n";
        return EXIT_FAILURE;
    }

    std::remove(pngPath.c_str());
    std::cout << "content_import_pipeline_tests passed\n";
    return EXIT_SUCCESS;
}
