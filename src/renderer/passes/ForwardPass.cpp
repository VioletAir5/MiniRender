#include "renderer/passes/ForwardPass.h"

#include "renderer/RenderFrame.h"
#include "renderer/passes/RenderPassContext.h"
#include "renderer/rhi/IRenderCommandList.h"

namespace renderlab {

void ForwardPass::execute(RenderPassContext& context) {
    const SelectionOutline* outline = context.frame.selectionOutline.has_value()
                                          ? &*context.frame.selectionOutline
                                          : nullptr;
    context.outlinedItem = nullptr;

    StencilState stencil;
    if (outline != nullptr) {
        stencil.enabled = true;
        stencil.comparison = CompareOperation::Always;
        stencil.stencilFail = StencilOperation::Keep;
        stencil.depthFail = StencilOperation::Keep;
        stencil.pass = StencilOperation::Replace;
        stencil.reference = 1;
    }

    for (const RenderItem& item : context.frame.items) {
        const bool selected = outline != nullptr && item.entity == outline->entity;
        stencil.writeMask = selected ? 0xFFU : 0x00U;
        context.commands.setStencilState(stencil);
        context.commands.drawMesh(item, MeshDrawParameters{.model = item.model});

        if (selected) {
            context.outlinedItem = &item;
        }
    }
}

} // namespace renderlab
