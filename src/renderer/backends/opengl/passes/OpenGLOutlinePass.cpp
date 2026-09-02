#include "renderer/backends/opengl/passes/OpenGLOutlinePass.h"

#include "renderer/RenderFrame.h"
#include "renderer/backends/opengl/passes/OpenGLPassContext.h"
#include "renderer/backends/opengl/passes/OpenGLSceneDraw.h"

#include <glad/glad.h>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>

namespace renderlab {

void OpenGLOutlinePass::render(const RenderFrame& frame,
                               const RenderItem* outlinedItem,
                               OpenGLPassContext& context) const {
    if (!frame.selectionOutline.has_value() || outlinedItem == nullptr) {
        return;
    }

    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glDepthMask(GL_FALSE);

    const SelectionOutline& outline = *frame.selectionOutline;
    const float scale = std::max(outline.scale, 1.0F);
    const glm::mat4 outlineModel =
        glm::scale(outlinedItem->model, glm::vec3{scale});
    drawOpenGLSceneItem(
        context, *outlinedItem, outlineModel, &outline.color);

    glDepthMask(GL_TRUE);
}

} // namespace renderlab
