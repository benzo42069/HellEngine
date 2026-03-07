#include <engine/danger_field.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace engine {

namespace {
std::uint8_t toByte(const float v) {
    const float clamped = std::clamp(v, 0.0F, 1.0F);
    return static_cast<std::uint8_t>(std::lround(clamped * 255.0F));
}
} // namespace

void DangerFieldOverlay::initialize(SDL_Renderer* renderer, const int fieldWidth, const int fieldHeight) {
    shutdown();

    fieldW_ = std::max(fieldWidth, 1);
    fieldH_ = std::max(fieldHeight, 1);
    density_.assign(static_cast<std::size_t>(fieldW_) * static_cast<std::size_t>(fieldH_), 0.0F);
    pixels_.assign(static_cast<std::size_t>(fieldW_) * static_cast<std::size_t>(fieldH_), 0U);

    if (renderer) {
        fieldTexture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, fieldW_, fieldH_);
        if (fieldTexture_) {
            SDL_SetTextureBlendMode(fieldTexture_, SDL_BLENDMODE_ADD);
        }
    }

    if (gradientLut_.empty()) {
        GradientDefinition gradient {
            .name = "danger_default",
            .stops = {
                GradientStop {0.0F, PaletteColor {0.05F, 0.12F, 0.45F, 1.0F}},
                GradientStop {0.5F, PaletteColor {0.95F, 0.85F, 0.18F, 1.0F}},
                GradientStop {1.0F, PaletteColor {0.95F, 0.16F, 0.10F, 1.0F}},
            },
        };
        gradientLut_ = generateGradientLut(gradient, 256U);
    }
}

void DangerFieldOverlay::shutdown() {
    if (fieldTexture_) {
        SDL_DestroyTexture(fieldTexture_);
        fieldTexture_ = nullptr;
    }
    density_.clear();
    pixels_.clear();
}

void DangerFieldOverlay::buildFromGrid(
    const std::vector<int>& gridHead,
    const std::vector<int>& gridNext,
    const std::vector<float>& posX,
    const std::vector<float>& posY,
    const std::vector<std::uint8_t>& active,
    const std::uint32_t gridX,
    const std::uint32_t gridY,
    const float worldHalfExtent
) {
    (void)posX;
    (void)posY;
    (void)worldHalfExtent;

    if (density_.empty()) {
        density_.assign(static_cast<std::size_t>(fieldW_) * static_cast<std::size_t>(fieldH_), 0.0F);
    }
    if (pixels_.empty()) {
        pixels_.assign(static_cast<std::size_t>(fieldW_) * static_cast<std::size_t>(fieldH_), 0U);
    }
    if (gridHead.empty() || gridNext.empty() || gridX == 0U || gridY == 0U) {
        std::fill(density_.begin(), density_.end(), 0.0F);
        return;
    }

    constexpr int gatherRadius = 1;
    constexpr float normalizeAtCount = 8.0F;

    for (int fy = 0; fy < fieldH_; ++fy) {
        const std::uint32_t gy = std::min(static_cast<std::uint32_t>((static_cast<std::int64_t>(fy) * static_cast<std::int64_t>(gridY)) / fieldH_), gridY - 1U);
        for (int fx = 0; fx < fieldW_; ++fx) {
            const std::uint32_t gx = std::min(static_cast<std::uint32_t>((static_cast<std::int64_t>(fx) * static_cast<std::int64_t>(gridX)) / fieldW_), gridX - 1U);

            int bulletCount = 0;
            const int minY = std::max(0, static_cast<int>(gy) - gatherRadius);
            const int maxY = std::min(static_cast<int>(gridY) - 1, static_cast<int>(gy) + gatherRadius);
            const int minX = std::max(0, static_cast<int>(gx) - gatherRadius);
            const int maxX = std::min(static_cast<int>(gridX) - 1, static_cast<int>(gx) + gatherRadius);

            for (int ny = minY; ny <= maxY; ++ny) {
                for (int nx = minX; nx <= maxX; ++nx) {
                    const std::uint32_t cell = static_cast<std::uint32_t>(ny) * gridX + static_cast<std::uint32_t>(nx);
                    for (int n = gridHead[cell]; n >= 0; n = gridNext[static_cast<std::size_t>(n)]) {
                        if (active[static_cast<std::size_t>(n)] == 0U) continue;
                        ++bulletCount;
                    }
                }
            }

            const std::size_t idx = static_cast<std::size_t>(fy) * static_cast<std::size_t>(fieldW_) + static_cast<std::size_t>(fx);
            density_[idx] = std::clamp(static_cast<float>(bulletCount) / normalizeAtCount, 0.0F, 1.0F);
        }
    }

    std::vector<float> blurred = density_;
    for (int fy = 0; fy < fieldH_; ++fy) {
        for (int fx = 0; fx < fieldW_; ++fx) {
            float sum = 0.0F;
            int taps = 0;
            for (int oy = -1; oy <= 1; ++oy) {
                const int sy = std::clamp(fy + oy, 0, fieldH_ - 1);
                for (int ox = -1; ox <= 1; ++ox) {
                    const int sx = std::clamp(fx + ox, 0, fieldW_ - 1);
                    const std::size_t sampleIdx = static_cast<std::size_t>(sy) * static_cast<std::size_t>(fieldW_) + static_cast<std::size_t>(sx);
                    sum += density_[sampleIdx];
                    ++taps;
                }
            }
            const std::size_t idx = static_cast<std::size_t>(fy) * static_cast<std::size_t>(fieldW_) + static_cast<std::size_t>(fx);
            blurred[idx] = taps > 0 ? sum / static_cast<float>(taps) : density_[idx];
        }
    }
    density_.swap(blurred);
}

void DangerFieldOverlay::render(SDL_Renderer* renderer, const Camera2D& camera, const float opacity) {
    if (!renderer || density_.empty() || pixels_.size() != density_.size()) return;

    if (!fieldTexture_) {
        initialize(renderer, fieldW_, fieldH_);
    }
    if (gradientLut_.empty()) {
        initialize(renderer, fieldW_, fieldH_);
    }
    if (!fieldTexture_ || gradientLut_.empty()) return;

    constexpr float visibleThreshold = 0.1F;
    for (std::size_t i = 0; i < density_.size(); ++i) {
        const float d = std::clamp(density_[i], 0.0F, 1.0F);
        if (d <= 0.0F) {
            pixels_[i] = 0U;
            continue;
        }

        const std::size_t lutIdx = std::min(static_cast<std::size_t>(d * static_cast<float>(gradientLut_.size() - 1U)), gradientLut_.size() - 1U);
        const PaletteColor c = gradientLut_[lutIdx];
        const std::uint8_t r = toByte(c.r);
        const std::uint8_t g = toByte(c.g);
        const std::uint8_t b = toByte(c.b);

        const float scaled = d <= visibleThreshold ? 0.0F : std::clamp((d - visibleThreshold) / (1.0F - visibleThreshold), 0.0F, 1.0F);
        const std::uint8_t a = toByte(scaled * std::clamp(opacity, 0.0F, 1.0F));

        pixels_[i] = static_cast<std::uint32_t>(r)
            | (static_cast<std::uint32_t>(g) << 8U)
            | (static_cast<std::uint32_t>(b) << 16U)
            | (static_cast<std::uint32_t>(a) << 24U);
    }

    SDL_UpdateTexture(fieldTexture_, nullptr, pixels_.data(), fieldW_ * static_cast<int>(sizeof(std::uint32_t)));

    const SDL_Rect dst {0, 0, camera.viewportWidth(), camera.viewportHeight()};
    SDL_SetTextureBlendMode(fieldTexture_, SDL_BLENDMODE_ADD);
    SDL_RenderCopy(renderer, fieldTexture_, nullptr, &dst);
}

void DangerFieldOverlay::setGradient(const std::vector<PaletteColor>& lut) {
    gradientLut_ = lut;
}

} // namespace engine
