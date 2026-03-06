#include <engine/bullet_palette.h>

#include <algorithm>
#include <cstddef>
#include <cmath>

namespace engine {
namespace {

Color toColor(const PaletteColor& color) {
    auto toByte = [](const float channel) {
        return static_cast<Uint8>(std::clamp(std::lround(channel * 255.0F), 0L, 255L));
    };
    return Color {
        toByte(color.r),
        toByte(color.g),
        toByte(color.b),
        toByte(color.a),
    };
}

} // namespace

BulletPaletteTable::BulletPaletteTable() {
    const PaletteFillResult defaultFill = deriveProjectileFillFromCore(PaletteColor {1.0F, 1.0F, 1.0F, 1.0F});
    for (std::uint8_t i = 0; i < kMaxPalettes; ++i) {
        setEntry(i, defaultFill);
    }
}

void BulletPaletteTable::setEntry(const std::uint8_t index, const PaletteFillResult& fill) {
    if (index >= kMaxPalettes) return;
    entries_[index] = Entry {
        .core = toColor(fill.core),
        .highlight = toColor(fill.highlight),
        .glow = toColor(fill.glow),
        .trail = toColor(fill.trail),
    };
}

void BulletPaletteTable::buildFromRegistry(const PaletteFxTemplateRegistry& registry) {
    const auto& db = registry.database();
    std::uint8_t idx = 1;
    for (const PaletteTemplate& templ : db.palettes) {
        if (idx >= kMaxPalettes) break;
        PaletteColor core = PaletteColor {1.0F, 1.0F, 1.0F, 1.0F};
        for (std::size_t layerIndex = 0; layerIndex < templ.layerNames.size() && layerIndex < templ.layerColors.size(); ++layerIndex) {
            if (templ.layerNames[layerIndex] == "Core") {
                core = templ.layerColors[layerIndex];
                break;
            }
        }
        setEntry(idx, deriveProjectileFillFromCore(core));
        ++idx;
    }
}

const BulletPaletteTable::Entry& BulletPaletteTable::get(const std::uint8_t index) const {
    if (index >= kMaxPalettes) return entries_[0];
    return entries_[index];
}

} // namespace engine
