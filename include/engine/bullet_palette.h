#pragma once

#include <engine/palette_fx_templates.h>
#include <engine/render2d.h>

#include <array>
#include <cstdint>

namespace engine {

class BulletPaletteTable {
  public:
    static constexpr std::uint8_t kMaxPalettes = 64;

    struct Entry {
        Color core;
        Color highlight;
        Color glow;
        Color trail;
    };

    BulletPaletteTable();

    void setEntry(std::uint8_t index, const PaletteFillResult& fill);
    void buildFromRegistry(const PaletteFxTemplateRegistry& registry);
    [[nodiscard]] const Entry& get(std::uint8_t index) const;

  private:
    std::array<Entry, kMaxPalettes> entries_;
};

} // namespace engine
