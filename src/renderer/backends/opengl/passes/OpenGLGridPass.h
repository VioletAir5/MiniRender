#pragma once

#include "renderer/backends/opengl/OpenGLGridRenderer.h"
#include "renderer/pipeline/IRenderPass.h"

namespace renderlab {

struct OpenGLPassContext;
struct RenderFrame;

// 在场景和轮廓之后绘制编辑器网格，并隔离覆盖层所需的 OpenGL 状态。
class OpenGLGridPass final : public IRenderPass {
  public:
    explicit OpenGLGridPass(OpenGLPassContext& context) noexcept;

    [[nodiscard]] bool initialize() override;
    void shutdown() noexcept override;
    void execute(const RenderPassExecutionContext& context) override;

  private:
    OpenGLPassContext& context_;
    OpenGLGridRenderer renderer_;
};

} // namespace renderlab
