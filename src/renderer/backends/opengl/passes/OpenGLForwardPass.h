#pragma once

#include "renderer/pipeline/IRenderPass.h"

namespace renderlab {

struct OpenGLPassContext;
struct RenderFrame;
struct RenderItem;

// 绘制场景表面，并在存在选择时把选中实体写入模板缓冲。
class OpenGLForwardPass final : public IRenderPass {
  public:
    explicit OpenGLForwardPass(OpenGLPassContext& context) noexcept;

    void execute(const RenderPassExecutionContext& context) override;

  private:
    OpenGLPassContext& context_;
};

} // namespace renderlab
