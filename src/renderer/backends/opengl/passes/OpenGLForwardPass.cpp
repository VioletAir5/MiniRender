#include "renderer/backends/opengl/passes/OpenGLForwardPass.h"

#include "renderer/RenderFrame.h"
#include "renderer/backends/opengl/passes/OpenGLPassContext.h"
#include "renderer/backends/opengl/passes/OpenGLSceneDraw.h"

#include <glad/glad.h>

namespace renderlab {

OpenGLForwardPass::OpenGLForwardPass(OpenGLPassContext& context) noexcept : context_(context) {}

void OpenGLForwardPass::execute(const RenderPassExecutionContext& execution) {
    const RenderFrame& frame = execution.frame;
    const SelectionOutline* outline =
        frame.selectionOutline.has_value() ? &*frame.selectionOutline : nullptr;
    context_.outlinedItem = nullptr;
    if (outline != nullptr) {
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    } else {
        glDisable(GL_STENCIL_TEST);
    }

    for (const RenderItem& item : frame.items) {
        const bool selected = outline != nullptr && item.entity == outline->entity;
        if (outline != nullptr) {
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(selected ? 0xFF : 0x00);
        }
        drawOpenGLSceneItem(context_, frame, item, item.model);
        if (selected) {
            context_.outlinedItem = &item;
        }
    }
}

} // namespace renderlab
