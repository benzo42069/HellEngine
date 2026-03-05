#include <engine/palette_fx_templates.h>

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
bool near(float a, float b, float eps = 0.01F) { return std::fabs(a - b) <= eps; }
}

int main() {
    engine::PaletteColor c;
    std::string error;
    if (!engine::parseHexColor("#00E5FF", c, &error) || !near(c.g, 229.0F / 255.0F) || !near(c.b, 1.0F)) {
        std::cerr << "hex parse failed: " << error << "\n";
        return EXIT_FAILURE;
    }

    engine::PaletteFxTemplateDatabase db;
    engine::PaletteTemplate t;
    t.category = "Player";
    t.name = "Unit";
    t.layerNames = {"Core", "Highlight", "Glow"};
    t.layerColors = {{0.1F, 0.2F, 0.3F, 1.0F}, {0.4F, 0.5F, 0.6F, 1.0F}, {0.7F, 0.8F, 0.9F, 1.0F}};
    db.palettes.push_back(t);
    db.gradients.push_back({"TestGradient", {{0.0F, {0, 0, 0, 1}}, {1.0F, {1, 1, 1, 1}}}});
    db.fxPresets.push_back({"Fx", 0.8F, 1.1F, 0.5F, 0.2F, 0.9F, 1.0F, 1.0F, 1.0F, 1.0F, 0.0F, 0.0F, 0.0F});

    nlohmann::json round = engine::toJson(db);
    engine::PaletteFxTemplateDatabase parsed;
    if (!engine::parsePaletteFxTemplateDatabase(round, parsed, error) || parsed.palettes.size() != 1 || parsed.gradients.size() != 1 || parsed.fxPresets.size() != 1) {
        std::cerr << "json roundtrip failed: " << error << "\n";
        return EXIT_FAILURE;
    }

    auto lut = engine::generateGradientLut(parsed.gradients.front(), 16);
    if (lut.size() != 16 || !near(lut.front().r, 0.0F) || !near(lut.back().r, 1.0F)) {
        std::cerr << "gradient LUT endpoint failure\n";
        return EXIT_FAILURE;
    }

    engine::GradientDefinition bad {"bad", {{0.9F, {1, 0, 0, 1}}, {0.1F, {0, 0, 1, 1}}}};
    auto badLut = engine::generateGradientLut(bad, 8);
    (void)badLut;

    auto params = engine::buildMaterialParamsFromTemplate(parsed.palettes.front());
    if (!near(params.paletteCore.r, 0.1F) || !near(params.paletteGlow.b, 0.9F)) {
        std::cerr << "material mapping failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "palette_fx_templates_tests passed\n";
    return EXIT_SUCCESS;
}
