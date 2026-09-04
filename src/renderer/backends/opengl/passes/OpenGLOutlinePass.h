#pragma once

#include "renderer/pipeline/IRenderPass.h"

namespace renderlab {

struct OpenGLPassContext;
struct RenderFrame;
struct RenderItem;

// 使用 Forward Pass 写入的模板值绘制选中实体外扩轮廓。
class OpenGLOutlinePass final : public IRenderPass {
  public:
    explicit OpenGLOutlinePass(OpenGLPassContext& context) noexcept;

    void execute(const RenderPassExecutionContext& context) override;

  private:
    OpenGLPassContext& context_;
};

} // namespace renderlab
