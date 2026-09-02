#pragma once

#include "renderer/backends/opengl/OpenGLGridRenderer.h"
#include "renderer/backends/opengl/OpenGLMeshCache.h"
#include "renderer/backends/opengl/OpenGLTextureCache.h"
#include "renderer/backends/opengl/OpenGLShaderProgram.h"
#include "renderer/rhi/IRenderBackend.h"
#include "renderer/ShaderLibrary.h"

#include <filesystem>

namespace renderlab {

class AssetRegistry;

// OpenGL 3.3 后端，负责着色器、GPU 网格缓存及逐帧绘制。
class OpenGLBackend final : public IRenderBackend {
public:
    // registry 必须比后端存活更久。
    OpenGLBackend(const AssetRegistry& registry,
                  std::filesystem::path shaderRoot);

    // 创建 OpenGL 资源；调用时必须已有当前上下文。
    bool initialize() override;
    // 在当前上下文中按依赖顺序释放缓存和着色器。
    void shutdown() override;
    // 设置 OpenGL viewport。
    void resize(int width, int height) override;
    // 解析网格句柄、上传缺失资源并绘制一帧。
    void render(const RenderFrame& frame) override;

private:
    // 使用当前着色器绘制一个网格项；overrideColor 非空时忽略材质颜色。
    void drawItem(const RenderItem& item, const glm::mat4& model,
                  const glm::vec4* overrideColor = nullptr);

    // 非拥有引用，用于把 MaterialHandle 解析为 API 无关材质参数。
    const AssetRegistry& registry_;
    ShaderLibrary shaderLibrary_;
    ShaderHandle pbrShader_;
    OpenGLShaderProgram shader_;
    OpenGLMeshCache meshCache_;
    OpenGLTextureCache textureCache_;
    OpenGLGridRenderer gridRenderer_;
    std::uint64_t frameNumber_{0};
    // 创建 OpenGL 资源；调用时必须已有当前上下文。
    bool initialized_{false};
};

} // namespace renderlab
