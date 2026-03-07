#include <engine/background_layers.h>

#include <engine/standards.h>

#include <SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace engine {
namespace {

constexpr int kMaxLayers = 6;

[[nodiscard]] std::uint8_t hashNoise(const int x, const int y) {
    const std::uint32_t mixed = static_cast<std::uint32_t>((x * 73856093) ^ (y * 19349663));
    return static_cast<std::uint8_t>(mixed & 0xFFU);
}

[[nodiscard]] SDL_Texture* makeTexture(SDL_Renderer* renderer, const int width, const int height, const std::vector<std::uint8_t>& pixels) {
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!texture) return nullptr;
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    if (SDL_UpdateTexture(texture, nullptr, pixels.data(), width * 4) != 0) {
        SDL_DestroyTexture(texture);
        return nullptr;
    }
    return texture;
}

[[nodiscard]] Color withOpacity(const Color tint, const float opacity) {
    Color result = tint;
    const float clamped = std::clamp(opacity, 0.0F, 1.0F);
    result.a = static_cast<Uint8>(std::clamp(static_cast<int>(std::lround(static_cast<float>(tint.a) * clamped)), 0, 255));
    return result;
}

} // namespace

void BackgroundSystem::initialize(SDL_Renderer* renderer, TextureStore& store) {
    clear();
    if (!renderer) return;

    {
        constexpr int w = 128;
        constexpr int h = 128;
        std::vector<std::uint8_t> pixels(w * h * 4, 0);
        constexpr int baseR = 10;
        constexpr int baseG = 14;
        constexpr int baseB = 24;
        constexpr int variation = 12;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const std::uint8_t n = hashNoise(x % w, y % h);
                const int delta = (static_cast<int>(n) * variation) / 255 - (variation / 2);
                const std::size_t i = static_cast<std::size_t>((y * w + x) * 4);
                pixels[i + 0] = static_cast<std::uint8_t>(std::clamp(baseR + delta, 0, 255));
                pixels[i + 1] = static_cast<std::uint8_t>(std::clamp(baseG + delta, 0, 255));
                pixels[i + 2] = static_cast<std::uint8_t>(std::clamp(baseB + delta, 0, 255));
                pixels[i + 3] = 255;
            }
        }

        if (SDL_Texture* texture = makeTexture(renderer, w, h, pixels)) {
            store.adoptTexture("bg.deep_space", texture, w, h);
            addLayer(BackgroundLayer {
                .textureId = "bg.deep_space",
                .parallaxFactor = 0.1F,
                .scrollSpeedX = 2.0F,
                .scrollSpeedY = 0.8F,
                .opacity = 1.0F,
                .tint = {255, 255, 255, 255},
            });
        }
    }

    {
        constexpr int w = 256;
        constexpr int h = 256;
        std::vector<std::uint8_t> pixels(w * h * 4, 0);
        constexpr int spacing = 32;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const bool onGrid = (x % spacing == 0) || (y % spacing == 0);
                const std::size_t i = static_cast<std::size_t>((y * w + x) * 4);
                pixels[i + 0] = onGrid ? 140 : 80;
                pixels[i + 1] = onGrid ? 155 : 90;
                pixels[i + 2] = onGrid ? 180 : 110;
                pixels[i + 3] = onGrid ? 60 : 18;
            }
        }

        if (SDL_Texture* texture = makeTexture(renderer, w, h, pixels)) {
            store.adoptTexture("bg.grid", texture, w, h);
            addLayer(BackgroundLayer {
                .textureId = "bg.grid",
                .parallaxFactor = 0.3F,
                .scrollSpeedX = 7.0F,
                .scrollSpeedY = 0.0F,
                .opacity = 0.55F,
                .tint = {150, 170, 210, 255},
            });
        }
    }

    {
        constexpr int w = 128;
        constexpr int h = 128;
        std::vector<std::uint8_t> pixels(w * h * 4, 0);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const std::uint8_t n = hashNoise((x + 19) % w, (y + 37) % h);
                const bool isParticle = n < 3;
                const std::size_t i = static_cast<std::size_t>((y * w + x) * 4);
                if (isParticle) {
                    const float brightN = static_cast<float>(hashNoise((x + 59) % w, (y + 83) % h)) / 255.0F;
                    const float brightness = 0.4F + brightN * 0.4F;
                    const std::uint8_t v = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(brightness * 255.0F)), 0, 255));
                    pixels[i + 0] = v;
                    pixels[i + 1] = v;
                    pixels[i + 2] = v;
                    pixels[i + 3] = static_cast<std::uint8_t>(std::min(255, static_cast<int>(v) + 20));
                }
            }
        }

        if (SDL_Texture* texture = makeTexture(renderer, w, h, pixels)) {
            store.adoptTexture("bg.particle_dust", texture, w, h);
            addLayer(BackgroundLayer {
                .textureId = "bg.particle_dust",
                .parallaxFactor = 0.6F,
                .scrollSpeedX = 12.0F,
                .scrollSpeedY = -5.0F,
                .opacity = 0.7F,
                .tint = {255, 255, 255, 255},
            });
        }
    }
}

void BackgroundSystem::addLayer(const BackgroundLayer& layer) {
    if (layers_.size() >= kMaxLayers) return;
    layers_.push_back(layer);
}

void BackgroundSystem::update(const float frameDelta) { scrollOffset_ += std::max(frameDelta, 0.0F); }

void BackgroundSystem::render(SpriteBatch& batch, const Camera2D& camera) const {
    const int viewportW = camera.viewportWidth();
    const int viewportH = camera.viewportHeight();
    if (viewportW <= 0 || viewportH <= 0) return;

    const Vec2 camCenter = camera.center();
    const float zoom = std::max(camera.zoom(), 0.1F);
    const float halfWorldW = (static_cast<float>(viewportW) * 0.5F) / zoom;
    const float halfWorldH = (static_cast<float>(viewportH) * 0.5F) / zoom;

    for (const BackgroundLayer& layer : layers_) {
        const float parallax = std::clamp(layer.parallaxFactor, 0.0F, 1.0F);
        const float worldMinX = camCenter.x - halfWorldW;
        const float worldMaxX = camCenter.x + halfWorldW;
        const float worldMinY = camCenter.y - halfWorldH;
        const float worldMaxY = camCenter.y + halfWorldH;

        const float offsetX = camCenter.x * parallax + scrollOffset_ * layer.scrollSpeedX;
        const float offsetY = camCenter.y * parallax + scrollOffset_ * layer.scrollSpeedY;

        const float tileSizeX = static_cast<float>(engineStandards().playfieldWidth / 4);
        const float tileSizeY = static_cast<float>(engineStandards().playfieldHeight / 4);

        const int startTileX = static_cast<int>(std::floor((worldMinX + offsetX) / tileSizeX)) - 1;
        const int endTileX = static_cast<int>(std::ceil((worldMaxX + offsetX) / tileSizeX)) + 1;
        const int startTileY = static_cast<int>(std::floor((worldMinY + offsetY) / tileSizeY)) - 1;
        const int endTileY = static_cast<int>(std::ceil((worldMaxY + offsetY) / tileSizeY)) + 1;

        const Color color = withOpacity(layer.tint, layer.opacity);
        for (int ty = startTileY; ty <= endTileY; ++ty) {
            for (int tx = startTileX; tx <= endTileX; ++tx) {
                const float x = static_cast<float>(tx) * tileSizeX - offsetX;
                const float y = static_cast<float>(ty) * tileSizeY - offsetY;
                batch.draw(SpriteDrawCmd {
                    .textureId = layer.textureId,
                    .dest = {x, y, tileSizeX, tileSizeY},
                    .src = std::nullopt,
                    .rotationDeg = 0.0F,
                    .color = color,
                });
            }
        }
    }
}


void BackgroundSystem::setPrimaryLayerTexture(const std::string& textureId) {
    if (textureId.empty()) return;
    if (layers_.empty()) {
        addLayer(BackgroundLayer {
            .textureId = textureId,
            .parallaxFactor = 0.1F,
            .scrollSpeedX = 2.0F,
            .scrollSpeedY = 0.8F,
            .opacity = 1.0F,
            .tint = {255, 255, 255, 255},
        });
        return;
    }
    layers_.front().textureId = textureId;
}

void BackgroundSystem::clear() {
    layers_.clear();
    scrollOffset_ = 0.0F;
}

} // namespace engine
