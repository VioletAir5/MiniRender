#pragma once

#include "renderer/passes/IRenderPass.h"

namespace renderlab {

class OutlinePass final : public IRenderPass {
public:
    void execute(RenderPassContext& context) override;
};

} // namespace renderlab
