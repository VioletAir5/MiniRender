#pragma once

#include "assets/AssetHandle.h"

#include <map>
#include <memory>
#include <string>

namespace renderlab {

class OpenGLShaderProgram;
class ShaderLibrary;

// 把 API 无关 ShaderHandle 按需编译成 OpenGL 程序，并缓存失败结果。
class OpenGLShaderCache final {
  public:
    explicit OpenGLShaderCache(const ShaderLibrary& library) noexcept;
    ~OpenGLShaderCache();
    OpenGLShaderCache(const OpenGLShaderCache&) = delete;
    OpenGLShaderCache& operator=(const OpenGLShaderCache&) = delete;

    [[nodiscard]] OpenGLShaderProgram* resolve(ShaderHandle handle, std::string& error);
    void clear() noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

  private:
    const ShaderLibrary& library_;
    std::map<ShaderHandle, std::unique_ptr<OpenGLShaderProgram>> programs_;
    std::map<ShaderHandle, std::string> failures_;
};

} // namespace renderlab
