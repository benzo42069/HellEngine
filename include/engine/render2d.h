#pragma once

#include <SDL.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine {

struct Vec2 {
    float x {0.0F};
    float y {0.0F};
};

struct Color {
    Uint8 r {255};
    Uint8 g {255};
    Uint8 b {255};
    Uint8 a {255};
};

class Camera2D {
  public:
    void setViewport(int width, int height);
    void setCenter(Vec2 center);
    void pan(Vec2 delta);
    void setZoom(float zoom);
    void addZoom(float delta);
    void setShake(float amplitude, float seconds);
    void update(float dt);

    [[nodiscard]] Vec2 worldToScreen(Vec2 world) const;
    [[nodiscard]] float zoom() const;
    [[nodiscard]] Vec2 center() const;
    [[nodiscard]] int viewportWidth() const;
    [[nodiscard]] int viewportHeight() const;

  private:
    int viewportWidth_ {1280};
    int viewportHeight_ {720};
    Vec2 center_ {0.0F, 0.0F};
    float zoom_ {1.0F};
    float shakeAmplitude_ {0.0F};
    float shakeTimeRemaining_ {0.0F};
    Vec2 shakeOffset_ {0.0F, 0.0F};
};

struct TextureResource {
    SDL_Texture* texture {nullptr};
    int width {0};
    int height {0};
};

struct AtlasRegion {
    SDL_Rect source {0, 0, 0, 0};
};

class TextureAtlas {
  public:
    explicit TextureAtlas(std::string textureId);

    void addRegion(const std::string& name, SDL_Rect rect);
    [[nodiscard]] std::optional<AtlasRegion> region(const std::string& name) const;
    [[nodiscard]] const std::string& textureId() const;

  private:
    std::string textureId_;
    std::unordered_map<std::string, AtlasRegion> regions_;
};

class TextureStore {
  public:
    explicit TextureStore(SDL_Renderer* renderer);
    ~TextureStore();

    TextureStore(const TextureStore&) = delete;
    TextureStore& operator=(const TextureStore&) = delete;

    bool loadTexture(const std::string& id, const std::string& filePath);
    bool createSolidTexture(const std::string& id, int width, int height, Color color);
    void adoptTexture(const std::string& id, SDL_Texture* texture, int width, int height);

    [[nodiscard]] const TextureResource* get(const std::string& id) const;

  private:
    SDL_Renderer* renderer_ {nullptr};
    std::unordered_map<std::string, TextureResource> textures_;
};

struct SpriteDrawCmd {
    std::string textureId;
    SDL_FRect dest {0, 0, 0, 0};
    std::optional<SDL_Rect> src;
    float rotationDeg {0.0F};
    Color color {};
};


struct RenderBatchStats {
    std::uint32_t drawCalls {0};
    std::uint32_t batchFlushes {0};
};

class SpriteBatch {
  public:
    void reserve(std::size_t spriteCount);
    void begin(const Camera2D& camera);
    void draw(const SpriteDrawCmd& cmd);
    void flush(SDL_Renderer* renderer, const TextureStore& textures);
    [[nodiscard]] const RenderBatchStats& lastStats() const;

  private:
    const Camera2D* camera_ {nullptr};
    std::vector<SpriteDrawCmd> commands_;
    std::vector<SDL_Vertex> vertices_;
    std::vector<int> indices_;
    RenderBatchStats lastStats_ {};
};

class DebugDraw {
  public:
    void line(Vec2 a, Vec2 b, Color color);
    void circle(Vec2 center, float radius, Color color, int segments = 24);
    void flush(SDL_Renderer* renderer, const Camera2D& camera);

  private:
    struct Line {
        Vec2 a;
        Vec2 b;
        Color color;
    };
    std::vector<Line> lines_;
};

class DebugText {
  public:
    bool init(SDL_Renderer* renderer);
    void registerTexture(TextureStore& store, const std::string& textureId);
    void drawText(SpriteBatch& batch, const std::string& text, Vec2 screenPos, float scale, Color color) const;

  private:
    struct Glyph {
        float u0 {0.0F};
        float v0 {0.0F};
        float u1 {0.0F};
        float v1 {0.0F};
        float x0 {0.0F};
        float y0 {0.0F};
        float x1 {0.0F};
        float y1 {0.0F};
        float advance {0.0F};
    };

    bool initialized_ {false};
    std::string textureId_;
    TextureResource fontTexture_ {};
    std::unordered_map<char, Glyph> glyphs_;

  public:
    [[nodiscard]] const TextureResource* texture() const { return initialized_ ? &fontTexture_ : nullptr; }
};

} // namespace engine
