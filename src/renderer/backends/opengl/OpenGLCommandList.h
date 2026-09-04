#pragma once

#include "renderer/rhi/IRenderCommandList.h"

#include <cstdint>

namespace renderlab {

class AssetRegistry;
class OpenGLGridRenderer;
class OpenGLMeshCache;
class OpenGLShaderProgram;
class OpenGLTextureCache;

// 将 API 无关的渲染命令翻译为 OpenGL 3.3 调用。
class OpenGLCommandList final : public IRenderCommandList {
public:
    OpenGLCommandList(const AssetRegistry& registry,
                      OpenGLShaderProgram& shader,
                      OpenGLMeshCache& meshCache,
                      OpenGLTextureCache& textureCache,
                      OpenGLGridRenderer& gridRenderer);

    void setFrameNumber(std::uint64_t frameNumber) noexcept;

    void setStencilState(const StencilState& state) override;
    void setDepthWriteEnabled(bool enabled) override;
    void setBlendEnabled(bool enabled) override;
    void setCullEnabled(bool enabled) override;
    void drawMesh(const RenderItem& item,
                  const MeshDrawParameters& parameters) override;
    void drawEditorGrid(const glm::mat4& view,
                        const glm::mat4& projection) override;

private:
    const AssetRegistry& registry_;
    OpenGLShaderProgram& shader_;
    OpenGLMeshCache& meshCache_;
    OpenGLTextureCache& textureCache_;
    OpenGLGridRenderer& gridRenderer_;
    std::uint64_t frameNumber_{0};
};

} // namespace renderlab
