#pragma once

#include "renderer/pipeline/IRenderPass.h"

namespace renderlab {

struct OpenGLPassContext;

class OpenGLDirectionalShadowPass final : public IRenderPass {
  public:
    explicit OpenGLDirectionalShadowPass(OpenGLPassContext& context) noexcept;
    [[nodiscard]] bool initialize() override;
    void execute(const RenderPassExecutionContext& execution) override;

  private:
    OpenGLPassContext& context_;
};

} // namespace renderlab
