#pragma once

#include <glad/glad.h>

#include <string>
#include <unordered_map>

namespace engine {

class ShaderCache {
  public:
    struct Program {
        GLuint id {0};
        GLint locViewProj {-1};
        GLint locTime {-1};
        GLint locAnimSpeed {-1};
        GLint locPerInstanceOffset {-1};
        GLint locEmissiveBoost {-1};
        GLint locAnimMode {-1};
        GLint locSpriteAtlas {-1};
        GLint locPaletteRamp {-1};
    };

    bool compileFromSource(const std::string& name, const std::string& vertSrc, const std::string& fragSrc);
    bool compileFromFiles(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    [[nodiscard]] const Program* get(const std::string& name) const;
    void shutdown();

  private:
    std::unordered_map<std::string, Program> programs_;

    bool compileShader(GLenum type, const std::string& source, GLuint& outShader, std::string& outError) const;
};

} // namespace engine
