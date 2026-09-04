#pragma once

#include "renderer/passes/IRenderPass.h"

namespace renderlab {

class OutlinePass final : public IRenderPass {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    void execute(RenderPassContext& context) override;
};

} // namespace renderlab
