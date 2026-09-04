#include "renderer/passes/OutlinePass.h"

#include "renderer/RenderFrame.h"
#include "renderer/passes/RenderPassContext.h"
#include "renderer/rhi/IRenderCommandList.h"

#include <glm/ext/matrix_transform.hpp>

#include <algorithm>

namespace renderlab {

std::string_view OutlinePass::name() const noexcept {
    return "Outline";
}

void OutlinePass::execute(RenderPassContext& context) {
    if (!context.frame.selectionOutline.has_value() ||
        context.outlinedItem == nullptr) {
        return;
    }

    StencilState stencil;
    stencil.enabled = true;
    stencil.comparison = CompareOperation::NotEqual;
    stencil.reference = 1;
    stencil.writeMask = 0x00U;
    context.commands.setStencilState(stencil);
    context.commands.setDepthWriteEnabled(false);

    const SelectionOutline& outline = *context.frame.selectionOutline;
    const float scale = std::max(outline.scale, 1.0F);
    const glm::mat4 outlineModel =
        glm::scale(context.outlinedItem->model, glm::vec3{scale});
    context.commands.drawMesh(
        *context.outlinedItem,
        MeshDrawParameters{
            .model = outlineModel,
            .overrideColor = outline.color,
        });

    context.commands.setDepthWriteEnabled(true);
}

} // namespace renderlab
