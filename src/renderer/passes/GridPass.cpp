#include "renderer/passes/GridPass.h"

#include "renderer/RenderFrame.h"
#include "renderer/passes/RenderPassContext.h"
#include "renderer/rhi/IRenderCommandList.h"

namespace renderlab {

std::string_view GridPass::name() const noexcept {
    return "Grid";
}

void GridPass::execute(RenderPassContext& context) {
    context.commands.setStencilState(StencilState{});
    context.commands.setBlendEnabled(true);
    context.commands.setCullEnabled(false);
    context.commands.setDepthWriteEnabled(false);

    context.commands.drawEditorGrid(context.frame.view, context.frame.projection);

    context.commands.setDepthWriteEnabled(true);
    context.commands.setBlendEnabled(false);
}

} // namespace renderlab
