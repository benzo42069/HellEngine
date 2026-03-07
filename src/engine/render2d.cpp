#include <engine/render2d.h>

#include <engine/logging.h>

#include <stb_image.h>
#include <stb_truetype.h>

#include <SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iterator>
#include <numbers>

namespace engine {

void Camera2D::setViewport(const int width, const int height) {
    viewportWidth_ = std::max(width, 1);
    viewportHeight_ = std::max(height, 1);
}

void Camera2D::setCenter(const Vec2 center) { center_ = center; }

void Camera2D::pan(const Vec2 delta) {
    center_.x += delta.x;
    center_.y += delta.y;
}

void Camera2D::setZoom(const float zoom) { zoom_ = std::clamp(zoom, 0.1F, 8.0F); }

void Camera2D::addZoom(const float delta) { setZoom(zoom_ + delta); }

void Camera2D::setShake(const float amplitude, const float seconds) {
    if (amplitude <= 0.0F || seconds <= 0.0F) {
        shakeSystem_.clear();
        return;
    }
    shakeSystem_.trigger(ShakeParams {
        .profile = ShakeProfile::Impact,
        .amplitude = amplitude,
        .duration = seconds,
        .direction = {1.0F, 0.0F},
        .frequency = 45.0F,
        .damping = 8.0F,
    });
}

void Camera2D::update(const float dt) {
    shakeOffset_ = shakeSystem_.update(std::max(dt, 0.0F));
}

CameraShakeSystem& Camera2D::shakeSystem() { return shakeSystem_; }

const CameraShakeSystem& Camera2D::shakeSystem() const { return shakeSystem_; }

Vec2 Camera2D::worldToScreen(const Vec2 world) const {
    const float halfW = static_cast<float>(viewportWidth_) * 0.5F;
    const float halfH = static_cast<float>(viewportHeight_) * 0.5F;
    return {
        (world.x - center_.x + shakeOffset_.x) * zoom_ + halfW,
        (world.y - center_.y + shakeOffset_.y) * zoom_ + halfH,
    };
}

float Camera2D::zoom() const { return zoom_; }

Vec2 Camera2D::center() const { return center_; }

int Camera2D::viewportWidth() const { return viewportWidth_; }

int Camera2D::viewportHeight() const { return viewportHeight_; }

TextureAtlas::TextureAtlas(std::string textureId)
    : textureId_(std::move(textureId)) {}

void TextureAtlas::addRegion(const std::string& name, const SDL_Rect rect) { regions_[name] = AtlasRegion {rect}; }

std::optional<AtlasRegion> TextureAtlas::region(const std::string& name) const {
    if (const auto found = regions_.find(name); found != regions_.end()) {
        return found->second;
    }
    return std::nullopt;
}

const std::string& TextureAtlas::textureId() const { return textureId_; }

TextureStore::TextureStore(SDL_Renderer* renderer)
    : renderer_(renderer) {}

TextureStore::~TextureStore() {
    for (auto& [_, resource] : textures_) {
        if (resource.texture) {
            SDL_DestroyTexture(resource.texture);
        }
    }
}

bool TextureStore::loadTexture(const std::string& id, const std::string& filePath) {
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_uc* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        logWarn("Texture load failed: " + filePath + " reason=" + stbi_failure_reason());
        return false;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!texture) {
        stbi_image_free(pixels);
        logWarn("SDL_CreateTexture failed for " + filePath + ": " + SDL_GetError());
        return false;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    if (SDL_UpdateTexture(texture, nullptr, pixels, width * 4) != 0) {
        stbi_image_free(pixels);
        SDL_DestroyTexture(texture);
        logWarn("SDL_UpdateTexture failed for " + filePath + ": " + SDL_GetError());
        return false;
    }

    stbi_image_free(pixels);

    if (auto old = textures_.find(id); old != textures_.end() && old->second.texture) {
        SDL_DestroyTexture(old->second.texture);
    }

    textures_[id] = TextureResource {texture, width, height};
    return true;
}

bool TextureStore::createSolidTexture(const std::string& id, const int width, const int height, const Color color) {
    SDL_Texture* texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, width, height);
    if (!texture) {
        return false;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer_);
    SDL_SetRenderTarget(renderer_, texture);
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer_);
    SDL_SetRenderTarget(renderer_, oldTarget);

    textures_[id] = TextureResource {texture, width, height};
    return true;
}


void TextureStore::adoptTexture(const std::string& id, SDL_Texture* texture, const int width, const int height) {
    if (auto old = textures_.find(id); old != textures_.end() && old->second.texture) {
        SDL_DestroyTexture(old->second.texture);
    }
    textures_[id] = TextureResource {texture, width, height};
}

const TextureResource* TextureStore::get(const std::string& id) const {
    if (const auto found = textures_.find(id); found != textures_.end()) {
        return &found->second;
    }
    return nullptr;
}

void SpriteBatch::reserve(const std::size_t spriteCount) {
    commands_.reserve(spriteCount);
    vertices_.reserve(spriteCount * 4);
    indices_.reserve(spriteCount * 6);
}

void SpriteBatch::begin(const Camera2D& camera) {
    camera_ = &camera;
    commands_.clear();
}

void SpriteBatch::draw(const SpriteDrawCmd& cmd) { commands_.push_back(cmd); }

void SpriteBatch::flush(SDL_Renderer* renderer, const TextureStore& textures) {
    if (!camera_) return;

    lastStats_ = {};
    lastStats_.drawCalls = static_cast<std::uint32_t>(commands_.size());

    std::string currentTextureId;

    auto flushGroup = [&](const TextureResource* texture) {
        if (!texture || vertices_.empty()) return;
        SDL_RenderGeometry(renderer, texture->texture, vertices_.data(), static_cast<int>(vertices_.size()), indices_.data(), static_cast<int>(indices_.size()));
        ++lastStats_.batchFlushes;
        vertices_.clear();
        indices_.clear();
    };

    for (const SpriteDrawCmd& cmd : commands_) {
        const TextureResource* texture = textures.get(cmd.textureId);
        if (!texture) continue;

        if (currentTextureId.empty()) {
            currentTextureId = cmd.textureId;
        }

        if (cmd.textureId != currentTextureId) {
            flushGroup(textures.get(currentTextureId));
            currentTextureId = cmd.textureId;
        }

        SDL_Rect srcPx {0, 0, texture->width, texture->height};
        if (cmd.src.has_value()) {
            srcPx = cmd.src.value();
        }

        const float u0 = static_cast<float>(srcPx.x) / static_cast<float>(texture->width);
        const float v0 = static_cast<float>(srcPx.y) / static_cast<float>(texture->height);
        const float u1 = static_cast<float>(srcPx.x + srcPx.w) / static_cast<float>(texture->width);
        const float v1 = static_cast<float>(srcPx.y + srcPx.h) / static_cast<float>(texture->height);

        const Vec2 center {cmd.dest.x + cmd.dest.w * 0.5F, cmd.dest.y + cmd.dest.h * 0.5F};
        const float radians = cmd.rotationDeg * std::numbers::pi_v<float> / 180.0F;
        const float c = std::cos(radians);
        const float s = std::sin(radians);

        const std::array<Vec2, 4> corners = {
            Vec2 {-cmd.dest.w * 0.5F, -cmd.dest.h * 0.5F},
            Vec2 { cmd.dest.w * 0.5F, -cmd.dest.h * 0.5F},
            Vec2 { cmd.dest.w * 0.5F,  cmd.dest.h * 0.5F},
            Vec2 {-cmd.dest.w * 0.5F,  cmd.dest.h * 0.5F},
        };

        const std::array<SDL_FPoint, 4> uvs = {
            SDL_FPoint {u0, v0}, SDL_FPoint {u1, v0}, SDL_FPoint {u1, v1}, SDL_FPoint {u0, v1}
        };

        const int base = static_cast<int>(vertices_.size());
        for (int i = 0; i < 4; ++i) {
            const float rx = corners[i].x * c - corners[i].y * s;
            const float ry = corners[i].x * s + corners[i].y * c;
            const Vec2 world {center.x + rx, center.y + ry};
            const Vec2 screen = camera_->worldToScreen(world);
            vertices_.push_back(SDL_Vertex {
                SDL_FPoint {screen.x, screen.y},
                SDL_Color {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a},
                uvs[i]
            });
        }

        indices_.insert(indices_.end(), {
            base + 0, base + 1, base + 2,
            base + 2, base + 3, base + 0
        });
    }

    flushGroup(textures.get(currentTextureId));
    commands_.clear();
}

void DebugDraw::line(const Vec2 a, const Vec2 b, const Color color) { lines_.push_back(Line {a, b, color}); }

void DebugDraw::circle(const Vec2 center, const float radius, const Color color, const int segments) {
    const int safeSegments = std::max(segments, 8);
    for (int i = 0; i < safeSegments; ++i) {
        const float t0 = static_cast<float>(i) / static_cast<float>(safeSegments);
        const float t1 = static_cast<float>(i + 1) / static_cast<float>(safeSegments);
        const float a0 = t0 * std::numbers::pi_v<float> * 2.0F;
        const float a1 = t1 * std::numbers::pi_v<float> * 2.0F;
        line(
            {center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius},
            {center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius},
            color
        );
    }
}

void DebugDraw::flush(SDL_Renderer* renderer, const Camera2D& camera) {
    for (const Line& lineCmd : lines_) {
        const Vec2 a = camera.worldToScreen(lineCmd.a);
        const Vec2 b = camera.worldToScreen(lineCmd.b);
        SDL_SetRenderDrawColor(renderer, lineCmd.color.r, lineCmd.color.g, lineCmd.color.b, lineCmd.color.a);
        SDL_RenderDrawLineF(renderer, a.x, a.y, b.x, b.y);
    }
    lines_.clear();
}

bool DebugText::init(SDL_Renderer* renderer) {
    constexpr std::array<const char*, 3> candidateFonts = {
        "assets/fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    std::vector<unsigned char> fontData;
    for (const char* path : candidateFonts) {
        std::ifstream in(path, std::ios::binary);
        if (!in.good()) continue;
        fontData.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        if (!fontData.empty()) break;
    }

    if (fontData.empty()) {
        logWarn("DebugText init skipped: no TTF font found (assets/fonts/DejaVuSans.ttf or system fallback)");
        return false;
    }

    stbtt_fontinfo info;
    if (stbtt_InitFont(&info, fontData.data(), stbtt_GetFontOffsetForIndex(fontData.data(), 0)) == 0) {
        logWarn("DebugText init failed: invalid TTF font data");
        return false;
    }

    constexpr int atlasW = 512;
    constexpr int atlasH = 512;
    constexpr float pxSize = 20.0F;

    std::vector<unsigned char> bitmap(atlasW * atlasH, 0);
    stbtt_pack_context pack {};
    if (!stbtt_PackBegin(&pack, bitmap.data(), atlasW, atlasH, 0, 1, nullptr)) {
        logWarn("DebugText init failed: stbtt_PackBegin failed");
        return false;
    }

    std::array<stbtt_packedchar, 96> packedChars {};
    stbtt_PackSetOversampling(&pack, 2, 2);
    if (!stbtt_PackFontRange(&pack, fontData.data(), 0, pxSize, 32, static_cast<int>(packedChars.size()), packedChars.data())) {
        stbtt_PackEnd(&pack);
        logWarn("DebugText init failed: unable to pack glyph range");
        return false;
    }
    stbtt_PackEnd(&pack);

    std::vector<std::uint32_t> rgba(atlasW * atlasH, 0);
    for (int i = 0; i < atlasW * atlasH; ++i) {
        const Uint8 alpha = bitmap[static_cast<std::size_t>(i)];
        rgba[static_cast<std::size_t>(i)] = (static_cast<std::uint32_t>(alpha) << 24) | 0x00FFFFFF;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, atlasW, atlasH);
    if (!texture) {
        logWarn("DebugText init failed: SDL_CreateTexture failed");
        return false;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    if (SDL_UpdateTexture(texture, nullptr, rgba.data(), atlasW * static_cast<int>(sizeof(std::uint32_t))) != 0) {
        SDL_DestroyTexture(texture);
        logWarn("DebugText init failed: SDL_UpdateTexture failed");
        return false;
    }

    textureId_ = "__debug_font";
    fontTexture_ = TextureResource {texture, atlasW, atlasH};

    for (int i = 0; i < static_cast<int>(packedChars.size()); ++i) {
        const char ch = static_cast<char>(i + 32);
        const stbtt_packedchar& pc = packedChars[static_cast<std::size_t>(i)];
        glyphs_[ch] = Glyph {
            static_cast<float>(pc.x0) / atlasW,
            static_cast<float>(pc.y0) / atlasH,
            static_cast<float>(pc.x1) / atlasW,
            static_cast<float>(pc.y1) / atlasH,
            pc.xoff,
            pc.yoff,
            pc.xoff2,
            pc.yoff2,
            pc.xadvance
        };
    }

    initialized_ = true;
    return true;
}


void DebugText::registerTexture(TextureStore& store, const std::string& textureId) {
    if (!initialized_) return;
    textureId_ = textureId;
    store.adoptTexture(textureId_, fontTexture_.texture, fontTexture_.width, fontTexture_.height);
    fontTexture_.texture = nullptr;
}

void DebugText::drawText(SpriteBatch& batch, const std::string& text, Vec2 screenPos, const float scale, const Color color) const {
    if (!initialized_) return;

    float penX = screenPos.x;
    float penY = screenPos.y;

    for (const char c : text) {
        if (c == '\n') {
            penX = screenPos.x;
            penY += 24.0F * scale;
            continue;
        }

        const auto found = glyphs_.find(c);
        if (found == glyphs_.end()) continue;

        const Glyph& g = found->second;
        const float w = static_cast<float>(g.x1 - g.x0) * scale;
        const float h = static_cast<float>(g.y1 - g.y0) * scale;

        SDL_Rect src {
            static_cast<int>(g.u0 * fontTexture_.width),
            static_cast<int>(g.v0 * fontTexture_.height),
            static_cast<int>((g.u1 - g.u0) * fontTexture_.width),
            static_cast<int>((g.v1 - g.v0) * fontTexture_.height)
        };

        batch.draw(SpriteDrawCmd {
            .textureId = textureId_,
            .dest = SDL_FRect {penX + static_cast<float>(g.x0) * scale, penY + static_cast<float>(g.y0) * scale, w, h},
            .src = src,
            .rotationDeg = 0.0F,
            .color = color,
        });

        penX += g.advance * scale;
    }
}

const RenderBatchStats& SpriteBatch::lastStats() const { return lastStats_; }

} // namespace engine
