#pragma once

#include "renderer/passes/IRenderPass.h"

namespace renderlab {

class ForwardPass final : public IRenderPass {
public:
    void execute(RenderPassContext& context) override;
};

} // namespace renderlab
