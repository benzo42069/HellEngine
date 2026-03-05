#include <engine/modern_renderer.h>

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
bool near(const float a, const float b, const float eps = 0.01F) {
    return std::fabs(a - b) <= eps;
}
}

int main() {
    engine::MaterialParamBlock input;
    input.materialName = "bullet/player_core";
    input.mode = engine::MaterialColorizationMode::GradientRamp;
    input.palette.paletteCore = {0.1F, 0.2F, 0.3F, 1.0F};
    input.palette.paletteHighlight = {0.4F, 0.5F, 0.6F, 1.0F};
    input.palette.paletteGlow = {0.7F, 0.8F, 0.9F, 1.0F};
    input.palette.gradientName = "Ion Beam Gradient";
    input.beamScrollSpeed = 1.5F;
    input.beamPulseSpeed = 0.8F;
    input.beamNoiseDistortion = 0.15F;

    const nlohmann::json saved = engine::toJson(input);
    engine::MaterialParamBlock loaded;
    std::string error;
    if (!engine::materialParamBlockFromJson(saved, loaded, error)) {
        std::cerr << "material param deserialize failed: " << error << "\n";
        return EXIT_FAILURE;
    }
    if (loaded.materialName != input.materialName || loaded.mode != engine::MaterialColorizationMode::GradientRamp
        || !near(loaded.palette.paletteGlow.b, 0.9F) || !near(loaded.beamScrollSpeed, 1.5F)) {
        std::cerr << "material parameter roundtrip mismatch\n";
        return EXIT_FAILURE;
    }

    engine::GradientDefinition gradient;
    gradient.name = "UnitGradient";
    gradient.stops = {
        {0.0F, {0.0F, 0.0F, 0.0F, 1.0F}},
        {0.5F, {1.0F, 0.0F, 0.0F, 1.0F}},
        {1.0F, {1.0F, 1.0F, 1.0F, 1.0F}},
    };
    const auto lut = engine::generateGradientLut(gradient, 32);
    if (lut.size() != 32 || !near(lut.front().r, 0.0F) || !near(lut.back().r, 1.0F)) {
        std::cerr << "gradient LUT test failed\n";
        return EXIT_FAILURE;
    }

    const nlohmann::json manifestJson = {
        {"shaders", {
            {
                {"name", "palette_vs"},
                {"stage", "vs"},
                {"entry", "VSMain"},
                {"blobPath", "shaders/palette_vs.dxil"},
            },
            {
                {"name", "palette_ps"},
                {"stage", "ps"},
                {"entry", "PSMain"},
                {"blobPath", "shaders/palette_ps.dxil"},
            },
        }},
    };

    engine::ShaderManifest manifest;
    if (!engine::parseShaderManifest(manifestJson, manifest, error) || manifest.entries.size() != 2) {
        std::cerr << "shader manifest parse failed: " << error << "\n";
        return EXIT_FAILURE;
    }

    engine::ShaderManifest badManifest;
    badManifest.entries.push_back({"broken", "gs", "main", "x.bin"});
    if (engine::validateShaderManifest(badManifest, error)) {
        std::cerr << "invalid shader manifest should fail validation\n";
        return EXIT_FAILURE;
    }

    std::cout << "modern_renderer_tests passed\n";
    return EXIT_SUCCESS;
}
