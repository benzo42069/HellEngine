#include <engine/palette_fx_templates.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace engine {
namespace {

float clamp01(float v) {
    return std::clamp(v, 0.0F, 1.0F);
}

struct Hsv {
    float h;
    float s;
    float v;
};

Hsv rgbToHsv(const PaletteColor& c) {
    const float maxv = std::max({c.r, c.g, c.b});
    const float minv = std::min({c.r, c.g, c.b});
    const float d = maxv - minv;
    Hsv hsv {0.0F, maxv <= 0.0F ? 0.0F : d / maxv, maxv};
    if (d <= 0.00001F) return hsv;
    if (maxv == c.r) hsv.h = std::fmod(((c.g - c.b) / d), 6.0F);
    else if (maxv == c.g) hsv.h = ((c.b - c.r) / d) + 2.0F;
    else hsv.h = ((c.r - c.g) / d) + 4.0F;
    hsv.h /= 6.0F;
    if (hsv.h < 0.0F) hsv.h += 1.0F;
    return hsv;
}

PaletteColor hsvToRgb(const Hsv& hsv) {
    const float h = hsv.h * 6.0F;
    const float c = hsv.v * hsv.s;
    const float x = c * (1.0F - std::fabs(std::fmod(h, 2.0F) - 1.0F));
    const float m = hsv.v - c;
    PaletteColor out {};
    if (h < 1.0F) out = {c, x, 0.0F, 1.0F};
    else if (h < 2.0F) out = {x, c, 0.0F, 1.0F};
    else if (h < 3.0F) out = {0.0F, c, x, 1.0F};
    else if (h < 4.0F) out = {0.0F, x, c, 1.0F};
    else if (h < 5.0F) out = {x, 0.0F, c, 1.0F};
    else out = {c, 0.0F, x, 1.0F};
    out.r += m;
    out.g += m;
    out.b += m;
    return out;
}

PaletteColor lerp(const PaletteColor& a, const PaletteColor& b, float t) {
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t,
    };
}

std::string_view getStringView(const nlohmann::json& obj, const char* key, std::string_view fallback = "") {
    if (!obj.contains(key) || !obj[key].is_string()) return fallback;
    return obj[key].get_ref<const std::string&>();
}

PaletteAnimationMode parseAnimationMode(std::string_view s) {
    if (s == "HueShift") return PaletteAnimationMode::HueShift;
    if (s == "GradientCycle") return PaletteAnimationMode::GradientCycle;
    if (s == "PulseBrightness") return PaletteAnimationMode::PulseBrightness;
    if (s == "PhaseScroll") return PaletteAnimationMode::PhaseScroll;
    return PaletteAnimationMode::None;
}

std::string toString(PaletteAnimationMode mode) {
    switch (mode) {
    case PaletteAnimationMode::HueShift: return "HueShift";
    case PaletteAnimationMode::GradientCycle: return "GradientCycle";
    case PaletteAnimationMode::PulseBrightness: return "PulseBrightness";
    case PaletteAnimationMode::PhaseScroll: return "PhaseScroll";
    default: return "None";
    }
}

} // namespace

bool parseHexColor(std::string_view hex, PaletteColor& out, std::string* error) {
    if (!hex.empty() && hex.front() == '#') hex.remove_prefix(1);
    if (hex.size() != 6 && hex.size() != 8) {
        if (error) *error = "hex color must be RRGGBB or RRGGBBAA";
        return false;
    }

    auto parseByte = [&](std::size_t offset, std::uint8_t& byte) -> bool {
        const std::string slice(hex.substr(offset, 2));
        char* end = nullptr;
        const auto value = std::strtoul(slice.c_str(), &end, 16);
        if (end == nullptr || *end != '\0' || value > 255) return false;
        byte = static_cast<std::uint8_t>(value);
        return true;
    };

    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
    if (!parseByte(0, r) || !parseByte(2, g) || !parseByte(4, b) || (hex.size() == 8 && !parseByte(6, a))) {
        if (error) *error = "invalid hex digits";
        return false;
    }

    out.r = static_cast<float>(r) / 255.0F;
    out.g = static_cast<float>(g) / 255.0F;
    out.b = static_cast<float>(b) / 255.0F;
    out.a = static_cast<float>(a) / 255.0F;
    return true;
}

std::string toHexColor(const PaletteColor& color, bool includeAlpha) {
    auto toByte = [](float c) {
        return static_cast<unsigned>(std::lround(clamp01(c) * 255.0F));
    };
    std::ostringstream oss;
    oss << '#';
    oss << std::uppercase << std::hex;
    oss.width(2); oss.fill('0'); oss << toByte(color.r);
    oss.width(2); oss.fill('0'); oss << toByte(color.g);
    oss.width(2); oss.fill('0'); oss << toByte(color.b);
    if (includeAlpha) {
        oss.width(2); oss.fill('0'); oss << toByte(color.a);
    }
    return oss.str();
}

PaletteFillResult deriveProjectileFillFromCore(const PaletteColor& core) {
    Hsv hsv = rgbToHsv(core);
    PaletteFillResult out;
    out.core = core;

    Hsv hi = hsv;
    hi.s = clamp01(hi.s * 0.75F);
    hi.v = clamp01(hi.v + 0.18F);
    out.highlight = hsvToRgb(hi);

    Hsv glow = hsv;
    glow.s = clamp01(glow.s * 0.45F);
    glow.v = clamp01(glow.v + 0.35F);
    out.glow = hsvToRgb(glow);
    return out;
}

PaletteFillResult deriveBackgroundFillFromAccent(const PaletteColor& accent) {
    Hsv hsv = rgbToHsv(accent);
    PaletteFillResult out;

    Hsv base = hsv;
    base.s = clamp01(base.s * 0.35F);
    base.v = clamp01(base.v * 0.25F);
    out.core = hsvToRgb(base);

    Hsv mid = hsv;
    mid.s = clamp01(mid.s * 0.45F);
    mid.v = clamp01(mid.v * 0.45F);
    out.highlight = hsvToRgb(mid);

    Hsv acc = hsv;
    acc.s = clamp01(acc.s * 0.65F);
    acc.v = clamp01(acc.v * 0.65F);
    out.glow = hsvToRgb(acc);
    return out;
}

std::vector<PaletteColor> generateGradientLut(const GradientDefinition& gradient, std::uint32_t width) {
    std::vector<PaletteColor> lut;
    if (width == 0) return lut;
    lut.resize(width);
    if (gradient.stops.empty()) return lut;

    std::vector<GradientStop> sorted = gradient.stops;
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.position < b.position; });

    for (std::uint32_t i = 0; i < width; ++i) {
        const float t = width == 1 ? 0.0F : static_cast<float>(i) / static_cast<float>(width - 1);
        if (t <= sorted.front().position) {
            lut[i] = sorted.front().color;
            continue;
        }
        if (t >= sorted.back().position) {
            lut[i] = sorted.back().color;
            continue;
        }
        for (std::size_t s = 1; s < sorted.size(); ++s) {
            if (t <= sorted[s].position) {
                const float a = sorted[s - 1].position;
                const float b = sorted[s].position;
                const float local = (t - a) / (b - a);
                lut[i] = lerp(sorted[s - 1].color, sorted[s].color, local);
                break;
            }
        }
    }
    return lut;
}

PaletteMaterialParams buildMaterialParamsFromTemplate(const PaletteTemplate& templ) {
    PaletteMaterialParams out;
    auto findLayer = [&](std::string_view layer, PaletteColor fallback) {
        for (std::size_t i = 0; i < templ.layerNames.size() && i < templ.layerColors.size(); ++i) {
            if (templ.layerNames[i] == layer) return templ.layerColors[i];
        }
        return fallback;
    };

    out.paletteCore = findLayer("Core", out.paletteCore);
    out.paletteHighlight = findLayer("Highlight", out.paletteCore);
    out.paletteGlow = findLayer("Glow", out.paletteHighlight);
    out.paletteTrail = findLayer("Trail", out.paletteGlow);
    out.paletteImpact = findLayer("Impact", out.paletteGlow);
    out.beamCore = findLayer("Core", out.paletteCore);
    out.beamInner = findLayer("Inner", out.paletteHighlight);
    out.beamOuter = findLayer("Outer", out.paletteGlow);
    out.beamGlow = findLayer("Glow", out.paletteGlow);
    out.emissiveBoost = findLayer("EmissiveBoost", {1.0F, 1.0F, 1.0F, 1.0F}).r;
    out.alphaBoost = findLayer("AlphaBoost", {1.0F, 1.0F, 1.0F, 1.0F}).r;
    out.gradientName = templ.gradientName;
    return out;
}

bool parsePaletteFxTemplateDatabase(const nlohmann::json& json, PaletteFxTemplateDatabase& out, std::string& error) {
    out = {};
    if (!json.is_object()) {
        error = "root must be object";
        return false;
    }

    if (json.contains("paletteTemplates") && json["paletteTemplates"].is_array()) {
        for (const auto& item : json["paletteTemplates"]) {
            if (!item.is_object()) continue;
            PaletteTemplate t;
            t.category = item.value("category", "Uncategorized");
            t.name = item.value("name", "Unnamed");
            t.gradientName = item.value("gradient", "");
            t.autoFxPreset = item.value("autoFxPreset", "");

            if (item.contains("layers") && item["layers"].is_array()) {
                for (const auto& l : item["layers"]) {
                    const std::string layerName = l.value("name", "");
                    PaletteColor c;
                    std::string parseError;
                    if (!parseHexColor(l.value("color", "#FFFFFF"), c, &parseError)) {
                        error = "invalid color in template '" + t.name + "': " + parseError;
                        return false;
                    }
                    t.layerNames.push_back(layerName);
                    t.layerColors.push_back(c);
                }
            }

            if (item.contains("animation") && item["animation"].is_object()) {
                const auto& a = item["animation"];
                t.animation.mode = parseAnimationMode(a.value("mode", "None"));
                t.animation.speed = a.value("speed", 0.0F);
                t.animation.phaseOffset = a.value("phaseOffset", 0.0F);
                t.animation.perInstanceOffset = a.value("perInstanceOffset", 0.0F);
            }
            out.palettes.push_back(t);
        }
    }

    if (json.contains("gradients") && json["gradients"].is_array()) {
        for (const auto& item : json["gradients"]) {
            GradientDefinition g;
            g.name = item.value("name", "Unnamed Gradient");
            if (item.contains("stops") && item["stops"].is_array()) {
                for (const auto& s : item["stops"]) {
                    GradientStop stop;
                    stop.position = s.value("position", 0.0F);
                    std::string parseError;
                    if (!parseHexColor(s.value("color", "#FFFFFF"), stop.color, &parseError)) {
                        error = "invalid gradient color in '" + g.name + "': " + parseError;
                        return false;
                    }
                    g.stops.push_back(stop);
                }
            }
            std::sort(g.stops.begin(), g.stops.end(), [](const auto& a, const auto& b) { return a.position < b.position; });
            for (std::size_t i = 1; i < g.stops.size(); ++i) {
                if (g.stops[i].position < g.stops[i - 1].position) {
                    error = "gradient positions must be monotonic";
                    return false;
                }
            }
            out.gradients.push_back(g);
        }
    }

    if (json.contains("fxPresets") && json["fxPresets"].is_array()) {
        for (const auto& item : json["fxPresets"]) {
            FxPreset p;
            p.name = item.value("name", "Unnamed FX");
            p.bloomThreshold = item.value("bloomThreshold", 1.0F);
            p.bloomIntensity = item.value("bloomIntensity", 0.0F);
            p.bloomRadius = item.value("bloomRadius", 0.0F);
            p.vignetteIntensity = item.value("vignetteIntensity", 0.0F);
            p.vignetteRoundness = item.value("vignetteRoundness", 1.0F);
            p.exposure = item.value("exposure", 1.0F);
            p.contrast = item.value("contrast", 1.0F);
            p.saturation = item.value("saturation", 1.0F);
            p.gamma = item.value("gamma", 1.0F);
            p.chromaticAberration = item.value("chromaticAberration", 0.0F);
            p.filmGrain = item.value("filmGrain", 0.0F);
            p.scanlineIntensity = item.value("scanlineIntensity", 0.0F);
            out.fxPresets.push_back(p);
        }
    }

    return true;
}

nlohmann::json toJson(const PaletteFxTemplateDatabase& db) {
    nlohmann::json j;
    j["paletteTemplates"] = nlohmann::json::array();
    for (const auto& t : db.palettes) {
        nlohmann::json item;
        item["category"] = t.category;
        item["name"] = t.name;
        item["gradient"] = t.gradientName;
        item["autoFxPreset"] = t.autoFxPreset;
        item["animation"] = {
            {"mode", toString(t.animation.mode)},
            {"speed", t.animation.speed},
            {"phaseOffset", t.animation.phaseOffset},
            {"perInstanceOffset", t.animation.perInstanceOffset},
        };
        item["layers"] = nlohmann::json::array();
        for (std::size_t i = 0; i < t.layerNames.size() && i < t.layerColors.size(); ++i) {
            item["layers"].push_back({{"name", t.layerNames[i]}, {"color", toHexColor(t.layerColors[i])}});
        }
        j["paletteTemplates"].push_back(item);
    }

    j["gradients"] = nlohmann::json::array();
    for (const auto& g : db.gradients) {
        nlohmann::json item;
        item["name"] = g.name;
        item["stops"] = nlohmann::json::array();
        for (const auto& s : g.stops) {
            item["stops"].push_back({{"position", s.position}, {"color", toHexColor(s.color)}});
        }
        j["gradients"].push_back(item);
    }

    j["fxPresets"] = nlohmann::json::array();
    for (const auto& p : db.fxPresets) {
        j["fxPresets"].push_back({
            {"name", p.name},
            {"bloomThreshold", p.bloomThreshold},
            {"bloomIntensity", p.bloomIntensity},
            {"bloomRadius", p.bloomRadius},
            {"vignetteIntensity", p.vignetteIntensity},
            {"vignetteRoundness", p.vignetteRoundness},
            {"exposure", p.exposure},
            {"contrast", p.contrast},
            {"saturation", p.saturation},
            {"gamma", p.gamma},
            {"chromaticAberration", p.chromaticAberration},
            {"filmGrain", p.filmGrain},
            {"scanlineIntensity", p.scanlineIntensity},
        });
    }
    return j;
}

bool PaletteFxTemplateRegistry::loadFromJsonFile(const std::filesystem::path& path, std::string* error) {
    std::ifstream in(path);
    if (!in.good()) {
        if (error) *error = "cannot open file: " + path.string();
        return false;
    }
    nlohmann::json j;
    in >> j;

    PaletteFxTemplateDatabase parsed;
    std::string parseError;
    if (!parsePaletteFxTemplateDatabase(j, parsed, parseError)) {
        if (error) *error = parseError;
        return false;
    }

    sourcePath_ = path;
    sourceWriteTime_ = std::filesystem::last_write_time(path);
    database_ = std::move(parsed);
    return true;
}

bool PaletteFxTemplateRegistry::reloadIfChanged(std::string* error) {
    if (sourcePath_.empty() || !std::filesystem::exists(sourcePath_)) return false;
    const auto newTime = std::filesystem::last_write_time(sourcePath_);
    if (newTime == sourceWriteTime_) return false;
    return loadFromJsonFile(sourcePath_, error);
}

const PaletteFxTemplateDatabase& PaletteFxTemplateRegistry::database() const { return database_; }

std::vector<std::string> PaletteFxTemplateRegistry::categories() const {
    std::vector<std::string> out;
    for (const auto& t : database_.palettes) {
        if (std::find(out.begin(), out.end(), t.category) == out.end()) out.push_back(t.category);
    }
    return out;
}

std::vector<const PaletteTemplate*> PaletteFxTemplateRegistry::templatesForCategory(const std::string& category) const {
    std::vector<const PaletteTemplate*> out;
    for (const auto& t : database_.palettes) {
        if (t.category == category) out.push_back(&t);
    }
    return out;
}

const FxPreset* PaletteFxTemplateRegistry::findFxPreset(const std::string& name) const {
    for (const auto& p : database_.fxPresets) {
        if (p.name == name) return &p;
    }
    return nullptr;
}

} // namespace engine
