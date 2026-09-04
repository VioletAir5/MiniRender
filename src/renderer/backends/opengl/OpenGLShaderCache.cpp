#include "renderer/backends/opengl/OpenGLShaderCache.h"

#include "renderer/ShaderLibrary.h"
#include "renderer/backends/opengl/OpenGLShaderProgram.h"

#include <memory>
#include <utility>

namespace renderlab {

OpenGLShaderCache::OpenGLShaderCache(const ShaderLibrary& library) noexcept : library_(library) {}

OpenGLShaderCache::~OpenGLShaderCache() = default;

OpenGLShaderProgram* OpenGLShaderCache::resolve(const ShaderHandle handle, std::string& error) {
    error.clear();
    if (!handle.valid()) {
        error = "Shader handle is invalid";
        return nullptr;
    }
    if (const auto existing = programs_.find(handle); existing != programs_.end()) {
        return existing->second.get();
    }
    if (const auto failed = failures_.find(handle); failed != failures_.end()) {
        error = failed->second;
        return nullptr;
    }

    const auto source = library_.load(handle, error);
    if (!source.has_value()) {
        failures_.emplace(handle, error);
        return nullptr;
    }

    auto program = std::make_unique<OpenGLShaderProgram>();
    if (!program->initialize(source->vertexSource, source->fragmentSource)) {
        error = "OpenGL failed to compile or link shader program";
        failures_.emplace(handle, error);
        return nullptr;
    }
    OpenGLShaderProgram* result = program.get();
    programs_.emplace(handle, std::move(program));
    return result;
}

void OpenGLShaderCache::clear() noexcept {
    for (auto& [handle, program] : programs_) {
        (void)handle;
        program->shutdown();
    }
    programs_.clear();
    failures_.clear();
}

std::size_t OpenGLShaderCache::size() const noexcept {
    return programs_.size();
}

} // namespace renderlab
