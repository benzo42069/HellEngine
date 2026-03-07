#include <engine/shader_cache.h>

#include <engine/logging.h>

#include <fstream>
#include <sstream>

namespace engine {

namespace {

std::string readFileText(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) return {};
    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

} // namespace

bool ShaderCache::compileShader(const GLenum type, const std::string& source, GLuint& outShader, std::string& outError) const {
    outShader = glCreateShader(type);
    if (outShader == 0) {
        outError = "glCreateShader returned 0.";
        return false;
    }

    const char* sourcePtr = source.c_str();
    glShaderSource(outShader, 1, &sourcePtr, nullptr);
    glCompileShader(outShader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(outShader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) return true;

    GLint infoLogLen = 0;
    glGetShaderiv(outShader, GL_INFO_LOG_LENGTH, &infoLogLen);
    std::string logText(static_cast<std::size_t>(infoLogLen > 1 ? infoLogLen : 1), '\0');
    GLsizei written = 0;
    glGetShaderInfoLog(outShader, infoLogLen, &written, logText.data());
    outError = logText;
    glDeleteShader(outShader);
    outShader = 0;
    return false;
}

bool ShaderCache::compileFromSource(const std::string& name, const std::string& vertSrc, const std::string& fragSrc) {
    if (name.empty()) {
        logError("Shader compile failed: program name is empty.");
        return false;
    }

    GLuint vertShader = 0;
    GLuint fragShader = 0;
    std::string error;
    if (!compileShader(GL_VERTEX_SHADER, vertSrc, vertShader, error)) {
        logError("Vertex shader compile failed for '" + name + "': " + error);
        return false;
    }
    if (!compileShader(GL_FRAGMENT_SHADER, fragSrc, fragShader, error)) {
        glDeleteShader(vertShader);
        logError("Fragment shader compile failed for '" + name + "': " + error);
        return false;
    }

    const GLuint programId = glCreateProgram();
    if (programId == 0) {
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        logError("Shader program link failed for '" + name + "': glCreateProgram returned 0.");
        return false;
    }

    glAttachShader(programId, vertShader);
    glAttachShader(programId, fragShader);
    glLinkProgram(programId);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(programId, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint infoLogLen = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLen);
        std::string logText(static_cast<std::size_t>(infoLogLen > 1 ? infoLogLen : 1), '\0');
        GLsizei written = 0;
        glGetProgramInfoLog(programId, infoLogLen, &written, logText.data());
        glDeleteProgram(programId);
        logError("Shader program link failed for '" + name + "': " + logText);
        return false;
    }

    Program program {};
    program.id = programId;
    program.locViewProj = glGetUniformLocation(programId, "uViewProj");
    program.locTime = glGetUniformLocation(programId, "uTime");
    program.locAnimSpeed = glGetUniformLocation(programId, "uAnimSpeed");
    program.locPerInstanceOffset = glGetUniformLocation(programId, "uPerInstanceOffset");
    program.locEmissiveBoost = glGetUniformLocation(programId, "uEmissiveBoost");
    program.locAnimMode = glGetUniformLocation(programId, "uAnimMode");
    program.locSpriteAtlas = glGetUniformLocation(programId, "uSpriteAtlas");
    program.locPaletteRamp = glGetUniformLocation(programId, "uPaletteRamp");

    auto found = programs_.find(name);
    if (found != programs_.end() && found->second.id != 0) glDeleteProgram(found->second.id);
    programs_[name] = program;

    logInfo("Compiled shader program '" + name + "'.");
    return true;
}

bool ShaderCache::compileFromFiles(const std::string& name, const std::string& vertPath, const std::string& fragPath) {
    const std::string vertSrc = readFileText(vertPath);
    if (vertSrc.empty()) {
        logError("Failed reading vertex shader file: " + vertPath);
        return false;
    }

    const std::string fragSrc = readFileText(fragPath);
    if (fragSrc.empty()) {
        logError("Failed reading fragment shader file: " + fragPath);
        return false;
    }

    return compileFromSource(name, vertSrc, fragSrc);
}

const ShaderCache::Program* ShaderCache::get(const std::string& name) const {
    const auto found = programs_.find(name);
    if (found == programs_.end()) return nullptr;
    return &found->second;
}

void ShaderCache::shutdown() {
    for (auto& [name, program] : programs_) {
        if (program.id != 0) {
            glDeleteProgram(program.id);
            program.id = 0;
        }
    }
    programs_.clear();
}

} // namespace engine
