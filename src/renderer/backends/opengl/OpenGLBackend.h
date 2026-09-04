#pragma once

#include "renderer/ShaderLibrary.h"
#include "renderer/backends/opengl/OpenGLMeshCache.h"
#include "renderer/backends/opengl/OpenGLRenderResources.h"
#include "renderer/backends/opengl/OpenGLShaderCache.h"
#include "renderer/backends/opengl/OpenGLShaderProgram.h"
#include "renderer/backends/opengl/OpenGLTextureCache.h"
#include "renderer/backends/opengl/passes/OpenGLPassContext.h"
#include "renderer/pipeline/RenderPipeline.h"
#include "renderer/rhi/IRenderBackend.h"

namespace renderlab {

class AssetRegistry;

// OpenGL 3.3 后端，负责着色器、GPU 网格缓存及逐帧绘制。
class OpenGLBackend final : public IRenderBackend {
  public:
    // registry 必须比后端存活更久。
    OpenGLBackend(const AssetRegistry& registry, const ShaderLibrary& shaderLibrary,
                  RenderPipelineDescriptor pipelineDescriptor);

    // 创建 OpenGL 资源；调用时必须已有当前上下文。
    bool initialize() override;
    // 在当前上下文中按依赖顺序释放缓存和着色器。
    void shutdown() override;
    // 设置 OpenGL viewport。
    void resize(int width, int height) override;
    // 解析网格句柄、上传缺失资源并绘制一帧。
    void render(const RenderFrame& frame) override;

  private:
    // 非拥有引用，用于把 MaterialHandle 解析为 API 无关材质参数。
    const AssetRegistry& registry_;
    const ShaderLibrary& shaderLibrary_;
    ShaderHandle pbrShader_;
    OpenGLShaderCache shaderCache_;
    OpenGLMeshCache meshCache_;
    OpenGLTextureCache textureCache_;
    OpenGLRenderResources renderResources_;
    // OpenGL 资源服务只对具体 OpenGL Pass 可见；通用 Pipeline 不依赖 glad 类型。
    OpenGLPassContext passContext_;
    RenderPassFactory passFactory_;
    RenderPipelineDescriptor pipelineDescriptor_;
    RenderPipeline pipeline_;
    std::uint64_t frameNumber_{0};
    int viewportWidth_{0};
    int viewportHeight_{0};
    // 创建 OpenGL 资源；调用时必须已有当前上下文。
    bool initialized_{false};
    bool passFactoryReady_{false};
};

} // namespace renderlab
